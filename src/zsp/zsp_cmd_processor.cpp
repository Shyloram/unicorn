#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <errno.h>

#include "zsp_cmd_processor.h"
#include "zsp_interfacedef.h"

#include "modinterface.h"
#include "buffermanage.h"
#include "audio.h"

static STRUCT_VIDEO_USER zsp_video_user_list[ZSP_VIDEO_USER_MAX] = {0}; /* 使用产测工具播放视频的用户列表 */

/* 获取用户列表操作时的互斥锁,已静态初始化，不需要再调用 pthread_mutex_init() 初始化 */
static pthread_mutex_t zsp_video_user_mutex = PTHREAD_MUTEX_INITIALIZER;
/* UDP 发送数据时对 socket 写入操作的互斥锁,已静态初始化，不需要再调用 pthread_mutex_init() 初始化 */
static pthread_mutex_t udp_send_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 声明只在本文件中使用的 static 的函数 begin */
static int zsp_get_software_version(TYPE_DEVICE_INFO *version);
static int zsp_tcp_cmd_data_processor(int socket_fd, char * buf, int len, void * arg);
/* 声明只在本文件中使用的 static 的函数 end */

/***********************************************
 *@brief 使用 select 机制监听 socket fd 是否可读
 *@param socket_fd  参数1 socket fd
 *@param timeout_ms 参数2 超时时间
 *@retrun
 *      成功：返回 ZSP_SUCCESS  0
 *      失败：返回 ZSP_FAILURE -1 
 **********************************************/
static int __zsp_select_fd_readable(int socket_fd, int timeout_ms)
{
    int ret = 0;
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    timeval timeout;
    timeout.tv_sec = timeout_ms/1000;   /* 秒 */
    timeout.tv_usec = (timeout_ms % 1000) * 1000;   /* 微秒 */

    ret = select(socket_fd+1, &readfds, NULL, NULL, &timeout);

    if((ret > 0) && (FD_ISSET(socket_fd, &readfds)))
    {
        //ZSPLOG("socket_fd is readable\r\n");
        return ZSP_SUCCESS;
    }

    //ZSPLOG("socket_fd is not readable\r\n");
    return ZSP_FAILURE;
}

/***********************************************
 *@brief 使用 select 机制监听 socket fd 是否可写
 *@param socket_fd  参数1 socket fd
 *@param timeout_ms 参数2 超时时间
 *@retrun
 *      成功：返回 ZSP_SUCCESS  0
 *      失败：返回 ZSP_FAILURE -1 
 **********************************************/
static int __zsp_select_fd_writable(int socket_fd, int timeout_ms)
{
    int ret = 0;
    fd_set writefds;

    FD_ZERO(&writefds);
    FD_SET(socket_fd, &writefds);

    timeval timeout;
    timeout.tv_sec = timeout_ms/1000;   /* 秒 */
    timeout.tv_usec = (timeout_ms % 1000) * 1000;   /* 微秒 */

    ret = select(socket_fd+1, NULL, &writefds, NULL, &timeout);

    if((ret > 0) && (FD_ISSET(socket_fd, &writefds)))
    {
        ZSPLOG("socket_fd is writable\r\n");
        return ZSP_SUCCESS;
    }

    ZSPLOG("socket_fd is not writable\r\n");
    return ZSP_FAILURE;
}

/***********************************************
 *@brief 往指定 socket fd 发送指定长度的数据
 *@param socket_fd 参数1 socket fd
 *@param buf 参数2 buf 地址
 *@param len 参数3 buf 长度
 *@param len 参数4 timeout_ms 每次select的超时时间，总超时时间为=(重试次数*每次超时时间)
 *@retrun
 *      成功：返回 ZSP_SUCCESS  0
 *      失败：返回 ZSP_FAILURE -1 
 **********************************************/
static int __zsp_send_buffer_to_socket(int socket_fd, char * buf, int len, int timeout_ms)
{
    int ret = 0;
    int total_len = 0;      /* 已发送的数据总字节数 */
    int retry_times = 0;    /* 发送失败的重试次数 */

    if((NULL == buf) || (len <= 0))
    {
        ZSPERR("buf is NULL or len %d <= 0", len);
        return ZSP_FAILURE;
    }

    do
    {
        retry_times ++;     /* 重试次数 ++ */
        /* len-total_len 为未发送的剩余的字节数 */
        ret = send(socket_fd, buf+total_len, len-total_len, 0);
        if(ret < 0)
        {
            int errno_tmp = errno;
            ZSPERR("send fd:%d, errno:%d, err:%s\r\n", 
                    socket_fd, errno_tmp, strerror(errno_tmp));

            if(errno_tmp == EAGAIN)
            {
                ZSPERR("EAGAIN, send again\r\n");
                __zsp_select_fd_writable(socket_fd, timeout_ms);  /* select timeout 2000ms */
                if(retry_times < 4) /* 发送失败的重试次数 */
                {
                    ZSPERR("zsp send buffer error, continue, ret:%d\r\n", ret);
                    usleep(20*1000);    /* 等待20ms后继续 */
                    continue;
                }
                else
                {
                    ZSPERR("zsp send buffer error, ret:%d\r\n", ret);
                    return ZSP_FAILURE;
                }
            }
            else
            {
                ZSPERR("send error, errno [%d], err:%s\r\n", errno_tmp, strerror(errno_tmp));
                usleep(20*1000);    /* 等待20ms后退出 */
                break;
            }

            if(retry_times < 4) /* 发送失败的重试次数 */
            {
                ZSPERR("zsp send buffer error, continue, ret:%d\r\n", ret);
                usleep(20*1000);    /* 等待20ms后继续 */
                continue;
            }
            else
            {
                ZSPERR("zsp send buffer error, ret:%d\r\n", ret);
                return ZSP_FAILURE;
            }
        }
        else if(0 == ret)
        {
            ZSPERR("socket closed, ret:%d\r\n", ret);
            return ZSP_FAILURE;
        }
        
        retry_times = 0;
        total_len += ret;
        ZSPLOG("zsp send buffer success, ret:%d, total_len:%d, len:%d\r\n", ret, total_len, len);
    }while(total_len < len);

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 从指定 socket fd 接收指定长度的数据
 *@param socket_fd 参数1 socket fd
 *@param buf 参数2 buf 地址
 *@param len 参数2 buf 长度
 *@retrun
 *      成功：返回 ZSP_SUCCESS  0
 *      失败：返回 ZSP_FAILURE -1 
 **********************************************/
static int __zsp_recv_buffer_from_socket(int socket_fd, char * buf, int len)
{
    int ret = 0;
    int total_len = 0;      /* 已收到的字节数 */
    int retry_times = 0;    /* 出错时重试次数 */

    if((NULL == buf) || (len <= 0))
    {
        ZSPERR("buf is NULL or len %d <= 0", len);
        return ZSP_FAILURE;
    }

    do
    {
        retry_times ++; /* 重试次数 ++ */
        /* MSG_DONTWAIT 设置此次 recv 为 非阻塞 */
        //ret = recv(socket_fd, (buf + total_len), (len - total_len), MSG_DONTWAIT);
        ret = recv(socket_fd, (buf + total_len), (len - total_len), 0);
        if(ret < 0)
        {
            int error_tmp = errno;
            ZSPERR("recv fd:%d, errno:%d, err:%s\r\n", socket_fd, error_tmp, strerror(error_tmp));
            if(retry_times < 4) /* 接收失败的重试次数 */
            {
                ZSPERR("zsp recv buffer error, continue, ret:%d\r\n", ret);
                usleep(20*1000);    /* 等待20ms后继续 */
                continue;
            }
            else
            {
                ZSPERR("zsp recv buffer error, ret:%d\r\n", ret);
                return ZSP_FAILURE;
            }
        }
        else if(0 == ret)
        {
            ZSPERR("socket closed, ret:%d\r\n", ret);
            return ZSP_FAILURE;
        }

        retry_times = 0;
        total_len += ret;
        //ZSPLOG("zsp recv buffer success, ret:%d, total_len:%d, len:%d\r\n", ret, total_len, len);
    }while(total_len < len);
    //ZSPLOG("zsp recv total_len:%d, len:%d\r\n", total_len, len);

    return ZSP_SUCCESS;
}


/***********************************************
 *@brief 初始化视频播放用户列表
 *@param video_user_list 视频播放用户列表数组
 *@retrun
 *      成功：返回 ZSP_SUCCESS  0
 *      失败：返回 ZSP_FAILURE -1 
 **********************************************/
static int __zsp_video_user_list_init(STRUCT_VIDEO_USER video_user_list[])
{
    int i = 0;

    if(NULL == video_user_list)
    {
        return ZSP_FAILURE;
    }

    pthread_mutex_lock(&zsp_video_user_mutex);
    for(i = 0; i < ZSP_VIDEO_USER_MAX; i++ )
    {
        video_user_list[i].used = VIDEO_USER_UNUSED;
        video_user_list[i].ip = 0;
        video_user_list[i].port = 0;
        video_user_list[i].userid = i;
        video_user_list[i].devType = 0;
    }
    pthread_mutex_unlock(&zsp_video_user_mutex);

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 初始化视频播放用户列表，提供给外部的接口，调用 __zsp_video_user_list_init()
 *@param void
 *@retrun
 *      成功：返回 ZSP_SUCCESS  0
 *      失败：返回 ZSP_FAILURE -1 
 **********************************************/
int zsp_video_user_list_init(void)
{
    return __zsp_video_user_list_init(zsp_video_user_list);
}

/***********************************************
 *@brief 获取一个空闲的视频播放用户ID
 *@param socket_fd 此用户所使用的 socket fd
 *@retrun
 *      成功：返回 userid   
 *      失败：返回 ZSP_FAILURE -1 用户列表中没有空闲的用户ID
 **********************************************/
static int zsp_get_video_user(int socket_fd)
{
    int userid = 0;
    int ret = ZSP_FAILURE;

    pthread_mutex_lock(&zsp_video_user_mutex);
    for(userid = 0; userid < ZSP_VIDEO_USER_MAX; userid++)
    {
        if(VIDEO_USER_UNUSED == zsp_video_user_list[userid].used)
        {
            zsp_video_user_list[userid].used = VIDEO_USER_USED;
            ret = userid;
            break;
        }
    }
    pthread_mutex_unlock(&zsp_video_user_mutex);

    return ret;
}

/***********************************************
 *@brief 设置当前的用户ID为 VIDEO_USER_UNUSED
 *@param userid 此用户 userid
 *@retrun
 *      成功：返回 ZSP_SUCCESS  0
 *      失败：返回 ZSP_FAILURE -1 
 **********************************************/
static int zsp_release_video_user(int userid)
{
    int ret = ZSP_FAILURE;

    pthread_mutex_lock(&zsp_video_user_mutex);
    if((userid < ZSP_VIDEO_USER_MAX) && (userid >= 0))
    {
        zsp_video_user_list[userid].used = VIDEO_USER_UNUSED;
        ret = ZSP_SUCCESS;
    }
    else
    {
        ZSPERR("userid [%d] error\r\n", userid);
    }
    pthread_mutex_unlock(&zsp_video_user_mutex);

    return ret;
}

/***********************************************
 *@brief 处理开始直播请求
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@param chh 参数4 视频通道，0：高清，1：标清，2：普清
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_live_start(char * data, int len, int socket_fd, int cch)
{
    int ret = 0;
    int retry_times = 0;    /* 发送视频数据失败的重试次数 */
    int get_video_data_retry_times = 0;    /* 从buffer获取视频数据失败的重试次数 */
    unsigned char * video_data = 0; /* 视频数据的地址 */
    int video_len = 0; /* 视频数据的长度 */
    int userid = 0; /* 视频播放用户ID */
    FrameInfo frame_info = {};  /* 视频帧信息，例如 I帧 P帧 帧大小 */
    ParaEncUserInfo uinf;  /* 用户信息 */
    int audio_send_flag = 0;    /* 当前socket链路下是否发送音频，0:不发送音频 1:发送音频 */

    /* 获取一个空闲的视频播放用户ID */
    ret = zsp_get_video_user(socket_fd);
    if(ret == ZSP_FAILURE)
    {
        ZSPERR("用户列表中没有空闲的用户ID ret:%d, socket_fd:%d\r\n", ret, socket_fd);
        return ZSP_FAILURE;
    }
    ZSPINF("获取到空闲的用户ID ret:%d, socket_fd:%d\r\n", ret, socket_fd);
    userid = ret;   /* 视频播放用户ID */

    /* 设置传入获取视频数据接口的参数信息 */
    uinf.ch = cch;    /* 通道，例如 1080P 720P ... */
    uinf.userid = userid;   /* 视频播放用户ID */
    uinf.buffer = &video_data;  /* 视频数据的地址 */
    uinf.frameinfo = &frame_info; /* 视频帧信息，例如 I帧 P帧 帧大小 */

    /* 发送IPC响应开始发送视频数据 */
    STRUCT_START_LIVING_ECHO resp;
    memcpy(&resp.header , data, sizeof(Cmd_Header));
    resp.result = 0;
    resp.header.length = GET_HEADER_LEN(STRUCT_START_LIVING_ECHO);
    ret = send(socket_fd, (char*)&resp, sizeof(resp), 0);
    if(ret <= 0)
    {
        ZSPLOG("发送IPC响应开始发送视频数据失败, socket_fd:%d\r\n", socket_fd);
        zsp_release_video_user(userid);    /* 释放用户ID */
        return ZSP_FAILURE;
    }
    ZSPLOG("发送IPC响应开始发送视频数据成功, socket_fd:%d\r\n", socket_fd);

    ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RESETUSER,(void*)&uinf);
    while(socket_fd > 0)
    {
        /* 检查此 socket_fd 是否可读 */
        ret = __zsp_select_fd_readable(socket_fd, 5);  /* 为尽量减少对视频发送的影响，超时设置为 5ms */
        if(ZSP_SUCCESS == ret)
        {
            /* 返回 ZSP_SUCCESS 说明可读，从PC端工具有数据发过来，接收数据并处理 */
            ret = zsp_tcp_cmd_data_processor(socket_fd, NULL, 0, (void *)&audio_send_flag);
            if(ZSP_SUCCESS == ret)
            {
                /* 返回 ZSP_SUCCESS 说明命令处理成功 */
                ZSPLOG("recv cmd/data and processor success\r\n");
            }
        }

        /* 从接口中获取视频数据 */
        ret = ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_GETFRAME,(void*)&uinf);
        if(ret != 0)
        {
            //ZSPLOG("没有获取到数据: ret:%d, socket_fd:%d\r\n", ret, socket_fd);
            usleep(30*1000); /* 30ms 后再取数据 */
            get_video_data_retry_times ++;  /* 重试次数++ */

            if(get_video_data_retry_times > GET_VIDEO_DATA_RETRY_MAX)
            {
               break;   /* 跳出循环，停止发送视频数据 */
            }
            else
            {
                continue;   /* 继续获取数据 */
            }
        }

        get_video_data_retry_times = 0; /* 获取到数据，重试次数清零 */
        video_len = frame_info.FrmLength; /* 取到的视频数据长度 */

        if(0 >= video_len)
        {
            ZSPERR("取到的[%s]数据长度为:%d\r\n", (1 == frame_info.Flag?"I帧"
                        :(2 == frame_info.Flag?"P帧"
                            :(3 == frame_info.Flag?"音频帧":"什么鬼"))), video_len);
            continue;
        }

        if(3 == frame_info.Flag)    /* 1:I帧, 2:P帧, 3:音频帧*/ 
        {
            if(0 == audio_send_flag)
            {
                continue;
            }
        }

        do{
            /* 开始发送视频数据 */
            ret = send(socket_fd, (char*)video_data, video_len, MSG_DONTWAIT);
            if(ret < 0)
            {
                int tmp_errno = errno;
                if(EAGAIN != tmp_errno)
                {
                    /* frame_info.Flag 1:I帧, 2:P帧, 3:音频帧 */
                    ZSPINF("已重试 %d 次，继续，socket_fd:%d, ret:%d, %s, frame flag:%s\r\n",
                            retry_times, socket_fd, ret, strerror(tmp_errno), 
                            (1 == frame_info.Flag)?"I帧":((2 == frame_info.Flag)?"P帧"
                                :((3 == frame_info.Flag)?"音频帧":"不知道是什么帧")));

                    retry_times ++;
                }

                usleep(10*1000);   /* 等待 10ms 后重试 */
            }
            else if(0 == ret)
            {
                /* 如果ret为0说明TCP客户端已经关闭连接 */
                ZSPLOG("TCP客户端已经关闭连接，socket_fd:%d, %s\r\n", socket_fd, strerror(errno));
                socket_fd = 0;  /* socket_fd 改为0，让循环退出 */
                break;
            }
            else
            {
                /* 如果发送成功则重试次数清0 */
                retry_times = 0; 

                /* 如果发送成功的数据 < 待发送数据，则继续发送剩余的数据 */
                if(ret < video_len)
                {
                    //ZSPLOG("视频数据少发%d字节，video_len:%d, ret:%d, socket_fd:%d\r\n",
                    //        (video_len - ret), video_len, ret, socket_fd);
                    video_len -= ret;   /* video_len 为剩余的视频数据长度 */
                    video_data += ret;  /* video_data 为剩余的视频数据 */
                    continue;
                }
                else
                {
                    break;  /* 如果发送成功的数据 == 待发送数据，说明发送成功 */
                }
                /* 如果发送成功则不打印日志 */
                //ZSPLOG("视频数据发送成功，socket_fd:%d, ret:%d\r\n", socket_fd, ret);
            }
        }while(retry_times < ZSP_CMD_TCP_RETRY_TIMES);

        if(retry_times >= ZSP_CMD_TCP_RETRY_TIMES)
        {
            ZSPLOG("已重试 %d 次，停止发送，socket_fd:%d\r\n", retry_times, socket_fd);
            break;
        }
    }

    zsp_release_video_user(userid);    /* 释放用户ID */
    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 处理指令 CMD_SET_AUDIOSWITCH 0x9066
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@param *arg 参数4 传入参数，void *型，可由各指令自定义类型
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_audioswitch(char * data, int len, int socket_fd, void * arg)
{
    int ret = ZSP_FAILURE;
    int * audio_send_flag = (int *)arg;    /* 当前socket链路下是否发送音频，0:不发送音频 1:发送音频 */
    STRUCT_SET_AUDIO_SWITCH* pReq = (STRUCT_SET_AUDIO_SWITCH*)data;
    STRUCT_SET_AUDIO_SWITCH_RESP resp ;

    ret = __zsp_recv_buffer_from_socket(socket_fd, data+len, pReq->header.length);
    if(ZSP_SUCCESS != ret)
    {
        ZSPERR("recv cmd data error, len:%d, pReq->header.length:%d\r\n", len, pReq->header.length);
        return ZSP_FAILURE;
    }

    len += pReq->header.length;
    ZSPLOG("开始处理 CMD_SET_AUDIOSWITCH 0x9066 \r\n");
    if((NULL != data) && (NULL != audio_send_flag)&& (len >= (int)sizeof(STRUCT_SET_AUDIO_SWITCH)))
    {
        memcpy(&resp.header, &pReq->header, sizeof(Cmd_Header));
        resp.echo = 0;
        ret = __zsp_send_buffer_to_socket(socket_fd, (char *)&resp, sizeof(resp), 500); /* timeout 500ms */
        if(ZSP_SUCCESS != ret)
        {
            ZSPERR("response CMD_SET_AUDIOSWITCH:0x%04x failure\r\n", CMD_SET_AUDIOSWITCH);
        }
        else
        {
            *audio_send_flag = pReq->sendAudio;
            ZSPLOG("response CMD_SET_AUDIOSWITCH:0x%04x success\r\n", CMD_SET_AUDIOSWITCH);
        }
    }
    else
    {
        ZSPERR("response CMD_SET_AUDIOSWITCH:0x%04x failure\r\n", CMD_SET_AUDIOSWITCH);
    }

    return ret; /* 成功，0:ZSP_SUCCESS  失败：-1:ZSP_FAILURE */
}

/***********************************************
 *@brief 处理停止直播请求
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_live_stop(char * data, int len, int socket_fd)
{

    /* 待实现 */
    ZSPLOG("停止发送视频数据\r\n");

    return ZSP_SUCCESS;
}


/***********************************************
 *@brief 处理获取视频加密密钥的消息
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_video_key(char * data, int len, int socket_fd)
{
    int ret = 0;
    char aes_key[36]={0};
    STRUCT_G_VIDEO_KEY_REQ *pReq = (STRUCT_G_VIDEO_KEY_REQ *)data;
    STRUCT_G_VIDEO_KEY_RESP resp;

    ModInterface::GetInstance()->CMD_handle(MOD_ENC, CMD_ENC_GETAESKEY, (void*)aes_key);
    memset(&resp, 0, sizeof(resp));
    memcpy(&resp.header, &pReq->header, sizeof(Cmd_Header));
    resp.header.length = sizeof(resp.key);
    strcpy(resp.key, aes_key);
    ret = send(socket_fd, (char*)&resp, sizeof(resp), 0);
    if(ret <= 0)
    {

        ZSPLOG("发送IPC响应, 获取AES KEY失败\r\n");
    }
    ZSPLOG("发送IPC响应，获取AES KEY成功\r\n");

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 处理获取设备信息的消息
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_dev_info(char * data, int len, int socket_fd)
{
    int ret = 0;
    STRUCT_READ_DEVINFO_REQUEST * pReq = (STRUCT_READ_DEVINFO_REQUEST *)data;
    STRUCT_READ_DEVINFO_ECHO resp;

    memset(&resp, 0, sizeof(resp));
    memcpy(&resp.header, &pReq->header, sizeof(Cmd_Header));
    resp.header.length = sizeof(resp) - sizeof(Cmd_Header);
    /* 获取IPC设备信息，如：软件版本，是否支持音频，是否加密等 */
    zsp_get_software_version(&(resp.devInfo));

    ret = send(socket_fd, (char*)&resp, sizeof(resp), 0);
    if(ret <= 0)
    {
        ZSPLOG("发送IPC响应，获取设备信息失败\r\n");
    }
    ZSPLOG("发送IPC响应，获取设备信息成功\r\n");

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 从设备接口中获取设备音频设置，如采样率、音频格式、位宽、音量、帧长度
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_get_audio_param(Audio_Coder_Param * audio_param)
{
#if 0
    audio_param->samplerate = 0; 
    audio_param->audiotype  = 0; 
    audio_param->enBitwidth = 1; 
    audio_param->inputvolume = 30;  /* 暂用固定值 */
    audio_param->inputvolume = 50;  /* 暂用固定值 */
    audio_param->framelen    = 160; 
#else
    /* 因为原 ZSP 库中结构体变量定义问题，暂时代码中先保留这两种 */
    audio_param->sampleRate = 0;    /* 采样率 0:8K ,1:12K,  2: 11.025K, 3:16K ,4:22.050K ,5:24K ,6:32K ,7:48K */
    audio_param->audioType  = 0;    /* 编码类型0 :g711 ,1:2726 */
    audio_param->enBitwidth = 1;    /* 位宽0 :8 ,1:16 2:32 */
    audio_param->recordVolume = 30; /* 设备录音音量 0-30，暂用固定值 30 */
    audio_param->speakVolume = 30;  /* 设备播放音量 0-30，暂用固定值 30 */
    audio_param->framelen = 160;    /* 音频帧大小(80/160/240/320/480/1024/2048) */
#endif

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 处理获取设备音频设置，如采样率、音频格式、位宽、音量、帧长度
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_get_talk_setting(char * data, int len, int socket_fd)
{
    int ret = -1;
    STRUCT_GET_TALK_SETTING_REQUEST* pReq = (STRUCT_GET_TALK_SETTING_REQUEST*)data;
    STRUCT_GET_TALK_SETTING_ECHO resp ;

    memset(&resp, 0, sizeof(resp));
    memcpy(&resp.header, &pReq->header , sizeof(Cmd_Header));
    resp.header.length = sizeof(resp) - sizeof(Cmd_Header);

//#ifdef SUPPORT_AUDIO
#if 1   /* support audio default */
    //AudioParm audio_param;
    Audio_Coder_Param audio_param;
    memset(&audio_param, 0, sizeof(audio_param));

    /* 目前都用固定值 */
    ret = zsp_get_audio_param(&audio_param);

    if(ZSP_SUCCESS != ret)
    {
        /* 占位，暂不处理 */
    }

#if 0
    resp.audioParam.audioType   = audio_param.audiotype ;
    resp.audioParam.enBitwidth  = audio_param.enBitwidth ;
    resp.audioParam.sampleRate  = audio_param.samplerate ;
    resp.audioParam.recordVolume    = audio_param.inputvolume ;
    resp.audioParam.speakVolume     = audio_param.outputvolume ;
    resp.audioParam.framelen    = audio_param.framelen ;
#else
    /* 因为原 ZSP 库中结构体变量定义问题，暂时代码中先保留这两种 */
    resp.audioParam.audioType   = audio_param.audioType;
    resp.audioParam.enBitwidth  = audio_param.enBitwidth;
    resp.audioParam.sampleRate  = audio_param.sampleRate;
    resp.audioParam.recordVolume= audio_param.recordVolume;
    resp.audioParam.speakVolume = audio_param.speakVolume;
    resp.audioParam.framelen    = audio_param.framelen;
#endif

#endif
    ret = __zsp_send_buffer_to_socket(socket_fd, (char *)&resp, sizeof(resp), 1000);    /* timeout 1000 ms*/
    if(ZSP_SUCCESS != ret)
    {
        ZSPERR("response CMD_G_TALK_SETTING:0x%04x failure\r\n", CMD_G_TALK_SETTING);
    }

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 处理 CMD_TALK_ON 0x9006 打开对讲指令
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_talk_on(char * data, int len, int socket_fd)
{
    int ret = ZSP_FAILURE;
    STRUCT_TALK_ON_REQUEST* pReq = (STRUCT_TALK_ON_REQUEST *)data;
    STRUCT_TALK_ON_ECHO resp;
    memset(&resp , 0, sizeof(resp));

    memcpy(&resp.header, &pReq->header , sizeof(Cmd_Header));
    resp.header.length = sizeof(resp) - sizeof(Cmd_Header);

    /* 0:成功，1:设备正在对讲，2:打开编解码器失败，其他:未知错误 */
    resp.talkFlag = 1;  /* 赋初值为 1 */

    do
    {
#if 0   /* 原来代码中这个是什么意思？ */
        if(!GetNetServerObj()->RequestTalkOn(m_nUserID))
        {
            break ;
        }

#endif

        /* 打开音频解码,0 is g711 */
#if 0
        if(StartAudioDecode(0) != 0)
        {
            /* 0:成功，1:设备正在对讲，2:打开编解码器失败，其他:未知错误 */
            resp.talkFlag = 2;  /* 2:打开编解码器失败 */
            break;
        }
#endif
        Audio_Coder_Param audio_param;
        memset(&audio_param, 0, sizeof(audio_param));

        /* 目前都用固定值 */
        ret = zsp_get_audio_param(&audio_param);
        if(ZSP_SUCCESS != ret)
        {
            /* 占位，暂不处理 */
        }

        resp.audioParam.audioType   = audio_param.audioType;
        resp.audioParam.enBitwidth  = audio_param.enBitwidth;
        resp.audioParam.sampleRate  = audio_param.sampleRate;
        resp.audioParam.recordVolume= audio_param.recordVolume;
        resp.audioParam.speakVolume = audio_param.speakVolume;
        resp.audioParam.framelen    = audio_param.framelen;

        /* 0:成功，1:设备正在对讲，2:打开编解码器失败，其他:未知错误 */
        resp.talkFlag = 0;  /* 0:成功 */

        ret = __zsp_send_buffer_to_socket(socket_fd, (char *)&resp, sizeof(resp), 2000);    /* timeout 2000 ms*/
        if(ZSP_SUCCESS != ret)
        {
            ZSPERR("response CMD_TALK_ON:0x%04x failure, timeout_ms 2000ms\r\n", CMD_TALK_ON);
            break;
        }

        while(1)
        {
            ret = __zsp_select_fd_readable(socket_fd, 5000);    /* timeout_ms 5000ms */
            if(ZSP_SUCCESS == ret)
            {
                //ZSPLOG("CMD_TALK_ON socket_fd [%d] readable \r\n", socket_fd);

                /* 返回 ZSP_SUCCESS 说明可读，从PC端工具有数据发过来，接收数据并处理，0x9008 */
                ret = zsp_tcp_cmd_data_processor(socket_fd, NULL, 0, (void *)NULL);
                if(ZSP_SUCCESS != ret)
                {
                    ZSPERR("recv cmd/data and processor error at CMD_TALK_ON\r\n");
                    break;
                }
            }
            else
            {
                ZSPINF("select CMD_TALK_ON socket_fd [%d] unreadablek, timeout_ms 5000ms\r\n", socket_fd);
                break;
            }
        }

    }while(0);  /* 加 while(0) 只是为了控制以上跳出流程 */

    return ret; /* 成功，0:ZSP_SUCCESS 失败，-1:ZSP_FAILURE */
}

/***********************************************
 *@brief 将音频数据送入音频播放接口，进行声音播放
 *@param data 参数1 音频数据地址
 *@param len  参数2 音频数据的长度
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_audio_play(char * data, int len)
{
    ParaEncAdecInfo padec;
    padec.buffer = (unsigned char *)data;
    padec.len = len;
    padec.block = true;

    /* 放入音频接口中播放 */
    ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_AUDIODECODE,(void*)&padec);

    return ZSP_SUCCESS;
}
/***********************************************
 *@brief 处理指令 CMD_TALK_DATA 0x9008
 *@param data 参数1 ZSP CMD 消息数据
 *@param len  参数2 ZSP CMD 消息长度
 *@param socket_fd 参数3 TCP socket fd
 *@param *arg 参数4 传入参数，void *型，可由各指令自定义类型
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_talk_data(char * data, int len, int socket_fd, void * arg)
{
    //ZSPLOG("recv 0x9008 audio data len:%d\r\n", len);
    /* 接收除头部之外，头部中指定的音频数据 */
    int ret = ZSP_FAILURE;
    STRUCT_TALK_DATA *pReq = (STRUCT_TALK_DATA *)data;
    char * recv_data = NULL;

    /* 为音频数据分配空间 */
    recv_data = (char *)malloc(pReq->header.length);   /* 记得要 free 释放 */
    if(NULL == recv_data)
    {
        ZSPERR("malloc error, pReq->header.length:[%d]\r\n", pReq->header.length);
        return ZSP_FAILURE;
    }
    
    ret = __zsp_recv_buffer_from_socket(socket_fd, recv_data, pReq->header.length);
    if(ZSP_SUCCESS == ret)
    {
        /* 将数据放入音频缓冲接口中进行播放 */
        zsp_audio_play(recv_data, pReq->header.length);

        free(recv_data);    /* 记得要 free 释放 */
        return ZSP_SUCCESS;
    }
    else
    {
        ZSPERR("recv audio data error, header len:%d, pReq->header.length:%d\r\n", len, pReq->header.length);
        free(recv_data);    /* 记得要 free 释放 */
        return ZSP_FAILURE;
    }
}

/***********************************************
 *@brief 获取本机 ip_addr/netmask/mac
 *@param ifname 参数1 网络接口：有线 eth0，无线 wlan0 
 *@param type 参数2 传给 ioctl 的第2个参数，如：SIOCGIFADDR/SIOCGIFNETMASK/SIOCGIFHWADDR
 *@param buf  参数3 存放返回数据的 buffer
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
int zsp_cmd_get_ip_addr(char * if_name, int type, char * buf)
{
    int ret = 0;
    int socket_fd = 0;  
    struct ifreq ifr;  
    unsigned char hwaddr[6] = {0};

    if((NULL == if_name) || (NULL == buf))
    {
        ZSPLOG("传入参数有误，if_name或buf 为 NULL\r\n");
    }

    strcpy(ifr.ifr_name, if_name);  /* 有线 eth0，无线 wlan0 */

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);  
    if(socket_fd <= 0)
    {
        ZSPLOG("获取本机IP地址时创建socket失败，ret:%d\r\n", ret);
        goto error;
    }

    /* SIOCGIFADDR/SIOCGIFNETMASK/SIOCGIFHWADDR */
    ret = ioctl(socket_fd, type, &ifr);  
    if(0 != ret)
    {
        ZSPLOG("获取本机IP地址失败\r\n");
        goto error;
    }

    switch(type)
    {
        case SIOCGIFADDR:
        case SIOCGIFNETMASK:
            strcpy(buf, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));  
            break;

        case SIOCGIFHWADDR:
            memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, sizeof(hwaddr)); 
            sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", 
                    hwaddr[0], hwaddr[1], hwaddr[2],
                    hwaddr[3], hwaddr[4], hwaddr[5]);
            break;

        default:
            ZSPLOG("输入的 type 不正确,type:0x%X\r\n", type);
            goto error;
            break;
    }

    close(socket_fd);
    return ZSP_SUCCESS;


error:
    if(socket_fd > 0)
    {
        close(socket_fd);
    }
    return ZSP_FAILURE;
}

/* 获取软件的版本信息，其他状态信息未完善，此为临时填充的版本 */
/* 获取IPC设备信息，如：软件版本，是否支持音频，是否加密等 */
static int zsp_get_software_version(TYPE_DEVICE_INFO *version)
{

    memcpy(version->DeviceName, ZSP_TEST_DEVNAME, strlen(ZSP_TEST_DEVNAME));
    memcpy(version->SerialNumber, ZSP_TEST_SERIALNUM, strlen(ZSP_TEST_SERIALNUM));
    memcpy(version->HardWareVersion, ZSP_TEST_HARDWARE_VERSION, strlen(ZSP_TEST_HARDWARE_VERSION));
    memcpy(version->SoftWareVersion, ZSP_TEST_SOFTWARE_VERSION, strlen(ZSP_TEST_SOFTWARE_VERSION));
    version->VideoNum = 1;
    version->AudioNum = 1; 
    version->SupportAudioTalk = 1;  
    version->SupportStore = 0;
    version->AlarmInNum = 0;
    version->AlarmOutNum = 0;
    version->SupportWifi = 0;
    version->LocalDeviceCapacity |= 0;

    version->resver|=(1<<1);/*加密为通用功能，所有版本都支持加密*/
    version->resver|=(1<<7);//视频加密
    /* CHIP_TYPE_HI3518E: 0x62000000 */
    /* REL_VERSION_COM: 0x00020000 */
    /* REL_SPECIAL_COM: 0x00000000 */
    version->DeviceType = 0x62000000 + 0x00020000 + 0x00000000;   
    version->DeviceType |= (1<<8); /*P2P支持现在默认都支持*/
    version->DeviceType |= (1<<11); /* 对讲支持 */

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 填充广播数据内容
 *@param *broadcast_info 用于填充广播数据内容
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_ping_fill_broadcast_info(STRUCT_PING_ECHO* broadcast_info)
{
    int ret = 0;
    char ip_addr[ZSP_IP_ADDR_LEN] = {0};
    char netmask[ZSP_IP_ADDR_LEN] = {0};

    if(NULL == broadcast_info)  /* 如果传入 NULL 返回失败 ZSP_FAILURE -1 */
    {
        ZSPLOG("broadcast_info 为 NULL\r\n");
        return ZSP_FAILURE;
    }

    /* 获取IPC设备信息，如：软件版本，是否支持音频，是否加密等 */
    zsp_get_software_version(&(broadcast_info->devInfo));

    /* 获取IPC ip/mac/geteway/submask */
    /* 获取本机IP地址 */
    ret = zsp_cmd_get_ip_addr(ZSP_IF_NAME, SIOCGIFADDR, broadcast_info->ipAddr.ipaddr);
    if(ZSP_SUCCESS != ret)
    {
        ZSPLOG("获取本机IP地址失败\r\n");
    }

    /* 获取本机子网掩码 */
    ret = zsp_cmd_get_ip_addr(ZSP_IF_NAME, SIOCGIFNETMASK, broadcast_info->ipAddr.submask);
    if(ZSP_SUCCESS != ret)
    {
        ZSPLOG("获取本机子网掩码失败\r\n");
    }

    /* 获取本机MAC地址*/
    ret = zsp_cmd_get_ip_addr(ZSP_IF_NAME, SIOCGIFHWADDR, broadcast_info->ipAddr.mac);
    if(ZSP_SUCCESS != ret)
    {
        ZSPLOG("获取本机MAC地址失败\r\n");
    }

    broadcast_info->portInfo.webPort = 80;
    broadcast_info->portInfo.videoPort = 8000;
    broadcast_info->portInfo.phonePort = 9000;


    /* 设置 CMD_PING 0x9001 响应的消息头 */
    broadcast_info->header.head   = 0xaaaa5555; /* ZSP 协议的头部标识 */
    broadcast_info->header.commd  = CMD_PING;
    broadcast_info->header.length = sizeof(STRUCT_PING_ECHO) - sizeof( Cmd_Header );

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 处理 CMD_ID_PING 命令 0x9050
 *@param udp_thpool_arg 参数1 UDP命令处理线程传入的参数地址
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_cmd_processor_id_ping(STRUCT_UDP_THPOOL_ARG * udp_thpool_arg)
{
    int ret = 0;
    STRUCT_ID_PING_ECHO resp;

    //ZSPLOG("开始处理 CMD_ID_PING 0x9050\r\n");
    /* 检查传入命令数据长度 */
    if(udp_thpool_arg->cmd_len < sizeof(Cmd_Header))
    {
        ZSPLOG("传入的命令长度 %d 小于头部Cmd_Header %d 的大小\r\n", 
                udp_thpool_arg->cmd_len, sizeof(Cmd_Header));
        return ZSP_FAILURE;
    }

    memset(&resp, 0, sizeof(STRUCT_ID_PING_ECHO));  /* 数据清0 */

    /* 填充广播数据内容 */
    ret = zsp_cmd_ping_fill_broadcast_info(&resp.device);
    if(ZSP_SUCCESS != ret)
    {
        ZSPLOG("填充广播数据内容出错，ret:%d\r\n", ret);
    }

    resp.device.header.length = sizeof(STRUCT_ID_PING_ECHO) - sizeof(Cmd_Header);
    strncpy(resp.deviceID, DEVICE_ID_TEST, 15);

    udp_thpool_arg->from_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    ret = sendto(udp_thpool_arg->socket_fd, &resp, sizeof(STRUCT_ID_PING_ECHO), 
            0, (sockaddr *)&(udp_thpool_arg->from_addr), sizeof(struct sockaddr_in));
    if(ret <= 0)
    {
        ZSPLOG("CMD_ID_PING 0x9050 命令响应失败,ret:%d\r\n", ret);
    }
    else
    {
        ZSPLOG("CMD_ID_PING 0x9050 命令响应成功,ret:%d\r\n", ret);
    }

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief ZSP TCP服务器专用于处理消息的线程池回调函数
 *@param arg 参数1 线程参数，需要在些手动 free
 *@retrun
 *      成功：返回 socket fd
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
void zsp_tcp_cmd_processor(void * arg)
{
    int ret = 0;
    int retry_times = 0;
    int socket_fd = 0;  /* TCP socket */
    char cmd_buf[ZSP_CMD_TCP_BUF_SIZE] = {0};    /* TCP socket 数据接收 buffer */
    int cmd_buf_len = 0;    /* 表示已接收到的字节数，并用于标记当前 cmd_buf 的接收位置 */
    Cmd_Header * cmd_header = (Cmd_Header *)NULL;

    if(NULL == arg)
    {
       ZSPLOG("传入线程的参数为 NULL，直接退出\r\n");
       return;
    }

    socket_fd = *((int*)arg);
    free(arg);  /* 直接释放，前面已判断是否为 NULL */
    ZSPINF("传入线程的参数 socket_fd = %d\r\n", socket_fd);

    /* 数组定义时已经清零，不需要再 memset 0 */
    //memset(cmd_buf, 0, sizeof(cmd_buf));

    /* socket_fd 为阻塞，开始 recv 接收数据 */
    for(retry_times = 0; retry_times < ZSP_CMD_TCP_RETRY_TIMES; retry_times ++)
    {
        /* 多次接收，cmd_buf 的起始位置需要相应增加 */
        ret = recv(socket_fd, cmd_buf+cmd_buf_len, ZSP_CMD_TCP_BUF_SIZE-cmd_buf_len, 0);
        if(ret < 0)
        {
            /* 出错， usleep 200ms 重试 */
            ZSPERR("接收数据出错，usleep 50ms后重试 socket_fd:%d\r\n", socket_fd);
            usleep(50*1000);
            continue;
        }
        else if(ret == 0)
        {
            /* 如果 ret 为 0，说明 TCP 客户端连接已关闭socket，退出 */
            ZSPERR("接收数据出错，TCP 客户端连接已关闭 socket_fd:%d\r\n", socket_fd);
            break;
        }
        else
        {
            /* 说明收到 ret 字节的数据, cmd_buf_len 记录已接收到的字节数 */
            cmd_buf_len += ret;
            ZSPINF("ret:%d，已收到 %d 字节，socket_fd:%d\r\n", ret, cmd_buf_len, socket_fd);
            /* 如果收到的数据大于Cmd_Header，则开始处理消息 */
            if(cmd_buf_len >= sizeof(Cmd_Header))
            {
                break;
            }
        }
    }

    //ZSPLOG("已收到字节数 cmd_buf_len:%d\r\n", cmd_buf_len);
    /* 如果接收到的数据 大于 Cmd_Header 才处理 */
    if(cmd_buf_len >= sizeof(Cmd_Header))   /* 收到的数据 >= Cmd_Header 才进行处理 */
    {
        cmd_header = (Cmd_Header *)&cmd_buf;
        /* 调试，打印头部数据*/
#if 1
        ZSPLOG("cmd_header->head:0x%x\r\n", cmd_header->head);
        ZSPLOG("cmd_header->length:%d\r\n", cmd_header->length);
        ZSPLOG("cmd_header->type:%d\r\n",   cmd_header->type);
        ZSPLOG("cmd_header->channel:%d\r\n", cmd_header->channel);
        ZSPINF("\033[34mcmd_header->commd:0x%x\033[0m\r\n", cmd_header->commd);
#endif
        switch(cmd_header->commd)
        {
			//case CMD_START_720P:    /* 0x5000 */
            case CMD_START_1080P:	/* 0x5000 */
				/* 处理直播请求 */
                zsp_cmd_processor_live_start(cmd_buf, cmd_buf_len, socket_fd, 0);
                break;
            case CMD_START_VGA:		/* 0x9002 */
				/* 处理直播请求 */
                zsp_cmd_processor_live_start(cmd_buf, cmd_buf_len, socket_fd, 1);
                break;
            case CMD_START_QVGA:	/* 0x90a2 */
                /* 处理直播请求 */
                zsp_cmd_processor_live_start(cmd_buf, cmd_buf_len, socket_fd, 2);
                break;
            case CMD_STOP_VIDEO:    /* 0x9003 */
                /* 停止直播 */
                zsp_cmd_processor_live_stop(cmd_buf, cmd_buf_len, socket_fd);
                break;
            case CMD_G_VIDEO_KEY:   /* 0x9636 */
                /* 获取IPC视频加密所用的 key */
                zsp_cmd_processor_video_key(cmd_buf, cmd_buf_len, socket_fd);
                break;
            case CMD_R_DEV_INFO:    /* 0x9800 */
                /* 获取IPC设备信息，如：软件版本，是否支持音频，是否加密等 */
                zsp_cmd_processor_dev_info(cmd_buf, cmd_buf_len, socket_fd);
                break;
            case CMD_G_TALK_SETTING: /* 0x9060 */          
                /* 获取设备音频设置，如采样率、音频格式、位宽、音量、帧长度 */
                zsp_cmd_processor_get_talk_setting(cmd_buf, cmd_buf_len, socket_fd);
                break;
            case CMD_TALK_ON: /* 0x9006 */          
                /* 打开对讲，如果成功，则PC端工具往IPC端发送音频数据 */
                zsp_cmd_processor_talk_on(cmd_buf, cmd_buf_len, socket_fd);
                break;
            default:
                ZSPERR("\033[34mUnsupported commd:%d, socket_fd:%d\033[0m\r\n", cmd_header->commd, socket_fd);
                break;
        }
    }

    ZSPLOG("线程结束，关闭：socket_fd:%d\r\n", socket_fd);
    if(socket_fd > 0)
    {
        /* 关闭 socket */
        close(socket_fd);
        socket_fd = 0;
    }

    return;
}

/***********************************************
 *@brief 阻塞处理 ZSP TCP 连接收发数据时收到的指令、数据、音频、文件等
         区别于 zsp_tcp_cmd_processor() 只处理TCP服务器有新建连接时收到的指令
 *@param socket_fd 参数1 对应链路的 socket fd
 *@param buf 参数2 已收到的数据
 *@param len 参数3 已收到的数据大小
 *@param *arg 参数4 传入参数，void *型，可由各指令自定义类型
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_tcp_cmd_data_processor(int socket_fd, char * buf, int len, void * arg)
{
    int ret = 0;
    int retry_times = 0;
    char cmd_buf[ZSP_CMD_TCP_BUF_SIZE] = {0};    /* TCP socket 数据接收 buffer */
    int cmd_buf_len = 0;    /* 表示已接收到的字节数，并用于标记当前 cmd_buf 的接收位置 */
    Cmd_Header * cmd_header = (Cmd_Header *)NULL;
    int cmd_header_len = sizeof(Cmd_Header);

    /* recv 接收PC端工具发过来的指令，非阻塞，重试 ZSP_CMD_TCP_RETRY_TIMES 3 次 */
    for(retry_times = 0; retry_times < ZSP_CMD_TCP_RETRY_TIMES; retry_times ++)
    {
        /* 多次接收，cmd_buf 的起始位置需要相应增加 */
        //ret = recv(socket_fd, cmd_buf+cmd_buf_len, ZSP_CMD_TCP_BUF_SIZE-cmd_buf_len, 0);
        ret = recv(socket_fd, cmd_buf+cmd_buf_len, cmd_header_len-cmd_buf_len, 0);
        if(ret < 0)
        {
            /* 出错， usleep 10ms 重试 */
            //ZSPERR("接收数据出错，usleep 50ms后重试 socket_fd:%d\r\n", socket_fd);
            ZSPERR("接收数据出错，socket_fd:%d, continue\r\n", socket_fd);
            //usleep(10*1000);
            continue;
        }
        else if(ret == 0)
        {
            /* 如果 ret 为 0，说明 TCP 客户端连接已关闭socket，退出 */
            ZSPERR("接收数据出错，TCP 客户端连接已关闭 socket_fd:%d\r\n", socket_fd);
            break;
        }
        else
        {
            /* 说明收到 ret 字节的数据, cmd_buf_len 记录已接收到的字节数 */
            cmd_buf_len += ret;
            //ZSPLOG("ret:%d，已收到 %d 字节，socket_fd:%d\r\n", ret, cmd_buf_len, socket_fd);
            /* 如果收到的数据大于Cmd_Header，则开始处理消息 */
            if(cmd_buf_len >= sizeof(Cmd_Header))
            {
                break;
            }
        }
    }

    if(cmd_buf_len >= sizeof(Cmd_Header))   /* 收到的数据 >= Cmd_Header 才进行处理 */
    {
        cmd_header = (Cmd_Header *)&cmd_buf;
        /* 调试，打印头部数据*/
#if 1
        if(CMD_TALK_DATA != cmd_header->commd)
        {
            ZSPLOG("cmd_header->head:0x%x\r\n", cmd_header->head);
            ZSPLOG("cmd_header->length:%d\r\n", cmd_header->length);
            ZSPLOG("cmd_header->type:%d\r\n",   cmd_header->type);
            ZSPLOG("cmd_header->channel:%d\r\n", cmd_header->channel);
            ZSPINF("\033[34mcmd_header->commd:0x%x\033[0m\r\n", cmd_header->commd);
        }
#endif
        switch(cmd_header->commd)
        {
            case CMD_SET_AUDIOSWITCH:
                zsp_cmd_processor_audioswitch(cmd_buf, cmd_buf_len, socket_fd, arg);
                break;
            case CMD_TALK_DATA:
                zsp_cmd_processor_talk_data(cmd_buf, cmd_buf_len, socket_fd, arg);
                break;

            default:
                ZSPERR("\033[34mUnsupported commd:%d, socket_fd:%d\033[0m\r\n", cmd_header->commd, socket_fd);
                break;
        }
    }
    else
    {
        ZSPERR("recv cmd/data from tcp client error\r\n");
        return ZSP_FAILURE;
    }

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief ZSP UDP服务器专用于处理UDP广播消息的线程池回调函数
 *@param arg 参数1 线程参数，需要在此函数手动 free
 *@retrun
 *      成功：返回 socket fd
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
void zsp_udp_cmd_processor(void * arg)
{
    STRUCT_UDP_THPOOL_ARG * udp_thpool_arg = NULL;
    Cmd_Header * cmd_header = (Cmd_Header *)NULL;

    if(NULL == arg)
    {
        /* 如果传入参数为 NULL 直接返回 */
        ZSPLOG("传入的 arg 为 NULL\r\n");
        return;
    }

    udp_thpool_arg = (STRUCT_UDP_THPOOL_ARG *)arg;

    ZSPLOG("cmd_len:%d\r\n", udp_thpool_arg->cmd_len);
    if(udp_thpool_arg->cmd_len >= sizeof(Cmd_Header))   /* 收到的数据 >= Cmd_Header 才进行处理 */
    {
        cmd_header = (Cmd_Header *)udp_thpool_arg->cmd_buf;

        ZSPLOG("打印收到的数据内容:\r\n");
        printf("cmd_header->head:0x%x\r\n",  cmd_header->head);
        printf("cmd_header->length:%d\r\n",  cmd_header->length);
        printf("cmd_header->type:%d\r\n",    cmd_header->type);
        printf("cmd_header->channel:%d\r\n", cmd_header->channel);
        printf("cmd_header->commd:0x%x\r\n", cmd_header->commd);

        switch(cmd_header->commd)
        {
            case CMD_ID_PING:   /* 0x9050 */
                zsp_cmd_processor_id_ping(udp_thpool_arg);
                break;

            case CMD_PING:   /* 0x9001 */
                ZSPLOG("收到 CMD_PING：0x9001 未实现处理\r\n");
                break;
            default:
                break;
        }   /* switch(cmd_header->commd) 结束 */
    }   /* if(udp_thpool_arg->cmd_len >= sizeof(Cmd_Header)) 结束 */

    /* 释放 udp_thpool_arg，函数在此之前不可以 return，否则会内存泄漏 */
    if(udp_thpool_arg->cmd_buf != NULL)
    {
        free(udp_thpool_arg->cmd_buf);
        udp_thpool_arg->cmd_buf = NULL;
    }

    if(NULL != udp_thpool_arg)
    {
        free(udp_thpool_arg);
        udp_thpool_arg = NULL;
    }

    return;
}

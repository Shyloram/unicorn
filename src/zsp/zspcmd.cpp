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

#include "zspcmd.h"
#include "zsp.h"
#include "modinterface.h"
#include "buffermanage.h"
#include "audio.h"

static int zsp_cmd_processor_audioswitch(char * data, int len, int socket_fd, void * arg);
static int zsp_cmd_processor_talk_data(char * data, int len, int socket_fd, void * arg);

static int GetSoftwareVersion(TYPE_DEVICE_INFO *version)
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

    return 0;
}

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
        ret = recv(socket_fd, cmd_buf+cmd_buf_len, cmd_header_len-cmd_buf_len, 0);
        if(ret < 0)
        {
            ZSPERR("recv error,socket_fd:%d, continue\r\n", socket_fd);
            continue;
        }
        else if(ret == 0)
        {
            ZSPERR("tcp server close socket_fd:%d\r\n", socket_fd);
            break;
        }
        else
        {
            cmd_buf_len += ret;
            if(cmd_buf_len >= sizeof(Cmd_Header))
            {
                break;
            }
        }
    }

    if(cmd_buf_len >= sizeof(Cmd_Header))
    {
        cmd_header = (Cmd_Header *)&cmd_buf;
        if(CMD_TALK_DATA != cmd_header->commd)
        {
            ZSPLOG("cmd_header->head:0x%x\n", cmd_header->head);
            ZSPLOG("cmd_header->length:%d\n", cmd_header->length);
            ZSPLOG("cmd_header->type:%d\n",   cmd_header->type);
            ZSPLOG("cmd_header->channel:%d\n", cmd_header->channel);
            ZSPLOG("cmd_header->commd:0x%x\n", cmd_header->commd);
        }
        switch(cmd_header->commd)
        {
            case CMD_SET_AUDIOSWITCH:
                zsp_cmd_processor_audioswitch(cmd_buf, cmd_buf_len, socket_fd, arg);
                break;
            case CMD_TALK_DATA:
                zsp_cmd_processor_talk_data(cmd_buf, cmd_buf_len, socket_fd, arg);
                break;

            default:
                ZSPERR("Unsupported commd:%d, socket_fd:%d\n", cmd_header->commd, socket_fd);
                break;
        }
    }
    else
    {
        ZSPERR("recv cmd/data from tcp client error\n");
        return -1;
    }
    return 0;
}

static int SelectFdReadable(int socket_fd, int timeout_ms)
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
        return 0;
    }
    return -1;
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
        return 0;
    }

    ZSPLOG("socket_fd is not writable\r\n");
    return -1;
}

static int __zsp_send_buffer_to_socket(int socket_fd, char * buf, int len, int timeout_ms)
{
    int ret = 0;
    int total_len = 0;      /* 已发送的数据总字节数 */
    int retry_times = 0;    /* 发送失败的重试次数 */

    if((NULL == buf) || (len <= 0))
    {
        ZSPERR("buf is NULL or len %d <= 0", len);
        return -1;
    }

    do
    {
        retry_times ++;     /* 重试次数 ++ */
        /* len-total_len 为未发送的剩余的字节数 */
        ret = send(socket_fd, buf+total_len, len-total_len, 0);
        if(ret < 0)
        {
            int errno_tmp = errno;
            ZSPERR("send fd:%d, errno:%d, err:%s\r\n", socket_fd, errno_tmp, strerror(errno_tmp));

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
                    return -1;
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
                return -1;
            }
        }
        else if(0 == ret)
        {
            ZSPERR("socket closed, ret:%d\r\n", ret);
            return -1;
        }
        
        retry_times = 0;
        total_len += ret;
        ZSPLOG("zsp send buffer success, ret:%d, total_len:%d, len:%d\r\n", ret, total_len, len);
    }while(total_len < len);

    return 0;
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
        return -1;
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
                return -1;
            }
        }
        else if(0 == ret)
        {
            ZSPERR("socket closed, ret:%d\r\n", ret);
            return -1;
        }

        retry_times = 0;
        total_len += ret;
        //ZSPLOG("zsp recv buffer success, ret:%d, total_len:%d, len:%d\r\n", ret, total_len, len);
    }while(total_len < len);
    //ZSPLOG("zsp recv total_len:%d, len:%d\r\n", total_len, len);

    return 0;
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
    int ret = -1;
    int * audio_send_flag = (int *)arg;    /* 当前socket链路下是否发送音频，0:不发送音频 1:发送音频 */
    STRUCT_SET_AUDIO_SWITCH* pReq = (STRUCT_SET_AUDIO_SWITCH*)data;
    STRUCT_SET_AUDIO_SWITCH_RESP resp = {};

    ret = __zsp_recv_buffer_from_socket(socket_fd, data+len, pReq->header.length);
    if(0 != ret)
    {
        ZSPERR("recv cmd data error, len:%d, pReq->header.length:%d\r\n", len, pReq->header.length);
        return -1;
    }

    len += pReq->header.length;
    ZSPLOG("开始处理 CMD_SET_AUDIOSWITCH 0x9066 \r\n");
    if((NULL != data) && (NULL != audio_send_flag)&& (len >= (int)sizeof(STRUCT_SET_AUDIO_SWITCH)))
    {
        memcpy(&resp.header, &pReq->header, sizeof(Cmd_Header));
        resp.echo = 0;
        ret = __zsp_send_buffer_to_socket(socket_fd, (char *)&resp, sizeof(resp), 500); /* timeout 500ms */
        if(0 != ret)
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

static int StopLive(char * data, int len, int socket_fd)
{
	//无意义
    ZSPLOG("stop live\n");
    return 0;
}


static int GetAesKey(char * data, int len, int socket_fd)
{
    int ret = 0;
    char aes_key[36]={0};
    STRUCT_G_VIDEO_KEY_REQ *pReq = (STRUCT_G_VIDEO_KEY_REQ *)data;
    STRUCT_G_VIDEO_KEY_RESP resp = {};

    ModInterface::GetInstance()->CMD_handle(MOD_ENC, CMD_ENC_GETAESKEY, (void*)aes_key);
    memcpy(&resp.header, &pReq->header, sizeof(Cmd_Header));
    resp.header.length = sizeof(resp.key);
    strcpy(resp.key, aes_key);
    ret = send(socket_fd, (char*)&resp, sizeof(resp), 0);
    if(ret <= 0)
    {
        ZSPERR("send aes key error\n");
    }
    ZSPLOG("send aes key success\n");

    return 0;
}

static int GetDevInfo(char * data, int len, int socket_fd)
{
    int ret = 0;
    STRUCT_READ_DEVINFO_REQUEST * pReq = (STRUCT_READ_DEVINFO_REQUEST *)data;
    STRUCT_READ_DEVINFO_ECHO resp = {};

    memcpy(&resp.header, &pReq->header, sizeof(Cmd_Header));
    resp.header.length = sizeof(resp) - sizeof(Cmd_Header);
    /* 获取IPC设备信息，如：软件版本，是否支持音频，是否加密等 */
    GetSoftwareVersion(&(resp.devInfo));

    ret = send(socket_fd, (char*)&resp, sizeof(resp), 0);
    if(ret <= 0)
    {
        ZSPERR("send dev info failed\n");
    }
    ZSPLOG("send dec info success\n");

    return 0;
}

static int GetAudioParam(Audio_Coder_Param * audio_param)
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
    return 0;
}

static int GetTalkSetting(char * data, int len, int socket_fd)
{
    int ret = -1;
    STRUCT_GET_TALK_SETTING_REQUEST* pReq = (STRUCT_GET_TALK_SETTING_REQUEST*)data;
    STRUCT_GET_TALK_SETTING_ECHO resp  = {};

    memcpy(&resp.header, &pReq->header , sizeof(Cmd_Header));
    resp.header.length = sizeof(resp) - sizeof(Cmd_Header);

//#ifdef SUPPORT_AUDIO
#if 1   /* support audio default */
    //AudioParm audio_param;
    Audio_Coder_Param audio_param = {};

    /* 目前都用固定值 */
    ret = GetAudioParam(&audio_param);

    if(0 != ret)
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
    if(0 != ret)
    {
        ZSPERR("response CMD_G_TALK_SETTING:0x%04x failure\r\n", CMD_G_TALK_SETTING);
    }

    return 0;
}

static int zsp_cmd_processor_talk_on(char * data, int len, int socket_fd)
{
    int ret = -1;
    STRUCT_TALK_ON_REQUEST* pReq = (STRUCT_TALK_ON_REQUEST *)data;
    STRUCT_TALK_ON_ECHO resp = {};

    memcpy(&resp.header, &pReq->header , sizeof(Cmd_Header));
    resp.header.length = sizeof(resp) - sizeof(Cmd_Header);

    /* 0:成功，1:设备正在对讲，2:打开编解码器失败，其他:未知错误 */
    resp.talkFlag = 1;  /* 赋初值为 1 */

    do
    {
        Audio_Coder_Param audio_param = {};

        /* 目前都用固定值 */
        ret = GetAudioParam(&audio_param);
        if(0 != ret)
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
        if(0 != ret)
        {
            ZSPERR("response CMD_TALK_ON:0x%04x failure, timeout_ms 2000ms\r\n", CMD_TALK_ON);
            break;
        }

        while(1)
        {
            ret = SelectFdReadable(socket_fd, 5000);    /* timeout_ms 5000ms */
            if(0 == ret)
            {
                //ZSPLOG("CMD_TALK_ON socket_fd [%d] readable \r\n", socket_fd);

                /* 返回 ZSP_SUCCESS 说明可读，从PC端工具有数据发过来，接收数据并处理，0x9008 */
                ret = zsp_tcp_cmd_data_processor(socket_fd, NULL, 0, (void *)NULL);
                if(0 != ret)
                {
                    ZSPERR("recv cmd/data and processor error at CMD_TALK_ON\r\n");
                    break;
                }
            }
            else
            {
                ZSPLOG("select CMD_TALK_ON socket_fd [%d] unreadablek, timeout_ms 5000ms\r\n", socket_fd);
                break;
            }
        }

    }while(0); 

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

    return 0;
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
    int ret = -1;
    STRUCT_TALK_DATA *pReq = (STRUCT_TALK_DATA *)data;
    char * recv_data = NULL;

    /* 为音频数据分配空间 */
    recv_data = (char *)malloc(pReq->header.length);   /* 记得要 free 释放 */
    if(NULL == recv_data)
    {
        ZSPERR("malloc error, pReq->header.length:[%d]\r\n", pReq->header.length);
        return -1;
    }
    
    ret = __zsp_recv_buffer_from_socket(socket_fd, recv_data, pReq->header.length);
    if(0 == ret)
    {
        /* 将数据放入音频缓冲接口中进行播放 */
        zsp_audio_play(recv_data, pReq->header.length);

        free(recv_data);    /* 记得要 free 释放 */
        return 0;
    }
    else
    {
        ZSPERR("recv audio data error, header len:%d, pReq->header.length:%d\r\n", len, pReq->header.length);
        free(recv_data);    /* 记得要 free 释放 */
        return -1;
    }
}

static int StartLive(char * data, int len, int socket_fd, int ch)
{
    int ret = 0;
    int retry_times = 0;    /* 发送视频数据失败的重试次数 */
    int get_video_data_retry_times = 0;    /* 从buffer获取视频数据失败的重试次数 */
    unsigned char * video_data = 0; /* 视频数据的地址 */
    int video_len = 0; /* 视频数据的长度 */
    FrameInfo frame_info = {};  /* 视频帧信息，例如 I帧 P帧 帧大小 */
    ParaEncUserInfo uinf;  /* 用户信息 */
    int audio_send_flag = 0;    /* 当前socket链路下是否发送音频，0:不发送音频 1:发送音频 */

    /* 设置传入获取视频数据接口的参数信息 */
    uinf.ch = ch;    /* 通道，例如 1080P 720P ... */
    uinf.buffer = &video_data;  /* 视频数据的地址 */
    uinf.frameinfo = &frame_info; /* 视频帧信息，例如 I帧 P帧 帧大小 */

    /* 发送IPC响应开始发送视频数据 */
    STRUCT_START_LIVING_ECHO resp = {};
    memcpy(&resp.header , data, sizeof(Cmd_Header));
    resp.result = 0;
    resp.header.length = GET_HEADER_LEN(STRUCT_START_LIVING_ECHO);
    ret = send(socket_fd, (char*)&resp, sizeof(resp), 0);
    if(ret <= 0)
    {
        ZSPERR("send error, socket_fd:%d\n", socket_fd);
        return -1;
    }

	ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_REGISTERUSER,(void*)&uinf);
    ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RESETUSER,(void*)&uinf);

    while(socket_fd > 0)
    {
        /* 检查此 socket_fd 是否可读 */
        ret = SelectFdReadable(socket_fd, 5);  /* 为尽量减少对视频发送的影响，超时设置为 5ms */
        if(0 == ret)
        {
            ret = zsp_tcp_cmd_data_processor(socket_fd, NULL, 0, (void *)&audio_send_flag);
            if(0 == ret)
            {
                ZSPLOG("recv cmd/data and processor success\r\n");
            }
        }

        /* 从接口中获取视频数据 */
        ret = ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_GETFRAME,(void*)&uinf);
        if(ret != 0)
        {
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
            ZSPERR("video_len:%d\n", video_len);
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
                    ZSPLOG("retry %d ,socket_fd:%d,ret:%d,%s,frame flag:%s\r\n",
                            retry_times, socket_fd, ret, strerror(tmp_errno), 
                            (1 == frame_info.Flag)?"I":((2 == frame_info.Flag)?"P"
                                :((3 == frame_info.Flag)?"A":"?")));

                    retry_times ++;
                }

                usleep(10*1000);   /* 等待 10ms 后重试 */
            }
            else if(0 == ret)
            {
                /* 如果ret为0说明TCP客户端已经关闭连接 */
                ZSPLOG("TCP server is close,socket_fd:%d, %s\n", socket_fd, strerror(errno));
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
            ZSPERR("retry %d ,stop send ,socket_fd:%d\n", retry_times, socket_fd);
            break;
        }
    }

	ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RELEASEUSER,(void*)&(uinf));
    return 0;
}

void *TcpCmdProcessor(void * arg)
{
    int ret = 0;
    int retry_times = 0;
    int socket_fd = 0;  
    char cmd_buf[ZSP_CMD_TCP_BUF_SIZE] = {0};
    int cmd_buf_len = 0;
    Cmd_Header * cmd_header = (Cmd_Header *)NULL;

    if(NULL == arg)
    {
       ZSPERR("arg is NULL\n");
       return NULL;
    }

    socket_fd = *((int*)arg);
    free(arg); 

    /* socket_fd 为阻塞，开始 recv 接收数据 */
    for(retry_times = 0; retry_times < ZSP_CMD_TCP_RETRY_TIMES; retry_times ++)
    {
        /* 多次接收，cmd_buf 的起始位置需要相应增加 */
        ret = recv(socket_fd, cmd_buf+cmd_buf_len, ZSP_CMD_TCP_BUF_SIZE-cmd_buf_len, 0);
        if(ret < 0)
        {
            ZSPERR("recv error,usleep 50ms retry, socket_fd:%d\r\n", socket_fd);
            usleep(50*1000);
            continue;
        }
        else if(ret == 0)
        {
            /* 如果 ret 为 0，说明 TCP 客户端连接已关闭socket，退出 */
            ZSPERR("tcp server is close, socket_fd:%d\r\n", socket_fd);
            break;
        }
        else
        {
            /* 说明收到 ret 字节的数据, cmd_buf_len 记录已接收到的字节数 */
            cmd_buf_len += ret;
            ZSPLOG("ret:%d，recv %d Byte，socket_fd:%d\r\n", ret, cmd_buf_len, socket_fd);
            /* 如果收到的数据大于Cmd_Header，则开始处理消息 */
            if(cmd_buf_len >= sizeof(Cmd_Header))
            {
                break;
            }
        }
    }

    /* 如果接收到的数据 大于 Cmd_Header 才处理 */
    if(cmd_buf_len >= sizeof(Cmd_Header))   
    {
        cmd_header = (Cmd_Header *)&cmd_buf;
        /* 调试，打印头部数据*/
        ZSPLOG("cmd_header->head:0x%x\n", cmd_header->head);
        ZSPLOG("cmd_header->length:%d\n", cmd_header->length);
        ZSPLOG("cmd_header->type:%d\n",   cmd_header->type);
        ZSPLOG("cmd_header->channel:%d\n", cmd_header->channel);
        ZSPLOG("cmd_header->commd:0x%x\n", cmd_header->commd);

        switch(cmd_header->commd)
        {
            case CMD_START_HD:	    /* 0x5000 */
                StartLive(cmd_buf, cmd_buf_len, socket_fd, 0);
                break;

            case CMD_START_MD:		/* 0x9002 */
                StartLive(cmd_buf, cmd_buf_len, socket_fd, 1);
                break;

            case CMD_START_LD:   	/* 0x90a2 */
                StartLive(cmd_buf, cmd_buf_len, socket_fd, 2);
                break;

            case CMD_STOP_VIDEO:    /* 0x9003 */
                StopLive(cmd_buf, cmd_buf_len, socket_fd);
                break;

            case CMD_G_VIDEO_KEY:   /* 0x9636 */
                GetAesKey(cmd_buf, cmd_buf_len, socket_fd);
                break;

            case CMD_R_DEV_INFO:    /* 0x9800 */
                GetDevInfo(cmd_buf, cmd_buf_len, socket_fd);
                break;

            case CMD_G_TALK_SETTING: /* 0x9060 */          
                GetTalkSetting(cmd_buf, cmd_buf_len, socket_fd);
                break;

            case CMD_TALK_ON: /* 0x9006 */          
                zsp_cmd_processor_talk_on(cmd_buf, cmd_buf_len, socket_fd);
                break;

            default:
                ZSPERR("Unsupported commd:0x%x, socket_fd:%d\n", cmd_header->commd, socket_fd);
                break;
        }
    }

    ZSPLOG("close socket_fd:%d\n\n", socket_fd);
	close(socket_fd);
    return NULL;
}

/**********************UDP SERVER**************************/

int GetIpAddr(char *if_name, char *ip, char *mask, char *mac)
{
    int ret = 0;
    int socket_fd = 0;  
    struct ifreq ifr;  
    unsigned char hwaddr[6] = {0};

    if((NULL == if_name) || (NULL == ip) || (NULL == mask) || (NULL == mac))
    {
        ZSPERR("input arg is NULL\n");
    }

    strcpy(ifr.ifr_name, if_name);

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);  
    if(socket_fd <= 0)
    {
        ZSPLOG("socket error ret:%d\n", ret);
		return -1;
    }

    ret = ioctl(socket_fd, SIOCGIFADDR, &ifr);  
    if(0 != ret)
    {
        ZSPERR("ioctl error\r\n");
		close(socket_fd);
		return -1;
    }
	strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));  

    ret = ioctl(socket_fd, SIOCGIFNETMASK, &ifr);  
    if(0 != ret)
    {
        ZSPERR("ioctl error\r\n");
		close(socket_fd);
		return -1;
    }
	strcpy(mask, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));  

    ret = ioctl(socket_fd, SIOCGIFHWADDR, &ifr);  
    if(0 != ret)
    {
        ZSPERR("ioctl error\r\n");
		close(socket_fd);
		return -1;
    }
	memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, sizeof(hwaddr)); 
	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
			hwaddr[0], hwaddr[1], hwaddr[2],
			hwaddr[3], hwaddr[4], hwaddr[5]);

    close(socket_fd);
    return 0;
}

static int FillBroadcastInfo(STRUCT_PING_ECHO* info)
{
    int ret = 0;

    if(NULL == info)
    {
        ZSPERR("info is NULL\r\n");
        return -1;
    }

    GetSoftwareVersion(&(info->devInfo));

    /* 获取IPC ip/mac/geteway/submask */
    ret = GetIpAddr(ZSP_IF_NAME, info->ipAddr.ipaddr, info->ipAddr.submask, info->ipAddr.mac);
    if(0 != ret)
    {
        ZSPLOG("get ip addr error\r\n");
    }

    info->portInfo.webPort = 80;
    info->portInfo.videoPort = 8000;
    info->portInfo.phonePort = 9000;

    /* 设置 CMD_PING 0x9001 响应的消息头 */
    info->header.head   = 0xaaaa5555; /* ZSP 协议的头部标识 */
    info->header.commd  = CMD_PING;
    info->header.length = sizeof(STRUCT_PING_ECHO) - sizeof( Cmd_Header );

    return 0;
}

static int CmdIdPing(STRUCT_UDP_THPOOL_ARG * udp_thpool_arg)
{
    int ret = 0;
    STRUCT_ID_PING_ECHO resp = {0};

    ret = FillBroadcastInfo(&resp.device);
    if(0 != ret)
    {
        ZSPERR("FillBroadcastInfo err,ret:%d\r\n", ret);
    }

    resp.device.header.commd = CMD_ID_PING;
    resp.device.header.length = sizeof(STRUCT_ID_PING_ECHO) - sizeof(Cmd_Header);
    strncpy(resp.deviceID, DEVICE_ID_TEST, 15);

    udp_thpool_arg->from_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    ret = sendto(udp_thpool_arg->socket_fd, &resp, sizeof(STRUCT_ID_PING_ECHO), 0, (sockaddr *)&(udp_thpool_arg->from_addr), sizeof(struct sockaddr_in));
    if(ret <= 0)
    {
        ZSPERR("CMD_ID_PING 0x9050 send error,ret:%d\n\n", ret);
    }
	ZSPLOG("CMD_ID_PING 0x9050 send success,ret:%d\n\n", ret);

    return ret;
}

void *UdpCmdProcessor(void *arg)
{
    STRUCT_UDP_THPOOL_ARG *udp_thpool_arg = NULL;
    Cmd_Header *cmd_header = NULL;

    if(NULL == arg)
    {
        ZSPERR("arg is NULL\n");
        return NULL;
    }

    udp_thpool_arg = (STRUCT_UDP_THPOOL_ARG *)arg;

    if(udp_thpool_arg->cmd_len >= sizeof(Cmd_Header))   
    {
        cmd_header = (Cmd_Header *)udp_thpool_arg->cmd_buf;

        ZSPLOG("cmd_header->head:0x%x\n",  cmd_header->head);
        ZSPLOG("cmd_header->length:%d\n",  cmd_header->length);
        ZSPLOG("cmd_header->type:%d\n",    cmd_header->type);
        ZSPLOG("cmd_header->channel:%d\n", cmd_header->channel);
        ZSPLOG("cmd_header->commd:0x%x\n", cmd_header->commd);

        switch(cmd_header->commd)
        {
            case CMD_ID_PING:   /* 0x9050 */
                CmdIdPing(udp_thpool_arg);
                break;

            case CMD_PING:   /* 0x9001 */
                ZSPLOG("recv CMD_PING：0x9001 nothing to do\r\n");
                break;
            default:
                break;
        }   
    }   

	free(udp_thpool_arg->cmd_buf);
	free(udp_thpool_arg);
    return NULL;
}

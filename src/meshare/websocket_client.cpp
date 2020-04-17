#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <liteos/tcp.h>

#include <json.h>

#include "dev_common.h"
#include "meshare_common.h"     /* meshare 库的公共头文件 */
#include "http_client.h"        /* http web 服务器相关的文件 */
#include "websocket_client.h"   /* websocket 相关的头文件 */
#include "nopoll.h"             /* nopoll 库的头文件 */
#include "aes.h"                /* aes 加密头文件 */
#include "modinterface.h"
#include "buffermanage.h"

/* 调用线程池所依赖的头文件 */
#include <pthread.h>
#include "thpool.h"

static STRUCT_VIDEO_USER msh_video_user_list[MSH_VIDEO_USER_MAX] = {0}; /* 使用手机APP播放视频的用户列表 */

/* 获取用户列表操作时的互斥锁,已静态初始化，不需要再调用 pthread_mutex_init() 初始化 */
static pthread_mutex_t msh_video_user_mutex = PTHREAD_MUTEX_INITIALIZER;

static int last_send_heartbeat_time = 0;   /* 最后一次发送心跳的时间，暂时这么写，后面再改到结构体中去 */
static threadpool msh_websocket_msg_thpool = NULL;    /* meshare 库 websocket 消息处理专用的线程池 */

#if 1
static int g_websocket_socket_fd = 0;   /* 用于调试的全局变量 */
int g_message_id = 0;         /* "message_id" */
char g_to_id[64] = { 0 };         /* "to_id" */
char g_from_id[64] = { 0 };         /* "from_id" */
static int malage_start = 0;   /* 启动视频发送 */
#endif

/***********************************************
 *@brief 获取当前系统运行的时间，不受系统时间被用户改变的影响
 *@param void 
 *@retrun
 *      当前系统运行的秒数
 **********************************************/
static int __websocket_get_uptime(void)
{
    struct timespec time_uptime = {};
    clock_gettime(CLOCK_MONOTONIC, &time_uptime);

    return time_uptime.tv_sec;  /* 当前系统运行的秒数 */
}
/***********************************************
 *@brief 获取 websocket 服务器的 IP 和 端口
 *@param *ip 参数1 用于返回ip地址参数
 *@param *port 参数2 用于返回port端口号
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_get_ip_port(char * ip, int * port)
{
    int ret = 0;
    char url[SERVER_URL_MAX_LEN] = { 0 };


    /* 调用 http 提供的接口函数 websocket 服务器 URL */
    ret = http_get_parameter_server(url, (char *)DEV_CONN_ADDR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("获取 websocket 服务器ip/port失败\r\n");
        return MSH_FAILURE;
    }

    /* 从websocket服务器的URL中解析出 ip 和 port */
    sscanf(url, "%[^:]:%d", ip, port);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief websocket send message 给服务器
 *@param timeout_ms 参数1 http 请求超时时间 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_send_msg_text(noPollConn * conn, char * msg, int len)
{
    int ret = 0;

    if((NULL == conn) || (0 == nopoll_conn_is_ready(conn)))
    {
        MSHLOG("nopoll_conn_is_ready error\r\n");
        return MSH_FAILURE;
    }
    ret = nopoll_conn_send_text(conn, msg, len);
    MSHLOG("nopoll_conn_send_text return:%d, len:%d\r\n", ret, len);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 对buffer进行加密
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_encrypt_buf(char * input_buf, int input_len, 
        char * output_buf, int * output_len, char * aes_key, int aes_mode)
{
    mbedtls_aes_context ctx; 
    int block_number = input_len/16;
    int remainder = input_len%16;
    int i = 0;
    unsigned char tmp_buf[16] = { 0 };

    mbedtls_aes_init(&ctx);  

    if(*output_len%16 != 0)
    {
        MSHLOG("output_len must %% 16==0\r\n");
        return MSH_FAILURE;
    }

    if(remainder)
    {
        input_len += 16-remainder;
    }

    if(input_len > *output_len)
    {
        MSHLOG("output buffer size is not enough!\n");
        return MSH_FAILURE;
    }

    *output_len = input_len;

    if(MBEDTLS_AES_ENCRYPT == aes_mode)
    {
        mbedtls_aes_setkey_enc(&ctx, (unsigned char*)aes_key, strlen(aes_key)*8); 
    }
    else    /* MBEDTLS_AES_DECRYPT */
    {
        mbedtls_aes_setkey_dec( &ctx, (unsigned char*)aes_key, strlen(aes_key)*8); 
    }

    for(i = 0; i < block_number; i++)
    {
        mbedtls_aes_crypt_ecb( &ctx, aes_mode, 
                (unsigned char*)input_buf+i*16, (unsigned char*)output_buf+i*16 ); 
    }

    if(remainder)
    {
        memset(tmp_buf, 0, sizeof(tmp_buf));
        memcpy(tmp_buf, input_buf+i*16, remainder);
        mbedtls_aes_crypt_ecb(&ctx, aes_mode, tmp_buf, (unsigned char*)output_buf+i*16 );
    }
    mbedtls_aes_free(&ctx);

    return MSH_SUCCESS;
}
/***********************************************
 *@brief 对 msg 进行加密，会重新分配内存，需要手动释放
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_encrypt_msg(char **msg, int *msg_len, char * aes_key, int aes_mode)
{
    char * out_buf = NULL;
    int out_len = 0;

    if((NULL == *msg) || (*msg_len <= 0) || (NULL == aes_key))
    {
        *msg = NULL;
        MSHLOG("encrypt error,mode %d\r\n", aes_mode);
        return MSH_FAILURE;
    }

    if(strlen(aes_key) > 0)
    {
        out_len = *msg_len + 16 - *msg_len%16; /* 对齐 ? */
        out_buf = (char *)calloc(1, out_len);

        __websocket_encrypt_buf(*msg, *msg_len, out_buf, &out_len, aes_key, aes_mode);

        *msg = out_buf;
        *msg_len = out_len;
        return MSH_SUCCESS;
    }

    *msg = NULL;
    return MSH_FAILURE;
}

/***********************************************
 *@brief 使用 select 机制监听 socket fd 是否可读或者可写
 *@param socket_fd  参数1 socket fd
 *@param timeout_ms 参数2 超时时间
 *@param read_status 参数3 读状态标志， 可读：MSH_FD_READABLE   不可读：MSH_FD_UNREADABLE
 *@param write_status 参数4 写状态标志，可写：MSH_FD_WRITABLE   不可写：MSH_FD_UNWRITABLE
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 说明 select超时 或 select 出错
 **********************************************/
static int __websocket_select_fd_readable_or_writable(int socket_fd, 
        int timeout_ms, int * read_status, int * write_status)
{
    int ret = 0;
    fd_set writefds;
    fd_set readfds;
    timeval timeout;    /* select 超时时间 */

    FD_ZERO(&writefds);
    FD_ZERO(&readfds);

    FD_SET(socket_fd, &writefds);
    FD_SET(socket_fd, &readfds);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000)* 1000;

    /* 重置 read_status 和 write_status 的状态分别为 不可读 不可写 */
    *read_status = MSH_FD_UNREADABLE;   /* 默认不可读 */
    *write_status = MSH_FD_UNWRITABLE;  /* 默认不可写 */

    ret = select(socket_fd + 1, &readfds, &writefds, NULL, &timeout);
    if(ret < 0)
    {
        MSHERR("select error\r\n");
        return MSH_FAILURE;
    }
    else if(0 == ret)
    {
        MSHERR("select timeout\r\n");
        return MSH_FAILURE;
    }
    else if(ret > 0)
    {
        //MSHLOG("select success ret:%d, socket_fd:%d\r\n", ret, socket_fd);

        if(FD_ISSET(socket_fd, &readfds))
        {
            /* 如果可读，则设置 read_status 为 MSH_FD_READABLE  */
            *read_status = MSH_FD_READABLE;   /* 设置为可读 */
        }

        if(FD_ISSET(socket_fd, &writefds))
        {
            /* 如果可写，则设置 write_status 为 MSH_FD_WRITABLE */
            *write_status = MSH_FD_WRITABLE;  /* 设置为可写 */
        }
        return MSH_SUCCESS;
    }
}

/***********************************************
 *@brief 使用 select 机制监听 socket fd 是否可读
 *@param socket_fd  参数1 socket fd
 *@param timeout_ms 参数2 超时时间
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_select_fd_readable(int socket_fd, int timeout_ms)
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
        MSHLOG("socket_fd is readable\r\n");
        return MSH_SUCCESS;
    }

    MSHLOG("socket_fd is not readable\r\n");
    return MSH_FAILURE;
}

/***********************************************
 *@brief 使用 select 机制监听 socket fd 是否可写
 *@param socket_fd  参数1 socket fd
 *@param timeout_ms 参数2 超时时间
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_select_fd_writable(int socket_fd, int timeout_ms)
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
        MSHLOG("socket_fd is writable\r\n");
        return MSH_SUCCESS;
    }

    MSHLOG("socket_fd is not writable\r\n");
    return MSH_FAILURE;
}

/***********************************************
 *@brief 连接到视频中转服务器
 *@param *ip  参数1 服务器IP
 *@param port 参数2 服务器端口
 *@param socket_fd 参数3 如果连接成功，返回的 socket fd
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_cmd_connect_trans_server(char * ip, int port, int *socket_fd)
{
    int ret = 0;
    sockaddr_in addr;
    int retry_times = 0;    /* 连接服务器失败重试次数 */

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port        = htons(port);

    *socket_fd = socket(AF_INET, SOCK_STREAM, 0); /* 创建TCP socket */
    if(*socket_fd <= 0)
    {
        MSHERR("connect trans server create socket error\r\n");
        return MSH_FAILURE;
    }

    /* 设置 socket 为非阻塞，以免 connect 长时间阻塞 */
    fcntl(*socket_fd, F_SETFL, fcntl(*socket_fd, F_GETFL, 0) | O_NONBLOCK);

    /* 连接到视频中转服务器 */
    for(retry_times = 0; retry_times < 3; retry_times++)    /* 3 表示重试次数 */
    {
        MSHLOG("connect trans server retry_times:%d, *socket_fd:%d\r\n", retry_times, *socket_fd);
        ret = connect(*socket_fd, (sockaddr *)&addr, sizeof(addr));
        if(ret < 0)
        {
            if((EINPROGRESS == errno) 
                    && (MSH_SUCCESS ==  __websocket_select_fd_writable(*socket_fd, 20*1000)))   /* 超时时间20s */
            {
                MSHERR("connect trans server success\r\n");
                /* 恢复设置 socket 为阻塞 */
                fcntl(*socket_fd, F_SETFL, fcntl(*socket_fd, F_GETFL, 0) & ~O_NONBLOCK);
                return MSH_SUCCESS; /* 成功：0 MSH_SUCCESS */
            }

            MSHERR("connect trans server error\r\n");
            usleep(50*1000);    /* 50ms 后重试 */
            continue;
        }
        MSHERR("connect trans server success\r\n");
        /* 恢复设置 socket 为阻塞 */
        fcntl(*socket_fd, F_SETFL, fcntl(*socket_fd, F_GETFL, 0) & ~O_NONBLOCK);
        return MSH_SUCCESS; /* 成功：0 MSH_SUCCESS */
    }

    if(*socket_fd > 0)
    {
        close(*socket_fd);
    }
    return MSH_FAILURE;     /* 失败：-1 MSH_FAILURE */
}

/***********************************************
 *@brief 从指定 socket fd 接收指定长度的数据
 *@param socket_fd 参数1 socket fd
 *@param buf 参数2 buf 地址
 *@param len 参数2 buf 长度
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_recv_buffer_from_socket(int socket_fd, char * buf, int len)
{
    int ret = 0;
    int total_len = 0;      /* 已收到的部字节数 */
    int retry_times = 0;    /* 出错时重试次数 */

    if((NULL == buf) || (len <= 0))
    {
        MSHERR("buf is NULL or len %d <= 0", len);
        return MSH_FAILURE;
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
            MSHERR("recv fd:%d, errno:%d, err:%s\r\n", socket_fd, error_tmp, strerror(error_tmp));
            if(retry_times < 4) /* 接收失败的重试次数 */
            {
                MSHERR("websocket recv buffer error, continue, ret:%d\r\n", ret);
                usleep(20*1000);    /* 等待20ms后继续 */
                continue;
            }
            else
            {
                MSHERR("websocket recv buffer error, ret:%d\r\n", ret);
                return MSH_FAILURE;
            }
        }
        else if(0 == ret)
        {
            MSHERR("socket closed, ret:%d\r\n", ret);
            return MSH_FAILURE;
        }

        retry_times = 0;
        total_len += ret;
        MSHLOG("websocket recv buffer success, ret:%d, total_len:%d, len:%d\r\n", ret, total_len, len);
    }while(total_len < len);
    MSHLOG("websocket recv total_len:%d, len:%d\r\n", total_len, len);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 往指定 socket fd 发送指定长度的数据
 *@param socket_fd 参数1 socket fd
 *@param buf 参数2 buf 地址
 *@param len 参数2 buf 长度
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_send_buffer_to_socket(int socket_fd, char * buf, int len)
{
    int ret = 0;
    int total_len = 0;      /* 已发送的数据总字节数 */
    int retry_times = 0;    /* 发送失败的重试次数 */

    if((NULL == buf) || (len <= 0))
    {
        MSHERR("buf is NULL or len %d <= 0", len);
        return MSH_FAILURE;
    }

    if(0)
    {
        printf("\r\n 3========================================================\r\n");
        char * tmp_buf_p = (char *)buf;
        int tmp_buf_i = 0;
        int tmp_buf_i_max = len;
        /*
        if(tmp_buf_i_max > 3000)
        {
            tmp_buf_i_max = 3000;
        }
        */

        for(tmp_buf_i = (tmp_buf_i_max-2000); tmp_buf_i < tmp_buf_i_max; tmp_buf_i ++)
        {
            printf("%02X, ", (char)tmp_buf_p[tmp_buf_i]);
            if(0 == (tmp_buf_i+1)%16)
            {
                printf("\n");
            }
        }
        printf("\n");
        printf("\r\n3========================================================\r\n");
    }

    //MSHLOG("buf addr:%p, len:%d\r\n", buf, len);

    do
    {
        retry_times ++;     /* 重试次数 ++ */
        /* len-total_len 为未发送的剩余的字节数 */
        ret = send(socket_fd, buf+total_len, len-total_len, 0);
        //ret = send(socket_fd, buf+total_len, (len-total_len)>16000?16000:(len-total_len), MSG_WAITALL);
        if(ret < 0)
        {
            int errno_tmp = errno;
            MSHERR("send fd:%d, errno:%d, err:%s\r\n", 
                    socket_fd, errno_tmp, strerror(errno_tmp));

            if(errno_tmp == EAGAIN)
            {
                MSHERR("EAGAIN\r\n");
                __websocket_select_fd_writable(socket_fd, 10000);
                if(retry_times < 4) /* 发送失败的重试次数 */
                {
                    MSHERR("websocket send buffer error, continue, ret:%d\r\n", ret);
                    usleep(20*1000);    /* 等待20ms后继续 */
                    continue;
                }
                else
                {
                    MSHERR("websocket send buffer error, ret:%d\r\n", ret);
                    return MSH_FAILURE;
                }
            }
            else
            {
                MSHERR("send error, errno [%d], err:%s\r\n", errno_tmp, strerror(errno_tmp));
                usleep(20*1000);    /* 等待20ms后退出 */
                break;
            }

            if(retry_times < 4) /* 发送失败的重试次数 */
            {
                MSHERR("websocket send buffer error, continue, ret:%d\r\n", ret);
                usleep(20*1000);    /* 等待20ms后继续 */
                continue;
            }
            else
            {
                MSHERR("websocket send buffer error, ret:%d\r\n", ret);
                return MSH_FAILURE;
            }
        }
        else if(0 == ret)
        {
            MSHERR("socket closed, ret:%d\r\n", ret);
            return MSH_FAILURE;
        }
        
        retry_times = 0;
        total_len += ret;
        //MSHLOG("websocket send buffer success, ret:%d, total_len:%d, len:%d\r\n", ret, total_len, len);
    }while(total_len < len);
    //MSHLOG("websocket send total_len:%d, len:%d\r\n", total_len, len);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 登录到视频中转服务器
 *@param socket_fd 与服务器连接的 socket fd
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
 static int __websocket_cmd_login_trans_server(int socket_fd, char *id)
{
    int ret = 0;
    char buf[512] = { 0 };  /* 临时存储从 socket_fd 接收到的数据 */
    /* 登录视频中转服务器消息定义并初始化为默认值 */
    msh_login_trans_t msg_trans = { TRANS_HEAD_INITIALIZER, { 0 }, { 0 }};
    
    MSHLOG("开始登录到视频中转服务器");
    msg_trans.head.cmd_type = htons(CMD_TYPE_IPC_TRANS);
    msg_trans.head.cmd      = htonl(IPC_TRANS_LOGIN_SYN);
    msg_trans.head.length   = htonl(sizeof(msh_login_trans_t) -  TRANS_HEAD_LEN);

    msg_trans.head.flag = 11; /* 参考meshare文档，11 表示支持加密 */
    strcpy(msg_trans.register_code, id);    /* 收到的json数据中的 id 字段 */
    strcpy(msg_trans.username_or_sn, DEVICE_ID_TEST);        /* 设备ID ，暂时用固定的宏 */

    MSHLOG("register_code:%s, username_or_sn:%s\r\n", 
            msg_trans.register_code, msg_trans.username_or_sn);

    if(0)
    {
        printf("\r\n55========================================================\r\n");
        char * tmp_buf_p = (char *)&msg_trans;
        int tmp_buf_i = 0;
        for(tmp_buf_i = 0; tmp_buf_i < (int)sizeof(msg_trans); tmp_buf_i ++)
        {

            printf("%02X, ", (char)tmp_buf_p[tmp_buf_i]);
            if(0 == (tmp_buf_i+1)%16)
            {
                printf("\n");
            }
        }
        printf("\n");
        printf("\r\n55========================================================\r\n");
    }

    /* 将 msg 数据发送给服务器 */
    ret = __websocket_send_buffer_to_socket(socket_fd, (char *)&msg_trans, sizeof(msh_login_trans_t));
    if(MSH_FAILURE == ret)
    {
        MSHERR("send buffer to trans server error\r\n");
        return MSH_FAILURE;
    }

    MSHLOG("send buffer to trans server success\r\n");
    /* 判断视频中转服务器socket是否可读 */
    ret = __websocket_select_fd_readable(socket_fd, 5000);  /* timeout_ms 为 5 秒 */
    if(MSH_FAILURE == ret)
    {
        MSHLOG("socket_fd is not readable\r\n");
        return MSH_FAILURE;
    }
    MSHLOG("socket_fd is readable\r\n");

    /* 读取socket接收数据 */
    ret = __websocket_recv_buffer_from_socket(socket_fd, buf, TRANS_HEAD_LEN+1);    /* 参考之前的代码要 +1 */
    if(MSH_FAILURE == ret)
    {
        MSHERR("recv buffer from trans server error\r\n");
        return MSH_FAILURE;
    }
    MSHERR("recv buffer from trans server success\r\n");

    /* 打印收到数据 */
#if 1
    msh_trans_msg_t * tmp_trans = (msh_trans_msg_t *)buf;
    printf("magic:0x%x\r\n", ntohl(tmp_trans->magic));
    printf("cmd_type:%d\r\n", ntohs(tmp_trans->cmd_type));
    printf("cmd:%d == 0x%08x\r\n", ntohl(tmp_trans->cmd), ntohl(tmp_trans->cmd));
#endif

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 初始化视频播放用户列表
 *@param video_user_list 视频播放用户列表数组
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __msh_video_user_list_init(STRUCT_VIDEO_USER video_user_list[])
{
    int i = 0;

    if(NULL == video_user_list)
    {
        return MSH_FAILURE;
    }

    pthread_mutex_lock(&msh_video_user_mutex);
    for(i = 0; i < MSH_VIDEO_USER_MAX; i++ )
    {
        video_user_list[i].used = VIDEO_USER_UNUSED;
        video_user_list[i].ip = 0;
        video_user_list[i].port = 0;
        /* 为避免与 ZSP 冲突，meshare 库使用的 userid 为 i+ZSP_VIDEO_USER_MAX */
        video_user_list[i].userid = i+ZSP_VIDEO_USER_MAX;
        video_user_list[i].devType = 0;
    }
    pthread_mutex_unlock(&msh_video_user_mutex);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 初始化视频播放用户列表，提供给外部的接口，调用 __msh_video_user_list_init()
 *@param void
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
int msh_video_user_list_init(void)
{
    return __msh_video_user_list_init(msh_video_user_list);
}

/***********************************************
 *@brief 获取一个空闲的视频播放用户ID
 *@param socket_fd 此用户所使用的 socket fd
 *@retrun
 *      成功：返回 userid   
 *      失败：返回 MSH_FAILURE -1 用户列表中没有空闲的用户ID
 **********************************************/
static int msh_get_video_user(int socket_fd)
{
    int userid = 0;
    int ret = MSH_FAILURE;

    pthread_mutex_lock(&msh_video_user_mutex);
    for(userid = 0; userid < MSH_VIDEO_USER_MAX; userid++)
    {
        if(VIDEO_USER_UNUSED == msh_video_user_list[userid].used)
        {
            msh_video_user_list[userid].used = VIDEO_USER_USED;
            ret = userid+ZSP_VIDEO_USER_MAX;
            break;
        }
    }
    pthread_mutex_unlock(&msh_video_user_mutex);

    return ret;
}

/***********************************************
 *@brief 设置当前的用户ID为 VIDEO_USER_UNUSED
 *@param userid 此用户 userid
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int msh_release_video_user(int userid)
{
    int ret = MSH_FAILURE;

    /* 为避免与 ZSP 冲突，meshare 库使用的 userid 为 i+ZSP_VIDEO_USER_MAX */
    userid -= ZSP_VIDEO_USER_MAX;
    pthread_mutex_lock(&msh_video_user_mutex);
    if((userid < MSH_VIDEO_USER_MAX) && (userid >= 0))
    {
        msh_video_user_list[userid].used = VIDEO_USER_UNUSED;
        MSHINF("release userid [%d] success\r\n", userid+ZSP_VIDEO_USER_MAX);
        ret = MSH_SUCCESS;
    }
    else
    {
        MSHERR("userid [%d] error\r\n", userid+ZSP_VIDEO_USER_MAX);
    }
    pthread_mutex_unlock(&msh_video_user_mutex);

    return ret;
}


/***********************************************
 *@brief websocket从 buffer 中获取 1 帧视频数据前初始化
 *@param userid 参数1 userid
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_get_video_frame_from_buffer_init(int userid)
{
    FrameInfo frame_info = {};  /* 视频帧信息，例如 I帧 P帧 帧大小 */
    ParaEncUserInfo uinf;   /* 用户信息 */
    char * buf = NULL;      /* 这里没什么用 */

    /* 设置传入获取视频数据接口的参数信息 */
    uinf.ch = 0;    /* 通道，例如 0:1080P/720P 1:VGA 2:QVGA ... */
    //uinf.ch = 2;    /* 通道，例如 0:1080P/720P 1:VGA 2:QVGA ... */
    uinf.userid = userid;   /* 视频播放用户ID */
    uinf.buffer = (unsigned char **)&buf;  /* 视频数据的地址 */
    uinf.frameinfo = &frame_info; /* 视频帧信息，例如 I帧 P帧 帧大小 */

    ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RESETUSER,(void*)&uinf);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief websocket从 buffer 中获取 1 帧视频数据
 *@param **buf 参数1 获取到视频数据的地址
 *@param *len 参数2 获取到视频数据的长度
 *@param userid 参数3 userid
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_get_video_frame_from_buffer(char ** buf, int * len, int userid)
{
    int ret = 0;
    FrameInfo frame_info = {};  /* 视频帧信息，例如 I帧 P帧 帧大小 */
    ParaEncUserInfo uinf;  /* 用户信息 */
    int retry_time = 0;     /* 出错重试的次数 */

    /* 设置传入获取视频数据接口的参数信息 */
    uinf.ch = 0;    /* 通道，例如 0:1080P/720P 1:VGA 2:QVGA ... */
    //uinf.ch = 2;    /* 通道，例如 0:1080P/720P 1:VGA 2:QVGA ... */
    uinf.userid = userid;   /* 视频播放用户ID */
    uinf.buffer = (unsigned char **)buf;  /* 视频数据的地址 */
    uinf.frameinfo = &frame_info; /* 视频帧信息，例如 I帧 P帧 帧大小 */

    //ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RESETUSER,(void*)&uinf);

    for(retry_time = 0; retry_time < 3; retry_time ++)
    {
        ret = ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_GETFRAME,(void*)&uinf);
        if(ret != 0)
        {
            MSHERR("ModInterface::GetInstance CMD_ENC_GETFRAME ret:%d\r\n", ret);
            usleep(30*1000); /* 30ms 后再取数据 */
            continue;
        }
        *len = frame_info.FrmLength; /* 取到的视频数据长度 */
        //MSHLOG("websocket get video frame success: *len=%d\r\n", *len);

        return MSH_SUCCESS;
    }

    return MSH_FAILURE;
}

/***********************************************
 *@brief 发送视频数据到中转服务器，此函数中会为 output_buf 分配内存，需要外面释放
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
#define  MSH_FRAME_SPLIT_SIZE  (1000)    /* 暂时放在这里吧 */
static int __websocket_video_frame_split(char * input_buf, int input_len, 
        char ** output_buf, int * output_len, int cmd_type, int frame_flag, int chlnum)
{
    int bsize_filled = 0;
    int slic_size = 0;
    int slic_num = 0;
    int slic_cnt = input_len/MSH_FRAME_SPLIT_SIZE + ((input_len%MSH_FRAME_SPLIT_SIZE)>0?1:0);

    msh_video_trans_t head = TRANS_HEAD_INITIALIZER;
    head.cmd_type = htons(cmd_type);
    head.cmd = htonl(PC_IPC_TRANSFER_DATA);
    head.chlnum = chlnum;
    head.frame_flag = frame_flag;

    if(0)
    {
        printf("\r\nvidoe begin========================================================\r\n");
        char * tmp_buf_p = (char *)input_buf;
        int tmp_buf_i = 0;
        int tmp_buf_i_max = input_len;
        /*
           if(tmp_buf_i_max > 3000)
           {
           tmp_buf_i_max = 3000;
           }
           */

        for(tmp_buf_i = (tmp_buf_i_max-1000); tmp_buf_i < tmp_buf_i_max; tmp_buf_i ++)
        {
            printf("%02X, ", (char)tmp_buf_p[tmp_buf_i]);
            if(0 == (tmp_buf_i+1)%16)
            {
                printf("\n");
            }
        }
        printf("\n");
        printf("\r\nvideo end========================================================\r\n");
    }


    *output_len = (sizeof(msh_video_trans_t))*slic_cnt + input_len;
    *output_buf = (char *)malloc(*output_len);
    //MSHLOG("input_len:%d, *output_len:%d\r\n", input_len, *output_len);
    
    if(NULL == *output_buf)
    {
        MSHERR("output_buf malloc error, output_len [%d],"
                " slic_cnt [%d], input_len [%d]\r\n", *output_len, slic_cnt, input_len);
        return MSH_FAILURE;
    }
    memset(*output_buf, 0, *output_len);

    while(bsize_filled < input_len)
    {
        if(input_len - bsize_filled > MSH_FRAME_SPLIT_SIZE)
        {
            slic_size = MSH_FRAME_SPLIT_SIZE;
        }
        else
        {
            slic_size = input_len - bsize_filled;
        }

        head.length = htonl(slic_size);
        head.seqnum = slic_cnt;
        head.seqnum |= slic_num++<<16;
        head.seqnum = htonl(head.seqnum);

        memcpy((*output_buf)+bsize_filled+((slic_num - 1) * TRANS_HEAD_LEN), (char*)&head, TRANS_HEAD_LEN);
        memcpy((*output_buf)+bsize_filled+(slic_num)*TRANS_HEAD_LEN, input_buf+bsize_filled, slic_size);
        bsize_filled += slic_size;
    }

    if(0)
    {
        printf("\r\n split begin========================================================\r\n");
        char * tmp_buf_p = (char *)*output_buf;
        int tmp_buf_i = 0;
        int tmp_buf_i_max = *output_len;
        /*
        if(tmp_buf_i_max > 3000)
        {
            tmp_buf_i_max = 3000;
        }
        */

        for(tmp_buf_i = (tmp_buf_i_max-2000); tmp_buf_i < tmp_buf_i_max; tmp_buf_i ++)
        {
            printf("%02X, ", (char)tmp_buf_p[tmp_buf_i]);
            if(0 == (tmp_buf_i+1)%16)
            {
                printf("\n");
            }
        }
        printf("\n");
        printf("\r\nsplit end========================================================\r\n");
    }

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 发送视频数据到中转服务器
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int trans_last_heartbeat_time = 0;   /* 暂时这样，测试 */
static int __websocket_send_video_frame_to_trans(int socket_fd)
{
    int ret = 0;
    int read_status = MSH_FD_UNREADABLE;    /* 读状态标志，默认为不可读 */
    int write_status = MSH_FD_UNWRITABLE;   /* 写状态标志，默认为不可写 */
    int timeout_ms = 0;     /* select 超时时间 */
    char buf_recv[512] = { 0 }; /* 接收socket数据缓冲 */
    int userid = 0; /* meshare 获取视频数据的 userid */

    //timeout_ms = 20;
    timeout_ms = 1000;

    userid = msh_get_video_user(socket_fd);
    if(MSH_FAILURE == userid)
    {
        MSHERR("get userid error, socket_fd [%d]\r\n", socket_fd);
        return MSH_FAILURE;
    }
    else
    {
        MSHINF("get userid success, userid [%d], socket_fd [%d]\r\n", userid, socket_fd);
    }

    /* 获取视频前初始化 */
    __websocket_get_video_frame_from_buffer_init(userid);

    /* 设置 socket 为非阻塞　*/
    fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK);
    /* 设置 socket 为阻塞　*/
    //fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) & ~O_NONBLOCK);
#if 1
    /* set TCP_CORK to improve network usage, but will lower real-time*/
    int tcp_cork_flag = 1;
    if(setsockopt(socket_fd, IPPROTO_TCP, TCP_CORK, (char*)&tcp_cork_flag, sizeof(int)))
    {
        MSHERR("set TCP_CORK error!\r\n") ;       
    }
#endif

    while(1)
    {
        read_status = MSH_FD_UNREADABLE;
        write_status = MSH_FD_UNWRITABLE;
        /* 等待 socket_fd 可读或者可写，通过参数 read_status 和 write_status 判断 */
        ret = __websocket_select_fd_readable_or_writable(socket_fd, timeout_ms, &read_status, &write_status);
         
        if(MSH_FD_READABLE == read_status)
        {
            /* 如果可读，读取数据，接收来自中转服务器返回的消息 */
            MSHLOG("websocket socket_fd %d ,read_status:%d\r\n", socket_fd, read_status);

            /* 读取socket接收数据 */
            memset(buf_recv, 0, sizeof(buf_recv));
            ret = __websocket_recv_buffer_from_socket(socket_fd, buf_recv, TRANS_HEAD_LEN);  
            if(MSH_FAILURE == ret)
            {
                //MSHERR("recv buffer from trans server error，暂时不 return \r\n");
                MSHERR("recv buffer from trans server error，return \r\n");
                msh_release_video_user(userid);
                return MSH_FAILURE;
            }
            MSHERR("recv buffer from trans server success\r\n");

            /* 打印收到数据 */
            msh_trans_msg_t * tmp_trans = (msh_trans_msg_t *)buf_recv;
            MSHINF("magic:0x%x\r\n", ntohl(tmp_trans->magic));
            MSHINF("cmd_type:%d\r\n", ntohs(tmp_trans->cmd_type));
            MSHINF("cmd:%d == 0x%08x\r\n", ntohl(tmp_trans->cmd), ntohl(tmp_trans->cmd));


            MSHINF("IPC_TRANS_HEART_ACK:%d == 0x%08x\r\n", IPC_TRANS_HEART_ACK, IPC_TRANS_HEART_ACK);
            if(IPC_TRANS_HEART_ACK == ntohl(tmp_trans->cmd))
            {
                MSHLOG("malage_start = %d\r\n", malage_start);
                malage_start = 1;
            }
            /* 对收到的数据进行判断是否为命令并进行对应的处理 */
        }
        else if(MSH_FD_WRITABLE == write_status)
        {
            int now_time = __websocket_get_uptime();
            if((now_time - trans_last_heartbeat_time) > 5)
            //if((now_time - trans_last_heartbeat_time) > 1)
            {
                trans_last_heartbeat_time = now_time;

                MSHLOG("now_time:%d, trans_last_heartbeat_time:%d, D-value:%d\r\n", 
                        now_time, trans_last_heartbeat_time, (now_time - trans_last_heartbeat_time));

                /* 配置中转服务器心跳包数据 */
                msh_trans_msg_t msg_heartbeat = TRANS_HEAD_INITIALIZER;
                msg_heartbeat.cmd_type = htons(CMD_TYPE_IPC_TRANS);
                msg_heartbeat.cmd = htonl(IPC_TRANS_HEART_SYN);

                /* 发送与中转服务器之间的心跳包 */
                ret = __websocket_send_buffer_to_socket(socket_fd, 
                        (char *)&msg_heartbeat, sizeof(msh_trans_msg_t));  
                if(MSH_FAILURE == ret)
                {
                    MSHERR("websocket trans send heartbeat error\r\n");
                }
                MSHLOG("websocket trans send heartbeat success\r\n");
            }
        }

        if((MSH_FD_WRITABLE == write_status) && (malage_start))
        //if(MSH_FD_WRITABLE == write_status)
        {
            /* 如果可写，发送视频数据 */
            //MSHLOG("websocket socket_fd %d write_status:%d\r\n", socket_fd, write_status); 

#if 1   /* 测试代码 */
            char * video_buf = NULL;
            int video_len = 0;

            /* 如果可写，获取视频数据 */
            ret = __websocket_get_video_frame_from_buffer(&video_buf, &video_len, userid);
            if(MSH_FAILURE == ret)
            {
                MSHERR("websocket get video frame error, continue\r\n");
                continue;
            }
            //MSHLOG("websocket get video frame success, video_len:%d\r\n", video_len);

            /* 将 video_buf 中的数据分片并加头 */
            char * tmp_video_buf = NULL;
            int tmp_video_len = 0;          

            tmp_video_buf = NULL;
            tmp_video_len = 0;

            VideoFrameHeader * frame_head = (VideoFrameHeader *) video_buf;
            int frame_flag = 0;
            if(frame_head->m_FrameType == 0)
            {
                frame_flag = 0;
            }
            else if (frame_head->m_FrameType == 1)
            {
                frame_flag = 3;
            }
            else if (frame_head->m_FrameType == 2)
            {
                frame_flag = 1;
            }
            else if (frame_head->m_FrameType == 4)
            {
                frame_flag = 2;
            }
            //MSHLOG("frame_head->m_FrameType:%d, frame_flag:%d\r\n", frame_head->m_FrameType, frame_flag);

            ret = __websocket_video_frame_split(video_buf, video_len, &tmp_video_buf, 
                    &tmp_video_len, CMD_TYPE_IPC_CLIENT, frame_flag, 0);
            //MSHERR("将 video_buf 中的数据分片并加头 ret:%d, tmp_video_len:%d\r\n", ret, tmp_video_len);

            //MSHLOG("send_buf_len:%d, tmp_video_len:%d\r\n", send_buf_len, tmp_video_len);
            //
            /* 发送视频数据给中转服务器 */
            //MSHLOG("video_buf addr:%p\r\n", video_buf);
            //MSHLOG("tmp_video_buf addr:%p, tmp_video_len:%d\r\n", tmp_video_buf, tmp_video_len);

            //ret = __websocket_send_buffer_to_socket(socket_fd, video_buf, video_len);
            //ret = __websocket_send_buffer_to_socket(socket_fd, tmp_video_buf, tmp_video_len);

            if(NULL != tmp_video_buf)
            {
                ret = __websocket_send_buffer_to_socket(socket_fd, tmp_video_buf, tmp_video_len);
                //MSHLOG("send_test_p_frame size:%d\r\n", sizeof(send_test_p_frame));
                //ret = __websocket_send_buffer_to_socket(socket_fd, send_test_p_frame, sizeof(send_test_p_frame));
                //MSHLOG("send_test_i_frame size:%d\r\n", sizeof(send_test_i_frame));
                //ret = __websocket_send_buffer_to_socket(socket_fd, send_test_i_frame, sizeof(send_test_i_frame));
                if(MSH_FAILURE == ret)
                {
                    MSHERR("sebsocket send video frame error\r\n");
                    break;    /* 暂时不退出 */
                }
                //MSHLOG("sebsocket send video frame success, continue\r\n");
            }

            if(NULL != tmp_video_buf)
            {
                free(tmp_video_buf);
                tmp_video_buf = NULL;
            }
#endif

        }

    }

    msh_release_video_user(userid);
    return MSH_SUCCESS;
}

/***********************************************
 *@brief 响应处理视频中转命令:DEV_SRV_RECV_TRANS_SYN , DEV_SRV_RECV_TRANS_ACK
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_cmd_do_trans_response(void)
{
    int ret = 0;
    json_object * json_obj = NULL;
    char * str_tmp = NULL;
    int str_tmp_len = 0;

    if(g_websocket_socket_fd <= 0)   /* 用于调试的全局变量 */
    {
        MSHERR("g_websocket_socket_fd %d <= 0\r\n", g_websocket_socket_fd);
        return MSH_FAILURE;
    }

    json_obj = json_object_new_object();

    json_object_object_add(json_obj, "type", json_object_new_int(MSH_MSG_DATA));
    json_object_object_add(json_obj, "length", json_object_new_int(0)); /* 没有body数据 */
    json_object_object_add(json_obj, "message_id", json_object_new_int(g_message_id));
    json_object_object_add(json_obj, "cmd", json_object_new_int(DEV_SRV_RECV_TRANS_ACK));
    json_object_object_add(json_obj, "to_id", json_object_new_string(g_from_id));
    json_object_object_add(json_obj, "from_id", json_object_new_string(g_to_id));
    json_object_object_add(json_obj, "result_code", json_object_new_int(0));
    json_object_object_add(json_obj, "result_reason", json_object_new_string("OK"));

    /**/
    str_tmp = (char *)json_object_get_string(json_obj);
    str_tmp_len = strlen(str_tmp);

    MSHLOG("trans response str:%s, len:%d\r\n", str_tmp, str_tmp_len);
    ret = __websocket_send_buffer_to_socket(g_websocket_socket_fd, str_tmp, str_tmp_len);
    MSHLOG("trans response ret:%d\r\n", ret);



    if(NULL != json_obj)
    {
        json_object_put(json_obj);  /* 释放 */
    }
    return MSH_SUCCESS;
}

/***********************************************
 *@brief 处理视频中转命令:DEV_SRV_RECV_TRANS_SYN 
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_cmd_do_trans(json_object * json_obj)
{
    int ret = 0;
    char * channel_id = NULL;   /* 服务器返回的 json 数据中的 channel_id string */
    char * media_type = NULL;   /* 服务器返回的 json 数据中的 media_type string */
    char * trans_ip = NULL;     /* 服务器返回的 json 数据中的 trans_ip string */
    char * id = NULL;           /* 服务器返回的 json 数据中的 id string */
    int trans_port = 0;         /* 服务器返回的 json 数据中的 trans_port */
    int socket_fd = 0;          /* 连接视频中转服务器成功返回的 socket fd */

    MSHLOG("处理视频中转命令:DEV_SRV_RECV_TRANS_SYN\r\n");
    if(NULL == json_obj)
    {
        MSHERR("json_obj is NULL\r\n");
        return MSH_FAILURE;
    }

    /* 获取 id/ip/port/channel/type ... 等参数 */
    channel_id = (char *)json_object_get_string(json_object_object_get(json_obj, "channel_id"));
    media_type = (char *)json_object_get_string(json_object_object_get(json_obj, "media_type"));
    trans_ip   = (char *)json_object_get_string(json_object_object_get(json_obj, "trans_ip"));
    id         = (char *)json_object_get_string(json_object_object_get(json_obj, "id"));
    trans_port = (int)  json_object_get_int    (json_object_object_get(json_obj, "trans_port"));
    
#if 1
    g_message_id = (int)json_object_get_int(json_object_object_get(json_obj, "message_id"));
    strcpy(g_to_id, (char *)json_object_get_string(json_object_object_get(json_obj, "to_id")));
    strcpy(g_from_id, (char *)json_object_get_string(json_object_object_get(json_obj, "from_id")));
    MSHLOG("message_id:%d\r\n", g_message_id);
    MSHLOG("g_to_id:%s\r\n", g_to_id);
    MSHLOG("g_from_id:%s\r\n", g_from_id);
#endif

    MSHLOG("channel_id:%s\r\n", channel_id);
    MSHLOG("media_type:%s\r\n", media_type);
    MSHLOG("trans_ip:%s\r\n", trans_ip);
    MSHLOG("trans_port:%d\r\n", trans_port);
    MSHLOG("id:%s\r\n", id);

    /* 连接到视频中转服务器 */
    ret = __websocket_cmd_connect_trans_server(trans_ip, trans_port, &socket_fd);
    if(MSH_FAILURE == ret)
    {
        MSHERR("trans server connect error\r\n");
        return MSH_FAILURE;
    }
    
    MSHLOG("trans server connect success\r\n");
    /* 登录到视频中转服务器 */
    ret = __websocket_cmd_login_trans_server(socket_fd, id);
    if(MSH_FAILURE == ret)
    {
        MSHERR("trans server login error\r\n");
        return MSH_FAILURE;
    }
    MSHERR("trans server login success\r\n");

#if 1
    /* 测试，先响应OK */
    ret = __websocket_cmd_do_trans_response();
    MSHLOG("__websocket_cmd_do_trans_response ret = %d\r\n", ret);
#endif

    if(1 == malage_start)
    {
        MSHERR("trans client is already running\r\n");
        return MSH_SUCCESS;
    }

    /* 发送视频数据到中转服务器 */
    ret = __websocket_send_video_frame_to_trans(socket_fd);
    malage_start = 0;
    MSHERR("set malage_start to %d\r\n", malage_start);
    if(MSH_FAILURE == ret)
    {
        MSHERR("websocket send video frame data error\r\n");
        return MSH_FAILURE;
    }
        
        
    MSHERR("websocket send video frame data success\r\n");
    

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 对收到的 websocket data msg 进行处理，type为3
 *       1: 客户端到服务器的登录请求以及服务器到客户端的登录响应
 *       2: 心跳包
 *       3: 数据包。
 *       4: 重定向数据包。
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_msg_cmd_processor_data(json_object * json_obj)
{
    int ret = 0;
    int msg_cmd = 0;    /* 收到的命令 */

    if(NULL == json_obj)
    {
        MSHERR("json obj is NULL\r\n");
        return MSH_FAILURE;
    }

    msg_cmd = (int)json_object_get_int(json_object_object_get(json_obj, "cmd"));
    MSHLOG("对收到的websocket数据包进行处理, msg_cmd:%d == 0x%08x\r\n", msg_cmd, msg_cmd);

    switch(msg_cmd)
    {
        case DEV_SRV_RECV_TRANS_SYN:
            /* 处理视频中转命令 */
            ret = __websocket_cmd_do_trans(json_obj);
            MSHLOG("__websocket_cmd_do_trans ret = %d\r\n", ret);

            break;

        default:
            MSHERR("msg_cmd:0x%08x unsupported\r\n");
            break;
    }

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 对收到的 websocket shake msg 进行处理，type为1
 *       1: 客户端到服务器的登录请求以及服务器到客户端的登录响应
 *       2: 心跳包
 *       3: 数据包。
 *       4: 重定向数据包。
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_msg_cmd_processor_shake(json_object * json_obj)
{
    int msg_code = -1;          /* 消息中的 code 字段 */
    int msg_heartbeat = -1;     /* 消息中的 heartbeat 字段 */
    int msg_timestamp = -1;     /* 消息中的 timestamp 字段 */

    if(NULL == json_obj)
    {
        MSHERR("json_obj is null\r\n");
        return MSH_FAILURE;
    }

    msg_code = (int)json_object_get_int(json_object_object_get(json_obj, "code"));
    if(200 == msg_code)
    {
        msg_heartbeat = (int)json_object_get_int(json_object_object_get(json_obj, "heartbeat"));
        msg_timestamp = (int)json_object_get_int(json_object_object_get(json_obj, "timestamp"));
        MSHLOG("msg_code:%d, msg_heartbeat:%d, msg_timestamp:%d\r\n", msg_code, msg_heartbeat, msg_timestamp);
    }
    else
    {
        MSHERR("msg_code error:%d\r\n", msg_code);
        return MSH_FAILURE;
    }

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 对收到的 websocket msg cmd 进行处理
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_msg_cmd_processor(char * msg_str, int msg_len)
{
    int ret = 0;
    json_object * json_obj = NULL;  /* 用于将字符串转为 json 对象 */
    int msg_type = 0;   /* websocket 协议中的 type 字段 */
    
    /* 判断传入参数是否为空 */
    if((NULL == msg_str) || (msg_len <= 0))
    {
        MSHERR("msg_str error, msg_len:%d\r\n", msg_len);
        return MSH_FAILURE;
    }

    /* 判断是不是 json 数据 */
    json_obj = json_tokener_parse(msg_str);
    if(NULL == json_obj)
    {
        MSHERR("json string error, msg_str:%s\r\n", msg_str);
        ret = MSH_FAILURE;
        goto error;
    }

    msg_type = (int)json_object_get_int(json_object_object_get(json_obj, "type"));
    MSHLOG("msg_type:%d\r\n", msg_type);

    //msg_type 表示websocket收到的数据包类型
    //1: 客户端到服务器的登录请求以及服务器到客户端的登录响应   
    //2: 心跳包
    //3: 数据包。
    //4: 重定向数据包。
    switch(msg_type)
    {
        case MSH_MSG_SHAKE:
            MSHLOG("MSH_MSG_SHAKE\r\n");
            __websocket_msg_cmd_processor_shake(json_obj);
            break;

        case MSH_MSG_HEART_BEAT:
            MSHLOG("MSH_MSG_HEART_BEAT\r\n");
            /* 对收到的心跳包进行相应处理 */

            break;

        case MSH_MSG_DATA:
        case MSH_MSG_REDIRECT:
            MSHLOG("MSH_MSG_DATA or MSH_MSG_REDIRECT\r\n");
            __websocket_msg_cmd_processor_data(json_obj);
            break;

        default:
            MSHERR("msg_type error:%d\r\n", msg_type);
            break;
    }



    ret = MSH_SUCCESS;

error:
    if(NULL != json_obj)
    {
        json_object_put(json_obj);  /* 释放 */
    }

    return ret;
}

/***********************************************
 *@brief 对收到的 websocket msg cmd 进行处理，线程池回调
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static void __websocket_msg_cmd_processor_thread(void *arg)
{
    int ret = 0;
    char * msg_str = NULL;
    int msg_len = 0;

    if(NULL == arg)
    {
        MSHERR("input string is NULL!\r\n");
        return;
    }

    msg_str = (char *)arg;
    msg_len = strlen(msg_str);

    MSHLOG("input string is:%s, len:%d\r\n", msg_str, msg_len);

    /* 处理 msg 数据内容 */
    ret = __websocket_msg_cmd_processor(msg_str, msg_len);
    if(MSH_FAILURE == ret)
    {
        MSHERR("websocket msg processor error:%s, msg_len:%d\r\n", msg_str, msg_len);
        free(arg);
        return;
    }
    MSHLOG("websocket msg processor success:%s, msg_len:%d\r\n", msg_str, msg_len);

    free(arg);
    return;
}

/***********************************************
 *@brief 对收到的 websocket msg 进行处理，阻塞，处理完成再返回数据
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static noPollMsg * msg_last = NULL;
static int __websocket_msg_processor_block(noPollMsg * msg_recv, char * encrypt_key)
{
    int ret = 0;
    char * msg_str = NULL;  /* 收到的消息内容 */
    int msg_len = 0;        /* 收到的消息长度 */
    char * tmp_str = NULL;  /* 用于临时存储一下变量 */
    noPollMsg * msg_tmp = NULL;

    msg_tmp = msg_last;
    msg_last = nopoll_msg_join(msg_last, msg_recv);
    nopoll_msg_unref(msg_tmp);

    if(nopoll_msg_is_final(msg_recv))
    {
        msg_str = (char*)nopoll_msg_get_payload(msg_last);
        msg_len = nopoll_msg_get_payload_size(msg_last);

        /* 对 msg 进行解密 */

        MSHLOG("对 msg 进行解密\r\n");

        /* 对数据进行解密，函数会对 tmp_str 重新分配内存，调用后需要 free 释放 */
        tmp_str = msg_str;
        ret = __websocket_encrypt_msg(&tmp_str, &msg_len, encrypt_key, MBEDTLS_AES_DECRYPT);
        if(MSH_SUCCESS != ret)
        {
            MSHERR("websocket recv msg string decrypt error\r\n");
            ret = MSH_FAILURE;
            goto error;
        }
        MSHLOG("解密后的websocket消息内容为:%s\r\n", tmp_str);

        /* 处理 msg 数据内容 */
        ret = __websocket_msg_cmd_processor(tmp_str, msg_len);
        if(MSH_FAILURE == ret)
        {
            MSHERR("websocket msg processor error:%s, msg_len:%d\r\n", tmp_str, msg_len);
            goto error;
        }

        nopoll_msg_unref(msg_last);
        msg_last = NULL;
    }
    else
    {
        MSHLOG("websocket收到的不是最后的数据\r\n");
        ret = MSH_FAILURE;
        goto error;
    }

    ret = MSH_SUCCESS;

error:
    nopoll_msg_unref(msg_recv);

    if(NULL != tmp_str)
    {
        free(tmp_str);
    }
    return ret;
}

/***********************************************
 *@brief 对收到的 websocket msg 进行处理，非阻塞，创建线程，在线程中处理消息
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_msg_processor_unblock(noPollMsg * msg_recv, char * encrypt_key)
{
    int ret = 0;
    char * msg_str = NULL;  /* 收到的消息内容 */
    int msg_len = 0;        /* 收到的消息长度 */
    char * tmp_str = NULL;  /* 用于临时存储一下变量 */
    noPollMsg * msg_tmp = NULL;

    msg_tmp = msg_last;
    msg_last = nopoll_msg_join(msg_last, msg_recv);
    nopoll_msg_unref(msg_tmp);

    if(nopoll_msg_is_final(msg_recv))
    {
        msg_str = (char*)nopoll_msg_get_payload(msg_last);
        msg_len = nopoll_msg_get_payload_size(msg_last);

        /* 对 msg 进行解密 */

        MSHLOG("对 msg 进行解密\r\n");

        /* 对数据进行解密，函数会对 tmp_str 重新分配内存，调用后需要 free 释放 */
        tmp_str = msg_str;
        ret = __websocket_encrypt_msg(&tmp_str, &msg_len, encrypt_key, MBEDTLS_AES_DECRYPT);
        if(MSH_SUCCESS != ret)
        {
            MSHERR("websocket recv msg string decrypt error\r\n");
            ret = MSH_FAILURE;
            goto error;
        }
        MSHLOG("解密后的websocket消息内容为:%s\r\n", tmp_str);

        /* 使用线程池 thread pool */
        if(NULL == msh_websocket_msg_thpool) /* meshare 库 websocket 消息处理专用的线程池 */
        {
            /* 如果为空，则创建初始化8个线程池 */
            msh_websocket_msg_thpool = thpool_init(8);
        }

        char * arg = NULL;
        arg = (char *)malloc(msg_len+1);    /* +1 是因为字符串结束符 \0 */
        if(NULL == arg)
        {
            MSHERR("thpool arg malloc error\r\n");
            ret = MSH_FAILURE;
            goto error;
        }
        memset(arg, 0, msg_len+1);
        memcpy(arg, tmp_str, msg_len);  /* 字符串结束符 \0 不需要复制 */

        /* 如果不为 NULL，则创建线程处理消息 */
        if(NULL != msh_websocket_msg_thpool)
        {
            ret = thpool_add_work(msh_websocket_msg_thpool, 
                    (void (*)(void*))__websocket_msg_cmd_processor_thread, (void *)arg);
            if(0 != ret)
            {
                MSHERR("websocket msg cmd processor thread create failure\r\n");
                /* 创建失败，释放 arg 内存，否则在线程中会释放 */
                if(NULL != arg)
                {
                    free(arg);
                }
                ret = MSH_FAILURE;
                goto error;
            }
            MSHLOG("websocket msg cmd processor thread create success\r\n");
        }

        nopoll_msg_unref(msg_last);
        msg_last = NULL;
    }
    else
    {
        MSHLOG("websocket收到的不是最后的数据\r\n");
        ret = MSH_FAILURE;
        goto error;
    }

    ret = MSH_SUCCESS;

error:
    nopoll_msg_unref(msg_recv);

    if(NULL != tmp_str)
    {
        free(tmp_str);
    }
    return ret;
}


/***********************************************
 *@brief 使用 select 机制接收 websocket 消息 
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_select_recv_msg(noPollConn * conn, char * encrypt_key, int timeout_ms)
{
    int ret = 0;
    int select_fd = 0;
    noPollMsg * msg_recv = NULL;
    fd_set readfds;
    timeval timeout; 
    FD_ZERO(&readfds);
    int retry_times = 0;    /* 接收 msg 的重试次数 */

    select_fd = nopoll_conn_socket(conn);
    g_websocket_socket_fd = select_fd;   /* 用于调试的全局变量 */
    FD_SET(select_fd, &readfds);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    ret = select(select_fd+1, &readfds, NULL, NULL, &timeout);
    if(ret > 0)
    {
        if(FD_ISSET(select_fd, &readfds))
        {
            for(retry_times = 0; retry_times < 3; retry_times ++)   /* 3 表示重试次数 */
            {
                MSHLOG("read websocket fd\r\n");
                msg_recv = nopoll_conn_get_msg(conn);
                if(NULL != msg_recv)
                {
                    /* 处理 msg_recv 收到的数据内容 */
                    MSHLOG("开始处理 msg_recv 收到的数据内容\r\n");
                    ret = __websocket_msg_processor_unblock(msg_recv, encrypt_key);
                    if(MSH_SUCCESS != ret)
                    {
                        MSHERR("消息处理出错\r\n");
                    }
                    MSHLOG("消息处理成功\r\n");
                    break;
                }
                MSHLOG("msg_recv error, retry_times:%d\r\n", retry_times);
            }
        }
    }

    return MSH_SUCCESS;
}

/***********************************************
 *@brief websocket 发送心跳包
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_send_heartbeat_msg(noPollConn * conn, char * encrypt_key)
{
    int ret = 0;
    char * msg = NULL;              /* 用于心跳消息的创建 */
    int msg_len = NULL;          /* 心跳消息的长度 */
    char * send_buf = NULL;         /* 发送给 websocket 服务器的数据 buffer */
    int send_len = { 0 };           /* 发送给 websocket 服务器的数据长度 */
    json_object * json_obj = NULL;  /* 用于将字符串转为 json 对象 */

    /* 创建发送心跳用的 msg */
    json_obj = json_object_new_object();
    if(NULL == json_obj)
    {
        MSHLOG("json_obj 创建失败\r\n");
        return MSH_FAILURE;
    }

    json_object_object_add(json_obj, "type", json_object_new_int(MSH_MSG_HEART_BEAT));
    json_object_object_add(json_obj, "length", json_object_new_int(0));

    msg = (char *)json_object_get_string(json_obj);
    msg_len = strlen(msg);
    MSHLOG("msg_len:%d, msg:%s\r\n", msg_len, msg);

    /* 对 msg 进行加密, 此函数中会重新分配内存，需要对msg进行手动释放 free */
    ret = __websocket_encrypt_msg(&msg, &msg_len, encrypt_key, MBEDTLS_AES_ENCRYPT);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("msg加密失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }

    /* 使用 msg 继续创建 send_buf (添加头部数据) */
    send_len = 2 + msg_len;
    send_buf = (char *)malloc(send_len);
    if(NULL == send_buf)
    {
        MSHLOG("send_buf malloc error, send_len:%d\r\n", send_len);
        ret = MSH_FAILURE;
        goto error;
    }
    MSHLOG("websocket heartbeat send_len:%d begin\r\n", send_len);

    *(short*)send_buf = htons(2);   /* 头部未加密部分长度 2 字节 */
    memcpy(send_buf+2, msg, msg_len);

    /* websocket 发送心跳信息给服务器 */
    ret = __websocket_send_msg_text(conn, send_buf, send_len);
    if(MSH_SUCCESS != ret)
    {
        MSHERR("websocket heartbeat send error\r\n");
        ret = MSH_FAILURE;
        goto error;
    }
    MSHLOG("websocket heartbeat send success\r\n");

    ret = MSH_SUCCESS;

error:
    if(NULL != msg)
    {
        free(msg);
    }

    if(NULL != send_buf)
    {
        free(send_buf);
    }

    if(NULL != json_obj)
    {
        json_object_put(json_obj);  /* 释放 */
    }

    return ret;
}

/***********************************************
 *@brief 执行 nopoll 库提供的相关接口登录websocket服务器 
 *@param timeout_ms 参数1 http 请求超时时间 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_do_login_begin(noPollCtx ** ctx, char * ip, int port)
{
    int ret = 0;
    noPollConn * conn = NULL;
    char port_str[8] = { 0 };
    char * msg = NULL;  /* websocket 登录消息 */
    int msg_len = 0;  /* websocket 登录消息的长度 */
    json_object * json_obj = NULL;  /* json 对象，用于创建 msg 消息 */
    char tokenid[128] = { 0 };      /* 用于存放web服务器返回的tokenid */
    char encrypt_key[128] = { 0 };  /* 用于存放web服务器返回的encrypt_key */
    char encrypt_key_id[128] = { 0 };  /* 用于存放web服务器返回的encrypt_key_id */
    char * send_buf = NULL;         /* 发送给 websocket 服务器的数据 buffer */
    int send_len = { 0 };           /* 发送给 websocket 服务器的数据长度 */
    int retry_times = 0;            /* 等待sebsocket数据发送成功的重试次数 */
    noPollMsg * msg_recv = NULL;    /* 用于接收服务器返回的消息 */

    MSHLOG("##############\r\n");
    *ctx = nopoll_ctx_new();
    MSHLOG("##############\r\n");
    if(NULL == *ctx)
    {
        MSHLOG("nopoll_ctx_new 失败\r\n");
        return MSH_FAILURE;
    }

    /* 把 int 型 port 转为字符串 */
    sprintf(port_str, "%d", port);
    MSHLOG("##############\r\n");
    conn = nopoll_conn_new(*ctx, ip, port_str, NULL, "/ws", NULL, NULL);
    MSHLOG("##############\r\n");
    if(0 == nopoll_conn_is_ok(conn))
    {
        MSHLOG("connect to %s:%s error.\r\n", ip, port_str);
        ret = MSH_FAILURE;
        goto error;
    }
    MSHLOG("connect to %s:%s success.\r\n", ip, port_str);

    if(0 == nopoll_conn_wait_until_connection_ready(conn, 7))   /* timeout: 7 */
    {
        MSHLOG("connect to %s:%s error.\r\n", ip, port_str);
        ret = MSH_FAILURE;
        goto error;
    }

    MSHLOG("websocket connect success\r\n");

    /* 创建登录用的 msg */
    json_obj = json_object_new_object();
    if(NULL == json_obj)
    {
        MSHLOG("json_obj 创建失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }
    json_object_object_add(json_obj, "type", json_object_new_int(MSH_MSG_SHAKE));
    json_object_object_add(json_obj, "length", json_object_new_int(sizeof(msh_login_body)));
    json_object_object_add(json_obj, "cmd_version", json_object_new_string("1.0.0.0"));
    json_object_object_add(json_obj, "data_type", json_object_new_int(1));
    http_get_parameter_web_info(tokenid, HTTP_ADDITION_STR);  /* 获取 tokenid */
    MSHLOG("tokenid: %s\r\n", tokenid);
    json_object_object_add(json_obj, "token_id", json_object_new_string(tokenid));
    json_object_object_add(json_obj, "client_type", json_object_new_int(0));
    json_object_object_add(json_obj, "client_version", json_object_new_string("V1.0"));
    json_object_object_add(json_obj, "client_id", json_object_new_string(DEVICE_ID_TEST));

    msg = (char *)json_object_get_string(json_obj);
    msg_len = strlen(msg);
    MSHLOG("msg_len:%d, msg:%s\r\n", msg_len, msg);

    ret = http_get_parameter_web_info(encrypt_key, HTTP_ENCRYPT_KEY_STR);  /* 获取 encrypt_key */
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("获取 encrypt_key 失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }
    MSHLOG("encrypt_key:%s\r\n", encrypt_key);

    ret = http_get_parameter_web_info(encrypt_key_id, HTTP_ENCRYPT_KEY_ID_STR);  /* 获取 encrypt_key_id */
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("获取 encrypt_key_id 失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }
    MSHLOG("encrypt_key_id:%s\r\n", encrypt_key_id);

    /* 对 msg 进行加密, 此函数中会重新分配内存，需要对msg进行手动释放 free */
    ret = __websocket_encrypt_msg(&msg, &msg_len, encrypt_key, MBEDTLS_AES_ENCRYPT);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("msg加密失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }

    /* 使用 msg 继续创建 send_buf (添加头部数据) */
    send_len = 4 + strlen(encrypt_key_id) + msg_len;
    send_buf = (char *)malloc(send_len);
    if(NULL == send_buf)
    {
        MSHLOG("send_buf malloc error, send_len:%d\r\n", send_len);
        ret = MSH_FAILURE;
        goto error;
    }
    MSHLOG("websocket send_len:%d begin\r\n", send_len);

    *(short*)send_buf = htons(strlen(encrypt_key_id)+4);
    *(short*)(send_buf+2) = htons(strlen(encrypt_key_id));
    strcpy(send_buf+4, encrypt_key_id);
    memcpy(send_buf+4+strlen(encrypt_key_id), msg, msg_len);

    /* websocket 发送登录信息给服务器 */
    ret = __websocket_send_msg_text(conn, send_buf, send_len);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("websocket send msg failure:%s\r\n", msg);
        ret = MSH_FAILURE;
        goto error;
    }

#if 1       /* 对数据进行解密，用于打印调试 */
    __websocket_encrypt_msg(&msg, &msg_len, encrypt_key, MBEDTLS_AES_DECRYPT);
    MSHLOG("解密后的 msg:%s\r\n", msg);
#endif
    
    /* 等待发送成功和 websocket 服务器返回数据 */
    for(retry_times = 50; retry_times > 0; retry_times --)
    {
        MSHLOG("等待发送成功和 websocket 服务器返回数据\r\n");
        if(nopoll_conn_pending_write_bytes(conn))
        {
            nopoll_conn_flush_writes(conn, 1000*1000, 0);
        }
        
        msg_recv = nopoll_conn_get_msg(conn);
        if(NULL != msg_recv)
        {
            /* 处理 msg_recv 收到的数据内容 */
            MSHLOG("开始处理 msg_recv 收到的数据内容\r\n");
            ret = __websocket_msg_processor_block(msg_recv, encrypt_key);
            if(MSH_SUCCESS == ret)
            {
                MSHLOG("消息处理流程\r\n");
                break;
            }
        }

        usleep(500*1000);
    }

#if 1       /* 调试代码 ----------------------------- */
    /* 持续接收消息测试 */
    while(1)
    {
        if(0 != nopoll_conn_is_ok(conn))
        {
            if(nopoll_conn_pending_write_bytes(conn))
            {
                nopoll_conn_flush_writes(conn, 1000*1000, 0);
            }

            /* 接收消息，超时时间 5 秒 */
            ret = __websocket_select_recv_msg(conn, encrypt_key, 5000);    /* 超时时间 5 秒 */
            
            MSHLOG("ret = %d\r\n", ret);

            /* 心跳发送 */
            int now_time = __websocket_get_uptime();

            MSHLOG("now_time:%d, last_send_heartbeat_time:%d\r\n", now_time, last_send_heartbeat_time);
            if((now_time - last_send_heartbeat_time) >= 30)
            {
                /* 发送心跳 */
                MSHLOG("heartbeat send begin\r\b");
                last_send_heartbeat_time = now_time;
                __websocket_send_heartbeat_msg(conn, encrypt_key);
            }
        }
        else
        {
            MSHERR("############\r\n");
            sleep(1);
            ret = MSH_FAILURE;
            goto error;
        }
    }
#endif       /* 调试代码 ----------------------------- */

    MSHLOG("websocket wait for login ack timeout\r\n");

    ret = MSH_SUCCESS;

error:
    if(NULL != conn)
    {
        nopoll_conn_close(conn);
    }

    if(NULL != msg)
    {
        free(msg);
    }

    if(NULL != json_obj)
    {
        json_object_put(json_obj);  /* 释放 */
    }

    if(NULL != send_buf)
    {
        free(send_buf);
    }

    return MSH_SUCCESS; /* ret, 成功：MSH_SUCCESS，失败：MSH_FAILURE */
}

/***********************************************
 *@brief 执行 websocket 登录到服务器 
 *@param timeout_ms 参数1 http 请求超时时间 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __websocket_do_login(int timeout_ms)
{
    int ret = 0;
    char websocket_ip[32] = { 0 };  /* websocket 服务器的 IP */
    int websocket_port = 0;         /* websocket 服务器的 端口 */
    noPollCtx * ctx = NULL;         /* nopoll context */

    /* 获取 websocket 服务器的 IP 和 端口，即 DEV_CONN_ADDR "dev_conn_addr" 的地址 */
    ret = __websocket_get_ip_port(websocket_ip, &websocket_port);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("获取websocket服务器的IP和端口失败\r\n");
        return MSH_FAILURE;
    }
    MSHLOG("websocket服务器IP:%s, port:%d\r\n", websocket_ip, websocket_port);
    
    /* 开始登录websocket服务器 */
    MSHLOG("##############\r\n");
    ret = __websocket_do_login_begin(&ctx, websocket_ip, websocket_port);
    MSHLOG("##############\r\n");
    if(MSH_SUCCESS != ret)
    {
        MSHERR("登录websocket服务器失败\r\n");
    }
    MSHINF("登录websocket服务器成功\r\n");

    if(NULL != ctx)
    {
        nopoll_ctx_unref(ctx);
        ctx = NULL;
    }

    return MSH_FAILURE; /* 调试 */
    //return MSH_SUCCESS;
}
/***********************************************
 *@brief websocket 登录到服务器，阻塞，登录成功才会返回
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
int websocket_login(void)
{
    int ret = 0;
    int timeout_ms_min = 5000;  /* 默认最小超时时间 ms */
    int timeout_ms = 0;         /* 请求的超时时间 ms */
    int times = 0;              /* 连接websocket服务器的次数 */

    /* 初始化 meshare userid 列表 */
    msh_video_user_list_init();

    timeout_ms = timeout_ms_min;
    ret = MSH_FAILURE;  /* 设置 ret 默认值为 MSH_FAILURE */
    MSHLOG("开始连接到 websocket 服务器 \r\n");
    do
    {
        timeout_ms += 300;  /* 每次失败，延时时间增加 300ms */
        times ++;   /* 连接websocket服务器的次数 */
        MSHLOG("第 [%d] 次尝试连接websocket服务器，超时时间设置为 [%d] ms\r\n", times, timeout_ms);
        ret = __websocket_do_login(timeout_ms);

        if(timeout_ms > 10000)
        {
            timeout_ms = timeout_ms_min;  /* 如果一直失败，超时时间大于10秒仍失败时，重新计时 */
        }
        usleep(100*1000);   /* 如果失败，延时 100ms 后重新登录 */
    }while(ret != MSH_SUCCESS);
    MSHLOG("连接到websocket服务器成功\r\n");
    MSHLOG("系统当前运行的秒数:%d\r\n", __websocket_get_uptime());

    return MSH_SUCCESS;
}


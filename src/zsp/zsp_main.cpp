#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
/* #include <netdb.h> */    /* liteos 不支持 */
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include "zsp_interfacedef.h"
/* mbedtls 库的头文件 */
//#include "aes.h"

/* 调用线程池所依赖的头文件 */
#include <pthread.h>
#include "thpool.h"

/* 处理TCP客户端消息 */
#include "zsp_cmd_processor.h"


static threadpool zsp_thpool = NULL;    /* ZSP库专用的线程池 */

/***********************************************
 *@brief 绑定TCP端口并监听 
 *@param port   参数1 端口，ZSP 库TCP服务器默认使用 ZSP_TCP_SERVER_PORT 8000  
 *@retrun
 *      成功：返回 socket fd
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_tcpserver_bind(int port)
{
    ZSPLOG("##########################zsp tcp server test start!\r\n");
    int ret = 0;
    int server_fd = 0;
    struct sockaddr_in addr;

    addr.sin_port=htons(port);  /* TCP服务器监听端口号 */
    addr.sin_addr.s_addr=htonl(INADDR_ANY); /* 监听任意IP */
    addr.sin_family=AF_INET;    /* IPv4 */

    server_fd = socket(AF_INET, SOCK_STREAM, 0);    /* 创建 TCP socket */
    if(server_fd < 0)
    {
        ZSPLOG("创建 TCP socket 失败:%s\r\n", strerror(errno));
        goto error;
    }
    ZSPLOG("创建 TCP socket 成功\r\n");

    ret = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(0 != ret)
    {
        ZSPLOG("绑定失败:%s\r\n", strerror(errno));
        goto error;
    }

    ret = listen(server_fd, 3); 
    if(0 != ret)
    {
        ZSPLOG("listen socket 失败:%s\r\n", strerror(errno));
        goto error;
    }
    ZSPLOG("设置监听 socket 成功\r\n");

    /* 设置 socket 为非阻塞 */
    //fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);    /* 注释掉，取消非阻塞设置 */
    
    return server_fd;

error:  /* 如果错误，则关闭 socket 返回 ZSP_FAILURE -1 */
    if(server_fd > 0)
    {
        close(server_fd);
    }

    return ZSP_FAILURE;
}

/***********************************************
 *@brief 处理客户端的TCP连接请求
 *@param server_fd 参数1 ZSP TCP 服务器监听的 socket fd
 *@retrun
 *      成功：返回 socket fd
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_tcpserver_accept(int server_fd)
{
    int ret = 0;
    int accept_fd = 0;  /* TCP连接创建后用于通信的 socket fd */
    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    int * thpool_arg = NULL;    /* 用于向线程池中传递 socket fd */
    //Cmd_Header * header = NULL;

    //ZSPLOG("-------------- start, server_fd:%d\r\n", server_fd);
    if(server_fd < 0)   /* 如果TCP服务器 socket fd 有问题，返回失败 -1 */
    {
        ZSPLOG("server_fd error:%d\r\n", server_fd);
        return ZSP_FAILURE;
    }

    /* accept new connection */
    accept_fd = accept(server_fd,(struct sockaddr *) &from_addr, &from_addr_len);
    ZSPLOG("-------------- accept_fd:%d\r\n", accept_fd);
    if(accept_fd < 0)   /* 检查accept返回的 socket fd，小于0说明出错，返回失败 -1 */
    {
        ZSPLOG("accept,accept_fd: %d, 应该是有消息处理线程阻塞，描述符不够用了\r\n", accept_fd);
        if(errno == EAGAIN)
        {
            ZSPLOG("accept:%s\r\n", strerror(errno));
            return ZSP_FAILURE;
        }
        else
        {
            ZSPLOG("accept_fd:%d\r\n", accept_fd);
            return ZSP_FAILURE;
        }
    }

    /* 打印TCP客户端的IP和端口号 */
    ZSPLOG("addr = %s; port = %d\r\n", inet_ntoa(from_addr.sin_addr), from_addr.sin_port);

    /* 设置 socket 为非阻塞 */
    //fcntl(accept_fd, F_SETFL, fcntl(accept_fd, F_GETFL, 0) | O_NONBLOCK); /* 注释掉，取消非阻塞设置，send 时设置*/

    /* 初始化传入线程池的参数 */
    /* 使用了 malloc 分配内存，如果加入线程池失败，则立即free */
    /*                         如果加入线程池成功，则在线程中free */
    thpool_arg = (int *)malloc(sizeof(int));
    if(NULL == thpool_arg)
    {
        ZSPLOG("thpool_arg malloc 失败！！！\r\n");
        /* 失败则关闭 socket 并返回失败 */
        if(accept_fd > 0)
        {
            close(accept_fd);
        }
        return ZSP_FAILURE;
    }
    *thpool_arg = accept_fd;    /* 将 accept_fd 传值给 thpool_arg */

    /* 将消息处理线程加入到线程池并传入参数 thpool_arg，此线程对各命令响应结束即返回 */
    /* 大部分命令为即时响应后立即退出，不会长时间占用线程池资源 */
    /* 少部分命令会较长时间占用线程池资源，如视频播放线程，直到结束播放时才会返回 */
    ret = thpool_add_work(zsp_thpool, zsp_tcp_cmd_processor, (void *)thpool_arg);
    if(0 != ret)
    {
        ZSPLOG("向线程池中添加消息处理任务失败\r\n");
        /* 如果失败，需要在此 free 掉 thpool_arg，否则内存泄露 */
        /* 如果成功，会在线程中 free */
        if(NULL != thpool_arg)
        {
            free(thpool_arg);
            thpool_arg = NULL;
        }
        /* 如果加入线程池失败，需要在此关闭 accept_fd */
        if(accept_fd > 0)
        {
            close(accept_fd);
            accept_fd = 0;
        }
        return ZSP_FAILURE;
    }

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 创建一个 TCP server 端，用于处理ZSP TCP请求，默认端口：8000
 *@param arg 参数1 NULL，暂时没有用到参数信息
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0 
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_tcpserver_create(void * arg)
{
    int ret = 0;
    int server_fd = 0;

    /* 初始化视频播放用户列表 */
    ret = zsp_video_user_list_init();
    if(ZSP_FAILURE == ret)
    {
        ZSPLOG("初始化视频播放用户列表 失败\r\n");
        return ZSP_FAILURE;
    }

    /* 绑定并监听TCP端口 */
    server_fd = zsp_tcpserver_bind(ZSP_TCP_SERVER_PORT);
    if(server_fd < 0)
    {
        ZSPLOG("绑定并监听TCP端口失败, server_fd=%d\r\n", server_fd);
        return ZSP_FAILURE;
    }

    ZSPINF("创建 TCP server 成功\r\n");
    while(1)
    {
        ret = zsp_tcpserver_accept(server_fd);
        if(ret < 0)
        {
            /* 如果出 accept 出错则 sleep 1秒后继续 */
            ZSPLOG("accept error, ret:%d\r\n", ret);
            sleep(1);
        }
        usleep(50*1000);
    }

    return ZSP_SUCCESS;
}

/***********************************************
 *@brief 绑定UDP端口
 *@param port   参数1 端口，ZSP 库UDP服务器默认使用 ZSP_UDP_SERVER_PORT 8080  
 *@retrun
 *      成功：返回 socket fd
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_udpserver_bind(int port)
{
    ZSPLOG("##########################zsp udp server test start!\r\n");
    int ret = 0;
    int server_fd = 0;
    struct sockaddr_in addr;
    int udp_optval = 0;

    addr.sin_port=htons(port);  /* UDP服务器使用端口号 */
    addr.sin_addr.s_addr=htonl(INADDR_ANY); /* 监听任意IP */
    addr.sin_family=AF_INET;    /* IPv4 */

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);    /* 创建 UDP socket */
    if(server_fd < 0)
    {
        ZSPLOG("创建 UDP socket 失败:%s\r\n", strerror(errno));
        goto error;
    }
    ZSPLOG("创建 UDP socket 成功\r\n");

    ret = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(0 != ret)
    {
        ZSPLOG("绑定UDP端口失败:%s\r\n", strerror(errno));
        goto error;
    }

    udp_optval = 1; /* 1表示 true:支持发送广播数据 */
    ret = setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, (const char *)&udp_optval, sizeof(udp_optval));
    if(0 != ret)
    {
        ZSPLOG("设置udp socket支持发送广播数据失败，ret:%d, %s\r\n", ret, strerror(errno));
        goto error;
    }

    return server_fd;

error:  /* 如果错误，则关闭 socket 返回 ZSP_FAILURE -1 */
    if(server_fd > 0)
    {
        close(server_fd);
    }

    return ZSP_FAILURE;
}

/***********************************************
 *@brief 创建一个 UDP server 端，用于处理ZSP UDP数据并响应，默认端口：8080
 *@param arg 参数1 NULL，暂时没有用到参数信息
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0 
 *      失败：返回 ZSP_FAILURE -1
 **********************************************/
static int zsp_udpserver_create(void * arg)
{
    int ret = 0;
    int server_fd = 0;
    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    char cmd_buf[ZSP_CMD_UDP_BUF_SIZE] = {0};    /* UDP socket 数据接收 buffer */
    int cmd_buf_len = 0;    /* 表示已接收到的字节数 */
    STRUCT_UDP_THPOOL_ARG * udp_thpool_arg = NULL;   /* 用于向 udp cmd 处理线程传入参数 */

    /* 绑定UDP端口 */
    server_fd = zsp_udpserver_bind(ZSP_UDP_SERVER_PORT);
    if(server_fd < 0)
    {
        ZSPLOG("绑定UDP端口失败, server_fd=%d\r\n", server_fd);
        return ZSP_FAILURE;
    }

    ZSPINF("创建 UDP server 成功\r\n");

    /* 循环接收UDP广播数据并分别进行处理 */
    while(1)
    {
        ZSPLOG("UDP recvfrom 开始接收数据\r\n");
        /* 默认会在此阻塞 */
        ret = recvfrom(server_fd, cmd_buf, ZSP_CMD_UDP_BUF_SIZE, 0,
                (sockaddr *)&from_addr, &from_addr_len);
        if(ret <= 0)    /* 出错或者socket已关闭 */
        {
            ZSPLOG("UDP recvfrom 数据接收出错, ret:%d\r\n", ret);
            usleep(500*1000);  /* sleep 500ms 后继续 */
            continue;
        }

        /* 打印接收到 ret 字节的数据 */
        //ZSPLOG("UDP recvfrom 接收到 %d 字节数据\r\n", ret);

        /* 初始化传入线程池的参数 */
        /* 使用了 malloc 分配内存，如果加入线程池失败，则立即free */
        /*                         如果加入线程池成功，则在线程中free */
        udp_thpool_arg = (STRUCT_UDP_THPOOL_ARG *)malloc(sizeof(STRUCT_UDP_THPOOL_ARG));
        if(NULL == udp_thpool_arg)
        {
            ZSPLOG("malloc udp_thpool_arg error\r\n");
            usleep(100*10000);  /* 如果出错，sleep 100ms 后继续 */
            continue;
        }
        memset(udp_thpool_arg, 0, sizeof(STRUCT_UDP_THPOOL_ARG)); /* 清理udp_thpool_arg为0 */
        udp_thpool_arg->cmd_len= ret;    /* ret 为 recvfrom 接收到数据长度 */
        udp_thpool_arg->cmd_buf = (char *)malloc(udp_thpool_arg->cmd_len);
        if(NULL == udp_thpool_arg->cmd_buf)
        {
            ZSPLOG("malloc udp_thpool_arg->cmd_buf error\r\n");
            if(udp_thpool_arg)
            {
                free(udp_thpool_arg);
                udp_thpool_arg = NULL;
            }
            usleep(100*10000);  /* 如果出错，sleep 100ms 后继续 */
            continue;
        }
        memset(udp_thpool_arg->cmd_buf, 0, sizeof(udp_thpool_arg->cmd_len));  /* 清理udp_thpool_arg->cmd_buf为0 */
        memcpy(udp_thpool_arg->cmd_buf, cmd_buf, udp_thpool_arg->cmd_len);    /* 复制收到的数据 */
        memcpy(&(udp_thpool_arg->from_addr), &from_addr, sizeof(struct sockaddr_in));    /* 复制数据来源地址信息 */
        udp_thpool_arg->socket_fd = server_fd;   /* 收到数据的socket，可能会多线程send，需要互斥锁 */

        /* 将消息处理线程加入到线程池并传入参数 udp_thpool_arg */
        ret = thpool_add_work(zsp_thpool, zsp_udp_cmd_processor, (void *)udp_thpool_arg);
        if(0 != ret)
        {
            ZSPLOG("向线程池中添加消息处理任务失败\r\n");
            /* 如果失败，需要在此 free 掉 udp_thpool_arg，否则内存泄露 */
            /* 如果成功，会在线程中 free */
            if(udp_thpool_arg->cmd_buf != NULL) /* 释放 udp_thpool_arg->cmd_buf */
            {
                free(udp_thpool_arg->cmd_buf);
                udp_thpool_arg->cmd_buf = NULL;
            }

            if(NULL != udp_thpool_arg) /* 释放 udp_thpool_arg */
            {
                free(udp_thpool_arg);
                udp_thpool_arg = NULL;
            }
        }
    }

    return ZSP_SUCCESS;
}


/***********************************************
 *@brief ZSP 启动的入口函数
 *@param void 不需要传入参数
 *@retrun
 *      成功：返回 ZSP_SUCCESS 0 
 *      失败：返回 ZSP_FAILURE -1 目前不考虑失败的情况
 **********************************************/
int zsp_main(void)
{
    /* 如果 zsp_thpool 为 NULL，则初始化线程池 */
    if(NULL == zsp_thpool)
    {
        zsp_thpool = thpool_init(8);
    }

    /* 如果 zsp_thpool 不为 NULL，则创建 ZSP TCP server 线程，此线程在线程池中常驻，会持续占用一个线程 */
    if(NULL != zsp_thpool)
    {
        thpool_add_work(zsp_thpool, (void (*)(void*))zsp_tcpserver_create, NULL);
    }

    /* 如果 zsp_thpool 不为 NULL，则创建 ZSP UDP server 线程，此线程在线程池中常驻，会持续占用一个线程 */
    if(NULL != zsp_thpool)
    {
        thpool_add_work(zsp_thpool, (void (*)(void*))zsp_udpserver_create, NULL);
    }

    /* 不需要销毁线程池 */
    //thpool_destroy(thpool);

    return ZSP_SUCCESS;
}

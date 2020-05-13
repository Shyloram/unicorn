#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>     
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "thpool.h"
#include "zsp.h"
#include "zspcmd.h"

static threadpool zsp_thpool = NULL;

static int CreateTcpServer(void * arg)
{
    int ret = 0;
    int server_fd = 0;
    int accept_fd = 0;  
    struct sockaddr_in addr;
    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    int * thpool_arg = NULL;    

    addr.sin_port = htons(ZSP_TCP_SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
    {
        ZSPERR("Create TCP socket failed:%s\r\n", strerror(errno));
		return -1;
    }

    ret = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(0 != ret)
    {
        ZSPERR("bind failed:%s\r\n", strerror(errno));
		close(server_fd);
		return -1;
    }

    ret = listen(server_fd, 3); 
    if(0 != ret)
    {
        ZSPERR("listen socket failed:%s\r\n", strerror(errno));
		close(server_fd);
		return -1;
    }

    while(1)
	{
		accept_fd = accept(server_fd, (struct sockaddr *) &from_addr, &from_addr_len);
		ZSPLOG("accept_fd:%d,addr: %s,port: %d\r\n", accept_fd, inet_ntoa(from_addr.sin_addr), from_addr.sin_port);
		if(accept_fd < 0)
		{
			ZSPERR("accept:%s\r\n", strerror(errno));
			sleep(1);
			continue;
		}

		thpool_arg = (int *)malloc(sizeof(int));
		if(NULL == thpool_arg)
		{
			ZSPERR("thpool_arg malloc failed!\r\n");
			close(accept_fd);
			sleep(1);
			continue;
		}
		*thpool_arg = accept_fd;

		ret = thpool_add_work(zsp_thpool, TcpCmdProcessor, (void *)thpool_arg);
		if(0 != ret)
		{
			ZSPERR("add work failed\r\n");
			free(thpool_arg);
			close(accept_fd);
			sleep(1);
			continue;
		}
		usleep(50*1000);
	}
    return 0;
}

static int CreateUdpServer(void * arg)
{
    int ret = 0;
    int server_fd = 0;
    struct sockaddr_in addr;
    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    char cmd_buf[ZSP_CMD_UDP_BUF_SIZE] = {0};
    STRUCT_UDP_THPOOL_ARG * udp_thpool_arg = NULL;

    addr.sin_port=htons(ZSP_UDP_SERVER_PORT);  
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_family=AF_INET;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_fd < 0)
    {
        ZSPERR("UDP socket failed:%s\n", strerror(errno));
		return -1;
    }

    ret = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(0 != ret)
    {
        ZSPERR("bind failed:%s\n", strerror(errno));
		close(server_fd);
		return -1;
    }

    int udp_optval = 1; /* 1表示 true:支持发送广播数据 */
    ret = setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, (const char *)&udp_optval, sizeof(udp_optval));
    if(0 != ret)
    {
        ZSPERR("set BROADCAST failed,ret:%d, %s\r\n", ret, strerror(errno));
		close(server_fd);
		return -1;
    }

    while(1)
    {
        ret = recvfrom(server_fd, cmd_buf, ZSP_CMD_UDP_BUF_SIZE, 0, (sockaddr *)&from_addr, &from_addr_len);
        if(ret <= 0)
        {
            ZSPERR("UDP recvfrom failed, ret:%d\r\n", ret);
            usleep(500*1000);
            continue;
        }

        //ZSPLOG("UDP recvfrom recv %d\r\n", ret);

        udp_thpool_arg = (STRUCT_UDP_THPOOL_ARG *)malloc(sizeof(STRUCT_UDP_THPOOL_ARG));
        if(NULL == udp_thpool_arg)
        {
            ZSPERR("malloc udp_thpool_arg error\r\n");
            usleep(100*10000);
            continue;
        }

        memset(udp_thpool_arg, 0, sizeof(STRUCT_UDP_THPOOL_ARG));
        udp_thpool_arg->cmd_len= ret;
        udp_thpool_arg->cmd_buf = (char *)malloc(udp_thpool_arg->cmd_len);
        if(NULL == udp_thpool_arg->cmd_buf)
        {
            ZSPERR("malloc udp_thpool_arg->cmd_buf error\r\n");
			free(udp_thpool_arg);
            usleep(100*10000); 
            continue;
        }
        memcpy(udp_thpool_arg->cmd_buf, cmd_buf, udp_thpool_arg->cmd_len);
        memcpy(&(udp_thpool_arg->from_addr), &from_addr, sizeof(struct sockaddr_in));
        udp_thpool_arg->socket_fd = server_fd;

        ret = thpool_add_work(zsp_thpool, UdpCmdProcessor, (void *)udp_thpool_arg);
        if(0 != ret)
        {
            ZSPERR("thpool add work failed\r\n");
			free(udp_thpool_arg->cmd_buf);
			free(udp_thpool_arg);
        }
    }
    return 0;
}

int StartZspServer(void)
{
    if(NULL == zsp_thpool)
    {
        zsp_thpool = thpool_init(8);
    }

    if(NULL != zsp_thpool)
    {
        thpool_add_work(zsp_thpool, (void (*)(void*))CreateTcpServer, NULL);
    }

    if(NULL != zsp_thpool)
    {
        thpool_add_work(zsp_thpool, (void (*)(void*))CreateUdpServer, NULL);
    }

    return 0;
}

int StopZspServer(void)
{
    return 0;
}

#ifndef __ZSP_H_
#define __ZSP_H_

#include <netinet/in.h>
#include "zmdconfig.h"

#ifdef ZMD_APP_DEBUG_ZSP
#define ZSPLOG(format, ...)     fprintf(stdout, "\033[37m[ZSPLOG][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define ZSPLOG(format, ...)
#endif
#define ZSPERR(format, ...)     fprintf(stdout, "\033[31m[ZSPERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)

#define ZSP_TCP_SERVER_PORT (8000)
#define ZSP_UDP_SERVER_PORT (8080)

/* 用于向 UDP 命令处理线程中传入参数的结构体 */
typedef struct
{
    struct sockaddr_in from_addr;   /* UDP数据来源IP地址信息 */
    int socket_fd;                  /* UDP socket fd */
    int cmd_len;                    /* 收到的消息数据长度 */
    char *cmd_buf;                  /* 收到的消息数据内容 */
}STRUCT_UDP_THPOOL_ARG;

int StartZspServer(void);
int StopZspServer(void);

#endif /* __ZSP_H_ */

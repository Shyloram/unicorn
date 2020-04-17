#ifndef __ZSP_CMD_PROCESSOR_H__
#define __ZSP_CMD_PROCESSOR_H__

#include <netinet/in.h>
#include "zmdconfig.h"

#define ZSP_IP_ADDR_LEN (64)    /* IP地址 buffer 的最大长度 */

#ifdef ZMD_APP_WIFI
#define ZSP_IF_NAME "wlan0"      /* 有线：eth0， 无线：wlan0 */
#else
#define ZSP_IF_NAME "eth0"      /* 有线：eth0， 无线：wlan0 */
#endif

#define ZSP_CMD_TCP_BUF_SIZE (256)  /* TCP 服务器接收数据的 buffer 大小 */
#define ZSP_CMD_TCP_RETRY_TIMES (3) /* TCP 服务器接收数据出错的重试次数 */

#define ZSP_CMD_UDP_BUF_SIZE (256)  /* UDP 服务器接收数据的 buffer 大小 */

#define GET_VIDEO_DATA_RETRY_MAX (25)   /* 从buffer中获取视频数据出错时，连续重试最大次数 */

/* 用于向 UDP 命令处理线程中传入参数的结构体 */
typedef struct
{
    struct sockaddr_in from_addr;   /* UDP数据来源IP地址信息 */
    int socket_fd;                  /* UDP socket fd */
    int cmd_len;                    /* 收到的消息数据长度 */
    char *cmd_buf;                  /* 收到的消息数据内容 */
}STRUCT_UDP_THPOOL_ARG;


extern void zsp_udp_cmd_processor(void * arg);
extern void zsp_tcp_cmd_processor(void * arg);
extern int zsp_video_user_list_init(void);


#endif /* __ZSP_CMD_PROCESSOR_H__ */


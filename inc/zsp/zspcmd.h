#ifndef __ZSP_CMD_H__
#define __ZSP_CMD_H__

#include "zmdconfig.h"
#include "dev_common.h"

#ifdef ZMD_APP_WIFI
#define ZSP_IF_NAME "wlan0"
#else
#define ZSP_IF_NAME "eth0"
#endif

#define ZSP_CMD_TCP_BUF_SIZE      (256)  /* TCP 服务器接收数据的 buffer 大小 */
#define ZSP_CMD_UDP_BUF_SIZE      (256)  /* UDP 服务器接收数据的 buffer 大小 */
#define ZSP_CMD_TCP_RETRY_TIMES   (3)    /* TCP 服务器接收数据出错的重试次数 */
#define GET_VIDEO_DATA_RETRY_MAX  (25)   /* 从buffer中获取视频数据出错时，连续重试最大次数 */

#define GET_HEADER_LEN(type)      (sizeof(type)-sizeof(Cmd_Header))

/* 消息头定义 */
typedef struct
{
    int                     head;
    int                     length;     //数据长度,去除head
    unsigned char           type;
    unsigned char           channel;
    unsigned short          commd;
}Cmd_Header ;

/* 用来定义消息头的宏.后期协议扩展后的消息体定义也要使用该宏 */
/* 保障每个消息体中消息头的定义都一样 */
#define DEF_CMD_HEADER      Cmd_Header      header

/* 请求实时视频直播 */
#define CMD_START_HD			0x5000  /* 请求主码流    对应产测工具的：高清  */
#define CMD_START_MD            0x9002  /* 请求子码流1   对应产测工具的：标清  */
#define CMD_START_LD            0x90a2  /* 请求子码流2   对应产测工具的：普清  */
#define CMD_STOP_VIDEO          0x9003  /* 关闭视频                         */

/* 音频对讲相关指令 begin */
#define CMD_G_TALK_SETTING      0x9060  /* 获取设备音频参数，如采样率、音量等 */
/* IPC设备音频开关，只决定此条指令对应socket下的音频是否发送给PC端工具，不操作IPC的音频相关硬件 */
#define CMD_SET_AUDIOSWITCH     0x9066  
#define CMD_TALK_ON             0x9006  /* 打开对讲，如果成功，则PC端工具往IPC端发送音频数据 */
#define CMD_TALK_OFF            0x9007  /* 关闭对讲，告诉IPC端停止往PC端工具发送音频数据 */
#define CMD_TALK_DATA           0x9008  /* 对讲时，PC端工具发给IPC的音频数据 */
/* 音频对讲相关指令 end */

#define CMD_G_VIDEO_KEY         0x9636  /* 获取视频加密的密钥 */
#define CMD_R_DEV_INFO          0x9800  /* 获取IPC设备信息，如：软件版本，是否支持音频，是否加密等 */
#define CMD_G_DEVICEID          0x9056  /* 获取设备ID */

#define CMD_ID_PING             0x9050  /* 手机局域网搜索信令 */
#define CMD_PING                0x9001  /* 局域网工具搜索信令 */

/* 定义视频通道种类 */
typedef enum
{
    VGA_CHN_TYPE    = 0,
    QVGA_CHN_TYPE   = 1,
    D720P_CHN_TYPE  = 2,
    ALARM_CHN_TYPE  = 3,
    RECORD_CHN_TYPE = 4,
    D1080P_CHN_TYPE = 5,
} ZMD_CHN_TYPE ;

/* 可以填写上面3个信令 */
typedef struct
{
    DEF_CMD_HEADER ;
}STRUCT_START_LIVING ;

/* 开始直播的响应数据包结构体 */
typedef struct
{
    DEF_CMD_HEADER  ;
    int                 result ;
}STRUCT_START_LIVING_ECHO ;

/* 获取视频加密的密钥 请求包 */
typedef struct
{
    DEF_CMD_HEADER;
} STRUCT_G_VIDEO_KEY_REQ;

/* 获取视频加密的密钥 响应包 */
typedef struct
{
    DEF_CMD_HEADER;
    char key[1024];
} STRUCT_G_VIDEO_KEY_RESP;

/* 音频对讲相关结构体 */
#pragma pack(1) /* 为适配产测工具，改为 1 字节对齐 */
typedef struct
{
    unsigned char   sampleRate;     //采样率 0:8K ,1:12K,  2: 11.025K, 3:16K ,4:22.050K ,5:24K ,6:32K ,7:48K;   
    unsigned char   audioType;      //编码类型0 :g711 ,1:2726
    unsigned char   enBitwidth;     //位宽0 :8 ,1:16 2:32
    unsigned char   recordVolume;   //设备当前输入音量0 --31 
    unsigned char   speakVolume;    //设备当前输出音量0 --31
    unsigned short  framelen;       //音频帧大小(80/160/240/320/480/1024/2048)
    unsigned char   reserved;       //保留      
}Audio_Coder_Param ;    /* IPC的音频编码参数 */
#pragma pack()

typedef struct
{
    DEF_CMD_HEADER ;
}STRUCT_GET_TALK_SETTING_REQUEST;   /* CMD_G_TALK_SETTING 0x9060 请求的结构体 */

typedef struct
{
    DEF_CMD_HEADER ;
    Audio_Coder_Param       audioParam ;
    //unsigned short        micVol;     //设备麦克音量 0-31;
    //unsigned short        spkVol ;    //设备扬声器音量 0-31;
}STRUCT_GET_TALK_SETTING_ECHO;  /* 响应 CMD_G_TALK_SETTING 0x9060 的结构体 */

typedef struct
{
    DEF_CMD_HEADER ;
    int     sendAudio ;         /* 0:不发送音频 , 1:发送音频 */
}STRUCT_SET_AUDIO_SWITCH;       /* CMD_SET_AUDIOSWITCH 0x9066 请求的结构体 */

typedef struct
{
    DEF_CMD_HEADER ;
    int     echo ;              /* 0:成功 , 其他失败，错误码 */
}STRUCT_SET_AUDIO_SWITCH_RESP;  /* 响应 CMD_SET_AUDIOSWITCH 0x9066 的结构体 */

typedef struct
{
    DEF_CMD_HEADER; 
    //int   audio_code;     //0:G711  1:G726
}STRUCT_TALK_ON_REQUEST ;   /* CMD_TALK_ON 0x9006 请求的结构体 */

typedef struct
{
    DEF_CMD_HEADER;
    int                 talkFlag ;
    Audio_Coder_Param   audioParam ;
}STRUCT_TALK_ON_ECHO;   /* 响应 CMD_TALK_ON 0x9006 的结构体 */

typedef struct
{
    DEF_CMD_HEADER;
}STRUCT_TALK_OFF_REQUEST;   /* CMD_TALK_OFF 0x9007 请求的结构体 */

typedef struct
{
    DEF_CMD_HEADER;
    char            audioData[0] ;      /* 音频数据长度在头部有标识 */
}STRUCT_TALK_DATA;      /* CMD_TALK_DATA 0x9008 PC端工具发给IPC的音频数据 */

/* CMD_R_DEV_INFO 获取IPC设备信息请求的结构体 */
typedef struct
{
    DEF_CMD_HEADER  ;
}STRUCT_READ_DEVINFO_REQUEST ;

/* CMD_R_DEV_INFO 获取IPC设备信息响应的结构体 */
typedef struct
{
    DEF_CMD_HEADER  ;
    TYPE_DEVICE_INFO    devInfo ;
    BlockDevInfo_S      storageDev[2]  ;    /* 2个存储设备信息 */
}STRUCT_READ_DEVINFO_ECHO ;

/* 给搜索工具返回的数据 */
typedef struct
{
    unsigned short  webPort;    /* 用于给搜索工具返回web监听端口 */
    unsigned short  videoPort;  /* 用于给搜索工具返回video监听端口 */
    unsigned short  phonePort;  /* 用于给搜索工具返回phone监听端口 */
    unsigned short  recver;
}devTypeInfo;

/* 局域网的IP、网关、掩码、MAC信息 */
typedef struct
{
    char            ipaddr[20];
    char            geteway[20];
    char            submask[20];
    char            mac[20];
}ipaddr_tmp;

/* 局域网广播搜设备信令 CMD_PING 0x9001 的响应数据包 */
typedef struct
{
    DEF_CMD_HEADER;
    TYPE_DEVICE_INFO    devInfo ;
    devTypeInfo         portInfo ;
    ipaddr_tmp          ipAddr ;
}STRUCT_PING_ECHO ;

/* 手机局域网搜索信令 CMD_ID_PING 0x9050 的响应数据包 */
typedef struct
{
    STRUCT_PING_ECHO    device; 
    char    deviceID[16] ;
}STRUCT_ID_PING_ECHO;   /* CMD_ID_PING 0x9050 的响应数据包 */

void *UdpCmdProcessor(void * arg);
void *TcpCmdProcessor(void * arg);

#endif /* __ZSP_CMD_H__ */

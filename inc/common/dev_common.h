#ifndef __DEV_COMMON_H__
#define __DEV_COMMON_H__

//此头文件禁止使用 #include "xxxx.h" 包含其他自定义的头文件

/*****************************************************************
 * 各个模块可能会共用的结构体类型请定义在此头文件中
 *
 * !!!!!!!!注意：此头文件真的禁止再包含其他头文件进行结构体类型定义
 *               否则可能导致一场灾难
 * 仅禁止包含自定义的头文件，系统默认支持的标准头文件可以包含
 *****************************************************************/


/****************** for test begin *********************************/
#define DEVICE_ID_TEST      "NEIL123456TEST0"    /* 长度:15位 */
#define ZSP_TEST_DEVNAME    "SD-H2001-A-H"
#define ZSP_TEST_SERIALNUM  "112233445566"
#define ZSP_TEST_HARDWARE_VERSION    "V8.8.8.88"
#define ZSP_TEST_SOFTWARE_VERSION    "V8.8.8.88"
/****************** for test end *********************************/

/* ZSP 产测工具同时播放视频的最大用户数量 */
#define ZSP_VIDEO_USER_MAX  (4) 

/* 为避免userid冲突，meshare 库使用的 userid 为 userid + ZSP_VIDEO_USER_MAX */
/* MSH 手机APP同时播放视频的最大用户数量 */ 
#define MSH_VIDEO_USER_MAX  (4) 

/* 视频播放用户列表中的用户状态 */
typedef enum
{
    VIDEO_USER_USED,    /* 0:已在使用中 */
    VIDEO_USER_UNUSED,  /* 1:未使用 */
}VIDEO_USER_STATUS;

/* 为了支持多个产测工具连接播放视频，增加视频用户列表 */
typedef struct
{
    int userid;
    unsigned int ip;
    unsigned int port;
    int devType;
    int used ;
}STRUCT_VIDEO_USER;

typedef struct
{
    unsigned int            m_u8Exist;      /* 0: 不存在，1 存在但是没有加载上，2，存在并加载上文件系统 */
    unsigned long           m_u32Capacity;  /* 以k为单位 */
    unsigned long           m_u32FreeSpace; /* 以k 为单位 */
    unsigned char           m_cDevName[16];
}BlockDevInfo_S;

typedef struct 
{
    //*8-15 bits保留
    //#define       REL_SPECIAL_COM     0x00000000
    //*16-23 bits 代表设备类型00:IPC CMOS VGA 01:IPC CMOS 720P 02:IPC CMOS 1080P 03 IPC CCD 04:DVR 05:NVR
    //#define       REL_VERSION_COM     0x00000000
    //*24-31表示芯片类型
    //#define       CHIP_TYPE_HI3511    0x50000000  //3507/3511芯片
    //#define       CHIP_TYPE_HI3515    0x52000000
    //#define       CHIP_TYPE_HI3520    0x54000000
    //#define       DEV_TYPE_INFO       CHIP_TYPE_HI3511+REL_VERSION_COM+REL_SPECIAL_COM+MAX_REC_CHANNEL

    int             DeviceType;             //设备类型DEV_TYPE_INFO
    char            DeviceName[32];         //设备名称
    char            SerialNumber[32];       //MAC地址
    char            HardWareVersion[16];    //硬件版本
    char            MCUVersion[16];         //单片机版本
    char            SoftWareVersion[16];    //软件版本
    unsigned int    LocalDeviceCapacity;    //能力位, bit0表示是否支持移动侦测区域设置新接口(通过坐标参数)
    char            reserved[12];
    char            VideoNum;               //视频通道数
    char            AudioNum;               //音频通道数
    char            AlarmInNum;             //报警输入
    char            AlarmOutNum;            //报警输出
    char            SupportAudioTalk;       //是否支持对讲1:支持0:不支持
    char            SupportStore;           //是否支持本地储存1:支持0:不支持
    char            SupportWifi;            //是否支持WIFI 1:支持0:不支持
    char            resver;                 //保留
}TYPE_DEVICE_INFO;




#endif  /* __DEV_COMMON_H__ */

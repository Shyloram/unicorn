#ifndef __DEV_COMMON_H__
#define __DEV_COMMON_H__

/****************** for test begin *********************************/
#define DEVICE_ID_TEST      "NEIL123456TEST0"    /* 长度:15位 */
#define ZSP_TEST_DEVNAME    "SD-H2001-A-H"
#define ZSP_TEST_SERIALNUM  "112233445566"
#define ZSP_TEST_HARDWARE_VERSION    "V8.8.8.88"
#define ZSP_TEST_SOFTWARE_VERSION    "V8.8.8.88"
/****************** for test end *********************************/

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

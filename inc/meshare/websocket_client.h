#ifndef __WEBSOCKET_CLIENT_H__
#define __WEBSOCKET_CLIENT_H__


/* websocket 消息类型, meshare 简写 msh/MSH */
typedef enum {
    MSH_MSG_SHAKE = 0x01,
    MSH_MSG_HEART_BEAT = 0x02,   
    MSH_MSG_DATA = 0x03,
    MSH_MSG_REDIRECT = 0x04
} msh_msg_t;

typedef struct _msh_login_body {
    char cmd_version[12];
    int  data_type;
    char token_id[32];
    int  client_type;
    char client_version[12];
    char client_id[32];
} msh_login_body;

/* websocket 消息中数据包命令 */
enum {
    CLI_DEV_PTZ_SYN = 0x01000009,
    CLI_DEV_PTZ_ACK = 0x01100009,
    CLI_DEV_SDCARD_OPERATION_SYN = 0x01000010,
    CLI_DEV_SDCARD_OPERATION_ACK = 0x01100010,
    CLI_DEV_SDCARD_STATUS_SYN   = 0x0100000f,
    CLI_DEV_SDCARD_STATUS_ACK   = 0x0110000f,
    CLI_DEV_MUTE_SYN   = 0x01000011,
    CLI_DEV_MUTE_ACK   = 0x01100011,
    CLI_DEV_RESET_SYN      = 0x01000012,
    CLI_DEV_RESET_ACK      = 0x01100012, 
    CLI_DEV_PLAY_VOICE_SYN = 0x01000013,
    CLI_DEV_PLAY_VOICE_ACK = 0x01100013,
    CLI_DEV_REFUSE_SYN = 0x01000014,
    CLI_DEV_REFUSE_ACK = 0x01100014,
    CLI_DEV_HANGUP_SYN = 0x01000015,
    CLI_DEV_HANGUP_ACK = 0x01100015,
    CLI_DEV_TEST_BUZZER_SYN = 0x01000016,
    CLI_DEV_TEST_BUZZER_ACK = 0x01100016,
    CLI_DEV_AREA_DETECTION_SYN = 0x01000017,
    CLI_DEV_AREA_DETECTION_ACK = 0x01100017,
    CLI_DEV_GET_AREA_DETECTION_SYN = 0x01000018,
    CLI_DEV_GET_AREA_DETECTION_ACK = 0x01100018,
    CLI_DEV_SPRINKLER_OP_SYN = 0x01000019,
    CLI_DEV_SPRINKLER_OP_ACK = 0x01100019,
    CLI_DEV_CURTAIN_STATUS_SYN = 0x01000020,
    CLI_DEV_CURTAIN_STATUS_ACK = 0x01100020,
    CLI_DEV_CURTAIN_OP_SYN = 0x01000021,
    CLI_DEV_CURTAIN_OP_ACK = 0x01100021,
    CLI_DEV_LOCK_OP_SYN = 0x01000022,
    CLI_DEV_LOCK_OP_ACK = 0x01100022,
    CLI_DEV_ADD_PRESET_SYN = 0x01000023,
    CLI_DEV_ADD_PRESET_ACK = 0x01100023,
    CLI_DEV_MODIFY_PRESET_SYN = 0x01000024,
    CLI_DEV_MODIFY_PRESET_ACK = 0x01100024,

    DEV_SRV_RECV_TRANS_SYN  =  0x03000001,
    DEV_SRV_RECV_TRANS_ACK  =  0x03100001,
    DEV_SRV_RECV_CONF_SYN   =  0x03000002,
    DEV_SRV_RECV_CONF_ACK   =  0x03100002,
    DEV_SRV_RECV_UPDATE_SYN =  0x03000003,
    DEV_SRV_RECV_UPDATE_ACK =  0x03100003,
    DEV_SRV_DOORBELL_SYN    =  0x03000005,
    DEV_SRV_DOORBELL_ACK    =  0x03100005,
    DEV_SRV_VOICEMSG_SYN    =  0x03000006,
    DEV_SRV_VOICEMSG_ACK    =  0x03100006,
    DEV_SRV_CHIME_CONF_SYN  =  0x03000007,
    DEV_SRV_CHIME_CONF_ACK  =  0x03100007,
    DEV_SRV_VOICEMSG_CONF_SYN = 0x03000008,
    DEV_SRV_VOICEMSG_CONF_ACK = 0x03100008,
    DEV_SRV_SENSITIVITY_CONF_SYN = 0x03000009,
    DEV_SRV_SENSITIVITY_CONF_ACK = 0x03100009,
    DEV_SRV_TIMETEMP_CONF_SYN = 0x0300000A,
    DEV_SRV_TIMETEMP_CONF_ACK = 0x0310000A,
    DEV_SRV_FORCE_I_FRAME_SYN   = 0x03000010,
    DEV_SRV_FORCE_I_FRAME_ACK   = 0x03100010,

    DEV_SRV_SYNC_SYN = 0x03000031,
    DEV_SRV_SYNC_ACK = 0x03100031,

    DEV_SRV_GET_TRANSINFO_SYN = 0x03000032,

    DEV_SRV_ANSWER_SYN = 0x03000033,
    DEV_SRV_ANSWER_ACK = 0x03100033,

    DEV_SRV_REFRESH_SYN = 0x03000034,
    DEV_SRV_REFRESH_ACK = 0x03100034,

    DEV_SRV_APMODE_SYN =  0x03000035,
    DEV_SRV_APMODE_ACK =  0x03100035,

    DEV_SRV_COM_NOTIFY_SYN = 0x03000036,
    DEV_SRV_COM_NOTIFY_ACK = 0x03100036,
};


enum cmd
{
    /* 与客户端通信协议 */
    PC_TRANS_LOGIN_SYN = 11000,
    PC_TRANS_LOGIN_ACK = 11100,
    PC_TRANS_HEART_SYN = 11001,
    PC_TRANS_HEART_ACK = 11101,

    /* 与设备通信协议 */
    IPC_TRANS_LOGIN_SYN = 21000,
    IPC_TRANS_LOGIN_ACK = 21100,
    IPC_TRANS_HEART_SYN = 21001,
    IPC_TRANS_HEART_ACK = 21101,
    IPC_TRANS_STOP_SYN  = 21002,
    IPC_TRANS_STOP_ACK  = 21102,

    IPC_DB_PLAYBACK_LOGIN_SYN = 21003,
    IPC_DB_PLAYBACK_LOGIN_ACK = 21103,
    IPC_PLAYBACK_START_SYN    = 21004,
    IPC_PLAYBACK_START_ACK    = 21104,
    IPC_PLAYBACK_LOGIN_SYN    = 21005,
    IPC_PLAYBACK_LOGIN_ACK    = 21105,

    /* 客户端与设备通信协议 */ 
    PC_IPC_TRANSFER_DATA       = 31000,
    PC_IPC_MEDIA_TYPE_SYN      = 31001,
    PC_IPC_MEDIA_TYPE_ACK      = 31101,
    PC_IPC_STOP_MEDIA_SYN      = 31002,
    PC_IPC_STOP_MEDIA_ACK      = 31102,
    PC_IPC_VOICE_STATUS_SYN    = 31003,
    PC_IPC_VOICE_STATUS_ACK    = 31103,
    PC_IPC_MIC_STATUS_SYN      = 31004,
    PC_IPC_MIC_STATUS_ACK      = 31104,
    PC_IPC_REPORT_STATUS_SYN   = 31005,
    PC_IPC_REPORT_STATUS_ACK   = 31105,
    PC_IPC_HEART_SYN           = 31006,
    PC_IPC_HEART_ACK           = 31106,
    PC_DB_PLAYBACK_SYN         = 31007,
    PC_DB_PLAYBACK_ACK         = 31107,
    PC_DB_PLAYBACK_STOP_SYN    = 31008,
    PC_DB_PLAYBACK_STOP_ACK    = 31108,
    PC_IPC_UPNP_LOGIN_SYN      = 31009,
    PC_IPC_UPNP_LOGIN_ACK      = 31109,
    PC_IPC_UPNP_CHANGE_PWD_SYN = 31010,
    PC_IPC_UPNP_CHANGE_PWD_ACK = 31110,
    PC_IPC_PTZ_SYN             = 31011,
    PC_IPC_PTZ_ACK             = 31111,
    PC_IPC_GET_PRESETPOINT_SYN = 31012,
    PC_IPC_GET_PRESETPOINT_ACK = 31112,
    PC_IPC_REPLY_DOORBELL_SYN  = 31013,
    PC_IPC_REPLY_DOORBELL_ACK  = 31113,
    PC_IPC_FLIP_SYN            = 31015,
    PC_IPC_FLIP_ACK            = 31115,
    PC_IPC_SDCARD_LIST_SYN     = 31016,
    PC_IPC_SDCARD_LIST_ACK     = 31116,
    PC_IPC_PLAY_SDCARD_SYN     = 31017,
    PC_IPC_PLAY_SDCARD_ACK     = 31117,
    PC_IPC_IR_SET_SYN          = 31018,
    PC_IPC_IR_SET_ACK          = 31118, 
    PC_IPC_IR_GET_SYN          = 31019, 
    PC_IPC_IR_GET_ACK          = 31119,
    PC_IPC_PLAYBACK_LIST_SYN   = 31020,
    PC_IPC_PLAYBACK_LIST_ACK   = 31120,
    PC_IPC_PLAYBACK_SYN        = 31021,
    PC_IPC_PLAYBACK_ACK        = 31121,
    PC_IPC_PLAYBACK_STOP_SYN   = 31022,
    PC_IPC_PLAYBACK_STOP_ACK   = 31122,
    IPC_PC_PLAYBACK_STOP_SYN   = 31023,
    IPC_PC_PLAYBACK_STOP_ACK   = 31123,

    IPC_PC_TALK_HEART_SYN = 31024,
    IPC_PC_TALK_HEART_ACK = 31124,

    PC_IPC_PLAYBACK_DATE_SYN = 31025,
    PC_IPC_PLAYBACK_DATE_ACK = 31125,

    /* 与SIP服务器通信协议 */
    SIP_TRANS_LOGIN_SYN   = 41000,
    SIP_TRANS_LOGIN_ACK   = 41100,
    SIP_TRANS_TCPCONN_SYN = 41001,
    SIP_TRANS_TCPCONN_ACK = 41101,
    SIP_TRANS_HEART_SYN   = 41002,
    SIP_TRANS_HEART_ACK   = 41102
};

/* 登录视频中转服务器消息结构体的头部 */
#define MSH_MAGIC (0x9F55FFFF)
typedef struct _msh_trans_msg_t{
    unsigned int    magic;       /* 特殊头 */
    //short         version;     /* 版本号 */
    unsigned char   channel_id;
    unsigned char   flag;
    short           cmd_type;    /* 指令类型 */
    unsigned int    cmd;         /* 指令字*/
    unsigned int    seqnum;      /* 流水号 */
    unsigned int    length;      /* 包体长度 */
    //char          *buf;        /* 包体内容 */
}msh_trans_msg_t;

#define TRANS_HEAD_LEN sizeof(msh_trans_msg_t)
#define TRANS_HEAD_INITIALIZER {MSH_MAGIC, 0, 0, 0, 0, 0, 0}

/* 登录视频中转服务器消息结构体 */ 
typedef struct _msh_login_trans_t {
    msh_trans_msg_t head;
    char register_code[32];
    char username_or_sn[32];
}msh_login_trans_t;

enum _cmd_type 
{
    CMD_TYPE_IPC_CLIENT=0,
    CMD_TYPE_IPC_TRANS,
};

/* select 机制监控 fd 是否可读或者可写的状态 */
enum _select_fd_status
{
    MSH_FD_UNREADABLE = 0,  /* fd 不可读 */
    MSH_FD_READABLE   = 1,  /* fd 可读 */
    MSH_FD_UNWRITABLE = 2,  /* fd 不可写 */
    MSH_FD_WRITABLE   = 3,  /* fd 可写 */
};

/* 视频帧传输头部 */
typedef struct _msh_video_trans_t
{
    unsigned int    magic;       /* 特殊头 */
    char            chlnum;      /* channel id */
    char            frame_flag;  /* frame flag*/
    short           cmd_type;    /* 指令类型 */
    unsigned int    cmd;         /* 指令字*/
    unsigned int    seqnum;      /* 流水号 */
    unsigned int    length;      /* 包体长度 */
    //char          *buf;        /* 包体内容 */
} msh_video_trans_t;


/***********************************************
 *@brief websocket 登录到服务器
 **********************************************/
extern int websocket_login(void);
extern int msh_video_user_list_init(void);


#endif  /* __WEBSOCKET_CLIENT_H__ */

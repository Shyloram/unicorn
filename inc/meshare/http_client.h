#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#define HTTP_URL_MAX_LEN    (1024)  /* web服务器（包含参数） url 的最大长度 */
#define HTTP_RESULT_MAX_LEN    (200*1024)   /* web服务器返回数据的最大长度 */
#define SERVER_URL_MAX_NUM (15) /* web服务器返回各其他服务器数据中，每个类型服务器的最大个数 */
#define SERVER_URL_MAX_LEN (256) /* web服务器返回各其他服务器数据中，每个服务器地址的最大长度 */


//#define DEV_ACCESS "https://11-DevAccess.myzmodo.com,https://12-DevAccess.myzmodo.com,https://13-DevAccess.myzmodo.com,https://14-DevAccess.myzmodo.com" 
#define DEV_ACCESS "https://12-DevAccess.myzmodo.com"

#define  UPGRADE_SERVER         "devupgrade.meshare.com"

#define  WEB_LOGIN_URI          "/factorydevice/devlogin"
#define  WEB_REPORT_URI         "/factorydevice/confrpt"
#define  WEB_SYNC_URI           "/factorydevice/sync"
#define  WEB_RECORD_REPORT_URI  "/factorydevice/devrcdlistrpt"
#define  WEB_SD_FORMAT_URI      "/factorydevice/sd_format"
#define  WEB_GET_TIMEZONE_URI   "/factorydevice/gettimezone"
#define  WEB_REPORT_UPNP_URI    "/factorydevice/upnp_rpt"
#define  WEB_TIMELINE_RPT_URI   "/factorydevice/timeline_rpt"
#define  WEB_PRESET_SET_URI     "/factorydevice/preset_set"
#define  WEB_GET_VOICEMSG_URI   "/factorydevice/get_voice_message"
#define  WEB_ADDING_DEVICE_URI  "/factorydevice/adding_passive_devs_rpt"
#define  WEB_SET_UPGRADE_URI    "/upgrade/setst"
#define  WEB_REST_RPT_URI       "/factorydevice/rest_rpt"
#define  WEB_ALARM_URI          "/message/msgnotify"
#define  WEB_GET_TIMEZONELIST   "/common/timezonelist"
#define  WEB_REPORT_PICTURE_URI "/fileserver/cover_report"
#define  FILESERVER_UPLOAD_URI  "/fileserver/dev_upload_file"
#define  WEB_PRESET_COVER_URI   "/fileserver/preset_cover_report"
#define  WEB_GETFILE_URI        "/fileserver/get_file"
#define  WEB_UPLOADFILE_URI     "/fileserver/com_upload_file"
#define  WEB_UPDATE_ALERT_URI   "/internal/msg_update"
#define  WEB_LPR_RPT            "/fileserver/lpr_rpt"
#define  WEB_FACE_RPT           "/fileserver/face_report"

#define DEV_CONN_ADDR       "dev_conn_addr"
#define DEV_ACCESS_ADDR     "dev_access_addr"
#define ALERTS_ADDR         "alerts_addr"
#define FILE_SERVER_ADDR    "file_server_addr"
#define DEV_MNG_ADDR        "dev_mng_addr"


typedef struct _http_result_t
{
    char *buf;      /* 用于存放 http 请求接收到的数据 */
    int len;        /* buf 中数据的长度 */
    int max_len;    /* buf 的最大长度 */
} http_result_t;

/* 用于标记各个服务器URL是否可用 */
typedef enum
{
    SERVER_URL_VALID,   /* 服务器可用 */
    SERVER_URL_UNVALID, /* 服务器不可用 */
} SERVER_URL_VALID_STATUS;

/* 存放web服务器返回的各其他服务器的地址 */
typedef struct _server_addr_t
{
    int url_num;    /* 收到的URL个数，最大为 SERVER_URL_MAX_NUM */
    int valid[SERVER_URL_MAX_NUM];      /* SERVER_URL_VALID:服务器可用， SERVER_URL_UNVALID:服务器不可用 */
    int last_change_time[SERVER_URL_MAX_NUM];   /* 最后一次更新状态的时间 */
    char url[SERVER_URL_MAX_NUM][SERVER_URL_MAX_LEN]; /* 分别存放收到的服务器的各个地址 */
}server_addr_t;

/* http login 登录 服务器返回数据中的其他参数信息 */
#define HTTP_TIMESTAMP_STR      "timestamp"
#define HTTP_ADDITION_STR       "addition"          /* web 服务器返回的 addition 参数，也就是 token */
#define HTTP_ENCRYPT_KEY_STR    "encrypt_key"
#define HTTP_ENCRYPT_KEY_ID_STR "encrypt_key_id"
typedef struct _http_web_info_t
{
    int timestamp;              /* web 服务器返回的时间戳 */
    char addition[128];         /* web 服务器返回的 addition 参数，也就是 token */
    char encrypt_key[128];      /* web 服务器返回的 encrypt_key */
    char encrypt_key_id[128];   /* web 服务器返回的 encrypt_key_id */
}http_web_info_t;

/* http post 请求的参数结构体 */
#define POST_NAME_MAX   (64)
#define POST_CONTENTS_MAX   (1024)
typedef struct _http_post_t
{
    char name[POST_NAME_MAX];
    char contents[POST_CONTENTS_MAX];
}http_post_t;

extern int http_init(void);
extern int http_login(void);
extern int http_report_info(void);
extern int http_get_parameter_server(char *url, char * server_name);
extern int http_get_parameter_web_info(void * para, char * para_name);

#endif /* __HTTP_CLIENT_H__ */

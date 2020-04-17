/***************************************************************
 * 描述：执行 http get/post 等操作，并参返回值进行处理
 * 本文件中所有 static 的函数统一以双下划线开头
 * 其他提供给外部调用的接口统一放在 http_client.h 文件中 
 * ************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl.h>
#include <json.h>
#include "dev_common.h"
#include "http_client.h"
#include "meshare_common.h" /* meshare 库的公共头文件 */

static server_addr_t    g_dev_conn_addr;       /* dev_conn 服务器的地址 */
static server_addr_t    g_dev_access_addr;     /* dev_access 服务器的地址 */
static server_addr_t    g_alerts_addr;         /* alerts 服务器的地址 */
static server_addr_t    g_file_server_addr;    /* file_server 服务器的地址 */
static server_addr_t    g_dev_mng_addr;        /* dev_mng 服务器的地址 */
static http_web_info_t  g_http_web_info;       /* http web 服务器返回的其他信息 */

/* 操作 server_addr_t 时的互斥锁,已静态初始化，不需要再调用 pthread_mutex_init() 初始化 */
static pthread_mutex_t g_dev_conn_addr_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_dev_access_addr_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_alerts_addr_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_file_server_addr_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_dev_mng_addr_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_http_web_info_mutex = PTHREAD_MUTEX_INITIALIZER;

/***********************************************
 *@brief http 全局初始化，LiteOS平台下此函数
         在机器每次开机运行只允许调用一次
 *@param void 无参数
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
int http_init(void)
{
    curl_global_init(CURL_GLOBAL_ALL);
    return 0;
}

/***********************************************
 *@brief http 请求数据接收
 *@param
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static size_t __http_write_data(void *ptr, size_t size, size_t nmemb, void *data)
{
    http_result_t * result = (http_result_t *) data;
    size_t left_size = 0;   /* result buf 剩余空间的大小 */
    size_t copy_size = 0;   /* 复制到 result buf 的数据大小 */

    if(NULL == data)
    {
        MSHLOG("传入参数data为NULL\r\n");
        return (size*nmemb);
    }

    left_size = result->max_len - result->len;    /* buf 剩余大小 = 最大值 - 已接收的数据长度 */
    if(left_size <= 0)
    {
        MSHLOG("result buffer 剩余空间 <= 0 字节\r\n");
        return (size*nmemb);
    }
    /* 复制到 buffer 的数据大小 */
    copy_size = (size * nmemb) > left_size ? left_size : (size * nmemb);
    memcpy(result->buf + result->len, ptr, copy_size);
    result->len += copy_size;

    return (size*nmemb);
}

/***********************************************
 *@brief 执行 http get 或 post 请求
 *       如果执行 http get 请求，post_data 传入参数为 NULL
 *@param url 参数1 URL
 *@param post_data 参数2 post 参数的结构体数组
 *@param post_data_num 参数3 post 参数的数组元素个数 
 *@param result 参数4 返回结果
 *@param timeout_ms 参数5 执行http请求的超时时间
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_do_get_post(char * url, http_post_t post_data[], 
        int post_data_num, http_result_t *result, int timeout_ms)
{
    CURL * curl = NULL;
    CURLcode ret = CURLE_OK;
    struct curl_httppost * formpost = NULL; /* http post 请求才用的参数 */
    struct curl_httppost * lastptr = NULL;  /* http post 请求才用的参数 */
    int i = 0;  /* 只给 for 循环使用的一个变量 */

    MSHLOG("开始执行 http 请求:%s, timeout_ms: %dms\r\n", url, timeout_ms);

    curl = curl_easy_init();
    if(NULL == curl)
    {
        MSHLOG("curl_easy_init 失败\r\n");
        return MSH_FAILURE;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);               /* 设置 URL */
    //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2*timeout_ms/1000);       /* 设置超时时间， 单位为 s */
    //curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms%1000);    /* 设置请求超时时间，单位为 ms */
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout_ms/1000);    /* 设置连接超时时间，单位为 s */
    //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms%1000); /* 设置连接超时时间，单位为 ms */
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);

    /* https url 设置 */
    if(0 == strncmp(url, "https://", strlen("https://")))   /* 如果是 https 暂不验证证书 */
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);                /* 设置调试信息可见 */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __http_write_data);    /* 接收数据处理 */
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);        /* 设置数据写入位置 */

    /* 设置 http post 请求参数 */
    if((NULL != post_data) && (post_data_num > 0))
    {
        for(i = 0; i < post_data_num; i++)
        {
            /* 还需要增加传入数据的判空 NULL */
            if(NULL != post_data[i].name)
            {
                curl_formadd(&formpost, &lastptr, 
                        CURLFORM_COPYNAME, post_data[i].name, 
                        CURLFORM_COPYCONTENTS, post_data[i].contents,
                        CURLFORM_END);
                MSHLOG("%d, name:%s, contents:%s\r\n", i, post_data[i].name, post_data[i].contents);
                curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
            }

        }
        //curl_easy_setopt(curl, CURLOPT_POST, 1L);
    }

    /* 执行 curl http 请求 */
    ret = curl_easy_perform(curl);
    if(CURLE_OK != ret)
    {
        MSHLOG("执行 curl http 请求失败，ret:%d\r\n", ret);
        ret = (CURLcode)MSH_FAILURE;
    }
    else
    {
        ret = (CURLcode)MSH_SUCCESS;
        MSHLOG("执行 curl http 请求成功\r\n");
    }

    if(NULL != formpost)
    {
        free(formpost);
        formpost = NULL;
    }

    if(NULL != curl)
    {
        curl_easy_cleanup(curl);
        curl = NULL;
    }

    return (int)ret;
}

/***********************************************
 *@brief 执行 http post 请求
 *@param url 参数1 URL
 *@param result 参数2 返回结果
 *@param len 参数3 存放返回结果buffer的长度
 *@param timeout_ms 参数4 执行http请求的超时时间
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_do_post(char * url, http_result_t *result, int len, int timeout_ms)
{

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 获取 web 服务器 URL 并设置设备ID参数
 *@param *url 参数1 用于存放url的buffer
 *@param url_len 参数2 用于存放url的buffer的长度
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_get_web_server_url(char * url, int url_len)
{
    strcpy(url, DEV_ACCESS);    /* 暂时设置url为固定的:https://11-DevAccess.myzmodo.com */
    strcat(url, WEB_LOGIN_URI); /* 连接url：/factorydevice/devlogin */
    strcat(url, "?physical_id=");   /* 连接url: ?physical_id= */
    strcat(url, DEVICE_ID_TEST);    /* 暂时设置 dev_id 为固定的：BOONE12345TEST0 */

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 获取各类服务器的地址参数，通过参数 url 返回
 *@param *url 参数1 返回url数据
 *@param *server_name 参数2 服务器名字，例如："dev_access_addr" "dev_conn_addr" 对应的宏
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_get_parameter_server(char *url, char * server_name)
{
    int rand_num = 0;   /* 用于保存一个随机的数值 */
    server_addr_t * server_addr = NULL;
    pthread_mutex_t * server_addr_mutex = NULL;

    if((NULL == url) || (NULL == server_name))
    {
        MSHLOG("传入参数 url 和 server_name 不允许为 NULL\r\n");
        return MSH_FAILURE;
    }

    if(0 == strcmp(server_name, DEV_CONN_ADDR)) /* "dev_conn_addr" */
    {
        server_addr = &g_dev_conn_addr;
        server_addr_mutex = &g_dev_conn_addr_mutex;
    }
    else if(0 == strcmp(server_name, DEV_ACCESS_ADDR))  /* "dev_access_addr" */
    {
        server_addr = &g_dev_access_addr;
        server_addr_mutex = &g_dev_access_addr_mutex;
    }
    else if(0 == strcmp(server_name, ALERTS_ADDR))  /* "alerts_addr" */
    {
        server_addr = &g_alerts_addr;
        server_addr_mutex = &g_alerts_addr_mutex;
    }
    else if(0 == strcmp(server_name, FILE_SERVER_ADDR))  /* "file_server_addr" */
    {
        server_addr = &g_file_server_addr;
        server_addr_mutex = &g_file_server_addr_mutex;
    }
    else if(0 == strcmp(server_name, DEV_MNG_ADDR))  /* "dev_mng_addr" */
    {
        server_addr = &g_dev_mng_addr;
        server_addr_mutex = &g_dev_mng_addr_mutex;
    }
    else
    {
        MSHLOG("没有这个类型的服务器：%s\r\n", server_name);
        return MSH_FAILURE;
    }

    pthread_mutex_lock(server_addr_mutex);

    if(server_addr->url_num <= 0)
    {
        MSHLOG("当前保存的 server_addr 地址个数为 0\r\n");
        pthread_mutex_unlock(server_addr_mutex);    /* 注意：return 前要解锁 */
        return MSH_FAILURE;
    }
    else
    {
        /* 创建一个小于 server_addr->url_num 的随机数 */
        rand_num = 0;   /* 暂不实现，每次使用第 0 个 */
    }

    strcpy(url, server_addr->url[rand_num]);
    MSHLOG("获取到 url:%s\r\n", url);

    pthread_mutex_unlock(server_addr_mutex);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 提供给外部接口函数，获取各类服务器的地址参数，通过参数 url 返回
 *@param *url 参数1 返回url数据
 *@param *server_name 参数2 服务器名字，例如："dev_access_addr" "dev_conn_addr" 对应的宏
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
int http_get_parameter_server(char *url, char * server_name)
{
    return __http_get_parameter_server(url, server_name);
}

/***********************************************
 *@brief 设置各类服务器的地址参数
 *@param *json_obj  参数1 web服务器返回的 json 数据
 *@param *server_name 参数2 对应服务器类型的server_name，例如："dev_access_addr" "dev_conn_addr" 对应的宏
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_set_parameter_server(json_object * json_obj, char * server_name)
{
    int ret = MSH_FAILURE;
    char * addr_str = NULL;
    char url_buf[SERVER_URL_MAX_LEN] = {0}; /* 用于解析 URL 时存放每个 URL 地址 */
    server_addr_t * server_addr = NULL;
    pthread_mutex_t * server_addr_mutex = NULL;

    if(0 == strcmp(server_name, DEV_CONN_ADDR)) /* "dev_conn_addr" */
    {
        server_addr = &g_dev_conn_addr;
        server_addr_mutex = &g_dev_conn_addr_mutex;
    }
    else if(0 == strcmp(server_name, DEV_ACCESS_ADDR))  /* "dev_access_addr" */
    {
        server_addr = &g_dev_access_addr;
        server_addr_mutex = &g_dev_access_addr_mutex;
    }
    else if(0 == strcmp(server_name, ALERTS_ADDR))  /* "alerts_addr" */
    {
        server_addr = &g_alerts_addr;
        server_addr_mutex = &g_alerts_addr_mutex;
    }
    else if(0 == strcmp(server_name, FILE_SERVER_ADDR))  /* "file_server_addr" */
    {
        server_addr = &g_file_server_addr;
        server_addr_mutex = &g_file_server_addr_mutex;
    }
    else if(0 == strcmp(server_name, DEV_MNG_ADDR))  /* "dev_mng_addr" */
    {
        server_addr = &g_dev_mng_addr;
        server_addr_mutex = &g_dev_mng_addr_mutex;
    }
    else
    {
        MSHLOG("没有这个类型的服务器：%s\r\n", server_name);
        return MSH_FAILURE;
    }

    addr_str = (char *)json_object_get_string(json_object_object_get(json_obj, server_name));
    if(NULL == addr_str)
    {
        MSHLOG("从json对象中获取 %s 失败\r\n", server_name);
        return MSH_FAILURE;
    }
    MSHLOG("server, %s : %s\r\n", server_name, addr_str);

    /* 解析 addr_str 字符串中的URL并保存到 server_addr */
    pthread_mutex_lock(server_addr_mutex);
    memset(server_addr, 0, sizeof(server_addr_t));
    server_addr->url_num = 0;   /* 当前 URL 个数为0 */

    do  
    {
        memset(url_buf, 0, sizeof(url_buf));
        sscanf(addr_str, "%[^,]", url_buf); /* 以 , 为分隔符，每次取出第一个 URL */
        if(strlen(url_buf) > 0)
        {
            server_addr->valid[server_addr->url_num] = SERVER_URL_VALID;
            server_addr->last_change_time[server_addr->url_num] = 888;   //p2p_get_uptime(); 暂时设置为固定时间
            strcpy(server_addr->url[server_addr->url_num], url_buf);
            //MSHLOG("server_addr->url[%d]:%s\r\n", server_addr->url_num, server_addr->url[url_num]);
            server_addr->url_num ++;
            if(server_addr->url_num >= SERVER_URL_MAX_NUM)
            {
                ret = MSH_SUCCESS;
                MSHLOG("最大支持保存 %d 个服务器地址\r\n", SERVER_URL_MAX_NUM);
                break;
            }
        }   
        else
        {
            MSHLOG("解析URL出错： %s : %s\r\n", server_name, addr_str);
            ret = MSH_FAILURE;
            break;
        }
        addr_str = strchr(addr_str, ',');

        if(NULL != addr_str) 
        {
            addr_str += 1;  /* 跳过一个字节 ',' */
        }
        else
        {
            ret = MSH_SUCCESS;
            break;
        }
        ret = MSH_SUCCESS;
    }while(*addr_str);  /* 直到遇到 \0 结束 */

    pthread_mutex_unlock(server_addr_mutex);

    return ret; /* 成功：MSH_SUCCESS 失败：MSH_FAILURE */
}

/***********************************************
 *@brief 获取web服务器返回的其他参数
 *@param *para 参数1 返回对应数据的地址
 *@param *para_name 参数2 参数的名字，如 "addition" "encrypt_key" "encrypt_key_id" ...
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_get_parameter_web_info(void * para, char * para_name)
{

    if((NULL == para) || (NULL == para_name))
    {
        MSHLOG("传入参数不允许为NULL\r\n");
        return MSH_FAILURE;
    }

    pthread_mutex_lock(&g_http_web_info_mutex);

    if(0 == strcmp(para_name, HTTP_TIMESTAMP_STR))
    {
        *((int *)para) = g_http_web_info.timestamp;
    }
    else if(0 == strcmp(para_name, HTTP_ADDITION_STR))
    {
        strcpy((char *)para, g_http_web_info.addition);
    }
    else if(0 == strcmp(para_name, HTTP_ENCRYPT_KEY_STR))
    {
        strcpy((char *)para, g_http_web_info.encrypt_key);
    }
    else if(0 == strcmp(para_name, HTTP_ENCRYPT_KEY_ID_STR))
    {
        strcpy((char *)para, g_http_web_info.encrypt_key_id);
    }
    else
    {
        pthread_mutex_unlock(&g_http_web_info_mutex);
        return MSH_FAILURE;
    }

    pthread_mutex_unlock(&g_http_web_info_mutex);

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 提供给其他文件调用的外部接口函数，用于获取web服务器返回的其他参数
 *@param *para 参数1 返回对应数据的地址
 *@param *para_name 参数2 参数的名字，如 "addition" "encrypt_key" "encrypt_key_id" ...
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
int http_get_parameter_web_info(void * para, char * para_name)
{
    return __http_get_parameter_web_info(para, para_name);
}

/***********************************************
 *@brief 设置web服务器返回的其他参数
 *@param *json_obj  参数1 web服务器返回的 json 数据
 *@param *para_name 参数2 参数的名字，如 "addition" "encrypt_key" "encrypt_key_id" ...
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_set_parameter_web_info(json_object * json_obj, char * para_name)
{
    char * tmp_str = NULL;  /* 临时存储字符串 */
    int tmp_int = 0;      /* 临时存储 json 数据中的 timestamp 值 */

    if((NULL == json_obj) || (NULL == para_name))
    {
        MSHLOG("传入参数不允许为NULL\r\n");
        return MSH_FAILURE;
    }

    MSHLOG("para_name:%s\r\n", para_name);

    pthread_mutex_lock(&g_http_web_info_mutex);
    if(0 == strcmp(para_name, HTTP_TIMESTAMP_STR))
    {
        /* json 数据中获取对应的 int timestamp */
        tmp_int = json_object_get_int(json_object_object_get(json_obj, para_name));
        g_http_web_info.timestamp = tmp_int;
        MSHLOG("g_http_web_info.timestamp:%d\r\n", g_http_web_info.timestamp);
        pthread_mutex_unlock(&g_http_web_info_mutex);
        return MSH_SUCCESS;
    }

    /* json 数据中获取对应的数据 */
    tmp_str = (char *)json_object_get_string(json_object_object_get(json_obj, para_name));
    if(NULL == tmp_str)
    {
        MSHLOG("从json对象中获取 %s 失败\r\n", para_name);
        return MSH_FAILURE;
    }

    MSHLOG("%s value:%s\r\n", para_name, tmp_str);
    if(0 == strcmp(para_name, HTTP_ADDITION_STR))
    {
        /* 设置到全局变量 g_http_web_info 中保存 */
        strcpy(g_http_web_info.addition, tmp_str);
    }
    else if(0 == strcmp(para_name, HTTP_ENCRYPT_KEY_STR))
    {
        /* 设置到全局变量 g_http_web_info 中保存 */
        strcpy(g_http_web_info.encrypt_key, tmp_str);
    }
    else if(0 == strcmp(para_name, HTTP_ENCRYPT_KEY_ID_STR))
    {
        /* 设置到全局变量 g_http_web_info 中保存 */
        strcpy(g_http_web_info.encrypt_key_id, tmp_str);
    }

    pthread_mutex_unlock(&g_http_web_info_mutex);
    
    return MSH_SUCCESS;
}

/***********************************************
 *@brief 已保证web服务器返回的result为OK之后，继续解析 json 串中的其他参数
 *@param *json_obj
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_set_parameter(json_object * json_obj)
{
    int ret = 0;

    ret = __http_set_parameter_server(json_obj, DEV_CONN_ADDR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", DEV_CONN_ADDR);
        return MSH_FAILURE;
    }

    ret = __http_set_parameter_server(json_obj, DEV_ACCESS_ADDR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", DEV_ACCESS_ADDR);
        return MSH_FAILURE;
    }

    ret = __http_set_parameter_server(json_obj, ALERTS_ADDR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", ALERTS_ADDR);
        return MSH_FAILURE;
    }

    ret = __http_set_parameter_server(json_obj, FILE_SERVER_ADDR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", FILE_SERVER_ADDR);
        return MSH_FAILURE;
    }

    ret = __http_set_parameter_server(json_obj, DEV_MNG_ADDR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", DEV_MNG_ADDR);
        return MSH_FAILURE;
    }

    /* 设置 encrypt_key encrypt_key_id addition */
    ret = __http_set_parameter_web_info(json_obj, HTTP_TIMESTAMP_STR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", HTTP_TIMESTAMP_STR);
        return MSH_FAILURE;
    }

    ret = __http_set_parameter_web_info(json_obj, HTTP_ADDITION_STR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", HTTP_ADDITION_STR);
        return MSH_FAILURE;
    }

    ret = __http_set_parameter_web_info(json_obj, HTTP_ENCRYPT_KEY_STR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", HTTP_ENCRYPT_KEY_STR);
        return MSH_FAILURE;
    }

    ret = __http_set_parameter_web_info(json_obj, HTTP_ENCRYPT_KEY_ID_STR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置 %s 失败\r\n", HTTP_ENCRYPT_KEY_ID_STR);
        return MSH_FAILURE;
    }

    if(1) /* 仅用于调试 */
    {
        char url[256] = { 0 };
        __http_get_parameter_server(url, DEV_CONN_ADDR);
        printf("get url:%s\r\n", url);
        __http_get_parameter_server(url, DEV_ACCESS_ADDR);
        printf("get url:%s\r\n", url);
        __http_get_parameter_server(url, ALERTS_ADDR);
        printf("get url:%s\r\n", url);
        __http_get_parameter_server(url, FILE_SERVER_ADDR);
        printf("get url:%s\r\n", url);
        __http_get_parameter_server(url, DEV_MNG_ADDR);
        printf("get url:%s\r\n", url);

        int timestamp = 0;
        char buf[256] = { 0 };
        __http_get_parameter_web_info(&timestamp, HTTP_TIMESTAMP_STR);
        printf("timestamp:%d\r\n", timestamp);
        __http_get_parameter_web_info(buf, HTTP_ADDITION_STR);
        printf("%s: %s\r\n", HTTP_ADDITION_STR, buf);
        __http_get_parameter_web_info(buf, HTTP_ENCRYPT_KEY_STR);
        printf("%s: %s\r\n", HTTP_ENCRYPT_KEY_STR, buf);
        __http_get_parameter_web_info(buf, HTTP_ENCRYPT_KEY_ID_STR);
        printf("%s: %s\r\n", HTTP_ENCRYPT_KEY_ID_STR, buf);
    }

    return MSH_SUCCESS;
}

/***********************************************
 *@brief 对传入的 http 返回数据 result 进行解析，取出其中参数并设置
 *@param *result 参数1 http 请求返回的数据
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_parse_result(http_result_t * result)
{
    int ret = 0;
    char * body = NULL; /* 用于指向 result->buf 的数据*/
    json_object * json_obj = NULL;  /* 用于将字符串转为 json 对象 */
    char * json_result = NULL;  /* 用于指向 json 对象中的 result */

    if(NULL == result)
    {
        return MSH_FAILURE;
    }

    body = result->buf; /* http 请求返回的数据 */
    /* 判断是不是 json 数据 */
    json_obj = json_tokener_parse(body);
    if(NULL == json_obj)
    {
        MSHLOG("json数据不正确\r\n");
        return MSH_FAILURE;
    }

    json_result = (char *)json_object_get_string(json_object_object_get(json_obj, "result"));
    if(NULL == json_result)
    {
        MSHLOG("从json对象中获取 result 失败\r\n");
        json_object_put(json_obj);  /* 释放 */
        return MSH_FAILURE;
    }
    MSHLOG("json result:%s\r\n", json_result);

    if(0 != strcmp((const char*)"ok", (const char*)json_result))
    {
        MSHLOG("web 服务器返回的 result:%s 不是 ok\r\n", json_result);
        json_object_put(json_obj);  /* 释放 */
        return MSH_FAILURE;
    }
    
    /* 根据收到的json数据设置各项参数*/
    ret = __http_set_parameter(json_obj);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("设置各项参数失败\r\n");
        json_object_put(json_obj);  /* 释放 */
        return MSH_FAILURE;
    }


    json_object_put(json_obj);  /* 释放 */
    return MSH_SUCCESS;
}

/***********************************************
 *@brief 执行 http 登录到web服务器
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_do_login(int timeout_ms)
{
    int ret = 0;
    char url[HTTP_URL_MAX_LEN] = {0};
    http_result_t result;   /* 存放 http 请求的返回数据 */

    /* 获取 web 服务器 URL 并设置参数 */
    ret = __http_get_web_server_url(url, HTTP_URL_MAX_LEN);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("获取web服务器URL失败\r\n");
        goto error;
    }
    MSHLOG("获取web服务器URL成功:%s\r\n", url);

    /* 初始化 result 变量 */
    memset(&result, 0, sizeof(http_result_t));
    result.max_len = HTTP_RESULT_MAX_LEN;
    result.len = 0;
    result.buf = (char *)malloc(result.max_len);
    if(NULL == result.buf)
    {
        MSHLOG("result.buf malloc 失败\r\n");
        goto error;
    }
    memset(result.buf, 0, result.max_len);  /* 清0 */

    /* 执行 http get 请求，并接收返回值 */
    //ret = __http_do_get(url, &result, timeout_ms);
    ret = __http_do_get_post(url, NULL, 0, &result, timeout_ms);    /* 0: get 请求无意义 */
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("执行 http get 请求失败\r\n");
        goto error;
    }
    MSHLOG("执行 http get 请求成功:\r\n%s\r\n", result.buf);

    /* 解析 web 服务器的返回结果，并设置IPC所需要的各项参数 */
    ret = __http_parse_result(&result);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("解析 http 返回的数据失败\r\n");
        goto error;
    }

    free(result.buf);
    return MSH_SUCCESS;

error:
    if(NULL != result.buf)
    {
        free(result.buf);
        result.buf = NULL;
    }
    return MSH_FAILURE;
}

/***********************************************
 *@brief http 登录到web服务器
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
int http_login(void)
{
    int ret = -1;
    int timeout_ms_min = 2500;  /* 默认最小超时时间 ms */
    int timeout_ms = 0;         /* http 请求的超时时间 ms */
    int times = 0;              /* 连接web服务器的次数 */

    timeout_ms = timeout_ms_min;
    MSHINF("开始登录web服务器\r\n");
    do
    {
        timeout_ms += 100;  /* 每次失败，延时时间增加 100ms */
        times ++;
        MSHLOG("第 [%d] 次登录，http 超时时间设置为 [%d] ms\r\n", times, timeout_ms);
        ret = __http_do_login(timeout_ms);
        if(timeout_ms > 3500)
        {
            timeout_ms = timeout_ms_min;  /* 如果一直失败，超时时间大于2秒仍失败时，重新计时 */
        }
        usleep(100*1000);   /* 如果失败，延时 100ms 后重新登录 */

    }while(ret != MSH_SUCCESS);
    MSHLOG("登录web服务器成功\r\n");


    return MSH_SUCCESS;
}

/***********************************************
 *@brief 执行 http post 上传相关信息到服务器
 *@param timeout_ms 参数1 http 请求超时时间 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
static int __http_do_report_info(int timeout_ms)
{
    int ret = 0;
    char url[SERVER_URL_MAX_LEN] = { 0 };   /* 执行 http post 请求的 url */
    http_post_t post_data[2] = { 0 };       /* http post 请求参数 */
    http_result_t result;   /* 存放 http 请求的返回数据 */
    json_object * base_json = NULL; /* post 参数 */
    json_object * conf_json = NULL; /* post 参数 */
    json_object * resolution_json = NULL; /* post 参数 */
    char tokenid[128] = { 0 };
    char aes_key[128] = { 0 };

    /* 获取 dev_mng_addr 服务器地址 */
    ret = __http_get_parameter_server(url, DEV_MNG_ADDR);
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("获取 %s 服务器地址失败\r\n", DEV_MNG_ADDR);
        return MSH_FAILURE;
    }
    MSHLOG("获取 %s 服务器地址成功:%s\r\n", DEV_MNG_ADDR, url);

    /* url 后面连接上报服务器对应路径 /factorydevice/confrpt */
    strcat(url, WEB_REPORT_URI);
    MSHLOG("%s 服务器地址为:%s\r\n", DEV_MNG_ADDR, url);

    /* 版本信息暂时用固定的 ZSP_TEST_SOFTWARE_VERSION */

    /* 初始化 http post 所需要的 json 数据信息 */
    memset(post_data, 0, sizeof(http_post_t));
    base_json = json_object_new_object();
    conf_json = json_object_new_object();
    resolution_json = json_object_new_object();
    if((NULL == base_json) || (NULL == conf_json) || (NULL == resolution_json))
    {
        MSHLOG("创建 json 对象失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }
    __http_get_parameter_web_info(tokenid, HTTP_ADDITION_STR);  /* 获取 tokenid */
    MSHLOG("tokenid: %s\r\n", tokenid);

    json_object_object_add(base_json, "tokenid", json_object_new_string(tokenid));
    json_object_object_add(base_json, "physical_id", json_object_new_string(DEVICE_ID_TEST));   /* 暂时用固定的 */
    json_object_object_add(base_json, "device_type", json_object_new_string("0"));   /* 暂时用固定的 */

    /* 将 json 数据放入 post_data 参数中 */
    strcpy(post_data[0].name, "baseinfo");
    strcpy(post_data[0].contents, json_object_get_string(base_json));


    json_object_object_add(conf_json, "device_channel", json_object_new_string("1"));
    json_object_object_add(conf_json, "device_ionum", json_object_new_string("0"));
    json_object_object_add(conf_json, "device_model", json_object_new_string(ZSP_TEST_DEVNAME));
    json_object_object_add(conf_json, "device_version",
            json_object_new_string("V8.8.8.88;V8.8.8.88;V8.8.8.88;V8.8.8.88"));
    //json_object_object_add(conf_json, "device_capacity", (0x62000000 + 0x00020000 + 0x00000000));
    json_object_object_add(conf_json, "device_capacity", json_object_new_string("1644306177"));
    json_object_object_add(conf_json, "device_extend_capacity", json_object_new_string("1208819571"));
    json_object_object_add(conf_json, "device_supply_capacity", json_object_new_string("1342177476"));
    //__http_get_parameter_web_info(aes_key, HTTP_ENCRYPT_KEY_STR);
    //json_object_object_add(conf_json, "aes_key", json_object_new_string(aes_key));
    json_object_object_add(conf_json, "aes_key", json_object_new_string("4D1FAC83E8F049CA9D68360219F98EAE"));

    json_object_object_add(resolution_json, "HD", json_object_new_string("1920*1080"));
    json_object_object_add(resolution_json, "SD", json_object_new_string("640*480"));
    json_object_object_add(resolution_json, "LD", json_object_new_string("320*240"));

    json_object_object_add(conf_json, "resolution", json_object_new_string(json_object_get_string(resolution_json)));

    /* 将 json 数据放入 post_data 参数中 */
    strcpy(post_data[1].name, "confinfo");
    strcpy(post_data[1].contents, json_object_get_string(conf_json));

    /**/
    MSHLOG("post_data[0].name:%s\r\n", post_data[0].name);
    MSHLOG("post_data[0].contents:%s\r\n", post_data[0].contents);
    MSHLOG("post_data[1].name:%s\r\n", post_data[1].name);
    MSHLOG("post_data[1].contents:%s\r\n", post_data[1].contents);

    
    /* 初始化 result 变量 */
    memset(&result, 0, sizeof(http_result_t));
    result.max_len = HTTP_RESULT_MAX_LEN;
    result.len = 0;
    result.buf = (char *)malloc(result.max_len);
    if(NULL == result.buf)
    {
        MSHLOG("result.buf malloc 失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }
    memset(result.buf, 0, result.max_len);  /* 清0 */

    /* 执行 http post 请求，并接收返回值 */
    ret = __http_do_get_post(url, post_data, 2, &result, timeout_ms);    /* 2: post 请求2个参数 */
    if(MSH_SUCCESS != ret)
    {
        MSHLOG("执行 http post 请求失败\r\n");
        ret = MSH_FAILURE;
        goto error;
    }

    /* 解析 web 服务器的返回结果 */
    MSHLOG("服务器返回：%s\r\n", result.buf);

    ret = MSH_SUCCESS;

error:
    if(NULL != result.buf)
    {
        free(result.buf);
        result.buf = NULL;
    }

    if(NULL != base_json) 
    {
        json_object_put(base_json);
        base_json = NULL;
    }

    if(NULL != conf_json)
    {
        json_object_put(conf_json);
        conf_json = NULL;
    }

    if(NULL != resolution_json)
    {
        json_object_put(resolution_json);
        resolution_json = NULL;
    }

    MSHLOG("return ret:%d\r\n", ret);
    return ret; /* 成功：MSH_SUCCESS 0 成功： MSH_FAILURE -1 */
}

/***********************************************
 *@brief http post 将IPC各种信息上报给服务器，阻塞，直到上报成功
 *@param 
 *@retrun
 *      成功：返回 MSH_SUCCESS  0
 *      失败：返回 MSH_FAILURE -1 
 **********************************************/
int http_report_info(void)
{
    /* http post 上传相关信息到服务器 */
    int ret = -1;
    int timeout_ms_min = 5000;  /* 默认最小超时时间 ms */
    int timeout_ms = 0;         /* http 请求的超时时间 ms */
    int times = 0;              /* 连接web服务器的次数 */

    timeout_ms = timeout_ms_min;
    MSHINF("开始上报信息到服务器\r\n");
    do
    {
        timeout_ms += 300;  /* 每次失败，延时时间增加 300ms */
        times ++;
        MSHLOG("第 [%d] 次上报信息，http 超时时间设置为 [%d] ms\r\n", times, timeout_ms);
        ret = __http_do_report_info(timeout_ms);
        if(timeout_ms > 10000)
        {
            timeout_ms = timeout_ms_min;  /* 如果一直失败，超时时间大于10秒仍失败时，重新计时 */
        }
        usleep(100*1000);   /* 如果失败，延时 100ms 后重新登录 */

    }while(ret != MSH_SUCCESS);
    MSHLOG("上报IPC信息到服务器成功\r\n");

    return MSH_SUCCESS;
}






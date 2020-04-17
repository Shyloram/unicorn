/*******************************************************
 *
 * LiteOS meshare 相关功能的入口
 * 
 *
 * ****************************************************/
#include <stdio.h>
#include "meshare_common.h"     /* meshare 库共用的头文件 */
#include "http_client.h"        /* http web 服务器相关的文件 */
#include "websocket_client.h"   /* websocket 相关的头文件 */

/* 调用线程池所依赖的头文件 */
#include <pthread.h>
#include "thpool.h"


static threadpool msh_thpool = NULL;    /* meshare 库专用的线程池 */


/***********************************************
 *@brief meshare 入口函数加入线程池的回调函数
 *@param *arg 参数1 目前无用 
 *@retrun
 *      void 无返回值 
 **********************************************/
static void __meshare_create(void * arg)
{
    /* http 初始化，LiteOS平台下每次开机运行，此函数只允许调用一次 */
    http_init();
    
    /* 等待网络准备好 */
    //之后再实现

    /* http 登录web服务器，阻塞，直到登录成功 */
    http_login();

    /* http 将IPC各种信息上报给服务器，阻塞，直到上报成功 */
    http_report_info();

    /* websocket client 初始化 */

    /* websocket client 登录 websocket 服务器 */
    websocket_login();
    
    



    return;
}



/***********************************************
 *@brief meshare 入口函数
 *@param 
 *@retrun
 *      void 无返回值 
 **********************************************/
void meshare_main(void)
{
    /* 如果 msh_thpool 为 NULL，则初始化线程池 */
    if(NULL == msh_thpool)
    {
        msh_thpool = thpool_init(8);
    }

    /* 如果 msh_thpool 不为 NULL，则创建 ZSP TCP server 线程，此线程在线程池中常驻，会持续占用一个线程 */
    if(NULL != msh_thpool)
    {
        thpool_add_work(msh_thpool, (void (*)(void*))__meshare_create, NULL);
    }

    /* 不需要销毁线程池 */
    //thpool_destroy(thpool);


    return;
}

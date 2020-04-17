/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : ittb_comm.h
Version       : Initial Draft
Author        : Hisilicon bvt reference design group
Created       : 2011/2/26
Description   :
History       :
1.Date        : 2011/2/26
Modification: Created file
******************************************************************************/
#ifndef _ITTB_COMM_H_
#define _ITTB_COMM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <unistd.h>
#include <errno.h>
#ifndef __LITEOS__
#include <sys/reboot.h>
#endif
#include <linux/reboot.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef __LITEOS__
#include <dlfcn.h>
#endif
#include <sys/stat.h>
#include <stdarg.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include "thttpd.h"
//#include "cgiinterface.h"
#include "hi_mt.h"

#include "hisnet_server_argparser.h"
#ifdef  __LITEOS__
#include "hi_out.h"

#endif
#include "libhttpd.h"

#include "mpi_sys.h"
#include "mpi_vb.h"

#include "ittb_vodsock.h"
#include "hi_product_icgi.h"
#include "ittb_config.h"
#include "hi_math.h"

#define HI3518 0x3518
#define HI3516 0x3516
#define HI3516A 0x3516a

#define BACKLOG 20
#define ITTB_CONFIG_FILE "ar0130_720p_line.ini"
#define CFG_FILE "config.cfg"
#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3518))
#define PQ_CHIP_NAME "hi3518"
#define CUR_VERSION "1.0.9.0"
#endif
#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3516a))
#define PQ_CHIP_NAME "hi3516a"
#define CUR_VERSION "1.0.2.0"
#endif
#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3518e))
#define PQ_CHIP_NAME "hi3518e"
#define CUR_VERSION "0.0.0.0"
#endif

#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3519))
#define PQ_CHIP_NAME "hi3519"
#define CUR_VERSION "0.0.0.0"
#endif

#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3519101))
#define PQ_CHIP_NAME "hi3519"
#define CUR_VERSION "0.0.0.0"
#endif

#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3516c300))
#define PQ_CHIP_NAME "hi3516C300"
#define CUR_VERSION "0.0.0.0"
#endif



#define ITTB_DBG_EMERG      0   /* system is unusable                   */
#define ITTB_DBG_ALERT      1   /* action must be taken immediately     */
#define ITTB_DBG_CRIT       2   /* critical conditions                  */
#define ITTB_DBG_ERR        3   /* error conditions                     */
#define ITTB_DBG_WARN       4   /* warning conditions                   */
#define ITTB_DBG_NOTICE     5   /* normal but significant condition     */
#define ITTB_DBG_INFO       6   /* informational                        */
#define ITTB_DBG_DEBUG      7   /* debug-level messages                 */

#define ITTB_SYS_ALIGN_WIDTH 64

#define UPGRADE_MAX_LEN 1024*1024*20
#define FILE_PATH_LEN       128
#define ECHO_BYE 0xB0
#define HEART_BAG 0xC0
#define ECHO_ERROR 0xE0
#define ECHO_FILE 0xE1
#define ECHO_DINIT 0xE2
#define ECHO_PARSE 0xE3
#define ECHO_INIT 0xE4

#define ITTB_BYPASSISP 0xF101



#define MAX_CHAN  2
static int ITTB_STREAM_Debug_FLPrintf(const char* pszFileName, const char* pszFunName,

                                      int s32Line, const char* pszFmt, ...)
{
    va_list struAp;
    int  iret = 0;

    va_start(struAp, pszFmt);
    printf("\r\n[%s]-<%s>(%d)",
           (NULL == pszFileName) ? "" : pszFileName,
           (NULL == pszFunName) ? "" : pszFunName, s32Line);
    iret = vprintf((NULL == pszFmt) ? "" : pszFmt, struAp);
    va_end(struAp);
    fflush(stdout);
    return iret;
}

#define ITTB_PRINTF(u32Level, args...) \
    do{\
        if (ITTB_DBG_DEBUG >= u32Level) \
        {\
			ITTB_STREAM_Debug_FLPrintf(NULL, __FUNCTION__, __LINE__, args);\
        }\
    }while(0)

#define H_usleep(usec) \
do {struct timespec req; \
    req.tv_sec  = (usec) / 1000000; \
    req.tv_nsec = ((usec) % 1000000) * 1000; \
    nanosleep (&req, NULL); \
} while (0)

#ifndef H_usleep_in_mutex(usec)
#ifdef  __LITEOS__
#define H_usleep_in_mutex(usec) H_usleep(usec)
#else
#define H_usleep_in_mutex(usec)\
do { \
	struct timeval timeout; \
	timeout.tv_sec 	= (usec) / 1000000; \
	timeout.tv_usec = usec; \
	select(0, NULL, NULL, NULL, &timeout); \
} while (0)
#endif
#endif

typedef struct hiWEBSERVER_ATTR_S
{
    int argc;
    char** argv;
} WEBSERVER_ATTR_S;

typedef enum hiITTB_VIDEO_FORMAT_E
{
    ITTB_VIDEO_FORMAT_H261  = 0,  /*H261  */
    ITTB_VIDEO_FORMAT_H263  = 1,  /*H263  */
    ITTB_VIDEO_FORMAT_MPEG2 = 2,  /*MPEG2 */
    ITTB_VIDEO_FORMAT_MPEG4 = 3,  /*MPEG4 */
    ITTB_VIDEO_FORMAT_H264  = 4,  /*H264  */
    ITTB_VIDEO_FORMAT_MJPEG = 5,  /*MOTION_JPEG*/
    ITTB_VIDEO_FORMAT_YUV   = 6,
    ITTB_VIDEO_FORMAT_JPEG  = 7,  /*JPEG*/
    ITTB_VIDEO_FORMAT_H265  = 8,  /*H265  */
    ITTB_VIDEO_FORMAT_BUTT
} ITTB_VIDEO_FORMAT_E;

typedef struct hiGetLiveFrame_S
{
    HI_BOOL bPauseFlag;
    HI_BOOL bStartFlag;
    HI_S32 s32Chn;
    pthread_t  pthH264;
} LIVE_H264_FRAME_S;


typedef struct hiGetLiveYUVFrame_S
{
    HI_BOOL bPauseFlag;
    HI_BOOL bStartFlag;
    HI_S32 s32Chn;
    pthread_t  pthyuv;
} LIVE_YUV_FRAME_S;



typedef struct GetYuvPthData_s
{
    VI_CHN   ViChn;
    HI_S32   s32ErrNo;
    HI_BOOL bStopFlag;
    HI_BOOL bPauseFlag;
    HI_U8*    pSendBuf;
    pthread_t  pthyuv;
} GetYuvPthData_S;



typedef struct hiISP_SENSOR_ATTR_S
{
    HI_CHAR*  pSensorName;
    HI_S32	  s32WDRMode;
    HI_CHAR*  pDllFile;
    HI_BOOL bmipi;
    HI_CHAR*  pMiPiFile;
    HI_BOOL bBas;
} ISP_SENSOR_ATTR_S;

typedef struct hiISP_BIND_S
{
    HI_S32 s32ViDev;
    HI_S32 s32ViChn;
    HI_S32 s32ViChnEx;
    HI_S32 s32VpssGrp;
    HI_S32 s32VpssChn;
    HI_S32 s32VpssChnEx;
    HI_S32 s32VencGrp;
    HI_S32 s32VencChn;
    HI_S32 s32VoDev;
    HI_S32 s32VoChn;
    HI_S32 s32ViSnapChn;
    HI_S32 s32VpssSnapGrp;
    HI_S32 s32VpssSnapChn;
    HI_S32 s32VencSnapGrp;
    HI_S32 s32VencSnapChn;
} ISP_BIND_S;



typedef enum hiITTB_FISHEYE_MODE_S
{
    ITTB_FISHEYE_VI  = 0,  /*calibrate in vi*/
    ITTB_FISHEYE_VPSS  = 1,  /*calibrate in vpss*/
    ITTB_FISHEYE_BUTT
} ITTB_FISHEYE_MODE_S;


typedef struct hiITTB_FISHEYE_S
{
    HI_BOOL bFishEye;
    ITTB_FISHEYE_MODE_S enFishEyeMode;
} ITTB_FISHEYE_S;

#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3516c300))
typedef enum hiITTB_SCENEAUTO_SEPCIAL_SCENE_E
{
    ITTB_SCENEAUTO_SPECIAL_SCENE_NONE    = 0,  
    ITTB_SCENEAUTO_SPECIAL_SCENE_IR      = 1, 
    ITTB_SCENEAUTO_SPECIAL_SCENE_HLC     = 2,
    ITTB_SCENEAUTO_SPECIAL_SCENE_TRAFFIC = 3,

    ITTB_SCENEAUTO_SPECIAL_SCENE_BUTT
} ITTB_SCENEAUTO_SEPCIAL_SCENE_E;

typedef struct hiITTB_SCENEAUTO_S
{
    HI_BOOL bSceneAutoEnable;
    ITTB_SCENEAUTO_SEPCIAL_SCENE_E enSceneAutoMode;
    HI_CHAR acSceneAutoFile[FILE_PATH_LEN];
} ITTB_SCENEAUTO_S;
#endif

#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3518e))
typedef enum hiITTB_SCENEAUTO_SEPCIAL_SCENE_E
{
    ITTB_SCENEAUTO_SPECIAL_SCENE_NONE = 0,
    ITTB_SCENEAUTO_SPECIAL_SCENE_BLC,
    ITTB_SCENEAUTO_SPECIAL_SCENE_IR,
    ITTB_SCENEAUTO_SPECIAL_SCENE_HLC,
    ITTB_SCENEAUTO_SPECIAL_SCENE_DYNAMIC,
    ITTB_SCENEAUTO_SPECIAL_SCENE_DRC,

    ITTB_SCENEAUTO_SPECIAL_SCENE_BUTT
} ITTB_SCENEAUTO_SEPCIAL_SCENE_E;

typedef struct hiITTB_SCENEAUTO_S
{
    HI_BOOL bSceneAutoEnable;
    ITTB_SCENEAUTO_SEPCIAL_SCENE_E enSceneAutoMode;
    HI_CHAR acSceneAutoFile[FILE_PATH_LEN];
} ITTB_SCENEAUTO_S;
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

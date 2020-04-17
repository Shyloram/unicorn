/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : ittb_media.h
Version       : Initial Draft
Author        : Hisilicon bvt reference design group
Created       : 2011/2/26
Description   :
History       :
1.Date        : 2011/2/26
Modification: Created file
******************************************************************************/

#ifndef _ITTB_MEDIA_H_
#define _ITTB_MEDIA_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#include <stdio.h>
#include "hi_type.h"
#include "hi_comm_video.h"
#include "hi_comm_isp.h"
#include "ittb_config.h"
#include "hi_comm_rc.h"
#include "ittb_sns.h"
#include "hi_comm_3a.h"
#include "mpi_ae.h"
#include "mpi_af.h"
#include "mpi_awb.h"
#include "mpi_isp.h"
#include "ittb_comm.h"
#include "ittb_jpeg.h"
#define MAX_SCENEAUTOMODE_NUM   8
#define MAX_SCENEAUTOMODE_STRLEN   64

/*format of video encode*/
typedef enum hiVS_STREAM_TYPE_E
{
    VS_STREAM_TYPE_1080p = 0,
    VS_STREAM_TYPE_720p,
    VS_STREAM_TYPE_D1,
    VS_STREAM_TYPE_CIF,
    VS_STREAM_TYPE_MOBILE,
    VS_STREAM_TYPE_YUV,
    VS_STREAM_TYPE_BUTT
} VS_STREAM_TYPE_E;

typedef struct hiYUV_DEC_FRAME_S
{
    HI_U8*  pY;                   //Y plane base address of the picture
    HI_U8*  pU;                   //U plane base address of the picture
    HI_U8*  pV;                   //V plane base address of the picture
    HI_U32  uWidth;               //The width of output picture in pixel
    HI_U32  uHeight;              //The height of output picture in pixel
    HI_U32  uYStride;             //Luma plane stride in pixel
    HI_U32  uUVStride;            //Chroma plane stride in pixel
    HI_U32  uCroppingLeftOffset;  //Crop information in pixel
    HI_U32  uCroppingRightOffset; //
    HI_U32  uCroppingTopOffset;   //
    HI_U32  uCroppingBottomOffset;//
    HI_U32  uDpbIdx;              //The index of dpb
    HI_U32  uPicFlag;             //0: Frame; 1: Top filed; 2: Bottom field
    HI_U32  bError;               //0: picture is correct; 1: picture is corrupted
    HI_U32  bIntra;               //0: intra picture; 1:inter picture
    HI_U64  ullPTS;               //Time stamp
    HI_U32  uPictureID;           //The sequence ID of this output picture decoded
    HI_U32  uReserved;            //Reserved for future
    HI_VOID* pUserData;   //Pointer to the first userdata
} YUV_DEC_FRAME_S;


typedef struct hiSceneAutoMode_S
{
    HI_CHAR aszSceneAutoModeName[MAX_SCENEAUTOMODE_STRLEN];
}SceneAutoMode_S;

typedef struct hiSceneAuto_S
{
    HI_S32 s32CurModeID;
    HI_S32 s32Num;
    HI_BOOL bSceneAutoEn;
    SceneAutoMode_S stArrSceneAutoMode[MAX_SCENEAUTOMODE_NUM];
}SceneAuto_S;

HI_S32 ITTB_MppInit(const HI_CHAR* pStrFileName, HI_BOOL bStartISP, HI_BOOL bStartVo, HI_BOOL bVencRx);
HI_S32 ITTB_MppDinit();

HI_S32 ITTB_NetInit(const HI_CHAR* pStrFileName);

HI_S32 ITTB_MPPGetFrame(ITTB_VIDEO_FORMAT_E stLiveFormat, HI_S32 s32Chn);
HI_S32 ITTB_MPPGetFd(ITTB_VIDEO_FORMAT_E stLiveFormat, HI_S32 s32Chn);
HI_S32 Ittb_MPPGetIFrame(HI_S32 chn);


HI_S32 ITTB_MPPPlayEnc();
HI_S32 ITTB_MPPStopEnc();
HI_S32 ITTB_MPP_GetChnAttr(void* pstAttr, HI_S32 s32ViChn);

HI_S32 ITTB_MPP_GetChnNum(const HI_CHAR* pStrFileName);
HI_S32 ITTB_MPP_GetBufNum(const HI_CHAR* pStrFileName);

HI_S32 ITTB_GetSceneAutoMode(SceneAuto_S *stSceneAuto);
HI_S32 ITTB_SetSceneAutoMode(HI_CHAR* pUseSceneAutoMode);
HI_S32 ITTB_SceneAuto_Init();
HI_S32 ITTB_SceneAuto_DeInit();
HI_S32 ITTB_SceneAuto_Start();
HI_S32 ITTB_SceneAuto_Stop();
    
#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP==0x3518))
#else
HI_S32 ITTB_MPP_SetWdrMode(WDR_MODE_E stWDRMode);
HI_S32 ITTB_ReStartVI(const HI_CHAR* pStrFileName);
#endif
#if ((defined(ITTB_HICHIP)) && ((ITTB_HICHIP==0x3516a)||(ITTB_HICHIP==0x3518e)))
HI_S32 ITTB_ReSetVencRx(const HI_CHAR* pStrFileName);
HI_S32 ITTB_SetIsliceEnable(const HI_CHAR* pStrFileName);
#endif
#define MBUF_H264_CHN       11
#define MBUF_YUV_CHN        11

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

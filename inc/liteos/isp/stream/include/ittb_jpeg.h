/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : ittb_jpeg.h
Version       : Initial Draft
Author        : Hisilicon bvt reference design group
Created       : 2011/2/26
Description   :
History       :
1.Date        : 2011/2/26
Modification: Created file
******************************************************************************/

#ifndef _ITTB_JPEG_H_
#define _ITTB_JPEG_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "mpi_venc.h"
#include "mpi_vpss.h"

#define FILE_PATH_LEN_JPEG 128
#define JPEG_SNAP_NAME "snap.jpg"
#define JPEG_SNAP_NAME_ERROR "snap_none.jpg"
#define JPEG_SNAP_NAME_L "snap.h265"

typedef struct stGetJpegSnapPthData
{
    HI_S32 s32VencGrp;
    HI_S32 s32VencChn;
    HI_S32  s32ErrNo;
    pthread_t  pthjpegsnap;
} GetSnapJpegPthData_S;

HI_S32 JPEG_COMM_VI_BindVenc();
HI_S32 JPEG_COMM_VENC_SnapStart(VENC_GRP VencGrp, VENC_CHN VencChn, SIZE_S* pstSize);
#ifdef  __LITEOS__
HI_S32 JPEG_COMM_VENC_SaveJPEG(VENC_STREAM_BUF_INFO_S* pstStreamBuf, FILE* fpJpegFile, VENC_STREAM_S* pstStream);
HI_S32 JPEG_COMM_VENC_SaveSnap(VENC_STREAM_BUF_INFO_S* pstStreamBuf, VENC_STREAM_S* pstStream);

#else
HI_S32 JPEG_COMM_VENC_SaveJPEG(FILE* fpJpegFile, VENC_STREAM_S* pstStream);
HI_S32 JPEG_COMM_VENC_SaveSnap(VENC_STREAM_S* pstStream);

#endif
HI_S32 JPEG_COMM_VENC_SnapStop(VENC_GRP VencGrp, VENC_CHN VencChn);

HI_S32 ITTB_JpegSnap_Befor(ISP_BIND_S* struBindAttr, SIZE_S g_stSize);

HI_S32 ITTB_StartJpegSnap();
HI_VOID* Ittb_CreateSnapThread();
HI_S32 Ittb_ExitSnapThread(HI_VOID* Handle);

void* JPEG_COMM_VENC_SnapProcess(void* pdata);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */




#endif


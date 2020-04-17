/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_raw.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : dump raw data
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_RAW_H__
#define __HI_RAW_H__

#include "hi_type.h"
#include "hi_comm_vi.h"
#pragma pack(1)



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


typedef struct hiRAW_ATTR
{
    HI_U32 u32ViDev;
    HI_U32 u32ViChn;
} HI_RAW_ATTR_S;


typedef struct hiRAW_HEAD
{
    HI_U8  au8MagicNum[8];
    HI_U16  u16Weight;
    HI_U16  u16Height;
    HI_U32  u32dept;
    ISP_BAYER_FORMAT_E enBayer;
} HI_RAW_HEAD_S;
#if((defined(ITTB_HICHIP)&& ITTB_HICHIP == 0x3518e))

typedef struct hiRAW_RGBIR_HEAD
{
    HI_U8  au8MagicNum[8];
    HI_U16  u16Weight;
    HI_U16  u16Height;
    HI_U32  u32dept;
    ISP_BAYER_FORMAT_E enBayer;
    ISP_IRPOS_TYPE_E posType;
} HI_RAW_RGBIR_HEAD_S;


typedef struct hiRAW_IR_HEAD
{
    HI_U8  au8MagicNum[8];
    HI_U16  u16Weight;
    HI_U16  u16Height;
    HI_U32  u32dept;
} HI_RAW_IR_HEAD_S;

HI_S32 HI_SetRawType(HI_U32 u32Type);
#endif
#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP==0x3518))
HI_S32 HI_StartRaw(HI_S32 chn, HI_U32 depth, HI_U8* pu8RAWBuffer, HI_U32 u32BufferLen, HI_U32 u32BayerX, HI_U32 u32BayerY);

#else
HI_S32 HI_StartRaw(HI_S32 chn, HI_U32 depth, HI_U8* pu8RAWLen, HI_U32 u32BufferLen, HI_U8* pu8Para);
#endif
HI_S32 HI_RAW_INIT(HI_RAW_ATTR_S* pstRawAttr);
#if((defined(ITTB_HICHIP)) && ((ITTB_HICHIP!= 0x3518)))
typedef struct hiVI_MST_RAW_FRM_S
{
    HI_U32 VbBlk;
    VI_RAW_DATA_INFO_S stRawDataInfo;
    HI_U32 u32FrmSize;
} VI_MST_RAW_FRM_S;

#define VI_MST_MAX_RAW_NUM		20
typedef struct hiVI_MST_RAW_INFO_S
{
    VI_MST_RAW_FRM_S 	  astMstRawFrm[VI_MST_MAX_RAW_NUM];
    HI_U32                u32FrameNum;
} VI_MST_RAW_INFO_S;

HI_S32 HI_StartDumpRaw(HI_U32 u32Depth, HI_U32 u32Mode);
HI_S32 HI_StopDumpRaw();
HI_S32 HI_DumpingRaw(HI_S32 chn, HI_U32 depth, HI_U8* pu8RAWLen, HI_U32 u32BufferLen, HI_U32 u32BayerX, HI_U32 u32BayerY);

HI_S32 HI_StartLoadRaw(HI_U8* pu8RAWLen, HI_U32 u32BufferLen);
HI_S32 HI_StopLoadRaw();

#if ((defined(ITTB_HICHIP)) && ((ITTB_HICHIP== 0x3519)) || (ITTB_HICHIP== 0x3516c300))
HI_S32 HI_LoadingRaw(HI_U8* pu8Buffer, HI_U32 u32DataLen, HI_U32 u32Depth);
#else
HI_S32 HI_LoadingRaw(HI_U8* pu8Buffer, HI_U32 u32DataLen);
#endif


HI_U32 VI_COMM_GetRawStride(PIXEL_FORMAT_E enPixelFormat, HI_U32 u32Width);
#endif


#if ((defined(ITTB_HICHIP)) && ((ITTB_HICHIP== 0x3519)))
void* HI_RAW_ReadMutil(void* args);
HI_S32 HI_StartLoadRawS(HI_U8* pu8Buffer, HI_U32 u32DataLen, HI_U32 u32FrameNumb, HI_U32 u32Type, HI_U32 u32LoopRaw);
HI_S32 HI_LoadingRawS(HI_U8* pu8Buffer, HI_U32 u32DataLen);
#endif
HI_S32 HI_RAW_GetRAWBufferLen(HI_S32 add);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_FPN_H__ */



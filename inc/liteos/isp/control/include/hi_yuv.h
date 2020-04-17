/******************************************************************************

  Copyright (C), 2001-2017, Hisilicon Tech. Co., Ltd.

******************************************************************************

******************************************************************************/
#ifndef __HI_YUV_H__
#define __HI_YUV_H__

#include "hi_type.h"
#include "hi_comm_isp.h"

#pragma pack(1)



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef struct hiYUV_HEAD
{
    HI_U8  au8Head[8];
    HI_U16 u32PicWith;
    HI_U16 u32PicHigtht;
    HI_U32 u32PicType;
} HI_YUV_HESD_S;

typedef struct hiYUV_ATTR
{
    HI_U32 u32ViDev;
    HI_U32 u32ViChn;
    HI_U32 u32VpssGrp;
    HI_U32 u32VpssChn;
    HI_U32 u32VoDev;
    HI_U32 u32VoChn;
} HI_YUV_ATTR_S;

#if((defined(ITTB_HICHIP))&& ITTB_HICHIP == 0x3518e)
typedef struct hiYUV_RGBIR_HEAD
{
    HI_U8  au8MagicNum[8];
    HI_U16  u16Weight;
    HI_U16  u16Height;
    HI_U32  u32dept;
    ISP_BAYER_FORMAT_E enBayer;
    ISP_IRPOS_TYPE_E posType;
} HI_YUV_RGBIR_HEAD_S;
HI_S32  HI_StartRaw_VI(HI_U8* pu8YUVBuffer, HI_U32 u32BufferLen);
HI_S32 HI_StartDumpVIRaw(HI_U32 u32Depth, HI_U32 u32Mode);
HI_S32 HI_StopDumpVIRaw();
HI_S32 HI_DumpingVIRaw(HI_S32 chn, HI_U32 depth, HI_U8* pu8RAWLen, HI_U32 u32BufferLen, HI_U32 u32BayerX, HI_U32 u32BayerY);

#endif

HI_S32 HI_StartYUV_VI(HI_U8* pu8YUVBuffer, HI_U32 u32BufferLen, HI_BOOL bDIS);
//HI_VOID* HI_StartYUV_VI();
HI_S32 HI_StartYUV_VPSS(HI_U8* pu8YUVBuffer, HI_U32 u32BufferLen);
#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP==0x3516a || ITTB_HICHIP==0x3518e))
HI_S32 HI_StartYUV_VO(HI_U8* pu8YUVBuffer, HI_U32 u32BufferLen);
#endif
HI_S32 HI_YUV_GetBufferLen(HI_U32 cmd);
HI_S32 HI_YUV_INIT(HI_YUV_ATTR_S* pstYUVAttr);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_FPN_H__ */



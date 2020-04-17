/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_hal_venc.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : hal for venc
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_HAL_VENC_H__
#define __HI_HAL_VENC_H__

#include "hi_type.h"
#pragma pack(1)



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */



HI_S32 HI_HAL_VENC_GetVencChn(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VENC_SetVencChn(HI_U8* pu8Data, HI_U32 u32Len);



HI_S32 HI_HAL_VENC_GetH264Attr(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VENC_SetH264Attr(HI_U8* pu8Data, HI_U32 u32Len);


HI_S32 HI_HAL_VENC_GetH265Attr(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VENC_SetH265Attr(HI_U8* pu8Data, HI_U32 u32Len);


HI_S32 HI_HAL_VENC_GetLostStrategy(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VENC_SetLostStrategy(HI_U8* pu8Data, HI_U32 u32Len);


HI_S32 HI_HAL_VENC_GetSuperFrame(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VENC_SetSuperFrame(HI_U8* pu8Data, HI_U32 u32Len);


HI_S32 HI_HAL_ISP_GetIntraRefresh(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetIntraRefresh(HI_U8* pu8Data, HI_U32 u32Len);

HI_S32 HI_HAL_ISP_SetROIcfg(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_GetROIcfg(HI_U8* pu8Data, HI_U32 u32Len);

HI_S32 HI_HAL_ISP_SetROIBgFrm(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_GetROIBgFrm(HI_U8* pu8Data, HI_U32 u32Len);

HI_S32 HI_HAL_ISP_SetColor2Grey(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_GetColor2Grey(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetRefEX(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_GetRefEX(HI_U8* pu8Data, HI_U32 u32Len);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_HAL_VENC_H__ */



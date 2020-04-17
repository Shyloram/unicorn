/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_fpn.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : hal for isp
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_FPN_H__
#define __HI_FPN_H__

#include "hi_type.h"
//#include "hi_parse.h"
//#pragma pack(1)



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */



typedef struct hiFPN_ATTR
{
    HI_U32 u32ViDev;
    HI_U32 u32ViChn;
} HI_FPN_ATTR_S;

HI_S32 HI_FPN_INIT(HI_FPN_ATTR_S* pstFPNAttr);

#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3518))
HI_S32 HI_StartFPN(HI_U32 AGainMax, HI_U32 DGainMax, HI_U32 ISPDGainMax);
HI_S32 HI_FPN_GetFileName(HI_CHAR* szFpnYuvName);
#endif

#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3516a))
HI_S32 HI_PQ_FPN_FPNCalibrate(HI_U8* pu8Data);
HI_S32 HI_PQ_FPN_FPNCorrection(HI_U8* pu8Data);

#endif




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_FPN_H__ */



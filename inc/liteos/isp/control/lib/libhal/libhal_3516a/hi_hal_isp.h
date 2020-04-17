/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_hal_isp.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : hal for isp
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
  2.Date        : 2013/11/6
    Modification: complete HI_HAL_ISP_GetExpStaInfo
                           HI_HAL_ISP_SetExpStaInfo
                           HI_HAL_ISP_GetWBStaInfo
                           HI_HAL_ISP_SetWBStaInfo
                           HI_HAL_ISP_GetInputTiming
                           HI_HAL_ISP_SetInputTiming
                           HI_HAL_ISP_GetDefectPixelAttr
                           HI_HAL_ISP_SetDefectPixelAttr
******************************************************************************/

#ifndef __HI_HAL_ISP_H__
#define __HI_HAL_ISP_H__

#include "hi_type.h"
#include "hi_hal_def.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_af.h"
#include "mpi_awb.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


/***********************AE************************************/
HI_S32 HI_HAL_ISP_GetExposureType(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetExposureType(HI_U8* pu8Data, HI_U32 u32Len);

HI_S32 HI_HAL_ISP_GetAERoute(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetAERoute(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetAERouteEx(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_GetAERouteEx(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_QueryInnerStateInfo(HI_U8* pu8Data, HI_U32 u32Len);
/***********************END AE*******************************/


/***********************AWB************************************/
HI_S32 HI_HAL_ISP_SetCCM(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_GetCCM(HI_U8* pu8Data, HI_U32 u32DataLen);
/***********************END AWB*******************************/


/***********************AF************************************/
HI_S32 HI_HAL_ISP_GetAFWind(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetAFWind(HI_U8* pu8Data, HI_U32 u32DataLen);
/***********************END AF*******************************/

/***********************ISP************************************/
HI_S32 HI_HAL_ISP_GetISPRegAttr(HI_U8* pstIspRegAttr);
HI_S32 HI_HAL_ISP_GetISPDev(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetISPDev(HI_U8* pu8Data, HI_U32 u32Len);


HI_S32 HI_HAL_ISP_GetInputTiming(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetInputTiming(HI_U8* pu8Data, HI_U32 u32Len);

HI_S32 HI_HAL_ISP_GetDefectPixelAttr(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetDefectPixelAttr(HI_U8* pu8Data);

HI_S32 HI_HAL_ISP_GetDPAttr(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetDPAttr(HI_U8* pu8Data, HI_U32 u32Len);


HI_S32 HI_HAL_ISP_GetShading(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetShading(HI_U8* pu8Data, HI_U32 u32Len);

HI_S32 HI_HAL_ISP_GetDemosaic(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_ISP_SetDemosaic(HI_U8* pu8Data, HI_U32 u32Len);

HI_S32 HI_HAL_ISP_FPNCORRECTION(HI_U8* pu8Data, HI_U32 u32DataLen);

HI_S32 HI_HAL_ISP_GetBlackLevel(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetBlackLevel(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetGamma(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_GetGamma(HI_U8* pu8Data, HI_U32 u32DataLen);

HI_S32 HI_HAL_ISP_GetDISAttr(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetDISAttr(HI_U8* pu8Data, HI_U32 u32DataLen);

HI_S32 HI_HAL_ISP_GetACMCONTROL(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetACMCONTROL(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetACMCof(HI_U32 mode, HI_U8* pu8Data, HI_U32 u32DataLen, HI_BOOL bNeedGain, HI_U32 fYGain, HI_U32 fSGain, HI_U32 fHGain);

HI_S32 HI_HAL_ISP_GetWDRMode(HI_U8* pu8Data, HI_U32 u32DataLen);

/***********************END ISP************************************/





HI_S32 HI_HAL_GetStatistics(HI_U8* pu8Data, HI_U32 u32Len, HI_U8* pu8Para);
HI_S32 HI_HAL_GetStatisticsLen();

HI_S32 HI_HAL_ISP_GetWBEWS(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetWBEWS(HI_U8* pu8Data, HI_U32 u32DataLen);

HI_S32 HI_HAL_ISP_GetModeParam(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetModeParam(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_GetNPTable(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetNPTable(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_GetGainByTemp(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetGainByTemp(HI_U8* pu8Data, HI_U32 u32DataLen);

HI_S32 HI_HAL_ISP_SetLSCS(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetCCMC(HI_U8* pu8Data, HI_U32 u32DataLen);
HI_S32 HI_HAL_ISP_SetACMC(HI_U8* pu8Data, HI_U32 u32DataLen);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_HAL_ISP_H__ */



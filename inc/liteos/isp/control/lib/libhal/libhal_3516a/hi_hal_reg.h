/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_hal_reg.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : hal for isp
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_HAL_REG_H__
#define __HI_HAL_REG_H__

#include "hi_type.h"
#include "hi_hal_def.h"


#pragma pack(1)



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

HI_VOID HI_HAL_INIT();
HI_VOID HI_HAL_DINIT();


HI_S32 HI_HAL_ReadReg(HI_S32 s32IsVirReg, HI_U32 u32Addr, HI_U32* pu32Data, PQ_DBG_MSG_PARA_S* pstMsgPara);
HI_S32 HI_HAL_WriteReg(HI_S32 s32IsVirReg, HI_U32 u32Addr, HI_U32 u32Value, PQ_DBG_MSG_PARA_S* pstMsgPara);

HI_S32 HI_HAL_ReadPhysicalReg32(HI_U32 u32Addr, HI_U32* pu32Value);
HI_S32 HI_HAL_WritePhysicalReg32(HI_U32 u32Addr, HI_U32 u32Value);

HI_S32 HI_HAL_ReadVirtualReg32(HI_U32 u32Addr, HI_U32* pu32Data);
HI_S32 HI_HAL_WriteVirtualReg32(HI_U32 u32Addr, HI_U32 u32Data);

HI_S32 HI_HAL_ReadRegs(HI_U32 u32Addr, HI_U8* pu8Value, HI_U8 u8Bytes, HI_U32 u32Len);
HI_S32 HI_HAL_WriteRegs(HI_U32 u32Addr, HI_U8* pu8Value, HI_U8 u8Bytes, HI_U32 u32Len);

HI_S32 HI_HAL_ReadVirRegs(HI_U32 u32Addr, HI_U8* pu8Value, HI_U8 u8Bytes, HI_U32 u32Len);
HI_S32 HI_HAL_WriteVirRegs(HI_U32 u32Addr, HI_U8* pu8Value, HI_U8 u8Bytes, HI_U32 u32Len);

HI_S32 HI_PQT_SetReg(HI_U32 u32Addr, HI_U32 u32Value);
HI_S32 HI_PQT_GetReg(HI_U32 u32Addr, HI_U32 *pu32Value);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_HAL_REG_H__ */




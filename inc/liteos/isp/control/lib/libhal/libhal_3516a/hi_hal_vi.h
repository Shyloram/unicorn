/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_hal_vi.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : hal for vi
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/

#ifndef __HI_HAL_VI_H__
#define __HI_HAL_VI_H__

#include "hi_type.h"
#pragma pack(1)



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

HI_S32 HI_HAL_VI_SetDCIParam(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VI_GetDCIParam(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VI_GetVIColor(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VI_SetVIColor(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VI_SetDCIParam(HI_U8* pu8Data, HI_U32 u32Len);
HI_S32 HI_HAL_VI_GetDCIParam(HI_U8* pu8Data, HI_U32 u32Len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_HAL_VI_H__ */


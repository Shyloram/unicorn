/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_comb.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : hal for isp
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_COMB_H__
#define __HI_COMB_H__

#include "hi_type.h"
#pragma pack(1)



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


HI_VOID HI_COMB_RegisterFunc(void (*pNotify)(HI_S32* pAddr, HI_S32 notify));

void* HI_COMB_START(void* args);
HI_VOID HI_COMB_INIT();
HI_VOID HI_COMB_DINIT();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_COMB_H__ */



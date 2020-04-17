/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_yuv.h
  Version       : Initial Draft
  Created       : 2015/1/19
  Description   :
  History       :
  1.Date        : 2015/1/19
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_YUV_H__
#define __HI_YUV_H__

#include "hi_type.h"



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

HI_S32 HI_ISPDebug_GetBufferLen(HI_U32 u32Addr, HI_S32 s32FrameNum);
HI_S32 HI_StartISPDebug(HI_U32 u32Addr, HI_U32 s32FrameNum, HI_U8* pu8DebugBuff, HI_U32 u32BufferLen);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_FPN_H__ */



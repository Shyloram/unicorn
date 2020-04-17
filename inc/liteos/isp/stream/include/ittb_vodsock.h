/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : vod_sock.h
Version       : Initial Draft
Author        : Hisilicon bvt reference design group
Created       : 2011/2/26
Description   :
History       :
1.Date        : 2011/2/26
Modification: Created file
******************************************************************************/


#ifndef _VOD_SOCK_H_
#define _VOD_SOCK_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "hi_mt.h"
#include "hi_mbuf_def.h"
#include "hi_mbuf.h"



HI_VOID ConfigMtransH264(MBUF_CHENNEL_INFO_S* pstMtConfig);

HI_VOID ConfigMtransYuv(MBUF_CHENNEL_INFO_S* pstMtConfig);


HI_S32  Init_MNT();

HI_S32  DeInit_MNT();
HI_S32 ITTB_MTInit(HI_S32 s32ChnNumber);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif



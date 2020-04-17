/***********************************************************************************
*              Copyright 2006 - 2006, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_out.h
* Description: out model .
*
* History:
* Version   Date         Author     DefectNum    Description
* 1.1       2006-04-07   qiubin  NULL         Create this file.
***********************************************************************************/

#ifndef __HI_OUT_H__
#define __HI_OUT_H__

#include <stdarg.h>
#include "hi_type.h"
//#include "hi_comm_inc.h"
//#include "hi_adpt_interface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define HI_VA_LIST va_list

typedef HI_VA_LIST OUT_VA_LIST;
typedef void* (*Print_FUN)(void* pDest, char* fmt, OUT_VA_LIST pstruArg);

#define DEMO_NAME_SIZE 17
typedef struct demoOUT_Reg_S
{
    HI_U32      u32Handle;
    Print_FUN   pfunPrint;
    HI_VOID*     pDest;
    struct demoOUT_Reg_S* pstruNext;
} OUT_Reg_S;

HI_S32 HI_OUT_Register( Print_FUN pfunPrint, HI_VOID* pDest, HI_U32* pu32Handle);
HI_S32 HI_OUT_Deregister(HI_U32 u32Handle);
HI_S32 HI_OUT_Editregister(HI_U32 u32Handle, Print_FUN pfunPrint, HI_VOID* pDest);
HI_S32 HI_OUT_Printf(HI_U32 u32Handle, HI_CHAR* fmt, ...);
HI_S32 OUT_DeINIT();


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __HI_OUT_H__ */

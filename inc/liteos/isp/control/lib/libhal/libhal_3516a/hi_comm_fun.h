/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_comm_fun.h
  Version       : Initial Draft
  Created       : 2014/07/5
  Description   : comm func
  History       :
  1.Date        : 2014/07/5
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_COMM_FUN_H__
#define __HI_COMM_FUN_H__



#ifndef  __LITEOS__
#include <dlfcn.h>
#endif


#include "hi_type.h"



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

HI_U32 SDK_PQ_LITTLE_ReadInt(HI_U8* pu8Data);

HI_U32 PQ_U8ToU32(HI_U8* pu8Data);



HI_U16 PQ_U8ToU16(HI_U8* pu8Data);


void PQ_U32ToU8(HI_U8* pu8Data, HI_U32 u32Data);


void PQ_U16ToU8(HI_U8* pu8Data, HI_U16 u16Data);
void PQ_U8ToU8(HI_U8* pu8Data, HI_U8 u8Data);
void PQ_S16ToU8(HI_U8* pu8Data, HI_S16 s16Data);

HI_S32 OpenDllFuncEx(HI_VOID* handle, char* function, char* data, HI_S32 IspDev);
HI_S32 OpenDllFunc(HI_VOID* handle, char* function, char* data);

#ifdef  __LITEOS__

int Ittb_control_Debug_FLPrintf(const char* pszFileName, const char* pszFunName,
                                int s32Line, const char* pszFmt, ...);

HI_S32 HI_PQ_Control_GetIniItem(const char* sFileName, const char* sSec, const char* sItem, char* sVal);

#define PQ_PRINTF(u32Level, args...) \
    do{\
        if (PQ_DBG_DEBUG >= u32Level) \
        {\
            Ittb_control_Debug_FLPrintf(NULL, __FUNCTION__, __LINE__, args);\
        }\
    }while(0)
#else
int Debug_FLPrintf(const char* pszFileName, const char* pszFunName,
                   int s32Line, const char* pszFmt, ...);
HI_CHAR *HI_PQT_Realpath(const HI_CHAR*path, HI_CHAR *resolved_path);
HI_S32 GetIniItem(const char* sFileName, const char* sSec, const char* sItem, char* sVal, int len);

#define PQ_PRINTF(u32Level, args...) \
    do{\
        if (PQ_DBG_DEBUG >= u32Level) \
        {\
            Debug_FLPrintf(NULL, __FUNCTION__, __LINE__, args);\
        }\
    }while(0)


#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_COMM_FUN_H__ */



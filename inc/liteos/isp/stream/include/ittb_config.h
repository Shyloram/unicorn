/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : ittb_config.h
Version       : Initial Draft
Author        : Hisilicon bvt reference design group
Created       : 2011/2/26
Description   :
History       :
1.Date        : 2011/2/26
Modification: Created file
******************************************************************************/
#ifndef _ITTB_CONFIG_H_
#define _ITTB_CONFIG_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
char *HI_PQT_Stream_Realpath(const char*path, char *resolved_path);
int PQ_GetIniItem(const char* sFileName, const char* sSec, const char* sItem, char* sVal, int len);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif


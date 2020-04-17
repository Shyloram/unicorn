/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : ittb_parseconfig.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : parse the config file
  History       :
  1.Date        : 2013/11/30
    Modification: This file is created.
******************************************************************************/
#ifndef __ITTB_PAESECONFIG_H__
#define __ITTB_PAESECONFIG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "hi_type.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ittb_comm.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpss.h"
#include "hi_comm_venc.h"
#include "hi_comm_isp.h"

#include "dictionary.h"
#include "iniparser.h"




typedef struct ittb_VB_ATTR
{
    HI_U32 u32VbCnt;      //vb count
    HI_U32 u32VbTimes;    //vb total times,when 16bit/14bit,2 times vb;when 12bit/10bit 1.5 times;when 8bit ,1times
} Ittb_VB_ATTR;

#ifdef  __LITEOS__
HI_S32 Ittb_LoadFile(dictionary**  dic, const char* FILENAME);
HI_VOID ITTB_FreeDict(dictionary*  dic);

#else
HI_S32 Ittb_LoadFile(const char* FILENAME);
HI_VOID ITTB_FreeDict();

#endif


#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3519 ||ITTB_HICHIP== 0x3519101||ITTB_HICHIP== 0x3516c300))
HI_S32 ITTB_PARSE_GetFishEyeAttr(ITTB_FISHEYE_S* struSensorAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVIChnExAttr(void* pstruVIChnExAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVpssChnExAttr(void* pstruVpssChnExAttr, const char* FILENAME);
HI_S32 ITTB_PARSE_GetLVDS_CROP(void* pstLvdsCrop, const char* FILENAME);
HI_S32 ITTB_PARSE_GetLVDS_MODE(void* pstMipiAttr, const char* FILENAME);
HI_S32 ITTB_PARSE_GetDISConfig(void* pstruDISConfig, const char* FILENAME);
HI_S32 ITTB_PARSE_GetDISAttr(void* pstruDISAttr, const char* FILENAME);
HI_S32 ITTB_PARSE_GetDISGyrate(void* pDISGyrate, const char* FILENAME);

#endif

HI_S32 Ittb_PARSE_GetSensorAttr(ISP_SENSOR_ATTR_S* struSensorAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVIDevAttr(void* struVIDevAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVIChnAttr(void* struVIChnAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetISPAttr(void* struIspAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVpssChnModeAttr(void* struVpssChnModeAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVPSSGrpAttr(void* pstruVpssGrpAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVpssChnAttr(void* struVpssChnAttr, const char* FILENAME);
HI_S32 Ittb_PARSE_GetISPTiming(void* struISPTiming, const char* FILENAME);
HI_S32 Ittb_PARSE_GetVENCAttr(void* struVencAttr, const char* FILENAME, HI_S32 s32Chn);
HI_S32 ITTB_PARSE_GetBindAttr(ISP_BIND_S* struBindAttr, const char* FILENAME);
HI_S32 ITTB_PARSE_GetMiPi(void* pstMipiAttr, const char* FILENAME);
HI_S32 ITTB_PARSE_GetVBCNT(void* s32VbCnt, const char* FILENAME);
HI_S32 ITTB_PARSE_GetCommChn(const char* FILENAME);
HI_S32 ITTB_PARSE_GetBufCnt(const char* FILENAME);
HI_S32 Ittb_PARSE_GetVPSSGrpCrop(void* pstruVpssCrop, const char* FILENAME);
HI_S32 ITTB_PARSE_GetWDRComMode(void* pWdrComMode, const char* FILENAME);
#if((defined(ITTB_HICHIP)) && ((ITTB_HICHIP== 0x3516c300) || (ITTB_HICHIP== 0x3518e)))
HI_S32 ITTB_PARSE_SceneAutoAttr(ITTB_SCENEAUTO_S *pstSceneAutoAttr);
#endif
#if((defined(ITTB_HICHIP)) && ((ITTB_HICHIP== 0x3516a) || (ITTB_HICHIP== 0x3518e)))
HI_S32 ITTB_PARSE_GetVencRx(void* pVencRx, const char* FILENAME, HI_S32 s32Chn);
HI_S32 ITTB_PARSE_GetIsliceEnable(void* pIntraRefresh, const char* FILENAME, HI_S32 s32Chn);
HI_S32 ITTB_PARSE_GetJpegConfig(HI_S32* pJpegConfig, const char* FILENAME);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __ITTB_PAESECONFIG_H__ */

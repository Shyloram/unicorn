/******************************************************************************

  Copyright (C), 2001-2017, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_api_def.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   :API address definations .
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/

#ifndef __HI_API_DEF_H__
#define __HI_API_DEF_H__

#include "hi_type.h"
#pragma pack(1)


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/*step 1:define the API_REG_HEAD*/
/*------------------------------------HEAD-------------------------------------*/
#define DBG_API_REG_HEAD                0xFF000000


/*step 2:define the groups  separated by modular */
/*------------------------------------GROUPS-----------------------------------*/


/*-----------------------------------------------------------------------------*/
/* DEFINE GROUPS                                                                                                        */
/*----------------------------------------------------------------------------*/
#define DBG_API_ISP                   0xFF010000
#define DBG_API_VI                     0xFF020000
#define DBG_API_VPSS                 0xFF030000
#define DBG_API_VEDU                0xFF040000
#define DBG_API_VOU                  0xFF050000
#define DBG_API_HDMI                0xFF060000
#define DBG_API_TDE                  0xFF070000
#define DBG_API_AD                    0xFF080000
#define DBG_API_DUMP                0xFF090000


/*step 3: define a groups's API    separated by smaller modular*/
/*------------------------------------GROUPS API -------------------------------*/

/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP    0xFF010000                                                                             */
/*----------------------------------------------------------------------------*/
#define DBG_API_ISP_EXPOSURE         0xFF011000
#define DBG_API_ISP_WB               0xFF012000
#define DBG_API_ISP_CCM              0xFF013000
#define DBG_API_ISP_IMP              0xFF014000
#define DBG_API_ISP_FOCUS            0xFF015000
#define DBG_API_ISP_GAMMA            0xFF016000
#define DBG_API_ISP_SYS              0xFF017000

#define DBG_API_ISP_CALIBRATE        0xFF01F000

/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_VI   0xFF020000                                                                               */
/*----------------------------------------------------------------------------*/
//HI_MPI_VI_GetDCIParam(VI_DEV ViDev, VI_DCI_PARAM_S *pstDciParam);
#define DBG_VI_DCI                  0xFF021000
#define DBG_VI_CSC                  0xFF021001


/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_VPSS   0xFF030000                                                                            */
/*----------------------------------------------------------------------------*/
//HI_MPI_VPSS_SetGrpParam(VPSS_GRP VpssGrp, VPSS_GRP_PARAM_S *pstVpssParam);
#define DBG_API_VPSS_GRPPARAM               0xFF031900
#define DBG_API_VPSS_GRPPARAM_V2            0xFF031901

#define DBG_API_VPSS_NRB_GRPPARAM           0xFF031902

/*step 4: define a groups's API    separated by fuction*/

/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP_DEV      0xFF010000                                                              */
/*------------------------------------------------------------------------- --*/
#define DBG_API_ISP_DEV         0xFF010001
#define DBG_API_ISP_PUBATTR     0xFF010100


/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP_EXPOSURE  0xFF011000                                                              */
/*------------------------------------------------------------------------- --*/
#define DBG_API_ISP_EXPOSURETYPE       0xFF011101
#define DBG_API_ISP_AEROUTE            0xFF011300
#define DBG_API_ISP_AEROUTE_EX         0xFF011301

#define DBG_API_ISP_QUERYINNERSTATEINFO       0xFF011111

//awb
/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP_EXPOSURE  0xFF011000                                                              */
/*------------------------------------------------------------------------- --*/
#define DBG_API_ISP_WBSTAINFO          0xFF012200
#define DBG_API_ISP_WBSTATYPE          0xFF012300
#define DBG_API_ISP_BLACLV             0xFF012400
#define DBG_API_ISP_CALGAINBYTEMP      0xFF012401
#define DBG_API_ISP_WBATTR             0xFF012402

//af

#define DBG_API_ISP_DIS                0xFF016001



/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP_IMP  0xFF014000                                                              */
/*------------------------------------------------------------------------- --*/
#define DBG_API_ISP_SHADING      0xFF014001
#define DBG_API_ISP_DEMOSAIC     0xFF014002
#define DBG_API_ISP_MODEPARAM    0xFF014004
#define DBG_API_ISP_NPTALBE      0xFF014005
/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP_GAMMA  0xFF016000                                                              */
/*------------------------------------------------------------------------- --*/

/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP_FOCUS  0xFF015000                                                              */
/*------------------------------------------------------------------------- --*/
#define DBG_API_ISP_FOCUS_WIND      0xFF015001

/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_VEDU  0xFF040000                                                              */
/*------------------------------------------------------------------------- --*/
#define DBG_API_VEDU_CHN                0xFF040001
#define DBG_API_VEDU_ATTR_H264          0xFF040002
#define DBG_API_VEDU_ATTR_H265          0xFF040003
#define DBG_API_VEDU_LOSTSTRATEGY       0xFF040004
#define DBG_API_VEDU_SUPERFRAME         0xFF040005
#define DBG_API_VEDU_INTRAREFRESH       0xFF040006
#define DBG_API_VEDU_ROICFG             0xFF040007
#define DBG_API_VEDU_ROIBGFRMRATE       0xFF040008
#define DBG_API_VEDU_COLOR2GREY         0xFF040009
#define DBG_API_VEDU_REF_EX             0xFF04000A
#define DBG_API_VEDU_ROI_INDEX          0xFF040010

/*-----------------------------------------------------------------------------*/
/* DEFINE calibtate  0xFF01F000                                                              */
/*------------------------------------------------------------------------- --*/
#define DBG_ISP_FPN_CALIBRATE   0xFF01F100
#define DBG_ISP_FPN_CORRECTION  0xFF01F101


#define DBG_API_ISP_ACMCONTROL  0xFF01F200
#define DBG_API_ISP_ACM_BLUE    0xFF01F201
#define DBG_API_ISP_ACM_GREEN   0xFF01F202
#define DBG_API_ISP_ACM_BG      0xFF01F203
#define DBG_API_ISP_ACM_SKIN    0xFF01F204
#define DBG_API_ISP_ACM_VIVID   0xFF01F205
#define DBG_API_ISP_ACM_CAL     0xFF01F206


#define DBG_API_ISP_DEFECTPIXEL 0xFF01F400
#define DBG_API_ISP_DPATTR      0xFF01F401


#define DBG_API_ISP_LSCCAL      0xFF01F500
#define DBG_API_ISP_CCMCAL      0xFF01F600
#define DBG_API_ISP_AWBCAL      0xFF01F700
#define DBG_API_ISP_IRCAL       0xFF01F800


//start DUMP
/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_DUMP            0xFF090000                                                                 */
/*------------------------------------------------------------------------- --*/
#define DBG_DUMP_YUV_ISP         0xFF091000
#define DBG_DUMP_YUV_RAW         0xFF091001
#define DBG_DUMP_YUV_VPSS        0xFF092000
#define DBG_DUMP_RAW_8           0xFF093000
#define DBG_DUMP_RAW_10          0xFF093100
#define DBG_DUMP_RAW_12          0xFF093200
#define DBG_DUMP_JPG             0xFF093300
#define DBG_DUMP_RAW_16          0xFF093400
#define DBG_DUMP_YUV_VO          0xFF094000
#define DBG_LOAD_RAW             0xFF095000
//stop DUMP

#define DBG_IMPORT_BIN_FILE        0xFF0F1000
#define DBG_EXPORT_BIN_FILE        0xFF0F2000
#define DBG_SOLIDIFY_BIN_FILE      0xFF0F3000
#if 0
#define DBG_LOAD_SCENE             0xFF101000
#define DBG_SEND_SCENE             0xFF102000
#endif
//start isp debug log
/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_ISP_DEBUG            0xFF200000                                                                 */
/*------------------------------------------------------------------------- --*/
#define DBG_ISP_DEBUG_ISP        0xFF200000
#define DBG_ISP_DEBUG_AE         0xFF201000
#define DBG_ISP_DEBUG_AWB        0xFF202000
#define DBG_ISP_DEBUG_DEHAZE     0xFF203000

#define DBG_VI_COLORBAR          0xFF300000

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_API_DEF_H__ */



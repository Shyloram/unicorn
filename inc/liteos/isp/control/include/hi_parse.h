/******************************************************************************

Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_parse.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : parse
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/
#ifndef __HI_PARSE_H__
#define __HI_PARSE_H__

#include "hi_type.h"
#include "hi_comm_isp.h"
#pragma pack(1)


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */



HI_S32 HI_PARSE_Start(HI_U8* pu8Data, HI_U32* cmd, HI_U32* add, HI_U32* len);


/*****************************************************************************
*   Prototype    : HI_PARSE_GetCmd
*   Description: start to parse cmd&addr
*   Input        : pu8Data:the real data
*   Output       : cmd:PQ_DBG_CMD_E
                       add: parse the addr
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*****************************************************************************/
HI_S32 HI_PARSE_GetCmd(HI_U8* pu8Data, HI_U32* cmd, HI_U32* add);

/*****************************************************************************
*   Prototype    : HI_PARSE_GetLen
*   Description: start to parse len
*   Input        : pu8Data:the real data
*   Output       : len:how long to read/write
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*****************************************************************************/
HI_S32 HI_PARSE_GetLen(HI_U8* pu8Data, HI_U32* len);

/*****************************************************************************
*   Prototype    : HI_PARSE_IsMPI
*   Description: if addr is MPI
*   Input        : add
*   Output       :
*   Return Value : HI_TRUE:isMPI ; HI_FALSE: IsVirtualReg
*****************************************************************************/
HI_BOOL HI_PARSE_IsMPI(HI_U32 u32Addr);


/*****************************************************************************
*   Prototype    : HI_PARSE_IsLongPlay
*   Description: if addr is longplay
*   Input        : add cmd
*   Output       :
*   Return Value : HI_TRUE:IsLongPlay ; HI_FALSE:Not LongPlay
*****************************************************************************/
HI_BOOL HI_PARSE_IsLongPlay(HI_U32 cmd, HI_U32 add);

/*****************************************************************************
*   Prototype    : HI_PARSE_IsVirtualReg
*   Description: if addr is virtualreg
*   Input        : add cmd
*   Output       :
*   Return Value : HI_TRUE:Isvirtualreg ; HI_FALSE:Not virtualreg
*****************************************************************************/
HI_S32 HI_PARSE_IsVirtualReg(HI_U8 u8AddrType);



/*****************************************************************************
*   Prototype    : HI_PARSE_SetISPRegAttr
*   Description: set the isp rags
*   Input        : ISP_REG_ATTR_S pstIspRegAttr
*   Output       :

*   Return Value : HI_SUCCESS: Success;HI_FAILURE: Failure.
*****************************************************************************/
HI_S32 HI_PARSE_SetISPRegAttr(ISP_REG_ATTR_S pstIspRegAttr);



/*****************************************************************************
*   Prototype    : HI_PARSE_GetVerAdd
*   Description: get the ver reg
*   Input        : u32InAddr
*   Output       : u32OutAddr

*   Return Value : HI_SUCCESS: Success;HI_FAILURE: Failure.
*****************************************************************************/
HI_S32 HI_PARSE_GetVerAdd(HI_U32 u32InAddr, HI_U32* u32OutAddr);

HI_U32 Cvt4SglChToInt(HI_U8* pu8Buffer, HI_U32 u32StartPt, HI_U32 u32TerminalPt, HI_U32* u32OutInt);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_PARSE_H__ */

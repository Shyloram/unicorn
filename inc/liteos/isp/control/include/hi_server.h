/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : hi_server.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   : main & server control declarations
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/

#ifndef __HI_SERVER_H__
#define __HI_SERVER_H__

#include "hi_type.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#ifdef  __LITEOS__

typedef enum MONITOR_STATS
{
    MONITOR_NO_SIGNAL = 0,
    MONITOR_STOP_CONTROL,
    MONITOR_READY_CONTROL,
    MONITOR_START_CONTROL
};
static enum MONITOR_STATS g_eMonitorProcess_tFlag ;
#endif
/*****************************************************************************
*   Prototype    : HI_SERVER_INIT
*   Description: init the server
*   Input        : void
*   Output       : void
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*****************************************************************************/

HI_S32 HI_SERVER_INIT();

/*****************************************************************************
*   Prototype    : HI_SERVER_DINIT
*   Description: DINIT the server
*   Input        : void
*   Output       : void
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*****************************************************************************/
HI_S32 HI_SERVER_DINIT();


/*****************************************************************************
*   Prototype    : HI_SERVER_START
*   Description: start the server
*   Input        : void
*   Output       : void
*   Return Value : HI_SUCCESS: Success;Error codes: Failure.
*****************************************************************************/
HI_S32 HI_SERVER_START();
HI_S32 HI_CreatMonitorThread();
HI_VOID HI_DealCombRetValue(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);

void* netprocess(void* args);
HI_VOID HI_StopComb(HI_S32* pAddr, HI_S32 notify);

HI_S32 Destroy();

HI_S32 GetIniItem(const char* sFileName, const char* sSec, const char* sItem, char* sVal, int len);
HI_S32 HI_ReadPort(const char* sFileName);

HI_VOID HI_RegisterFunc(void (*pfReadFun)(HI_S32 client_sockfd, HI_U8* pu8Buf,
                        HI_S32 s32BufferLen, HI_S32* ps32ReadSize), void (*pfWriteFun)(HI_S32 client_sockfd,
                                HI_U8* pu8Buf, HI_S32 s32BufferLen));
//HI_S32 ReadData(HI_S32 client_sockfd,HI_U8* pu8Buf, HI_S32 s32Count, HI_S32* ps32ReadSize);
//HI_S32 ReadData(HI_S32 client_sockfd, PQT_ADDR_S *pstPqtAddr);
HI_S32 WriteData(HI_S32 client_sockfd, HI_U8* pu8Buf, HI_S32 s32Count);

HI_S32 HI_SetData(HI_S32 client_sockfd, HI_U32 cmd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8Data, HI_U32 u32UnHandleAddr);
HI_S32 HI_SendData(HI_S32 client_sockfd, HI_U8* pu8Data, HI_U32 u32Conunt);
HI_S32 HI_SendEnd(HI_S32 client_sockfd, HI_CHAR ch);

HI_S32 HI_CheckData(HI_S32 client_sockfd, HI_U8* pu8Buf);
HI_S32 HI_CheckHeadData(HI_S32 client_sockfd, HI_U8* pu8Buf);
HI_S32 HI_CheckRealData(HI_S32 client_sockfd, HI_U8* pu8Buf);
HI_BOOL HI_CheckMagic(HI_U8* au8MagicNum);
HI_BOOL HI_CheckChip(HI_U8* au8Chip);
HI_BOOL HI_CheckVersion(HI_U8* u8Version);
HI_S32 HI_SendErrorMsg(HI_S32 client_sockfd, HI_CHAR error);
HI_S32 HI_SendHeartMsg(HI_S32 client_sockfd, HI_CHAR ch);
HI_S32 HI_CalCheckSum(HI_U8* pu8Data, HI_U32 u32Conunt, HI_U32* u32CheckSum);
HI_S32 HI_ReadReg(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8Data);
HI_S32 HI_ReadRegs(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8Data);
HI_S32 HI_WriteReg(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32Data, HI_U8* pu8Data);
HI_S32 HI_WriteRegs(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8Data);
HI_S32 HI_ReadMatrix(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8Data);
HI_S32 HI_WriteMatrix(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8Data);

HI_S32 HI_ReadYUV(HI_U32 u32Addr, HI_U8** ppu8Buffer, HI_U32* pu32DataLen, HI_U32 u32HeadTpye, HI_U8* pu8Data);
//HI_S32 HI_ReadRAW(HI_U32 u32Addr, HI_U8** ppu8Buffer, HI_U32* pu32DataLen);
HI_S32 HI_ReadRAW(HI_U8** ppu8Buffer, HI_U8* pu8Buffer);

HI_S32 HI_ReadMutilRAW(HI_S32 s32SocketID, HI_S32* pflag_addr, HI_U32 u32Addr, HI_U8* pu8MsgPara);
HI_S32 HI_LoadRaw(HI_U32 u32Addr, HI_U8* ppu8Buffer, HI_U32 pu32DataLen);

HI_S32 HI_SetMpi(HI_U32 u32Addr, HI_U8* pu8Value, HI_U32 u32Len);
HI_S32 HI_GetMpi(HI_U32 u32Addr, HI_U8* pu8Value, HI_U32 u32Len);
HI_S32 HI_ReadGamma(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_WriteGamma(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_ExportBin(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_ImportBin(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_SolidifyBin(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_ReadAWBStruct(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);

HI_S32 HI_PQ_CvtoGammaTable(HI_U16* u16Gamma, HI_U8* pu8ValidData, HI_U32 u32GammaNum);

HI_S32 HI_ReadI2C(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_WriteI2C(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);

HI_S32 HI_ReadSPI(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_WriteSPI(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);



HI_S32 HI_SendScene(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32* pu32DataLen, HI_U8** pu8ValidData);
HI_S32 HI_LoadScene(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_LoadDebug(HI_U32 u32Addr, HI_S32 s32FrameNum , HI_U8** ppu8Buffer, HI_U32* pu32DataLen);
HI_S32 HI_ResetParam(HI_VOID);


HI_S32 HI_WriteACM(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);

HI_VOID HI_RReg_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_WReg_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_RMatrix_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_WMatrix_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_WGamma_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_RRegs_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_WRegs_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_RYUV_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_LRAW_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_RRAW_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_ExBin_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_ImBin_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_SdBin_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_DEBUG_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_RAWB_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_WFPN_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_SetDP_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_RI2C_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_WI2C_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_RSPI_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);


HI_VOID HI_RSCENE_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_WSCENE_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
HI_VOID HI_ACM_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);

HI_S32 HI_GetWBWES(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_S32 HI_SetWBWES(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
HI_VOID HI_WBWES_SendToPcPrint(HI_S32 client_sockfd, HI_U32 u32Cmd, HI_U32 u32Addr, HI_S32 s32Ret);
#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3518e || ITTB_HICHIP== 0x3519) || ITTB_HICHIP== 0x3516a )
HI_S32 HI_SetCalibrates(HI_S32 client_sockfd, HI_U8* pu8Buffer, HI_U32 u32Addr, HI_U32 u32DataLen, HI_U8* pu8ValidData);
#endif
HI_S32 InitIspRegAttr(HI_S32 s32TryNum);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_SERVER_H__ */


#ifndef __ITTB_SUBS_H__
#define __ITTB_SUBS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "ittb_media.h"

int InportFile2ISPReg(char* pCfgFile);
int ExportISPReg2File(char* pCfgFile);
HI_S32 InportViVpp();
HI_S32 Ittb_Sns_Init(char* sDllFile, sns_fun_st* pstSnsFun);

//
HI_S32 Ittb_InitVideo(HI_BOOL bRaw);
HI_S32 Ittb_StartVO();
HI_S32 Ittb_StopVideo(HI_BOOL bSnapRGB);
HI_S32 Ittb_PlayVideo(HI_PLAY_STATE_E enPlayState);
HI_S32 Ittb_PauseVideo(HI_PLAY_STATE_E enPlayState);

//Vi
HI_S32 Ittb_StartupVideoInput(HI_S32 s32ViDev, HI_BOOL bBayer, HI_BOOL bSnapRGB,
                              HI_U32 u32Width, HI_U32 u32Height);
HI_S32 Ittb_EnableVpssChn(HI_S32 s32VpssDev, HI_S32 s32VpssChn, HI_BOOL bStartEx);

HI_S32 Ittb_StopVpss(HI_S32 s32VpssDev, HI_S32 s32VpssChn, HI_BOOL bSnapRGB);
HI_S32 Ittb_StopVideoInput(HI_S32 s32ViDev, HI_BOOL bSnapRGB);
HI_S32 Ittb_InitViDev(HI_S32 s32ViDev, HI_BOOL bBayer);
HI_S32 Ittb_DeInitViDev(HI_S32 s32ViDev);
HI_S32 Ittb_DeInitViChn(HI_S32 s32ViChn);
HI_S32 Ittb_SetViFrameDepth(HI_S32 s32ViChn, HI_U32 u32Depth);
HI_VOID* Ittb_StartYuv(HI_S32 s32ViChn);
HI_S32 Ittb_StopYuv(HI_VOID* pHandle);
HI_S32 Ittb_PlayYuv(HI_VOID* pHandle);
HI_S32 Ittb_PauseYuv(HI_VOID* pHandle);
HI_S32 Ittb_GetYuvPthState(HI_VOID* pHandle);

HI_S32 Ittb_VI_Start(VI_DEV ViDev, VI_CHN ViChn, HI_BOOL bRaw, HI_BOOL binit);

HI_S32  DeInit_MNT();
HI_S32 StopWebServer();
HI_S32 DeInitSystem();


HI_S32 Ittb_StartupCreateVPSS(HI_S32 VpssGrp, HI_U32 MaxW, HI_U32  MaxH, PIXEL_FORMAT_E PixFmt);

HI_S32 Ittb_VPSSVENC_Bind(HI_S32 s32VpssGrp, HI_S32 s32VencChn, HI_S32 s32GrpID);

//Vo
HI_S32 Ittb_StartupVideoOutput(HI_S32 s32VoDev, HI_S32 s32VoChn, HI_U32 u32VoWidth, HI_U32 u32VoHeight, HI_U32 u32Frame);
HI_S32 Ittb_StopVideoOutput(HI_S32 s32VoDev, HI_S32 s32VoChn);


//Venc
HI_S32 Ittb_StartEncoder(HI_S32 s32Chn, HI_U32 u32Frame);
HI_S32 Ittb_StopEncoder(HI_S32 s32Chn);
HI_S32 Ittb_PlayEnc(HI_S32 s32Chn);
HI_S32 Ittb_PauseEnc(HI_S32 s32Chn);
HI_S32 Ittb_RequstIFrame(HI_S32 s32Chn);


//Snap
HI_VOID* Ittb_StartupSnapRaw(HI_S32 s32ViDev, HI_U32 u32Nbit, HI_CHAR* pSnapFileName);
HI_S32 Ittb_StopSnapRaw(HI_VOID* Handle);
HI_S32 Ittb_GetRawFile(HI_VOID* Handle);

//Isp
HI_S32 Ittb_ISP_Start();
HI_S32 Ittb_ISP_Stop(void);
HI_S32 Ittb_ISP_UnRegister(void);

//bind for vi2vo vi2venc
HI_S32 Ittb_InitVideoAfterSnapRaw();

HI_S32 Ittb_VPSSVENC_Bind(HI_S32 s32VpssGrp, HI_S32 s32VencChn, HI_S32 s32GrpID);
HI_S32 Ittb_VPSSVENC_UnBind(HI_S32 s32ViDev, HI_S32 s32ViChn, HI_S32 s32GrpID );
HI_S32 Ittb_VOVPSS_Bind(HI_S32 s32ViDev, HI_S32 s32ViChn, HI_S32 s32VoDev, HI_S32 s32VoChn);
HI_S32 Ittb_VOVPSS_UnBind(HI_S32 s32ViDev, HI_S32 s32ViChn, HI_S32 s32VoDev, HI_S32 s32VoChn);

HI_S32 Ittb_VIVPSS_Bind(HI_S32 s32VpssDev, HI_S32 s32VpssChn, HI_S32 s32ViDev, HI_S32 s32ViChn);
HI_S32 Ittb_VIVPSS_UNBind(HI_S32 s32VpssDev, HI_S32 s32VpssChn, HI_S32 s32ViDev, HI_S32 s32ViChn, HI_BOOL bSnapRGB);


void Ittb_SaveViRawData(VIDEO_FRAME_S* pVBuf, HI_U32 u32Nbit, FILE* pfd);
void Ittb_SaveViYUVData(VIDEO_FRAME_S* pVBuf, FILE* pfd);

HI_S32 EnableVideoEncode(HI_S32 s32Chn, HI_U32  u32PicWidth, HI_U32  u32PicHeight, HI_U32 u32Frame);
HI_S32 StartEncoderThread(HI_S32 s32Chn);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __ITTB_SUBS_H__ */

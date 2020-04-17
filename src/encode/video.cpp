#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/prctl.h>

#include "zmdconfig.h"
#include "video.h"
#include "buffermanage.h"
#include "sample_comm.h"

#define VB_MAX_NUM            10
#define ONLINE_LIMIT_WIDTH    2304
#define WRAP_BUF_LINE_EXT     192

typedef struct hiSAMPLE_VPSS_ATTR_S
{   
	SIZE_S            stMaxSize;  
	DYNAMIC_RANGE_E   enDynamicRange;
	PIXEL_FORMAT_E    enPixelFormat;
	COMPRESS_MODE_E   enCompressMode[VPSS_MAX_PHY_CHN_NUM];
	SIZE_S            stOutPutSize[VPSS_MAX_PHY_CHN_NUM];
	FRAME_RATE_CTRL_S stFrameRate[VPSS_MAX_PHY_CHN_NUM];
	HI_BOOL           bMirror[VPSS_MAX_PHY_CHN_NUM];
	HI_BOOL           bFlip[VPSS_MAX_PHY_CHN_NUM];
	HI_BOOL           bChnEnable[VPSS_MAX_PHY_CHN_NUM];

	SAMPLE_SNS_TYPE_E enSnsType; 
	HI_U32            BigStreamId; 
	HI_U32            SmallStreamId;
	VI_VPSS_MODE_E    ViVpssMode;
	HI_BOOL           bWrapEn;     
	HI_U32            WrapBufLine; 
} SAMPLE_VPSS_CHN_ATTR_S;

typedef struct hiSAMPLE_VB_ATTR_S
{           
	HI_U32            validNum;
	HI_U64            blkSize[VB_MAX_NUM];
	HI_U32            blkCnt[VB_MAX_NUM];
	HI_U32            supplementConfig;
} SAMPLE_VB_ATTR_S;

HI_S32 SAMPLE_VENC_CheckSensor(SAMPLE_SNS_TYPE_E   enSnsType,SIZE_S  stSize)
{   
	HI_S32 s32Ret;    
	SIZE_S          stSnsSize;
	PIC_SIZE_E      enSnsSize;

	s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
		return s32Ret;
	}
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		return s32Ret;
	}

	if((stSnsSize.u32Width < stSize.u32Width) || (stSnsSize.u32Height < stSize.u32Height))
	{
		SAMPLE_PRT("Sensor size is (%d,%d), but encode chnl is (%d,%d) !\n",
				stSnsSize.u32Width,stSnsSize.u32Height,stSize.u32Width,stSize.u32Height);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
} 

HI_S32 SAMPLE_VENC_ModifyResolution(SAMPLE_SNS_TYPE_E   enSnsType,PIC_SIZE_E *penSize,SIZE_S *pstSize)
{
    HI_S32 s32Ret;
    SIZE_S          stSnsSize;
    PIC_SIZE_E      enSnsSize;

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    *penSize = enSnsSize;
    pstSize->u32Width  = stSnsSize.u32Width;
    pstSize->u32Height = stSnsSize.u32Height;

    return HI_SUCCESS;
}

static HI_VOID GetSensorResolution(SAMPLE_SNS_TYPE_E enSnsType, SIZE_S *pSnsSize)
{
    HI_S32          ret;
    SIZE_S          SnsSize;
    PIC_SIZE_E      enSnsSize;

    ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return;
    }
    ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &SnsSize);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return;
    }

    *pSnsSize = SnsSize;

    return;
}

static VI_VPSS_MODE_E GetViVpssModeFromResolution(SAMPLE_SNS_TYPE_E SnsType)
{
    SIZE_S SnsSize = {0};
    VI_VPSS_MODE_E ViVpssMode;

    GetSensorResolution(SnsType, &SnsSize);

    if (SnsSize.u32Width > ONLINE_LIMIT_WIDTH)
    {
        ViVpssMode = VI_OFFLINE_VPSS_ONLINE;
    }
    else
    {
        ViVpssMode = VI_ONLINE_VPSS_ONLINE;
    }

    return ViVpssMode;
}

static HI_VOID SAMPLE_VENC_GetDefaultVpssAttr(SAMPLE_SNS_TYPE_E enSnsType, HI_BOOL *pChanEnable, SIZE_S stEncSize[], SAMPLE_VPSS_CHN_ATTR_S *pVpssAttr)
{
    HI_S32 i;

    memset(pVpssAttr, 0, sizeof(SAMPLE_VPSS_CHN_ATTR_S));

    pVpssAttr->enDynamicRange = DYNAMIC_RANGE_SDR8;
    pVpssAttr->enPixelFormat  = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pVpssAttr->bWrapEn        = HI_FALSE;
    pVpssAttr->enSnsType      = enSnsType;
    pVpssAttr->ViVpssMode     = GetViVpssModeFromResolution(enSnsType);

    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if (HI_TRUE == pChanEnable[i])
        {
            pVpssAttr->enCompressMode[i]          = (i == 0)? COMPRESS_MODE_SEG : COMPRESS_MODE_NONE;
            pVpssAttr->stOutPutSize[i].u32Width   = stEncSize[i].u32Width;
            pVpssAttr->stOutPutSize[i].u32Height  = stEncSize[i].u32Height;
            pVpssAttr->stFrameRate[i].s32SrcFrameRate  = -1;
            pVpssAttr->stFrameRate[i].s32DstFrameRate  = -1;
            pVpssAttr->bMirror[i]                      = HI_FALSE;
            pVpssAttr->bFlip[i]                        = HI_FALSE;

            pVpssAttr->bChnEnable[i]                   = HI_TRUE;
        }
    }

    return;
}

static HI_VOID SAMPLE_VENC_GetCommVbAttr(const SAMPLE_SNS_TYPE_E enSnsType, const SAMPLE_VPSS_CHN_ATTR_S *pstParam,
    HI_BOOL bSupportDcf, SAMPLE_VB_ATTR_S * pstcommVbAttr)
{
    if (pstParam->ViVpssMode != VI_ONLINE_VPSS_ONLINE)
    {
        SIZE_S snsSize = {0};
        GetSensorResolution(enSnsType, &snsSize);

        if (pstParam->ViVpssMode == VI_OFFLINE_VPSS_ONLINE || pstParam->ViVpssMode == VI_OFFLINE_VPSS_OFFLINE)
        {
            pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = VI_GetRawBufferSize(snsSize.u32Width, snsSize.u32Height,
                                                                                  PIXEL_FORMAT_RGB_BAYER_12BPP,
                                                                                  COMPRESS_MODE_NONE,
                                                                                  DEFAULT_ALIGN);
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
            pstcommVbAttr->validNum++;
        }

        if (pstParam->ViVpssMode == VI_OFFLINE_VPSS_OFFLINE)
        {
            pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(snsSize.u32Width, snsSize.u32Height,
                                                                                      PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                                      DATA_BITWIDTH_8,
                                                                                      COMPRESS_MODE_NONE,
                                                                                      DEFAULT_ALIGN);
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 2;
            pstcommVbAttr->validNum++;
        }

        if (pstParam->ViVpssMode == VI_ONLINE_VPSS_OFFLINE)
        {
            pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(snsSize.u32Width, snsSize.u32Height,
                                                                                      PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                                      DATA_BITWIDTH_8,
                                                                                      COMPRESS_MODE_NONE,
                                                                                      DEFAULT_ALIGN);
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
            pstcommVbAttr->validNum++;

        }
    }
    if(HI_TRUE == pstParam->bWrapEn)
    {
        pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = VPSS_GetWrapBufferSize(pstParam->stOutPutSize[pstParam->BigStreamId].u32Width,
                                                                                 pstParam->stOutPutSize[pstParam->BigStreamId].u32Height,
                                                                                 pstParam->WrapBufLine,
                                                                                 pstParam->enPixelFormat,DATA_BITWIDTH_8,COMPRESS_MODE_NONE,DEFAULT_ALIGN);
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 1;
        pstcommVbAttr->validNum++;
    }
    else
    {
        pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(pstParam->stOutPutSize[0].u32Width, pstParam->stOutPutSize[0].u32Height,
                                                                                  pstParam->enPixelFormat,
                                                                                  DATA_BITWIDTH_8,
                                                                                  pstParam->enCompressMode[0],
                                                                                  DEFAULT_ALIGN);

        if (pstParam->ViVpssMode == VI_ONLINE_VPSS_ONLINE)
        {
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
        }
        else
        {
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 2;
        }

        pstcommVbAttr->validNum++;
    }



    pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(pstParam->stOutPutSize[1].u32Width, pstParam->stOutPutSize[1].u32Height,
                                                                              pstParam->enPixelFormat,
                                                                              DATA_BITWIDTH_8,
                                                                              pstParam->enCompressMode[1],
                                                                              DEFAULT_ALIGN);

    if (pstParam->ViVpssMode == VI_ONLINE_VPSS_ONLINE)
    {
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
    }
    else
    {
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 2;
    }
    pstcommVbAttr->validNum++;


    //vgs dcf use
    if(HI_TRUE == bSupportDcf)
    {
        pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(160, 120,
                                                                                  pstParam->enPixelFormat,
                                                                                  DATA_BITWIDTH_8,
                                                                                  COMPRESS_MODE_NONE,
                                                                                  DEFAULT_ALIGN);
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 1;
        pstcommVbAttr->validNum++;
    }

}

HI_S32 SAMPLE_VENC_SYS_Init(SAMPLE_VB_ATTR_S *pCommVbAttr)
{
    HI_U32 i;
    HI_S32 s32Ret;
    VB_CONFIG_S stVbConf;

    if (pCommVbAttr->validNum > VB_MAX_COMM_POOLS)
    {
        SAMPLE_PRT("SAMPLE_VENC_SYS_Init validNum(%d) too large than VB_MAX_COMM_POOLS(%d)!\n", pCommVbAttr->validNum, VB_MAX_COMM_POOLS);
        return HI_FAILURE;
    }

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    for (i = 0; i < pCommVbAttr->validNum; i++)
    {
        stVbConf.astCommPool[i].u64BlkSize   = pCommVbAttr->blkSize[i];
        stVbConf.astCommPool[i].u32BlkCnt    = pCommVbAttr->blkCnt[i];
        //printf("%s,%d,stVbConf.astCommPool[%d].u64BlkSize = %lld, blkSize = %d\n",__func__,__LINE__,i,stVbConf.astCommPool[i].u64BlkSize,stVbConf.astCommPool[i].u32BlkCnt);
    }

    stVbConf.u32MaxPoolCnt = pCommVbAttr->validNum;

    if(pCommVbAttr->supplementConfig == 0)
    {
        s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    }
    else
    {
        s32Ret = SAMPLE_COMM_SYS_InitWithVbSupplement(&stVbConf,pCommVbAttr->supplementConfig);
    }

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_VENC_VI_Init( SAMPLE_VI_CONFIG_S *pstViConfig, VI_VPSS_MODE_E ViVpssMode)
{
    HI_S32              s32Ret;
    SAMPLE_SNS_TYPE_E   enSnsType;
    ISP_CTRL_PARAM_S    stIspCtrlParam;
    HI_U32              u32FrameRate;


    enSnsType = pstViConfig->astViInfo[0].stSnsInfo.enSnsType;

    pstViConfig->as32WorkingViId[0]                           = 0;

    pstViConfig->astViInfo[0].stSnsInfo.MipiDev            = SAMPLE_COMM_VI_GetComboDevBySensor(pstViConfig->astViInfo[0].stSnsInfo.enSnsType, 0);
    pstViConfig->astViInfo[0].stSnsInfo.s32BusId           = 0;
    pstViConfig->astViInfo[0].stDevInfo.enWDRMode          = WDR_MODE_NONE;
    pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode    = ViVpssMode;

    //pstViConfig->astViInfo[0].stPipeInfo.aPipe[0]          = ViPipe0;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[1]          = -1;

    //pstViConfig->astViInfo[0].stChnInfo.ViChn              = ViChn;
    //pstViConfig->astViInfo[0].stChnInfo.enPixFormat        = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    //pstViConfig->astViInfo[0].stChnInfo.enDynamicRange     = enDynamicRange;
    pstViConfig->astViInfo[0].stChnInfo.enVideoFormat      = VIDEO_FORMAT_LINEAR;
    pstViConfig->astViInfo[0].stChnInfo.enCompressMode     = COMPRESS_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_SetParam(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed with %d!\n", s32Ret);
        return s32Ret;
    }

    SAMPLE_COMM_VI_GetFrameRateBySensor(enSnsType, &u32FrameRate);

    s32Ret = HI_MPI_ISP_GetCtrlParam(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0], &stIspCtrlParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_ISP_GetCtrlParam failed with %d!\n", s32Ret);
        return s32Ret;
    }
    stIspCtrlParam.u32StatIntvl  = u32FrameRate/30;

    s32Ret = HI_MPI_ISP_SetCtrlParam(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0], &stIspCtrlParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_ISP_SetCtrlParam failed with %d!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 SAMPLE_VENC_VPSS_CreateGrp(VPSS_GRP VpssGrp, SAMPLE_VPSS_CHN_ATTR_S *pParam)
{
    HI_S32          s32Ret;
    PIC_SIZE_E      enSnsSize;
    SIZE_S          stSnsSize;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(pParam->enSnsType, &enSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    stVpssGrpAttr.enDynamicRange          = pParam->enDynamicRange;
    stVpssGrpAttr.enPixelFormat           = pParam->enPixelFormat;
    stVpssGrpAttr.u32MaxW                 = stSnsSize.u32Width;
    stVpssGrpAttr.u32MaxH                 = stSnsSize.u32Height;
    stVpssGrpAttr.bNrEn                   = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enNrType       = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL;
    stVpssGrpAttr.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_U32 GetFrameRateFromSensorType(SAMPLE_SNS_TYPE_E enSnsType)
{
    HI_U32 FrameRate;

    SAMPLE_COMM_VI_GetFrameRateBySensor(enSnsType, &FrameRate);

    return FrameRate;
}

static HI_U32 GetFullLinesStdFromSensorType(SAMPLE_SNS_TYPE_E enSnsType)
{
    HI_U32 FullLinesStd = 0;

    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
            FullLinesStd = 1125;
            break;
        case SONY_IMX307_MIPI_2M_30FPS_12BIT:
        case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SONY_IMX307_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX307_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SMART_SC2235_DC_2M_30FPS_12BIT:
        case SMART_SC2230_MIPI_2M_30FPS_10BIT:
            FullLinesStd = 1125;
            break;
        case SONY_IMX335_MIPI_5M_30FPS_12BIT:
        case SONY_IMX335_MIPI_5M_30FPS_10BIT_WDR2TO1:
            FullLinesStd = 1875;
            break;
        case SONY_IMX335_MIPI_4M_30FPS_12BIT:
        case SONY_IMX335_MIPI_4M_30FPS_10BIT_WDR2TO1:
            FullLinesStd = 1375;
            break;
        case SMART_SC4236_MIPI_3M_30FPS_10BIT:
            FullLinesStd = 1600;
            break;
        default:
            SAMPLE_PRT("Error: Not support this sensor now! ==> %d\n",enSnsType);
            break;
    }

    return FullLinesStd;
}

static HI_VOID AdjustWrapBufLineBySnsType(SAMPLE_SNS_TYPE_E enSnsType, HI_U32 *pWrapBufLine)
{
    /*some sensor as follow need to expand the wrapBufLine*/
    if ((enSnsType == SMART_SC4236_MIPI_3M_30FPS_10BIT) ||
        (enSnsType == SMART_SC2235_DC_2M_30FPS_12BIT))
    {
        *pWrapBufLine += WRAP_BUF_LINE_EXT;
    }

    return;
}

static HI_S32 SAMPLE_VENC_VPSS_ChnEnable(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SAMPLE_VPSS_CHN_ATTR_S *pParam, HI_BOOL bWrapEn)
{
    HI_S32 s32Ret;
    VPSS_CHN_ATTR_S     stVpssChnAttr;
    VPSS_CHN_BUF_WRAP_S stVpssChnBufWrap;

    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    stVpssChnAttr.u32Width                     = pParam->stOutPutSize[VpssChn].u32Width;
    stVpssChnAttr.u32Height                    = pParam->stOutPutSize[VpssChn].u32Height;
    stVpssChnAttr.enChnMode                    = VPSS_CHN_MODE_USER;
    stVpssChnAttr.enCompressMode               = pParam->enCompressMode[VpssChn];
    stVpssChnAttr.enDynamicRange               = pParam->enDynamicRange;
    stVpssChnAttr.enPixelFormat                = pParam->enPixelFormat;
    stVpssChnAttr.stFrameRate.s32SrcFrameRate  = pParam->stFrameRate[VpssChn].s32SrcFrameRate;
    stVpssChnAttr.stFrameRate.s32DstFrameRate  = pParam->stFrameRate[VpssChn].s32DstFrameRate;
    stVpssChnAttr.u32Depth                     = 0;
    stVpssChnAttr.bMirror                      = pParam->bMirror[VpssChn];
    stVpssChnAttr.bFlip                        = pParam->bFlip[VpssChn];
    stVpssChnAttr.enVideoFormat                = VIDEO_FORMAT_LINEAR;
    stVpssChnAttr.stAspectRatio.enMode         = ASPECT_RATIO_NONE;

    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stVpssChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr chan %d failed with %#x\n", VpssChn, s32Ret);
        goto exit0;
    }

    if (bWrapEn)
    {
        if (VpssChn != 0)   //vpss limit! just vpss chan0 support wrap
        {
            SAMPLE_PRT("Error:Just vpss chan 0 support wrap! Current chan %d\n", VpssChn);
            goto exit0;
        }


        HI_U32 WrapBufLen = 0;
        VPSS_VENC_WRAP_PARAM_S WrapParam;

        memset(&WrapParam, 0, sizeof(VPSS_VENC_WRAP_PARAM_S));
        WrapParam.bAllOnline      = (pParam->ViVpssMode == VI_ONLINE_VPSS_ONLINE) ? HI_TRUE : HI_FALSE;
        WrapParam.u32FrameRate    = GetFrameRateFromSensorType(pParam->enSnsType);
        WrapParam.u32FullLinesStd = GetFullLinesStdFromSensorType(pParam->enSnsType);
        WrapParam.stLargeStreamSize.u32Width = pParam->stOutPutSize[pParam->BigStreamId].u32Width;
        WrapParam.stLargeStreamSize.u32Height= pParam->stOutPutSize[pParam->BigStreamId].u32Height;
        WrapParam.stSmallStreamSize.u32Width = pParam->stOutPutSize[pParam->SmallStreamId].u32Width;
        WrapParam.stSmallStreamSize.u32Height= pParam->stOutPutSize[pParam->SmallStreamId].u32Height;

        if (HI_MPI_SYS_GetVPSSVENCWrapBufferLine(&WrapParam, &WrapBufLen) == HI_SUCCESS)
        {
            AdjustWrapBufLineBySnsType(pParam->enSnsType, &WrapBufLen);

            stVpssChnBufWrap.u32WrapBufferSize = VPSS_GetWrapBufferSize(WrapParam.stLargeStreamSize.u32Width,
                WrapParam.stLargeStreamSize.u32Height, WrapBufLen, pParam->enPixelFormat, DATA_BITWIDTH_8,
                COMPRESS_MODE_NONE, DEFAULT_ALIGN);
            stVpssChnBufWrap.bEnable = HI_TRUE;
            stVpssChnBufWrap.u32BufLine = WrapBufLen;
            s32Ret = HI_MPI_VPSS_SetChnBufWrapAttr(VpssGrp, VpssChn, &stVpssChnBufWrap);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VPSS_SetChnBufWrapAttr Chn %d failed with %#x\n", VpssChn, s32Ret);
                goto exit0;
            }
        }
        else
        {
            SAMPLE_PRT("Current sensor type: %d, not support BigStream(%dx%d) and SmallStream(%dx%d) Ring!!\n",
                pParam->enSnsType,
                pParam->stOutPutSize[pParam->BigStreamId].u32Width, pParam->stOutPutSize[pParam->BigStreamId].u32Height,
                pParam->stOutPutSize[pParam->SmallStreamId].u32Width, pParam->stOutPutSize[pParam->SmallStreamId].u32Height);
        }

    }

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_EnableChn (%d) failed with %#x\n", VpssChn, s32Ret);
        goto exit0;
    }

exit0:
    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_StartGrp(VPSS_GRP VpssGrp)
{
    HI_S32          s32Ret;

    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_ChnDisable(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_DestoryGrp(VPSS_GRP VpssGrp)
{
    HI_S32          s32Ret;

    s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_Init(VPSS_GRP VpssGrp, SAMPLE_VPSS_CHN_ATTR_S *pstParam)
{
    HI_S32 i,j;
    HI_S32 s32Ret;
    HI_BOOL bWrapEn;

    s32Ret = SAMPLE_VENC_VPSS_CreateGrp(VpssGrp, pstParam);
    if (s32Ret != HI_SUCCESS)
    {
        goto exit0;
    }

    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if (pstParam->bChnEnable[i] == HI_TRUE)
        {
            bWrapEn = (i==0)? pstParam->bWrapEn : HI_FALSE;

            s32Ret = SAMPLE_VENC_VPSS_ChnEnable(VpssGrp, i, pstParam, bWrapEn);
            if (s32Ret != HI_SUCCESS)
            {
                goto exit1;
            }
        }
    }

    i--; // for abnormal case 'exit1' prossess;

    s32Ret = SAMPLE_VENC_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        goto exit1;
    }

    return s32Ret;

exit1:
    for (j = 0; j <= i; j++)
    {
        if (pstParam->bChnEnable[j] == HI_TRUE)
        {
            SAMPLE_VENC_VPSS_ChnDisable(VpssGrp, i);
        }
    }

    SAMPLE_VENC_VPSS_DestoryGrp(VpssGrp);
exit0:
    return s32Ret;
}

void *VideoEncoderThreadEntry(void *para)
{
	VIDLOG("threadid %d\n",(unsigned)pthread_self()); 
	ZMDVideo *pvideo = (ZMDVideo *)para;
	pvideo->VideoEncStreamThreadBody();
	return NULL;
} 

ZMDVideo* ZMDVideo::m_instance = new ZMDVideo;

ZMDVideo* ZMDVideo::GetInstance(void)
{
	return m_instance;
}

ZMDVideo::ZMDVideo()
{
}

ZMDVideo::~ZMDVideo()
{
}

int ZMDVideo::IspSeting()
{
#if 0

	HI_S32 s32Ret;

	//AE
	ISP_EXPOSURE_ATTR_S  stAEAttr;
	s32Ret =  HI_MPI_ISP_GetExposureAttr(0,&stAEAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_GetExposureAttr failed with %#x!\n", s32Ret);
	}

	stAEAttr.stAuto.stExpTimeRange.u32Max= 0xffffffff;
	stAEAttr.stAuto.stExpTimeRange.u32Min = 0x2;
	stAEAttr.stAuto.stAGainRange.u32Max = 0x1400;
	stAEAttr.stAuto.stDGainRange.u32Max = 0x1000;
	stAEAttr.stAuto.stISPDGainRange.u32Max = 0x1000;
	stAEAttr.stAuto.stSysGainRange.u32Max = 0x3000;
		
	s32Ret =  HI_MPI_ISP_SetExposureAttr(0,&stAEAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_SetAEAttr failed with %#x!\n", s32Ret);
	}	

#if 0
	VIDLOG("---------------ISPseting----------------------------\n");
	VIDLOG("stAEAttr.enAEMode = %x\n",stAEAttr.enOpType);
	VIDLOG("stExpTimeRange.u32Max = %x\n",stAEAttr.stAuto.stExpTimeRange.u32Max);
	VIDLOG("stExpTimeRange.u32Min = %x\n",stAEAttr.stAuto.stExpTimeRange.u32Min);
	VIDLOG("stAGainRange.u32Max = %x\n",stAEAttr.stAuto.stAGainRange.u32Max);
	VIDLOG("stAGainRange.u32Min = %x\n",stAEAttr.stAuto.stAGainRange.u32Min);
	VIDLOG("stDGainRange.u16DGainMax = %x\n",stAEAttr.stAuto.stDGainRange.u32Max );
	VIDLOG("stDGainRange.u16DGainMin = %x\n",stAEAttr.stAuto.stDGainRange.u32Min);
	VIDLOG("stISPDGainRange.u16DGainMax = %x\n",stAEAttr.stAuto.stISPDGainRange.u32Max);
	VIDLOG("stSysGainRange.u16DGainMin = %x\n",stAEAttr.stAuto.stSysGainRange.u32Max);
	VIDLOG("--------------------------------------------------------\n");
#endif

	//AWB
	ISP_WB_ATTR_S	  stAWBAttr;
	s32Ret =  HI_MPI_ISP_GetWBAttr(0,&stAWBAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_GetAEAttr failed with %#x!\n",s32Ret);
	}

#if 0
	VIDLOG("stAWBAttr.stAuto.u8RGStrength = %x\n",stAWBAttr.stAuto.u8RGStrength);
	VIDLOG("stAWBAttr.stAuto.u8BGStrength = %x\n",stAWBAttr.stAuto.u8BGStrength);
#endif

	stAWBAttr.stAuto.u8RGStrength = 0x80;
	stAWBAttr.stAuto.u8BGStrength = 0x80;
	stAWBAttr.stAuto.au16StaticWB[0]=0x15a;
	stAWBAttr.stAuto.au16StaticWB[3]=0x19a;
	
	s32Ret =  HI_MPI_ISP_SetWBAttr(0,&stAWBAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_SetAEAttr failed with %#x!\n", s32Ret);
	}	

	//SATURATION
	ISP_SATURATION_ATTR_S stSatAttr;

	s32Ret =  HI_MPI_ISP_GetSaturationAttr(0,&stSatAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_GetSaturationAttr failed with %#x!\n", s32Ret);
	}	
	int index=0;
	stSatAttr.stAuto.au8Sat[index++]=0x98;
	stSatAttr.stAuto.au8Sat[index++]=0x90;
	stSatAttr.stAuto.au8Sat[index++]=0x88;
	stSatAttr.stAuto.au8Sat[index++]=0x80;
	stSatAttr.stAuto.au8Sat[index++]=0x78;
	stSatAttr.stAuto.au8Sat[index++]=0x70;
	stSatAttr.stAuto.au8Sat[index++]=0x68;
	stSatAttr.stAuto.au8Sat[index++]=0x60;
	s32Ret =  HI_MPI_ISP_SetSaturationAttr(0,&stSatAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_SetSaturationAttr failed with %#x!\n", s32Ret);
	}	

	//3NDR
	VPSS_NR_PARAM_U unNrParam = {{0}};
	s32Ret = HI_MPI_VPSS_GetNRParam(0, &unNrParam);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_VPSS_GetNRParam failed with %#x!\n", s32Ret);
	}	
	
#if 0
	VIDLOG("\t\typk 	 %d\n",  unNrParam.stNRParam_V1.s32YPKStr);
	VIDLOG("\t\tysf 	  %d\n", unNrParam.stNRParam_V1.s32YSFStr);
	VIDLOG("\t\tytf 	  %d\n", unNrParam.stNRParam_V1.s32YTFStr);
	VIDLOG("\t\tytfmax	  %d\n", unNrParam.stNRParam_V1.s32TFStrMax);
	VIDLOG("\t\tyss 	  %d\n", unNrParam.stNRParam_V1.s32YSmthStr);
	VIDLOG("\t\tysr 	  %d\n", unNrParam.stNRParam_V1.s32YSmthRat);
	VIDLOG("\t\tysfdlt	  %d\n", unNrParam.stNRParam_V1.s32YSFStrDlt);
	VIDLOG("\t\tytfdlt	  %d\n", unNrParam.stNRParam_V1.s32YTFStrDlt);
	VIDLOG("\t\tytfdl	  %d\n", unNrParam.stNRParam_V1.s32YTFStrDl);
	VIDLOG("\t\tysfbr	  %d\n", unNrParam.stNRParam_V1.s32YSFBriRat);
	VIDLOG("\t\tcsf 	  %d\n", unNrParam.stNRParam_V1.s32CSFStr);
	VIDLOG("\t\tctf 	  %d\n", unNrParam.stNRParam_V1.s32CTFstr);
#endif

	unNrParam.stNRParam_V1.s32YSFStr = 0x79;
	s32Ret = HI_MPI_VPSS_SetNRParam(0, &unNrParam);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_VPSS_SetNRParam failed with %#x!\n", s32Ret);
	}	

	//Defect Pixel
	ISP_DP_DYNAMIC_ATTR_S stDPDynamicAttr;
	s32Ret = HI_MPI_ISP_GetDPDynamicAttr(0, &stDPDynamicAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_GetDPDynamicAttr failed with %#x!\n", s32Ret);
	}	

	index = 0;
	stDPDynamicAttr.stAuto.au16Slope[index++]=160;
	stDPDynamicAttr.stAuto.au16Slope[index++]=160;
	stDPDynamicAttr.stAuto.au16Slope[index++]=160;
	stDPDynamicAttr.stAuto.au16Slope[index++]=170;
	stDPDynamicAttr.stAuto.au16Slope[index++]=170;
	stDPDynamicAttr.stAuto.au16Slope[index++]=170;
	stDPDynamicAttr.stAuto.au16Slope[index++]=180;
	stDPDynamicAttr.stAuto.au16Slope[index++]=180;
	stDPDynamicAttr.stAuto.au16Slope[index++]=180;
	stDPDynamicAttr.stAuto.au16Slope[index++]=190;
	stDPDynamicAttr.stAuto.au16Slope[index++]=190;
	stDPDynamicAttr.stAuto.au16Slope[index++]=190;
	stDPDynamicAttr.stAuto.au16Slope[index++]=200;
	stDPDynamicAttr.stAuto.au16Slope[index++]=200;
	stDPDynamicAttr.stAuto.au16Slope[index++]=200;
	stDPDynamicAttr.stAuto.au16Slope[index++]=200;

	s32Ret = HI_MPI_ISP_SetDPDynamicAttr(0, &stDPDynamicAttr);
	if (s32Ret !=HI_SUCCESS )
	{
		VIDERR("HI_MPI_ISP_SetDPDynamicAttr failed with %#x!\n", s32Ret);
	}	

#endif
	return HI_SUCCESS;
}

int ZMDVideo::VideoInit(void)
{
	HI_S32                 i;
	HI_S32                 s32Ret;
	SIZE_S                 stSize[3];
	PIC_SIZE_E             enSize[3]       = {PIC_1080P, PIC_VGA, PIC_QVGA};
	HI_S32                 s32ChnNum       = 3;
	VENC_CHN               VencChn[3]      = {0,1,2};
	HI_U32                 u32Profile[3]   = {0,0,0};
	PAYLOAD_TYPE_E         enPayLoad       = PT_H264;
	VENC_GOP_MODE_E        enGopMode;
	VENC_GOP_ATTR_S        stGopAttr;
	SAMPLE_RC_E            enRcMode;
	HI_BOOL                bRcnRefShareBuf = HI_TRUE;

	VI_DEV                 ViDev           = 0;
	VI_PIPE                ViPipe          = 0;
	VI_CHN                 ViChn           = 0;
	SAMPLE_VI_CONFIG_S     stViConfig;

	VPSS_GRP               VpssGrp         = 0;
	VPSS_CHN               VpssChn[3]      = {0,1,2};
	HI_BOOL                abChnEnable[3]  = {HI_TRUE,HI_TRUE,HI_TRUE};
	SAMPLE_VPSS_CHN_ATTR_S stParam;
	SAMPLE_VB_ATTR_S       commVbAttr;

	for(i=0; i<s32ChnNum; i++)
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize[i], &stSize[i]);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
			return s32Ret;
		}
	}

	SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);
	if(SAMPLE_SNS_TYPE_BUTT == stViConfig.astViInfo[0].stSnsInfo.enSnsType)
	{
		SAMPLE_PRT("Not set SENSOR%d_TYPE !\n",0);
		return HI_FAILURE;
	}

	s32Ret = SAMPLE_VENC_CheckSensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType,stSize[0]);
	if(s32Ret != HI_SUCCESS)
	{
		s32Ret = SAMPLE_VENC_ModifyResolution(stViConfig.astViInfo[0].stSnsInfo.enSnsType,&enSize[0],&stSize[0]);
		if(s32Ret != HI_SUCCESS)
		{
			return HI_FAILURE;
		}
	}

	SAMPLE_VENC_GetDefaultVpssAttr(stViConfig.astViInfo[0].stSnsInfo.enSnsType, abChnEnable, stSize, &stParam);


	/******************************************
	  step  1: init sys alloc common vb
	 ******************************************/
	memset(&commVbAttr, 0, sizeof(commVbAttr));
	commVbAttr.supplementConfig = HI_FALSE;
	SAMPLE_VENC_GetCommVbAttr(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &stParam, HI_FALSE, &commVbAttr);

	/******************************************
	  step 2: mpp system init.
	 ******************************************/
	s32Ret = SAMPLE_VENC_SYS_Init(&commVbAttr);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("Init SYS err for %#x!\n", s32Ret);
		return s32Ret;
	}

	/******************************************
	  step 3: start vi dev & chn to capture
	 ******************************************/
	stViConfig.s32WorkingViNum       = 1;
	stViConfig.astViInfo[0].stDevInfo.ViDev     = ViDev;
	stViConfig.astViInfo[0].stPipeInfo.aPipe[0] = ViPipe;
	stViConfig.astViInfo[0].stChnInfo.ViChn     = ViChn;
	stViConfig.astViInfo[0].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
	stViConfig.astViInfo[0].stChnInfo.enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

	s32Ret = SAMPLE_VENC_VI_Init(&stViConfig, stParam.ViVpssMode);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("Init VI err for %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	/******************************************
	  step 4: start vpss and vi bind vpss
	 ******************************************/

	s32Ret = SAMPLE_VENC_VPSS_Init(VpssGrp,&stParam);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Init VPSS err for %#x!\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("VI Bind VPSS err for %#x!\n", s32Ret);
		return s32Ret;
	}

	//setup ISP 
	IspSeting();

	/******************************************
	  step 5: start stream venc
	 ******************************************/
	enRcMode = SAMPLE_RC_VBR;
	enGopMode = VENC_GOPMODE_NORMALP;//VENC_GOPMODE_DUALP;VENC_GOPMODE_SMARTP;
	s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode,&stGopAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Venc Get GopAttr for %#x!\n", s32Ret);
		return s32Ret;
	}

	for(i=0; i<s32ChnNum; i++)
	{
		s32Ret = SAMPLE_COMM_VENC_Start(VencChn[i], enPayLoad, enSize[i], enRcMode,u32Profile[i],bRcnRefShareBuf,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
			return s32Ret;
		}

		s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[i],VencChn[i]);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Venc bind Vpss failed for %#x!\n", s32Ret);
			return s32Ret;
		}
	}

	return HI_SUCCESS;
}

int ZMDVideo::VideoRelease(void)
{
#if 0
	VPSS_GRP VpssGrp = 0;
	VPSS_CHN VpssChn = 0;
	VENC_CHN VencChn = 0;
	HI_S32 i;

	VideoStop();
	VIDLOG("video stop is ok!\n");

	//UnBindVpssVencvi
	for(i = ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM - 1; i >= 0; i--)
	{
		VpssChn = i;
		VencChn = i;
		SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		SAMPLE_COMM_VENC_Stop(VencChn);
	}
	SAMPLE_COMM_VI_UnBindVpss(SENSOR_TYPE);

	//vpss stop
	for(i = ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM - 1; i >= 0; i--)
	{
		VpssChn = i;
		SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
	}
	SAMPLE_COMM_VPSS_StopGroup(VpssGrp);

	//vi stop
	SAMPLE_COMM_VI_StopIsp();

	//system exit
	SAMPLE_COMM_SYS_Exit();
#endif

	return 0;
}

int ZMDVideo::VideoEncStreamThreadBody(void)
{
    HI_S32 i;
    HI_S32 s32ChnTotal;
    VENC_CHN_ATTR_S stVencChnAttr;
    HI_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 VencFd[VENC_MAX_CHN_NUM];
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    VENC_CHN VencChn;
    VENC_STREAM_BUF_INFO_S stStreamBufInfo[VENC_MAX_CHN_NUM];
	BufferManage* pbuffer;

    prctl(PR_SET_NAME, "GetVencStream", 0,0,0);

    s32ChnTotal = ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM;

    /******************************************
     step 1:  check & venc-fd
    ******************************************/
    if (s32ChnTotal >= VENC_MAX_CHN_NUM)
    {
        VIDERR("input count invaild\n");
        return HI_FAILURE;
    }

    for (i = 0; i < s32ChnTotal; i++)
    {
        VencChn = i;
        s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
        if(s32Ret != HI_SUCCESS)
        {
            VIDERR("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", VencChn, s32Ret);
            return s32Ret;
        }


        /* Set Venc Fd. */
        VencFd[i] = HI_MPI_VENC_GetFd(i);
        if (VencFd[i] < 0)
        {
            VIDERR("HI_MPI_VENC_GetFd failed with %#x!\n", VencFd[i]);
            return HI_FAILURE;
        }
        if (maxfd <= VencFd[i])
        {
            maxfd = VencFd[i];
        }

        s32Ret = HI_MPI_VENC_GetStreamBufInfo (i, &stStreamBufInfo[i]);
        if (HI_SUCCESS != s32Ret)
        {
            VIDERR("HI_MPI_VENC_GetStreamBufInfo failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while (1 == m_ThreadStat)
    {
        FD_ZERO(&read_fds);
        for (i = 0; i < s32ChnTotal; i++)
        {
            FD_SET(VencFd[i], &read_fds);
        }

        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            VIDERR("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            VIDERR("get venc stream time out, exit thread\n");
            continue;
        }
        else
        {
			static int flag = 1;
			if(flag)
			{
				flag = 0;
				continue;
			}
			
            for (i = 0; i < s32ChnTotal; i++)
            {
                if (FD_ISSET(VencFd[i], &read_fds))
                {
                    /*******************************************************
		             step 2.1 : query how many packs in one-frame stream.
		            *******************************************************/
                    memset(&stStream, 0, sizeof(stStream));
                    s32Ret = HI_MPI_VENC_QueryStatus(i, &stStat);
                    if (HI_SUCCESS != s32Ret)
                    {
                        VIDERR("HI_MPI_VENC_QueryStatus chn[%d] failed with %#x!\n", i, s32Ret);
                        break;
                    }
					/*******************************************************
					step 2.2 :suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
					 if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
					 {
						VIDERR("NOTE: Current  frame is NULL!\n");
						continue;
					 }
					*******************************************************/
					if(0 == stStat.u32CurPacks)
					{
						  VIDERR("NOTE: Current  frame is NULL!\n");
						  continue;
					}
                    /*******************************************************
                     step 2.3 : malloc corresponding number of pack nodes.
                    *******************************************************/
                    stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                    if (NULL == stStream.pstPack)
                    {
                        VIDERR("malloc stream pack failed!\n");
                        break;
                    }
                    /*******************************************************
                     step 2.4 : call mpi to get one-frame stream
                    *******************************************************/
                    stStream.u32PackCount = stStat.u32CurPacks;
                    s32Ret = HI_MPI_VENC_GetStream(i, &stStream, HI_TRUE);
                    if (HI_SUCCESS != s32Ret)
                    {
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        VIDERR("HI_MPI_VENC_GetStream failed with %#x!\n", \
                               s32Ret);
                        break;
                    }

                    /*******************************************************
                     step 2.5 : send frame to buffer manage
                    *******************************************************/
					pbuffer = BufferManageCtrl::GetInstance()->GetBMInstance(i);
					if(pbuffer != NULL)
					{
#ifndef __HuaweiLite__
						pbuffer->PutOneVFrameToBuffer(NULL, &stStream);
#else
						pbuffer->PutOneVFrameToBuffer(&stStreamBufInfo[i], &stStream);
#endif
					}

                    /*******************************************************
                     step 2.6 : release stream
                    *******************************************************/
                    s32Ret = HI_MPI_VENC_ReleaseStream(i, &stStream);
                    if (HI_SUCCESS != s32Ret)
                    {
                        VIDERR("HI_MPI_VENC_ReleaseStream failed!\n");
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        break;
                    }
                    /*******************************************************
                     step 2.7 : free pack nodes
                    *******************************************************/
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                }
            }
        }
    }
    return HI_SUCCESS;
}

int ZMDVideo::VideoStart(void)
{
	HI_S32 s32Ret = HI_SUCCESS;
	m_ThreadStat = 1;

	/******************************************
	 step 6: stream venc process -- get stream, then put it to BufferManage.
	******************************************/
    s32Ret = pthread_create(&m_pid, 0, VideoEncoderThreadEntry, (void*)this);
	if (HI_SUCCESS != s32Ret)
	{
		VIDERR("Start Venc failed!\n");
		return s32Ret;
	}
	return HI_SUCCESS;
}

int ZMDVideo::VideoStop(void)
{
	if(m_ThreadStat == 1)
	{
		m_ThreadStat = 0;
		pthread_join(m_pid,0);
	}
	return 0;
}

int ZMDVideo::RequestIFrame(int ch)
{
	HI_MPI_VENC_RequestIDR(ch,HI_TRUE);
	return 0;
}

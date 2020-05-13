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

HI_S32 SAMPLE_VENC_SYS_Init(HI_U32 u32SupplementConfig,SAMPLE_SNS_TYPE_E  enSnsType)
{
	HI_S32 s32Ret;
	HI_U64 u64BlkSize;
	VB_CONFIG_S stVbConf;
	PIC_SIZE_E enSnsSize;
	SIZE_S     stSnsSize;

	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

	s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
	if (HI_SUCCESS != s32Ret)
	{    
		SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
		return s32Ret;
	}    

	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);    if (HI_SUCCESS != s32Ret)
	{    
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		return s32Ret;
	}    

	u64BlkSize = COMMON_GetPicBufferSize(stSnsSize.u32Width, stSnsSize.u32Height, PIXEL_FORMAT_YVU_SEMIPLANAR_422, DATA_BITWIDTH_8, COMPRESS_MODE_SEG,DEFAULT_ALIGN);
	stVbConf.astCommPool[0].u64BlkSize   = u64BlkSize;    stVbConf.astCommPool[0].u32BlkCnt    = 15;

	u64BlkSize = COMMON_GetPicBufferSize(1920, 1080, PIXEL_FORMAT_YVU_SEMIPLANAR_422, DATA_BITWIDTH_8, COMPRESS_MODE_SEG,DEFAULT_ALIGN);
	stVbConf.astCommPool[1].u64BlkSize   = u64BlkSize;
	stVbConf.astCommPool[1].u32BlkCnt    = 15;

	stVbConf.u32MaxPoolCnt = 2; 

	if(0 == u32SupplementConfig)
	{    
		s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	}    
	else 
	{    
		s32Ret = SAMPLE_COMM_SYS_InitWithVbSupplement(&stVbConf,u32SupplementConfig);
	}    
	if (HI_SUCCESS != s32Ret)
	{    
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		return s32Ret;    
	}    

	return HI_SUCCESS;
}

HI_S32 SAMPLE_VENC_VI_Init(SAMPLE_VI_CONFIG_S *pstViConfig, HI_BOOL bLowDelay, HI_U32 u32SupplementConfig)
{
	HI_S32              s32Ret;
	SAMPLE_SNS_TYPE_E   enSnsType;
	ISP_CTRL_PARAM_S    stIspCtrlParam;
	HI_U32              u32FrameRate;

	enSnsType = pstViConfig->astViInfo[0].stSnsInfo.enSnsType;

	pstViConfig->as32WorkingViId[0]                           = 0;
	//pstViConfig->s32WorkingViNum                              = 1;

	pstViConfig->astViInfo[0].stSnsInfo.MipiDev            = SAMPLE_COMM_VI_GetComboDevBySensor(pstViConfig->astViInfo[0].stSnsInfo.enSnsType, 0);

	//pstViConfig->astViInfo[0].stDevInfo.ViDev              = ViDev0;
	pstViConfig->astViInfo[0].stDevInfo.enWDRMode          = WDR_MODE_NONE;

	if(HI_TRUE == bLowDelay)
	{
		pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode     = VI_ONLINE_VPSS_ONLINE;
	}
	else
	{
		pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode     = VI_OFFLINE_VPSS_OFFLINE;
	}
	s32Ret = SAMPLE_VENC_SYS_Init(u32SupplementConfig,enSnsType);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("Init SYS err for %#x!\n", s32Ret);
		return s32Ret;
	}

	//if(8k == enSnsType)
	//{
	//   pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode       = VI_PARALLEL_VPSS_OFFLINE;
	//}

	//pstViConfig->astViInfo[0].stPipeInfo.aPipe[0]          = ViPipe0;
	pstViConfig->astViInfo[0].stPipeInfo.aPipe[1]          = -1;
	pstViConfig->astViInfo[0].stPipeInfo.aPipe[2]          = -1;
	pstViConfig->astViInfo[0].stPipeInfo.aPipe[3]          = -1;

	//pstViConfig->astViInfo[0].stChnInfo.ViChn              = ViChn;
	//pstViConfig->astViInfo[0].stChnInfo.enPixFormat        = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
	//pstViConfig->astViInfo[0].stChnInfo.enDynamicRange     = enDynamicRange;
	pstViConfig->astViInfo[0].stChnInfo.enVideoFormat      = VIDEO_FORMAT_LINEAR;
	pstViConfig->astViInfo[0].stChnInfo.enCompressMode     = COMPRESS_MODE_SEG;//COMPRESS_MODE_SEG;
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

static HI_S32 SAMPLE_VENC_VPSS_Init(VPSS_GRP VpssGrp, HI_BOOL* pabChnEnable, DYNAMIC_RANGE_E enDynamicRange,PIXEL_FORMAT_E enPixelFormat,SIZE_S stSize[],SAMPLE_SNS_TYPE_E enSnsType)
{
	HI_S32 i;
	HI_S32 s32Ret;
	PIC_SIZE_E      enSnsSize;
	SIZE_S          stSnsSize;
	VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
	VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

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

	stVpssGrpAttr.enDynamicRange = enDynamicRange;
	stVpssGrpAttr.enPixelFormat  = enPixelFormat;
	stVpssGrpAttr.u32MaxW        = stSnsSize.u32Width;
	stVpssGrpAttr.u32MaxH        = stSnsSize.u32Height;
	stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
	stVpssGrpAttr.bNrEn = HI_TRUE;

	for(i=0; i<VPSS_MAX_PHY_CHN_NUM; i++)
	{
		if(HI_TRUE == pabChnEnable[i])
		{
			stVpssChnAttr[i].u32Width                     = stSize[i].u32Width;
			stVpssChnAttr[i].u32Height                    = stSize[i].u32Height;            
			stVpssChnAttr[i].enChnMode                    = VPSS_CHN_MODE_USER;
			stVpssChnAttr[i].enCompressMode               = COMPRESS_MODE_NONE;//COMPRESS_MODE_SEG;
			stVpssChnAttr[i].enDynamicRange               = enDynamicRange;
			stVpssChnAttr[i].enPixelFormat                = enPixelFormat;
			stVpssChnAttr[i].stFrameRate.s32SrcFrameRate  = -1;
			stVpssChnAttr[i].stFrameRate.s32DstFrameRate  = -1;
			stVpssChnAttr[i].u32Depth                     = 0;
			stVpssChnAttr[i].bMirror                      = HI_FALSE;
			stVpssChnAttr[i].bFlip                        = HI_FALSE;
			stVpssChnAttr[i].enVideoFormat                = VIDEO_FORMAT_LINEAR;
			stVpssChnAttr[i].stAspectRatio.enMode         = ASPECT_RATIO_NONE;
		}
	}

	s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, pabChnEnable,&stVpssGrpAttr,stVpssChnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
	}

	return s32Ret;
}

void *VideoEncoderThreadEntry(void *para)
{
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

int ZMDVideo::VideoInit(void)
{
	HI_S32                 i;
	HI_S32                 s32Ret;
	SIZE_S                 stSize[3];
	PIC_SIZE_E             enSize[3]       = {PIC_3840x2160, PIC_1080P, PIC_720P};
	HI_S32                 s32ChnNum       = ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM;
	VENC_CHN               VencChn[3]      = {0,1,2};
	HI_U32                 u32Profile[3]   = {0,0,0};
	VENC_GOP_MODE_E        enGopMode;
	VENC_GOP_ATTR_S        stGopAttr;
	SAMPLE_RC_E            enRcMode;

	VI_DEV                 ViDev           = 0;
	VI_PIPE                ViPipe          = 0;
	VI_CHN                 ViChn           = 0;

	VPSS_GRP               VpssGrp         = 0;
	VPSS_CHN               VpssChn[3]      = {0,1,2};

	HI_U32 u32SupplementConfig = HI_FALSE;

	for(i=0; i<3; i++)
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize[i], &stSize[i]);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
			return s32Ret;
		}
	}

	SAMPLE_COMM_VI_GetSensorInfo(&m_stViConfig);
	if(SAMPLE_SNS_TYPE_BUTT == m_stViConfig.astViInfo[0].stSnsInfo.enSnsType)
	{
		SAMPLE_PRT("Not set SENSOR%d_TYPE !\n",0);
		return HI_FAILURE;
	}

	s32Ret = SAMPLE_VENC_CheckSensor(m_stViConfig.astViInfo[0].stSnsInfo.enSnsType,stSize[0]);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("Check Sensor err!\n");
		return HI_FAILURE;
	}

	/******************************************
	  step 3: start vi dev & chn to capture
	 ******************************************/
	m_stViConfig.s32WorkingViNum       = 1;
	m_stViConfig.astViInfo[0].stDevInfo.ViDev     = ViDev;
	m_stViConfig.astViInfo[0].stPipeInfo.aPipe[0] = ViPipe;
	m_stViConfig.astViInfo[0].stChnInfo.ViChn     = ViChn;
	m_stViConfig.astViInfo[0].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
	m_stViConfig.astViInfo[0].stChnInfo.enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

	s32Ret = SAMPLE_VENC_VI_Init(&m_stViConfig, HI_FALSE,u32SupplementConfig);
	if(s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("Init VI err for %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	/******************************************
	  step 4: start vpss and vi bind vpss
	 ******************************************/

	s32Ret = SAMPLE_VENC_VPSS_Init(VpssGrp,m_abChnEnable,DYNAMIC_RANGE_SDR8,PIXEL_FORMAT_YVU_SEMIPLANAR_420,stSize,m_stViConfig.astViInfo[0].stSnsInfo.enSnsType);
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
		s32Ret = SAMPLE_COMM_VENC_Start(VencChn[i], m_enPayLoad[i], enSize[i], enRcMode,u32Profile[i],&stGopAttr);
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
	VPSS_GRP VpssGrp = 0;
	VPSS_CHN VpssChn = 0;
	VENC_CHN VencChn = 0;
	VI_PIPE  ViPipe  = 0;
	VI_CHN   ViChn   = 0;
	HI_S32 i;

	VideoStop();
	VIDLOG("video stop is ok!\n");

	//UnBindVpssVencvi
	for(i = ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM - 1; i >= 0; i--)
	{
		VpssChn = i;
		VencChn = i;
		SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp,VpssChn,VencChn);
		SAMPLE_COMM_VENC_Stop(VencChn);
	}
	SAMPLE_COMM_VI_UnBind_VPSS(ViPipe,ViChn,VpssGrp);

	//vpss stop
	SAMPLE_COMM_VPSS_Stop(VpssGrp,m_abChnEnable);

	//vi stop
	SAMPLE_COMM_VI_StopVi(&m_stViConfig);

	//system exit
	SAMPLE_COMM_SYS_Exit();

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
						pbuffer->PutOneVFrameToBuffer(NULL, &stStream, m_enPayLoad[i]);
#else
						pbuffer->PutOneVFrameToBuffer(&stStreamBufInfo[i], &stStream, m_enPayLoad[i]);
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

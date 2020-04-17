/******************************************************************************
  Some simple Hisilicon HI3531 video input functions.

  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-8 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include "hi_mipi.h"
#include "hi_common.h"
#include "sample_comm.h"

VI_DEV_ATTR_S DEV_ATTR_SC2235_DC_1080P =
{
	/*接口模式*/
	VI_MODE_DIGITAL_CAMERA,
	/*1、2、4路工作模式*/
	VI_WORK_MODE_1Multiplex,
	/* r_mask    g_mask    b_mask*/
	{0x3FF0000,    0x0}, //{0xFFF00000,    0x0}, 
	/*逐行or隔行输入*/
	VI_SCAN_PROGRESSIVE,
	/*AdChnId*/
	{-1, -1, -1, -1},
	/*enDataSeq, 仅支持YUV格式*/
	VI_INPUT_DATA_YUYV,

	/*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
	{
		/*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
		VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,

		/*timing信息，对应reg手册的如下配置*/
		/*hsync_hfb    hsync_act    hsync_hhb*/
		{0,            1920,        0,
			/*vsync0_vhb vsync0_act vsync0_hhb*/
			0,            1080,        0,                 //     6,            720,        6,
			/*vsync1_vhb vsync1_act vsync1_hhb*/ 
			0,            0,            0}
	},    
	/*使用内部ISP*/
	VI_PATH_ISP,
	/*输入数据类型*/
	VI_DATA_TYPE_RGB,
	/* Data Reverse */
	HI_FALSE,
	{0, 0, 1920, 1080}//{0, 0, 1280, 720}
};

combo_dev_attr_t MIPI_CMOS3V3_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_CMOS_33V,
    {

    }
};

combo_dev_attr_t MIPI_CMOS1V8_ATTR =
{
    /* input mode */
    .input_mode = INPUT_MODE_CMOS_18V,
    {

    }
};

HI_S32 SAMPLE_COMM_VI_Mode2Param(SAMPLE_VI_MODE_E enViMode, SAMPLE_VI_PARAM_S *pstViParam)
{
    switch (enViMode)
    {
        default:
            pstViParam->s32ViDevCnt      = 1;
            pstViParam->s32ViDevInterval = 1;
            pstViParam->s32ViChnCnt      = 1;
            pstViParam->s32ViChnInterval = 1;
            break;
    }
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_VI_SetMipiAttr(SAMPLE_VI_CONFIG_S* pstViConfig)
{   
	HI_S32 fd;
	combo_dev_attr_t *pstcomboDevAttr = NULL;

	/* mipi reset unrest */
	fd = open("/dev/hi_mipi", O_RDWR);
	if (fd < 0)
	{
		printf("warning: open hi_mipi dev failed\n");
		return -1;
	}
	//printf("=============SAMPLE_COMM_VI_SetMipiAttr enWDRMode: %d\n", pstViConfig->enWDRMode);
	//printf("=============SAMPLE_COMM_VI_SetMipiAttr enViMode: %d\n", pstViConfig->enViMode);

	pstcomboDevAttr = &MIPI_CMOS1V8_ATTR;

	if (NULL == pstcomboDevAttr)
	{
		printf("Func %s() Line[%d], unsupported enViMode: %d\n", __FUNCTION__, __LINE__, pstViConfig->enViMode);
		close(fd);
		return HI_FAILURE;   
	}

	if (ioctl(fd, HI_MIPI_SET_DEV_ATTR, pstcomboDevAttr))
	{
		printf("set mipi attr failed\n");
		close(fd);
		return -1;
	}
	close(fd);
	return HI_SUCCESS;
}

/*****************************************************************************
* function : star vi dev (cfg vi_dev_attr; set_dev_cfg; enable dev)
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_StartDev(VI_DEV ViDev, SAMPLE_VI_MODE_E enViMode)
{
	HI_S32 s32Ret;
	HI_S32 s32IspDev = 0;
	ISP_WDR_MODE_S stWdrMode;
	VI_DEV_ATTR_S  stViDevAttr;

	memset(&stViDevAttr,0,sizeof(stViDevAttr));

	switch (enViMode)
	{
		case OMNIVISION_OV2235_DC_1080P_20FPS:
			memcpy(&stViDevAttr,&DEV_ATTR_SC2235_DC_1080P,sizeof(stViDevAttr));
			stViDevAttr.stDevRect.s32X = 0;
			stViDevAttr.stDevRect.s32Y = 0;
			stViDevAttr.stDevRect.u32Width  = 1920;
			stViDevAttr.stDevRect.u32Height = 1080;
			break;

		default:
			memcpy(&stViDevAttr,&DEV_ATTR_SC2235_DC_1080P,sizeof(stViDevAttr));
	}

	s32Ret = HI_MPI_VI_SetDevAttr(ViDev, &stViDevAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_ISP_GetWDRMode(s32IspDev, &stWdrMode);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_ISP_GetWDRMode failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	if (stWdrMode.enWDRMode)  //wdr mode
	{
		VI_WDR_ATTR_S stWdrAttr;

		s32Ret = HI_MPI_VI_GetWDRAttr(ViDev, &stWdrAttr);
		if (s32Ret)
		{
			SAMPLE_PRT("HI_MPI_VI_GetWDRAttr failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}

		stWdrAttr.enWDRMode = stWdrMode.enWDRMode;
		stWdrAttr.bCompress = HI_FALSE;

		s32Ret = HI_MPI_VI_SetWDRAttr(ViDev, &stWdrAttr);
		if (s32Ret)
		{
			SAMPLE_PRT("HI_MPI_VI_SetWDRAttr failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}

	s32Ret = HI_MPI_VI_EnableDev(ViDev);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VI_EnableDev failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/*****************************************************************************
* function : star vi chn
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_StartChn(VI_CHN ViChn, RECT_S *pstCapRect, SIZE_S *pstTarSize, SAMPLE_VI_CONFIG_S* pstViConfig)
{
    HI_S32 s32Ret;
    VI_CHN_ATTR_S stChnAttr;
    ROTATE_E enRotate = ROTATE_NONE;
    SAMPLE_VI_CHN_SET_E enViChnSet = VI_CHN_SET_NORMAL;

    if(pstViConfig)
    {
        enViChnSet = pstViConfig->enViChnSet;
        enRotate = pstViConfig->enRotate;
    }

    /* step  5: config & start vicap dev */
    memcpy(&stChnAttr.stCapRect, pstCapRect, sizeof(RECT_S));
    stChnAttr.enCapSel = VI_CAPSEL_BOTH;
    /* to show scale. this is a sample only, we want to show dist_size = D1 only */
    stChnAttr.stDestSize.u32Width = pstTarSize->u32Width;
    stChnAttr.stDestSize.u32Height = pstTarSize->u32Height;
    stChnAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;   /* sp420 or sp422 */

    stChnAttr.bMirror = HI_FALSE;
    stChnAttr.bFlip = HI_FALSE;

    switch(enViChnSet)
    {
        case VI_CHN_SET_MIRROR:
            stChnAttr.bMirror = HI_TRUE;
            break;

        case VI_CHN_SET_FLIP:
            stChnAttr.bFlip = HI_TRUE;
            break;
            
        case VI_CHN_SET_FLIP_MIRROR:
            stChnAttr.bMirror = HI_TRUE;
            stChnAttr.bFlip = HI_TRUE;
            break;
            
        default:
            break;
    }

    stChnAttr.s32SrcFrameRate = -1;
    stChnAttr.s32DstFrameRate = -1;
    stChnAttr.enCompressMode = COMPRESS_MODE_NONE;

    s32Ret = HI_MPI_VI_SetChnAttr(ViChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    if(ROTATE_NONE != enRotate)
    {
        s32Ret = HI_MPI_VI_SetRotate(ViChn, enRotate);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VI_SetRotate failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
    }
    
    s32Ret = HI_MPI_VI_EnableChn(ViChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/*****************************************************************************
* function : Vi chn bind vpss group
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_BindVpss(SAMPLE_VI_MODE_E enViMode)
{
    HI_S32 j, s32Ret;
    VPSS_GRP VpssGrp;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    SAMPLE_VI_PARAM_S stViParam;
    VI_CHN ViChn;

    s32Ret = SAMPLE_COMM_VI_Mode2Param(enViMode, &stViParam);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_Mode2Param failed!\n");
        return HI_FAILURE;
    }
    
    VpssGrp = 0;
    for (j=0; j<stViParam.s32ViChnCnt; j++)
    {
        ViChn = j * stViParam.s32ViChnInterval;
        
        stSrcChn.enModId  = HI_ID_VIU;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = ViChn;
    
        stDestChn.enModId  = HI_ID_VPSS;
        stDestChn.s32DevId = VpssGrp;
        stDestChn.s32ChnId = 0;
    
        s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
        
        VpssGrp ++;
    }
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_VI_StartIspAndVi(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    HI_S32 i, s32Ret = HI_SUCCESS;
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_U32 u32DevNum = 1;
    HI_U32 u32ChnNum = 1;
    SIZE_S stTargetSize;
    RECT_S stCapRect;
    SAMPLE_VI_MODE_E enViMode;
	PIC_SIZE_E enSize;

    if(!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }
    enViMode = pstViConfig->enViMode;
	enSize = pstViConfig->enSize;

    /******************************************
     step 1: mipi configure
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_SetMipiAttr(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("%s: MIPI init failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }     

    /******************************************
     step 2: configure sensor and ISP (include WDR mode).
     note: you can jump over this step, if you do not use Hi3516A interal isp. 
    ******************************************/
    s32Ret = SAMPLE_COMM_ISP_Init(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("%s: Sensor init failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    /******************************************
     step 3: run isp thread 
     note: you can jump over this step, if you do not use Hi3516A interal isp.
    ******************************************/
    s32Ret = SAMPLE_COMM_ISP_Run();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("%s: ISP init failed!\n", __FUNCTION__);
	    /* disable videv */
        return HI_FAILURE;
    }
    
    /******************************************************
     step 4 : config & start vicap dev
    ******************************************************/
    for (i = 0; i < u32DevNum; i++)
    {
        ViDev = i;
        s32Ret = SAMPLE_COMM_VI_StartDev(ViDev, enViMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("%s: start vi dev[%d] failed!\n", __FUNCTION__, i);
            return HI_FAILURE;
        }
    }
    
    /******************************************************
    * Step 5: config & start vicap chn (max 1) 
    ******************************************************/
    for (i = 0; i < u32ChnNum; i++)
    {
        ViChn = i;

        stCapRect.s32X = 0;
        stCapRect.s32Y = 0;
        switch (enSize)
        {
            case PIC_HD720:
                stCapRect.u32Width  = 1280;
                stCapRect.u32Height = 720;
                break;

            case PIC_HD1080:
                stCapRect.u32Width  = 1920;
                stCapRect.u32Height = 1080;
                break;

            default:
                stCapRect.u32Width  = 1920;
                stCapRect.u32Height = 1080;
                break;
        }

        stTargetSize.u32Width = stCapRect.u32Width;
        stTargetSize.u32Height = stCapRect.u32Height;

        s32Ret = SAMPLE_COMM_VI_StartChn(ViChn, &stCapRect, &stTargetSize, pstViConfig);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_COMM_ISP_Stop();
            return HI_FAILURE;
        }
    }

    return s32Ret;
}

HI_S32 SAMPLE_COMM_VI_StopIsp(void)
{
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_S32 i;
    HI_S32 s32Ret;
    HI_U32 u32DevNum = 1;
    HI_U32 u32ChnNum = 1;

    /*** Stop VI Chn ***/
    for(i=0;i < u32ChnNum; i++)
    {
        /* Stop vi phy-chn */
        ViChn = i;
        s32Ret = HI_MPI_VI_DisableChn(ViChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_DisableChn failed with %#x\n",s32Ret);
            return HI_FAILURE;
        }
    }

    /*** Stop VI Dev ***/
    for(i=0; i < u32DevNum; i++)
    {
        ViDev = i;
        s32Ret = HI_MPI_VI_DisableDev(ViDev);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_DisableDev failed with %#x\n", s32Ret);
            return HI_FAILURE;
        }
    }

    SAMPLE_COMM_ISP_Stop();
    return HI_SUCCESS;
}
/*****************************************************************************
* function : Vi chn unbind vpss group
*****************************************************************************/
HI_S32 SAMPLE_COMM_VI_UnBindVpss(SAMPLE_VI_MODE_E enViMode)
{
    HI_S32 i, j, s32Ret;
    VPSS_GRP VpssGrp;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    SAMPLE_VI_PARAM_S stViParam;
    VI_DEV ViDev;
    VI_CHN ViChn;

    s32Ret = SAMPLE_COMM_VI_Mode2Param(enViMode, &stViParam);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_Mode2Param failed!\n");
        return HI_FAILURE;
    }

    VpssGrp = 0;
    for (i=0; i<stViParam.s32ViDevCnt; i++)
    {
        ViDev = i * stViParam.s32ViDevInterval;

        for (j=0; j<stViParam.s32ViChnCnt; j++)
        {
            ViChn = j * stViParam.s32ViChnInterval;

            stSrcChn.enModId = HI_ID_VIU;
            stSrcChn.s32DevId = ViDev;
            stSrcChn.s32ChnId = ViChn;

            stDestChn.enModId = HI_ID_VPSS;
            stDestChn.s32DevId = VpssGrp;
            stDestChn.s32ChnId = 0;

            s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }

            VpssGrp ++;
        }
    }
    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

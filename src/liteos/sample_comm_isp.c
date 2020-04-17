/******************************************************************************
  Some simple Hisilicon Hi35xx isp functions.

  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
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
#include "sample_comm.h"

static pthread_t gs_IspPid = 0;
static HI_BOOL gbIspInited = HI_FALSE;


/******************************************************************************
* funciton : ISP init
******************************************************************************/
HI_S32 SAMPLE_COMM_ISP_Init(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    ISP_PUB_ATTR_S stPubAttr;
    ALG_LIB_S stLib;
	SAMPLE_VI_MODE_E enViMode = pstViConfig->enViMode;
    
#if 0   
    /* 0. set cmos iniparser file path */
    s32Ret = sensor_set_inifile_path("configs/");
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: set cmos iniparser file path failed with %#x!\n", \
               __FUNCTION__, s32Ret);
        return s32Ret;
    }
#endif

    /* 1. sensor register callback */
    s32Ret = sensor_register_callback();
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: sensor_register_callback failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 2. register hisi ae lib */
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AE_LIB_NAME, 20);
    s32Ret = HI_MPI_AE_Register(IspDev, &stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AE_Register failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 3. register hisi awb lib */
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AWB_LIB_NAME, 20);
    s32Ret = HI_MPI_AWB_Register(IspDev, &stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AWB_Register failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 4. register hisi af lib */
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AF_LIB_NAME, 20);
    s32Ret = HI_MPI_AF_Register(IspDev, &stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AF_Register failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 5. isp mem init */
    s32Ret = HI_MPI_ISP_MemInit(IspDev);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_MemInit failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 6. isp set WDR mode */
    ISP_WDR_MODE_S stWdrMode;
    stWdrMode.enWDRMode  = pstViConfig->enWDRMode;
    s32Ret = HI_MPI_ISP_SetWDRMode(0, &stWdrMode);    
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_ISP_SetWDRMode failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 7. isp set pub attributes */
    /* note : different sensor, different ISP_PUB_ATTR_S define.
              if the sensor you used is different, you can change
              ISP_PUB_ATTR_S definition */
              
    switch(enViMode)
    {
        case OMNIVISION_OV2235_DC_1080P_20FPS:
            stPubAttr.enBayer               = BAYER_BGGR;
            stPubAttr.f32FrameRate          = 20;
            stPubAttr.stWndRect.s32X        = 0;
            stPubAttr.stWndRect.s32Y        = 0;
            stPubAttr.stWndRect.u32Width    = 1920;
            stPubAttr.stWndRect.u32Height   = 1080;
            break;
            
        default:
            stPubAttr.enBayer      = BAYER_GRBG;
            stPubAttr.f32FrameRate          = 30;
            stPubAttr.stWndRect.s32X        = 0;
            stPubAttr.stWndRect.s32Y        = 0;
            stPubAttr.stWndRect.u32Width    = 1920;
            stPubAttr.stWndRect.u32Height   = 1080;
            break;
    }

    s32Ret = HI_MPI_ISP_SetPubAttr(IspDev, &stPubAttr);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_SetPubAttr failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 8. isp init */
    s32Ret = HI_MPI_ISP_Init(IspDev);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_Init failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    gbIspInited = HI_TRUE;

    return HI_SUCCESS;
}

HI_VOID* Test_ISP_Run(HI_VOID *param)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    s32Ret = HI_MPI_ISP_Run(IspDev);
	if(s32Ret != HI_SUCCESS)
	{
        printf("%s: HI_MPI_ISP_Run failed with %#x!\n", __FUNCTION__, s32Ret);
        return HI_NULL;
	}

    return HI_NULL;
}

/******************************************************************************
* funciton : ISP Run
******************************************************************************/
HI_S32 SAMPLE_COMM_ISP_Run()
{
	//neil 20180630 no use stack malloc
    //pthread_attr_t attr;
    //pthread_attr_init(&attr);
    //pthread_attr_setstacksize(&attr, 4096 * 1024);
    //if (0 != pthread_create(&gs_IspPid, &attr, (void* (*)(void*))Test_ISP_Run, NULL))
    if (0 != pthread_create(&gs_IspPid, NULL, (void* (*)(void*))Test_ISP_Run, NULL))
    {
        printf("%s: create isp running thread failed!\n", __FUNCTION__);
        //pthread_attr_destroy(&attr);
        return HI_FAILURE;
    }
	//printf("%s tid:[%d]\n", __FUNCTION__,(int)gs_IspPid);
	//usleep(1000);
	//pthread_attr_destroy(&attr);
    return HI_SUCCESS;
}

/******************************************************************************
* funciton : stop ISP, and stop isp thread
******************************************************************************/
HI_VOID SAMPLE_COMM_ISP_Stop()
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    ALG_LIB_S stLib;

    if (!gbIspInited)
    {
        return;
    }
    gbIspInited = HI_FALSE;
    
    HI_MPI_ISP_Exit(IspDev);

    if (gs_IspPid)
    {
        pthread_join(gs_IspPid, NULL);
        gs_IspPid = 0;
    }    
    
    /* unregister hisi af lib */
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AF_LIB_NAME, 20);
    s32Ret = HI_MPI_AF_UnRegister(IspDev, &stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AF_UnRegister failed!\n", __FUNCTION__);
        return;
    }

    /* unregister hisi awb lib */
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AWB_LIB_NAME, 20);
    s32Ret = HI_MPI_AWB_UnRegister(IspDev, &stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AWB_UnRegister failed!\n", __FUNCTION__);
        return;
    }

    /* unregister hisi ae lib */
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AE_LIB_NAME, 20);
    s32Ret = HI_MPI_AE_UnRegister(IspDev, &stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AE_UnRegister failed!\n", __FUNCTION__);
        return;
    }

    /* sensor unregister callback */
    s32Ret = sensor_unregister_callback();
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: sensor_unregister_callback failed with %#x!\n", \
               __FUNCTION__, s32Ret);
        return;
    }
    return;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


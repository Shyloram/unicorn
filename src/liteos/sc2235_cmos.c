#if !defined(__SC2235_CMOS_H_)
#define __SC2235_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_af.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define SC2235_ID 2235
#define FULL_LINES_MAX  (0xFFFF)

extern const unsigned int sensor_i2c_addr;
extern unsigned int sensor_addr_byte;
extern unsigned int sensor_data_byte;

#define INCREASE_LINES (0) /* make real fps less than stand fps because NVR require*/
#define VMAX_1080P_LINEAR     (1125+INCREASE_LINES)
#define VMAX_960P_LINEAR     (1000+INCREASE_LINES)
#define VMAX_720P_LINEAR      (750+INCREASE_LINES)

#define SENSOR_1080P_20FPS_MODE  (1)
#define SENSOR_960P_30FPS_MODE  (2)

HI_U8 gu8SensorImageMode = SENSOR_1080P_20FPS_MODE;
WDR_MODE_E genSensorMode = WDR_MODE_NONE;
static HI_U32 gu32FullLinesStd = VMAX_1080P_LINEAR;
static HI_U32 gu32FullLines = VMAX_1080P_LINEAR;

static HI_BOOL bInit = HI_FALSE;
HI_BOOL bSensorInit = HI_FALSE;
ISP_SNS_REGS_INFO_S g_stSnsRegsInfo = {0};
ISP_SNS_REGS_INFO_S g_stPreSnsRegsInfo = {0};

static HI_S32 cmos_get_ae_default(AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    if (HI_NULL == pstAeSnsDft)
    {
        printf("null pointer when get ae default value!\n");
        return -1;
    }

	if(SENSOR_1080P_20FPS_MODE == gu8SensorImageMode)
	{
		pstAeSnsDft->u32LinesPer500ms = VMAX_1080P_LINEAR * 20 / 2;
	}		
	else if(SENSOR_960P_30FPS_MODE == gu8SensorImageMode)
	{
		pstAeSnsDft->u32LinesPer500ms = VMAX_960P_LINEAR * 30 / 2;
	}
	else
	{
		printf("Not support mode: %d\n", gu8SensorImageMode);
		return 0;
	}

    pstAeSnsDft->u32FullLinesStd = gu32FullLinesStd;
    pstAeSnsDft->u32FlickerFreq = 0;
	pstAeSnsDft->u32FullLinesMax = FULL_LINES_MAX;

	pstAeSnsDft->au8HistThresh[0] = 0xd;
	pstAeSnsDft->au8HistThresh[1] = 0x28;
	pstAeSnsDft->au8HistThresh[2] = 0x60;
	pstAeSnsDft->au8HistThresh[3] = 0x80;
	pstAeSnsDft->u8AeCompensation = 0x40;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->stIntTimeAccu.f32Offset = 0;
    pstAeSnsDft->u32MaxIntTime = gu32FullLinesStd - 4;	//vts-4
    pstAeSnsDft->u32MinIntTime = 1;
	pstAeSnsDft->u32MaxIntTimeTarget = 65535;//2808;
    pstAeSnsDft->u32MinIntTimeTarget = pstAeSnsDft->u32MinIntTime;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 1;
	pstAeSnsDft->u32MaxAgain = 1024*15.5;	//15.5x
	pstAeSnsDft->u32MinAgain = 1024;	//1x
	pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
	pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

	pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
	pstAeSnsDft->stDgainAccu.f32Accuracy = 1;//invalidate
	pstAeSnsDft->u32MaxDgain = 1024*4;	//4x;
	pstAeSnsDft->u32MinDgain = 1024;	//1x;
	pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
	pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

	pstAeSnsDft->u32ISPDgainShift = 8;
	pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
	pstAeSnsDft->u32MaxISPDgainTarget = 4 << pstAeSnsDft->u32ISPDgainShift;

    return 0;
}

static HI_VOID cmos_fps_set(HI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
	HI_U32 u32VblankingLines = 0xFFFF;
	if ((f32Fps <= 30) && (f32Fps >= 0.5))
	{
		if(SENSOR_1080P_20FPS_MODE == gu8SensorImageMode)
		{
			u32VblankingLines = VMAX_1080P_LINEAR * 20 / f32Fps;
		}		
		else if(SENSOR_960P_30FPS_MODE == gu8SensorImageMode)
		{
			u32VblankingLines = VMAX_960P_LINEAR * 30 / f32Fps;
		}		
		else
		{
			printf("Not support mode: %d\n", gu8SensorImageMode);
			return;
		}
	}
	else
	{
		printf("Not support Fps: %f\n", f32Fps);
		return;
	}
	
	gu32FullLinesStd = u32VblankingLines;
	gu32FullLinesStd = gu32FullLinesStd > FULL_LINES_MAX ? FULL_LINES_MAX : gu32FullLinesStd;
	g_stSnsRegsInfo.astI2cData[0].u32Data = (u32VblankingLines >> 8) & 0xff;
	g_stSnsRegsInfo.astI2cData[1].u32Data = u32VblankingLines & 0xff;
	pstAeSnsDft->f32Fps = f32Fps;
	pstAeSnsDft->u32MaxIntTime = u32VblankingLines - 2;
	pstAeSnsDft->u32LinesPer500ms = gu32FullLinesStd * f32Fps / 2;
	pstAeSnsDft->u32FullLinesStd = gu32FullLinesStd;
	gu32FullLines = gu32FullLinesStd;	
	pstAeSnsDft->u32FullLines = gu32FullLines;  

	return ;
}

static HI_VOID cmos_slow_framerate_set(HI_U32 u32FullLines, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    gu32FullLines = u32FullLines;
	g_stSnsRegsInfo.astI2cData[0].u32Data = (gu32FullLines >> 8) & 0xff;
	g_stSnsRegsInfo.astI2cData[1].u32Data = (gu32FullLines & 0xff);
    pstAeSnsDft->u32MaxIntTime = gu32FullLines - 2;
    pstAeSnsDft->u32FullLines = gu32FullLines; 
	
    return;
}

static HI_VOID cmos_inttime_update(HI_U32 u32IntTime)
{
	g_stSnsRegsInfo.astI2cData[2].u32Data = ((u32IntTime & 0xF000) >> 12);
	g_stSnsRegsInfo.astI2cData[3].u32Data = ((u32IntTime & 0x0FF0) >> 4);        
   	g_stSnsRegsInfo.astI2cData[4].u32Data = ((u32IntTime & 0x000F) << 4);            	

	g_stSnsRegsInfo.astI2cData[7].u32Data =0x84;
	g_stSnsRegsInfo.astI2cData[8].u32Data =0x04;

    unsigned int data = sensor_read_register(0x3e01);
    unsigned int gainHigh = sensor_read_register(0x3e08);
    unsigned int gainLow = sensor_read_register(0x3e09);
    unsigned int again = (gainHigh << 8) | gainLow;
    if (data > 0x0a || again > 0x0718)
    {
        sensor_write_register(0x3314, 0x02);
    }
    else if (data < 0x05)
    {
        sensor_write_register(0x3314, 0x12);
    }

    return;
}

#define SENSOR_BLC_TOP_VALUE (0x10c0)
#define SENSOR_BLC_BOT_VALUE (0x1060)
#define SENSOR_AGAIN_STEP (0x400)
#define SENSOR_MAX_AGAIN (0x3C66)
#define SENSOR_MIN_AGAIN (0xC00)

int gSaturation_flag = 0;
ISP_SATURATION_ATTR_S gpSaturation;
static HI_VOID sc2235_saturation_Get(void)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
	memset(&gpSaturation, 0, sizeof(ISP_SATURATION_ATTR_S));
	
    s32Ret = HI_MPI_ISP_GetSaturationAttr(IspDev, &gpSaturation);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_GetSaturationAttr fail,Error(%#x)\n",s32Ret);
    }

	return ;
}

static HI_VOID sc2235_saturation_Restore(void)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
	
	s32Ret = HI_MPI_ISP_SetSaturationAttr(IspDev, &gpSaturation);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetSaturationAttr fail,Error(%#x)\n",s32Ret);
    }

	return ;
}

float gGain_Saturation_Table[10][2] = 
{
	{1.0,155.0},{2.0,148.0},{4.0,144.0},{8.0,110.0},{11.0,97.0},
	{14.0,84.0},{17.0,75.0},{20.0,72.0},{23.0,69.0},{25.0,65.0}
};

static float sc2235_saturation_Linear(float pre_gain, float next_gain, float pre_val, float next_val, float curr_gain)
{
	float effective = pre_val - ((pre_val - next_val) / ((next_gain - pre_gain) * 10)) * ((curr_gain - pre_gain) * 10);
    return effective;
}


static HI_VOID sc2235_saturation_Limit(float gain)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
	ISP_SATURATION_ATTR_S pSaturation;
	memcpy(&pSaturation, &gpSaturation, sizeof(ISP_SATURATION_ATTR_S));

	pSaturation.enOpType = OP_TYPE_MANUAL;

	if(gain <= gGain_Saturation_Table[0][0])
	{
		pSaturation.stManual.u8Saturation = gGain_Saturation_Table[0][1];
	}
	else if(gain <= gGain_Saturation_Table[1][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[0][0],gGain_Saturation_Table[1][0],
										gGain_Saturation_Table[0][1],gGain_Saturation_Table[1][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[2][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[1][0],gGain_Saturation_Table[2][0],
										gGain_Saturation_Table[1][1],gGain_Saturation_Table[2][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[3][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[2][0],gGain_Saturation_Table[3][0],
										gGain_Saturation_Table[2][1],gGain_Saturation_Table[3][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[4][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[3][0],gGain_Saturation_Table[4][0],
										gGain_Saturation_Table[3][1],gGain_Saturation_Table[4][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[5][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[4][0],gGain_Saturation_Table[5][0],
										gGain_Saturation_Table[4][1],gGain_Saturation_Table[5][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[6][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[5][0],gGain_Saturation_Table[6][0],
										gGain_Saturation_Table[5][1],gGain_Saturation_Table[6][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[7][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[6][0],gGain_Saturation_Table[7][0],
										gGain_Saturation_Table[6][1],gGain_Saturation_Table[7][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[8][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[7][0],gGain_Saturation_Table[8][0],
										gGain_Saturation_Table[7][1],gGain_Saturation_Table[8][1],gain);
	}
	else if(gain <= gGain_Saturation_Table[9][0])
	{
		pSaturation.stManual.u8Saturation = 
			sc2235_saturation_Linear(gGain_Saturation_Table[8][0],gGain_Saturation_Table[9][0],
										gGain_Saturation_Table[8][1],gGain_Saturation_Table[9][1],gain);
	}
	
	s32Ret = HI_MPI_ISP_SetSaturationAttr(IspDev, &pSaturation);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_SetSaturationAttr fail,Error(%#x)\n",s32Ret);
    }

	return ;
}

float sc2235_GainValue_Get(void)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo,  0, sizeof(ISP_EXP_INFO_S));
	s32Ret = HI_MPI_ISP_QueryExposureInfo(IspDev, &stExpInfo);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_ISP_QueryExposureInfo fail,Error(%#x)\n",s32Ret);
    }
	
	return (stExpInfo.u32AGain / 1024.0) * (stExpInfo.u32DGain / 1024.0) * (stExpInfo.u32ISPDGain / 1024.0);
}

static HI_VOID sc2235_AgainMax_limit(HI_VOID)
{
	HI_U16 BlcLimitHi = 0;
	HI_U16 BlcLimitLow = 0;
	HI_U16 u16BlcLimit = 0;
	BlcLimitHi = sensor_read_register(0x3911);
	BlcLimitLow = sensor_read_register(0x3912);
	u16BlcLimit = (BlcLimitHi << 8) | BlcLimitLow;

    ISP_EXPOSURE_ATTR_S pExpAttr;
	memset(&pExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
    int ret = HI_MPI_ISP_GetExposureAttr(0, &pExpAttr);
    if (HI_SUCCESS != ret)
    {
        printf("HI_MPI_ISP_GetExposureAttr fail,Error(%#x)\n",ret);
    }
	
	HI_U32 AgainLimit = 0x0;
	if(u16BlcLimit > SENSOR_BLC_TOP_VALUE)
	{
		AgainLimit = pExpAttr.stAuto.stAGainRange.u32Max - SENSOR_AGAIN_STEP;
	}
	else if(u16BlcLimit < SENSOR_BLC_BOT_VALUE)
	{
		AgainLimit = pExpAttr.stAuto.stAGainRange.u32Max + SENSOR_AGAIN_STEP;
	}
	else
	{
		AgainLimit = pExpAttr.stAuto.stAGainRange.u32Max;
	}

    HI_U32 u32CurrAgainVal = 0x0;
	if(AgainLimit < SENSOR_MIN_AGAIN)
	{
		u32CurrAgainVal = SENSOR_MIN_AGAIN;
	}
	else if(AgainLimit > SENSOR_MAX_AGAIN) 
	{
 		u32CurrAgainVal = SENSOR_MAX_AGAIN;
	}
	else
	{
		u32CurrAgainVal = AgainLimit;
	}

	if(pExpAttr.stAuto.stAGainRange.u32Max != u32CurrAgainVal)
	{
		pExpAttr.stAuto.stAGainRange.u32Max = u32CurrAgainVal;
		ret = HI_MPI_ISP_SetExposureAttr(0, &pExpAttr);
		if (HI_SUCCESS != ret)
		{
		    printf("HI_MPI_ISP_SetExposureAttr fail,Error(%#x)\n",ret);
		}
	}

	float gain = sc2235_GainValue_Get();
	if((u32CurrAgainVal < SENSOR_MAX_AGAIN) && (gain > 20.0))
	{
		sensor_write_register(0x5781,0x01);
		sensor_write_register(0x5785,0x08);
	}
	else if(gain < 10.0)
	{
		sensor_write_register(0x5781,0x04);
		sensor_write_register(0x5785,0x18);
	}
	else if(gain >= 10.0)
	{
		sensor_write_register(0x5781,0x02);
		sensor_write_register(0x5785,0x08);
	}

	if(access("/tmp/night_mode.flag", 0))
	{
		if((u32CurrAgainVal < SENSOR_MAX_AGAIN) && (gain > 7.0))
		{
			if(gSaturation_flag == 0)
			{	
				gSaturation_flag = 1;
				sc2235_saturation_Get();
			}
			sc2235_saturation_Limit(gain);
		}
		else if(gSaturation_flag == 1)
		{
			gSaturation_flag = 0;
			sc2235_saturation_Restore();
		}
	}
	
	return ;
}


static const HI_U16 gau16AgainTab[64]={
	1024,1088,1152,1216,1280,1344,1408,1472,1536,1600,1664,1728,1792,1856,
	1920,1984,2048,2176,2304,2432,2560,2688,2816,2944,3072,3200,3328,3456,
	3584,3712,3840,3968,4096,4352,4608,4864,5120,5376,5632,5888,6144,6400,
	6656,6912,7168,7424,7680,7936,8192,8704,9216,9728,10240,10752,11264,
	11776,12288,12800,13312,13824,14336,14848,15360,15872
};

static HI_VOID cmos_again_calc_table(HI_U32 *pu32AgainLin, HI_U32 *pu32AgainDb)
{
	int again;
	int i;
	static HI_U8 againmax = 63;
	again = *pu32AgainLin;

	if(again >= gau16AgainTab[againmax])
	{
		*pu32AgainDb = againmax;
	}
	else
	{
		for(i=1;i<64;i++)
		{
			if(again<gau16AgainTab[i])
			{
				*pu32AgainDb = i-1;
				break;
			}
		}
	}
	
	*pu32AgainLin = gau16AgainTab[*pu32AgainDb];

    return;
}

static HI_VOID cmos_dgain_calc_table(HI_U32 *pu32DgainLin, HI_U32 *pu32DgainDb)
{
	*pu32DgainDb = *pu32DgainLin/1024;
	if(*pu32DgainDb == 3)
	{
		*pu32DgainDb = 2;
	}
	else if(*pu32DgainDb >= 4 && *pu32DgainDb < 8)
	{
		*pu32DgainDb = 4;
	}
	else if(*pu32DgainDb >= 8)
	{
		*pu32DgainDb = 8;
	}
	
	*pu32DgainLin = *pu32DgainDb*1024;
    return;
}

HI_U32 g_frame_cnt = 0;
static HI_VOID cmos_gains_update(HI_U32 u32Again, HI_U32 u32Dgain)
{
#if 1

	g_frame_cnt++;

	if(!(g_frame_cnt % 10))
	{
		sc2235_AgainMax_limit();
	}

	HI_U8 u32Reg0x3e09,u32Reg0x3e08;
	HI_U8 i;
	int gain_all = gau16AgainTab[u32Again] * u32Dgain;

//	printf("%8d, %6d, %6d, %2d\n", gain_all, gau16AgainTab[u32Again], u32Dgain, u32Again);
	
	// Again
	u32Reg0x3e09 = 0x10 | (u32Again&0x0f);
	u32Reg0x3e08 = 0x03;
	u32Again = u32Again/16;
	for(i=0;i <  u32Again;i++)
	{
		u32Reg0x3e08 = (u32Reg0x3e08<<1)|0x01;
	}

	// Dgain
	if(u32Dgain == 2)
	{
		u32Reg0x3e08 |= 0x20; 
	}
	else if(u32Dgain == 4)
	{
		u32Reg0x3e08 |= 0x60; 
	}
	else if(u32Dgain == 8)
	{
		u32Reg0x3e08 |= 0xe0; 
	}

	g_stSnsRegsInfo.astI2cData[5].u32Data = u32Reg0x3e08;
	g_stSnsRegsInfo.astI2cData[6].u32Data = u32Reg0x3e09;

	g_stSnsRegsInfo.astI2cData[7].u32Data = 0x84;
	g_stSnsRegsInfo.astI2cData[8].u32Data = 0x04;

	g_stSnsRegsInfo.astI2cData[9].u32Data = 0x00;
	if(gain_all < 2*1024){
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0x04;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xd0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x84;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x2f;
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x2f;
	}
	else if(gain_all < 4*1024){
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0x0e;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x88;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x2f;
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x42;
	}
	else if(gain_all < 8*1024){
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0x11;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x88;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x2f;
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x42;
	}
	else if(gain_all < 15.5*1024){
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0x14;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x88;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x2f;		
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x43;
	}
	else{
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x88;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x3c; //20170818		
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x43;
	}
	g_stSnsRegsInfo.astI2cData[15].u32Data = 0x30;
#else

	g_frame_cnt++;
	if(!(g_frame_cnt % 10))
	{
	//	sc2235_AgainMax_limit();
	}

	g_stSnsRegsInfo.astI2cData[5].u32Data = (u32Again & 0xF00) >> 8;
	g_stSnsRegsInfo.astI2cData[6].u32Data = u32Again & 0xFF;

	g_stSnsRegsInfo.astI2cData[7].u32Data = 0x84;
	g_stSnsRegsInfo.astI2cData[8].u32Data = 0x04;

	g_stSnsRegsInfo.astI2cData[9].u32Data = 0x00;
	if(u32Again < (2 * 16)){
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0x04;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xd0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x84;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x2f;
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x2f;
	}
	else if(u32Again < (8 * 16)){
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0x0c;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x88;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x2f;
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x22;
	}
	else if(u32Again <= (15 * 16)){
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0x14;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x88;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x2f;		
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x42;
	}
	else{
		g_stSnsRegsInfo.astI2cData[10].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[11].u32Data = 0xc0;
		g_stSnsRegsInfo.astI2cData[12].u32Data = 0x88;
		g_stSnsRegsInfo.astI2cData[13].u32Data = 0x3c; //20170818		
		g_stSnsRegsInfo.astI2cData[14].u32Data = 0x42;
	}
	g_stSnsRegsInfo.astI2cData[15].u32Data = 0x30;
#endif

    return; 
}

HI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set= cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table  = cmos_dgain_calc_table;

    return 0;
}

/* AWB default parameter and function */
static AWB_CCM_S g_stAwbCcm =
{  
	6500,
	{
		0x147,0x8053,0xc, 
		0x804B,0x184,0x8039, 
		0x8002,0x80CF,0x1D1, 
	},
	4100,
	{
		0x243,0x8149,0x6, 
		0x806E,0x191,0x8023, 
		0x7,0x8145,0x23E,
	},
	2856,
	{
		0x18A,0x8079,0x8011, 
		0x8074,0x175,0x8001, 
		0x8002,0x81D8,0x2DA
	}
};

static AWB_AGC_TABLE_S g_stAwbAgcTable =
{
    /* bvalid */
    1,
	
    /*1,  2,  4,  8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768*/
    /* saturation */   
	{0x88,0x88,0x88,0x76,0x6c,0x6c,0x3C,0x3C,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00}

};
static HI_S32 cmos_get_awb_default(AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    if (HI_NULL == pstAwbSnsDft)
    {
        printf("null pointer when get awb default value!\n");
        return -1;
    }

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));
	pstAwbSnsDft->u16WbRefTemp = 5082;
	pstAwbSnsDft->au16GainOffset[0] = 0x16A;	
	pstAwbSnsDft->au16GainOffset[1] = 0x100;	
	pstAwbSnsDft->au16GainOffset[2] = 0x100;	
	pstAwbSnsDft->au16GainOffset[3] = 0x19A;	
	pstAwbSnsDft->as32WbPara[0] = 207;	  
	pstAwbSnsDft->as32WbPara[1] = -138;    
	pstAwbSnsDft->as32WbPara[2] = -187;    
	pstAwbSnsDft->as32WbPara[3] = 200586;	 
	pstAwbSnsDft->as32WbPara[4] = 128;	  
	pstAwbSnsDft->as32WbPara[5] = -141634;

    memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm, sizeof(AWB_CCM_S));
    memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));

    return 0;
}

HI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));
    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;
    return 0;
}

/*此处与18EV100不同，不同的增益下的降噪*/
#define DMNR_CALIB_CARVE_NUM_SC2235 8

float g_coef_calib_SC2235[DMNR_CALIB_CARVE_NUM_SC2235][4] = 
{
    {100.000000f, 2.000000f, 0.039561f, 6.095408f, }, 
    {200.000000f, 2.301030f, 0.040519f, 6.380517f, }, 
    {400.000000f, 2.602060f, 0.042396f, 6.888251f, }, 
    {800.000000f, 2.903090f, 0.046129f, 7.650892f, }, 
    {1600.000000f, 3.204120f, 0.051765f, 9.290079f, }, 
    {3200.000000f, 3.505150f, 0.058894f, 12.351269f, }, 
    {6200.000000f, 3.792392f, 0.074431f, 18.024876f, }, 
    {6200.000000f, 3.792392f, 0.095554f, 27.922104f, }, 
};

static ISP_NR_ISO_PARA_TABLE_S g_stNrIsoParaTab[HI_ISP_NR_ISO_LEVEL_MAX] = 
{
     //u16Threshold//u8varStrength//u8fixStrength//u8LowFreqSlope	
       {0x5dc,       0x72,         0x8c,         0x2 },  //100    //                      //                                                
       {0x5dc,       0x72,         0x8c,         0x2 },  //200    // ISO                  // ISO //u8LowFreqSlope
       {0x5dc,       0xE4,         0x8C,       	 0x2 },  //400    //{400,  1200, 96,256}, //{400 , 0  }
       {0x5dc,       0xE4,         0x8C,         0x2 },  //800    //{800,  1400, 80,256}, //{600 , 2  }
       {0x5dc,       0xE4,         0xA9,         0x3 },  //1600   //{1600, 1200, 72,256}, //{800 , 8  }
       {0x5dc,       0xE4,         0xA9,         0x3 },  //3200   //{3200, 1200, 64,256}, //{1000, 12 }
       {0x5dc,       0xE4,         0xA9,         0x3 },  //6400   //{6400, 1100, 56,256}, //{1600, 6  }
       {0x5dc,       0xE4,         0xA9,         0x3 },  //12800  //{12000,1100, 48,256}, //{2400, 0  }
       {0x5dc,       0xE4,         0xA9,         0x3 },  //25600  //{36000,1100, 48,256}, //
       {0x5dc,       0xE7,         0xA9,         0x3 },  //51200  //{64000,1100, 96,256}, //
       {0,       	 0,            0,            0   },  //102400 //{82000,1000,240,256}, //
       {0,       	 0,            0,            0 },  //204800 //                           //
       {0,       	 0,            0,            0 },  //409600 //                           //
       {0,       	 0,            0,            0 },  //819200 //                           //
       {0,       	 0,            0,            0 },  //1638400//                           //
       {0,       	 0,            0,            0 },  //3276800//                           //
};

static ISP_CMOS_DEMOSAIC_S g_stIspDemosaic =
{
	/*For Demosaic*/
	1, /*bEnable*/			
	24,/*u16VhLimit*/	
	40-24,/*u16VhOffset*/
	24,   /*u16VhSlope*/
	/*False Color*/
	1,    /*bFcrEnable*/
	{ 8, 8, 8, 8, 8, 8, 8, 8, 3, 0, 0, 0, 0, 0, 0, 0},    /*au8FcrStrength[ISP_AUTO_ISO_STENGTH_NUM]*/
	{24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24},    /*au8FcrThreshold[ISP_AUTO_ISO_STENGTH_NUM]*/
	/*For Ahd*/
	400, /*u16UuSlope*/	
	{512,512,512,512,512,512,512,400,0,0,0,0,0,0,0,0}    /*au16NpOffset[ISP_AUTO_ISO_STENGTH_NUM]*/
};

static ISP_CMOS_GE_S g_stIspGe =
{
	/*For GE*/
	1,    /*bEnable*/			
	7,    /*u8Slope*/	
	7,    /*u8Sensitivity*/
	8192, /*u16Threshold*/
	8192, /*u16SensiThreshold*/	
	{1024,1024,1024,2048,2048,2048,2048,  2048,  2048,2048,2048,2048,2048,2048,2048,2048}    /*au16Strength[ISP_AUTO_ISO_STENGTH_NUM]*/	
};

static ISP_CMOS_RGBSHARPEN_S g_stIspRgbSharpen =
{      
  //{100,200,400,800,1600,3200,6400,12800,25600,51200,102400,204800,409600,819200,1638400,3276800};
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},/* enPixSel = ~bEnLowLumaShoot */
    
	 {0x78,0x78,0x71,0x71,0x5E,0x5E,0x4D,0x4B,0x4B,0x38,0x00,0x00,0x00,0x00,0x00,0x00},/*maxSharpAmt1 = SharpenUD*16 */
	 {0x92,0x92,0x89,0x89,0x7B,0x7B,0x7B,0x6D,0x6D,0x58,0x00,0x00,0x00,0x00,0x00,0x00},/*maxEdgeAmt = SharpenD*16 */
	
	 {0x2,0x2,0x2,0x4,0x4,0x4,0x4,0x6,0x6,0x8,0x00,0x00,0x00,0x00,0x00,0x00},/*sharpThd2 = TextureNoiseThd*4 */
   	 {0x2,0x2,0x2,0x4,0x4,0x4,0x4,0x6,0x6,0x8,0x00,0x00,0x00,0x00,0x00,0x00},/*edgeThd2 = EdgeNoiseThd*4 */
    
	 {0x5C,0x57,0x57,0x57,0x55,0x55,0x45,0x3F,0x3F,0x20,0x00,0x00,0x00,0x00,0x00,0x00}, /*overshootAmt*/
     {0x97,0x87,0x87,0x87,0x71,0x71,0x5E,0x52,0x52,0x3C,0x00,0x00,0x00,0x00,0x00,0x00},/*undershootAmt*/
};

static ISP_CMOS_UVNR_S g_stIspUVNR = 
{
	/*中值滤波切换到UVNR的ISO阈值*/
	/*UVNR切换到中值滤波的ISO阈值*/
    /*0.0   -> disable，(0.0, 1.0]  -> weak，(1.0, 2.0]  -> normal，(2.0, 10.0) -> strong*/
	/*高斯滤波器的标准差*/
  //{100,	200,	400,	800,	1600,	3200,	6400,	12800,	25600,	51200,	102400,	204800,	409600,	819200,	1638400,	3276800};
	{1,	    2,       4,      5,      7,      48,     32,     16,     16,     16,      16,     16,     16,     16,     16,        16},      /*UVNRThreshold*/
 	{0,		0,		0,		0,		0,		0,		0,		0,		0,		1,			1,		2,		2,		2,		2,		2},  /*Coring_lutLimit*/
	{0,		0,		0,		16,		34,		34,		34,		34,		34,		34,		34,		34,		34,		34,		34,			34}  /*UVNR_blendRatio*/
};

static ISP_CMOS_DPC_S g_stCmosDpc = 
{
	//1,/*IR_channel*/
	//1,/*IR_position*/
	{0,0,0,1,1,1,2,2,2,3,3,3,3,3,3,3},/*au16Strength[16]*/
	{0,0,0,0,0,0,0,0,0x24,0x80,0x80,0x80,0xE5,0xE5,0xE5,0xE5},/*au16BlendRatio[16]*/
};

static ISP_CMOS_DRC_S g_stIspDrc =
{
    0,
    10,
    0,
    2,
    192,
    60,
    0,
    0,
    0,
    {1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024}
};

static HI_U32 cmos_get_isp_default(ISP_CMOS_DEFAULT_S *pstDef)
{
    if (HI_NULL == pstDef)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));
	memcpy(&pstDef->stDrc, &g_stIspDrc, sizeof(ISP_CMOS_DRC_S));
    memcpy(&pstDef->stDemosaic, &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));
    memcpy(&pstDef->stRgbSharpen, &g_stIspRgbSharpen, sizeof(ISP_CMOS_RGBSHARPEN_S));
    memcpy(&pstDef->stGe, &g_stIspGe, sizeof(ISP_CMOS_GE_S));			
    pstDef->stNoiseTbl.stNrCaliPara.u8CalicoefRow = DMNR_CALIB_CARVE_NUM_SC2235;
    pstDef->stNoiseTbl.stNrCaliPara.pCalibcoef = (HI_FLOAT (*)[4])g_coef_calib_SC2235;
	memcpy(&pstDef->stNoiseTbl.stIsoParaTable[0], &g_stNrIsoParaTab[0],sizeof(ISP_NR_ISO_PARA_TABLE_S)*HI_ISP_NR_ISO_LEVEL_MAX);
	memcpy(&pstDef->stUvnr, &g_stIspUVNR, sizeof(ISP_CMOS_UVNR_S));
    memcpy(&pstDef->stDpc, &g_stCmosDpc, sizeof(ISP_CMOS_DPC_S));

    pstDef->stSensorMaxResolution.u32MaxWidth  = 1920;
    pstDef->stSensorMaxResolution.u32MaxHeight = 1080;

    return 0;
}

HI_U32 cmos_get_isp_black_level(ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_FALSE;
    pstBlackLevel->au16BlackLevel[0] = 68;
    pstBlackLevel->au16BlackLevel[1] = 68;
    pstBlackLevel->au16BlackLevel[2] = 68;
    pstBlackLevel->au16BlackLevel[3] = 68;

    return 0;
}

static HI_VOID cmos_set_pixel_detect(HI_BOOL bEnable)
{
    if (bEnable) /* setup for ISP pixel calibration mode */
    {

    }
    else /* setup for ISP 'normal mode' */
    {
        bInit = HI_FALSE;
    }

    return;
}

HI_VOID cmos_set_wdr_mode(HI_U8 u8Mode)
{
    bInit = HI_FALSE;

    switch(u8Mode)
    {
        case WDR_MODE_NONE:
            if (SENSOR_1080P_20FPS_MODE == gu8SensorImageMode)
            {
                gu32FullLinesStd = VMAX_1080P_LINEAR;
            }
            else if (SENSOR_960P_30FPS_MODE == gu8SensorImageMode)
            {
                gu32FullLinesStd = VMAX_960P_LINEAR;
            }			
            else
            {
                printf("NOT support this mode!\n");
                return;
            }
            genSensorMode = WDR_MODE_NONE;
            //printf("linear mode\n");
        break;
        default:
            printf("NOT support this mode!\n");
            return;
        break;
    }
    return;
}

static HI_U32 cmos_get_sns_regs_info(ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    if (HI_NULL == pstSnsRegsInfo)
    {
        printf("null pointer when get sns reg info!\n");
        return -1;
    }

    HI_S32 i;
    if (HI_FALSE == bInit)
    {
        g_stSnsRegsInfo.enSnsType = ISP_SNS_I2C_TYPE;
        g_stSnsRegsInfo.u8Cfg2ValidDelayMax = 2;		
        g_stSnsRegsInfo.u32RegNum = 16;
	
        for (i = 0; i < g_stSnsRegsInfo.u32RegNum; i++)
        {	
            g_stSnsRegsInfo.astI2cData[i].bUpdate = HI_TRUE;
            g_stSnsRegsInfo.astI2cData[i].u8DevAddr = sensor_i2c_addr;
            g_stSnsRegsInfo.astI2cData[i].u32AddrByteNum = sensor_addr_byte;
            g_stSnsRegsInfo.astI2cData[i].u32DataByteNum = sensor_data_byte;
        }

        g_stSnsRegsInfo.astI2cData[0].u8DelayFrmNum = HI_FALSE;         //Fps
        g_stSnsRegsInfo.astI2cData[0].u32RegAddr = 0x320e;
        g_stSnsRegsInfo.astI2cData[1].u8DelayFrmNum = HI_FALSE;
        g_stSnsRegsInfo.astI2cData[1].u32RegAddr = 0x320f;
		
        g_stSnsRegsInfo.astI2cData[2].u8DelayFrmNum = HI_FALSE;
        g_stSnsRegsInfo.astI2cData[2].u32RegAddr = 0x3e00;        //Exp
        g_stSnsRegsInfo.astI2cData[3].u8DelayFrmNum = HI_FALSE;       
        g_stSnsRegsInfo.astI2cData[3].u32RegAddr = 0x3e01;
	 	g_stSnsRegsInfo.astI2cData[4].u8DelayFrmNum = HI_FALSE;       
        g_stSnsRegsInfo.astI2cData[4].u32RegAddr = 0x3e02;	
		
        g_stSnsRegsInfo.astI2cData[5].u8DelayFrmNum = HI_FALSE;         //Gain
        g_stSnsRegsInfo.astI2cData[5].u32RegAddr = 0x3e08;
        g_stSnsRegsInfo.astI2cData[6].u8DelayFrmNum = HI_FALSE;         
        g_stSnsRegsInfo.astI2cData[6].u32RegAddr = 0x3e09;

        g_stSnsRegsInfo.astI2cData[7].u8DelayFrmNum = 1;        //aec/agc timing
        g_stSnsRegsInfo.astI2cData[7].u32RegAddr = 0x3903;
        g_stSnsRegsInfo.astI2cData[8].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[8].u32RegAddr = 0x3903;
        g_stSnsRegsInfo.astI2cData[9].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[9].u32RegAddr = 0x3812;
        g_stSnsRegsInfo.astI2cData[10].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[10].u32RegAddr = 0x3301;
        g_stSnsRegsInfo.astI2cData[11].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[11].u32RegAddr = 0x330b;
        g_stSnsRegsInfo.astI2cData[12].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[12].u32RegAddr = 0x3631;
        g_stSnsRegsInfo.astI2cData[13].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[13].u32RegAddr = 0x366f;
        g_stSnsRegsInfo.astI2cData[14].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[14].u32RegAddr = 0x3633;
		g_stSnsRegsInfo.astI2cData[15].u8DelayFrmNum = 1;
        g_stSnsRegsInfo.astI2cData[15].u32RegAddr = 0x3812;
		
        bInit = HI_TRUE;
    }
    else    
    {        
        for (i = 0; i < g_stSnsRegsInfo.u32RegNum; i++)        
        {            
            if (g_stSnsRegsInfo.astI2cData[i].u32Data == g_stPreSnsRegsInfo.astI2cData[i].u32Data)            
            {                
                g_stSnsRegsInfo.astI2cData[i].bUpdate = HI_FALSE;
            }            
            else            
            {                
                g_stSnsRegsInfo.astI2cData[i].bUpdate = HI_TRUE;
            }        
        }    
		if( HI_TRUE == (g_stSnsRegsInfo.astI2cData[2].bUpdate || g_stSnsRegsInfo.astI2cData[3].bUpdate || g_stSnsRegsInfo.astI2cData[4].bUpdate))
		{
			g_stSnsRegsInfo.astI2cData[7].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[8].bUpdate = HI_TRUE;
		}
		if( HI_TRUE == (g_stSnsRegsInfo.astI2cData[5].bUpdate ||  g_stSnsRegsInfo.astI2cData[6].bUpdate))
		{
			g_stSnsRegsInfo.astI2cData[7].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[8].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[9].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[10].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[11].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[12].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[13].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[14].bUpdate = HI_TRUE;
			g_stSnsRegsInfo.astI2cData[15].bUpdate = HI_TRUE;
		}
    }

    memcpy(pstSnsRegsInfo, &g_stSnsRegsInfo, sizeof(ISP_SNS_REGS_INFO_S)); 
    memcpy(&g_stPreSnsRegsInfo, &g_stSnsRegsInfo, sizeof(ISP_SNS_REGS_INFO_S)); 

    return 0;
}

static HI_S32 cmos_set_image_mode(ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    HI_U8 u8SensorImageMode = gu8SensorImageMode;

    bInit = HI_FALSE;

    if (HI_NULL == pstSensorImageMode )
    {
        printf("null pointer when set image mode\n");
        return -1;
    }

    if ((pstSensorImageMode->u16Width <= 1280) && (pstSensorImageMode->u16Height <= 960))
    {
		if (WDR_MODE_NONE == genSensorMode)
		{
			if(pstSensorImageMode->f32Fps <= 30)
			{
				u8SensorImageMode = SENSOR_960P_30FPS_MODE;
				if (HI_FALSE == bSensorInit)
				{
					gu8SensorImageMode = u8SensorImageMode;
					return 0;
				}
				if (u8SensorImageMode == gu8SensorImageMode)
				{
					return -1;
				}
				gu8SensorImageMode = u8SensorImageMode;
				return 0;
			}
		}
    }
    else if ((pstSensorImageMode->u16Width <= 1920) && (pstSensorImageMode->u16Height <= 1080))
	{
        if (WDR_MODE_NONE == genSensorMode)
        {
            if(pstSensorImageMode->f32Fps <= 20)
            {
                u8SensorImageMode = SENSOR_1080P_20FPS_MODE;
				/* Sensor first init */
				if (HI_FALSE == bSensorInit)
				{
					gu8SensorImageMode = u8SensorImageMode;
					return 0;
				}
				if (u8SensorImageMode == gu8SensorImageMode)
				{
					return -1;
				}
				gu8SensorImageMode = u8SensorImageMode;
				return 0;
            }
        }
	}

    printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
        pstSensorImageMode->u16Width,
        pstSensorImageMode->u16Height,
        pstSensorImageMode->f32Fps,
        genSensorMode);

    return -1;
}

HI_VOID sensor_global_init()
{
    gu8SensorImageMode = SENSOR_1080P_20FPS_MODE;
    genSensorMode = WDR_MODE_NONE;
    gu32FullLinesStd = VMAX_1080P_LINEAR;
    gu32FullLines = VMAX_1080P_LINEAR;
    bInit = HI_FALSE;
    bSensorInit = HI_FALSE;

    memset(&g_stSnsRegsInfo, 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&g_stPreSnsRegsInfo, 0, sizeof(ISP_SNS_REGS_INFO_S));
}

HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init = sensor_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit = sensor_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    return 0;
}

int sensor_register_callback(void)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    ALG_LIB_S stLib;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;

    cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret = HI_MPI_ISP_SensorRegCallBack(IspDev, SC2235_ID, &stIspRegister);
    if (s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret = HI_MPI_AE_SensorRegCallBack(IspDev, &stLib, SC2235_ID, &stAeRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret = HI_MPI_AWB_SensorRegCallBack(IspDev, &stLib, SC2235_ID, &stAwbRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    return 0;
}

int sensor_unregister_callback(void)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    ALG_LIB_S stLib;

    s32Ret = HI_MPI_ISP_SensorUnRegCallBack(IspDev, SC2235_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    s32Ret = HI_MPI_AE_SensorUnRegCallBack(IspDev, &stLib, SC2235_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    s32Ret = HI_MPI_AWB_SensorUnRegCallBack(IspDev, &stLib, SC2235_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    return 0;
}

#define PATHLEN_MAX 256
#define CMOS_CFG_INI "sc2235_cfg.ini"
static char pcName[PATHLEN_MAX] = "configs/sc2235_cfg.ini";
int  sensor_set_inifile_path(const char *pcPath)
{
    memset(pcName, 0, sizeof(pcName));

    if (HI_NULL == pcPath)
    {
        strncat(pcName, "configs/", strlen("configs/"));
        strncat(pcName, CMOS_CFG_INI, sizeof(CMOS_CFG_INI));
    }
    else
    {
		if(strlen(pcPath) > (PATHLEN_MAX - 30))
		{
		      printf("Set inifile path is larger PATHLEN_MAX!\n");
		      return -1;
		}
        strncat(pcName, pcPath, strlen(pcPath));
        strncat(pcName, CMOS_CFG_INI, sizeof(CMOS_CFG_INI));
    }

    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __SC2235_CMOS_H_ */

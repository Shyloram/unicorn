/**************************************/
/*  read-write config param interface */
/**************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "driver_hisi_lib_api.h"
#include "rwconfig.h"


#ifdef USING_SPI_FLASH

#ifdef __cplusplus
	extern "C" {
#endif
	extern int hispinor_erase(unsigned long start, unsigned long size);
	extern int hispinor_write(void* memaddr, unsigned long start, unsigned long size);
	extern int hispinor_read(void* memaddr, unsigned long start, unsigned long size);

	#define Zmodo_erase hispinor_erase
	#define Zmodo_write hispinor_write
	#define Zmodo_read  hispinor_read
#ifdef __cplusplus
	}
#endif

#else
	
#endif


RWConfig* RWConfig::m_instance = new RWConfig;

RWConfig* RWConfig::GetInstance()
{
	return m_instance;
}



pthread_mutex_t RWConfig::mutex;
SPENODE RWConfig::addrTable[CONFIG_METHOD_MAX] = {0};

#ifndef PAGE_CORRECT_IN_USE
//init default arg array
SYSTEM_PARAMETER defaultARR = 
{
	{0},//NETWORK_PARA			m_NetWork;			//网络设置
	{0},//MACHINE_PARA			m_Machine;			//机器设置
	{
		{  
			{
				1,//unsigned char 			m_uChannelValid; 	// 主码流视频通道开关1:on  0:off
				1,//unsigned char				m_uAudioSwitch;	// 主码流音频通道开关1:on  0:off
				1,//unsigned char				m_uSubEncSwitch; // 子码流视频开关1:on 0:off
				1,//unsigned char				m_uSubAudioEncSwitch; // 子码流音频开关1:on 0:off
				"stream3",//char 					m_Title[17];	 	 // 通道标题
				12,//unsigned char 			m_uFrameRate;	 // 帧率	PAL(1-25) NTSC(1-30)
				4,//unsigned char 			m_uResolution;	 // 清晰度  0:D1, 1: HD1, 2:CIF 3:QCIF 4:1080P 5:720P 6:VGA 7:QVGA 8:960H
				0,//unsigned char 			m_uQuality;		 // 画质quality	values:0--4 (最好)(很好)(好)(一般)(差)
				0,//unsigned char 			m_uEncType;	// 1:CBR 0:VBR
				4,//unsigned char			m_uSubRes;  	 // 子码流清晰度0:D1, 1: HD1, 2:CIF 3:QCIF 4:1080P 5:720P 6:VGA 7:QVGA
				4,//unsigned char			m_uSubQuality; 	 // 子码流画质 values:0--4 (最好)(很好)(好)(一般)(差)	
				12,//unsigned char			m_uSubFrameRate; // 子码流帧率 PAL(1-25) NTSC(1-30)
				0,//unsigned char 		m_uSubEncType;// 1:CBR 0:VBR
				5,//unsigned char			m_u16RecDelay;	 // 录像延时
				1,//unsigned char			m_u8PreRecTime;  // 预录时间
				1,//unsigned char			m_TltleSwitch;	 // 通道标题开关0:off 1:on
				1,//unsigned char			m_TimeSwitch;	 // 时间标题开关0:off 1:on
				0,//unsigned char			m_u8Reserved[3];
			},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
		},	
		0,
	},//CAMERA_PARA				m_Camera;			//编码设置
	{0},//CAMERA_ANALOG			m_Analog;			//模拟量设置
	{
		1,//int	m_colorMode;			// 1:彩色模式2:黑白模式
		1,//int	m_picMode;				// 1:普通2:镜像+翻转
		0,//int	m_picFlip;				
		0,//int	m_picMirrorn;
		3,//int	m_PowerFreq;			// 0:室外抗闪1:50HZ 2:60HZ 3:自动抗闪
	},//CAMERASENSOR_PARA		m_Sensor;			//sensor工作状态
	{0},//COMMON_PARA				m_CommPara;		//基本设置
	{0},//CAMERA_BLIND			m_CamerBlind;		//视频遮挡检测
	{0},//AlarmOutPort			m_AlarmPort;		//报警输出口配置
	{0},//CAMERA_VideoLoss		m_CamerVideoLoss;	//视频丢失
	{
		{
			{0},//ALARMINDATESET		m_TimeTblSet[8];    //m_TimeTblSet[0]:周日m_TimeTblSet[1]:周一类推
			1,//unsigned char			m_MDMask[20*15];//设置移动区域
			2,//unsigned char			m_uAlarmInterval;//报警间隔的设定值：“1分钟、5分钟、15分钟、30分钟、1小时、无延时”出厂默认值设定为1分钟,分别对应:1,5,15,30,60,0 min
			1,//unsigned int			m_uMDSwitch;		// 移动侦测检查开关 -- 1:开启,0:关闭
			3,//unsigned int			m_uMDSensitive;		// 移动侦测灵敏度  4个等级:0-高，1-较高，2-中，3:低
			1,//unsigned int  			m_uMDAlarm;			// 报警输出每一位代表一个输出
			5,//unsigned int 			m_uOutputDelay;		//报警间隔的设定值：“1分钟、5分钟、15分钟、30分钟、1小时、无延时”出厂默认值设定为1分钟,分别对应:1,5,15,30,60,0 min
			0,//unsigned int			m_uAalarmOutMode;  	// 每一位代表处理方式 bit0   声音报警   bit1  抓拍   bit2  FTP上传bit3 发送邮件bit4 屏幕提示bit5录像bit6 是否上传报警信息
		},
		0,
	},//CAMERA_MD				m_CameraMd;			//移动侦测
	{0},//PTZ_PARA				m_PTZ;				//云台设置
	{0},//USERGROUPSET			m_Users;			//用户管理
	{0},//SYSTEMMAINETANCE		m_SysMainetance;	//系统维护
	{0},//VIDEODISPLAYSET			m_DisplaySet;		//视频输出
	{0},//GROUPEXCEPTHANDLE		m_SysExcept;		//异常处理
	{0},//PresetPointSet				m_presetPointSet;
	{0},//SubDevices              m_subDev;           //被动设备列表  768字节 		
	{0},//char 					m_res8[resp1_len];		
	{0},//VIDEOOSDINSERT			m_OsdInsert;		//视频遮挡
	{0},//GROUPRECORDTASK			m_RecordSchedule; 	//录像任务
	{0},//ALARMZONEGROUPSET		m_ZoneGroup;		//报警输入设置及处理
	{0},//PARAMETEREXTEND			m_ParaExtend;		// ntp扩展使用，
	{0},//web_sync_param_t		m_web;				//	360
	{0},//P2P_MD_REGION_CHANNEL	m_mdset;			// 16 
	{0},//MotAlarmStrategy   		m_MotAlarmStrategy;  //252
	{0},//PirAlarmStrategy		m_PirAlarmStrategy;		//264
	{
		8.0,//float x;      //横坐标比
		4.0,//float y;      //纵坐标比
		3.0,//float width;  //宽比
		2.0,//float height; //高比
#ifdef _BUILD_FOR_NVR_
		1,//int   channel;  //通道号
		1,//char  physical_id[16];  //通道号对应的设备ID
#endif
		0,//int data_type;	//0 是矩形 1 是八边形
		{0},//p2p_point_t octagon[8];
	},//MD_Region				m_MdRegion;			//84
	{"init"},//char					m_res[resp2_len];		//	2052+60-360 =1752-16=1736
	FLASH_HAVE_INIT_VALUE,//int                     m_have_flash_init;
};
#endif


//dolphin: 20180612
//for spi flash is written by page ,so sometimes the address is not fit to it's require,so
//we allocate address in pages.this fun is use for input unligned(not times of page-size),
//then return the nearby page address
unsigned int pageAddrCorrect(unsigned int input)
{

	unsigned int returnValue = input;
#ifdef PAGE_CORRECT_IN_USE
	if((input%CONFIG_SPI_FLASH_PAGE_SIZE)==0)
	{
		returnValue = input;
	}
	else  //not aligned
	{
		returnValue = (input/CONFIG_SPI_FLASH_PAGE_SIZE+1)*CONFIG_SPI_FLASH_PAGE_SIZE;
	}
	return returnValue;
#endif
	return returnValue;
}


//as erase addr&size should be times of 4k ,so this fun is correct input to  times of 4k
unsigned int blockAddrCorrect(unsigned int input)
{
	unsigned int returnValue = 0;
#ifdef BLOCK_CORRECT_IN_USE
	if((input%ERASE_FLASH_BLOCK_SIZE)==0)
	{
		returnValue = input;
	}
	else  //not aligned
	{
		returnValue = (input/ERASE_FLASH_BLOCK_SIZE+1)*ERASE_FLASH_BLOCK_SIZE;
	}
#endif
	return returnValue;
}


//self definition function
int simpleCheckSum(char *str,int size)
{
	int ret = 0;
	char *p = str;
	
	while(size--)
	{
		ret += (*p%10); 
		p++;
	}
	return ret;
}

void stringPrint(char *in,unsigned int len,unsigned int relativeaddr ,unsigned int size)
{
	char *p = in;
	unsigned int line=0;
	unsigned int total=0;
	RWCLOG("\nlen %d rela %d size%d\n",len,relativeaddr,size);
	while(len--)
	{
		if((total == relativeaddr) && (size!=0))
		{
			RWCLOG("<<<<<<\n");
		}
		if((total == relativeaddr+size)&& (size!=0))
		{
			RWCLOG(">>>>>>\n");
		}
		RWCLOG("[%d]",*p);
		p++;
		line++;
		if(line>=20)
		{
			RWCLOG("\n");
			line=0;
		}
		total++;
	}
	RWCLOG("\n");
	return;
}


/* 
dolphin: 
	int ZMD_WRITE(unsigned int addr ,unsigned int size ,char *arg)
	
descption:
	In spi flash we need to write in block ,thus sometime user struct is not aligned to block
,then we use this fun to write in flash ,if succeed return -1 that means meet error ,else 
return 0

date:
	20180613

---------------------------------------------------------------+
| B-SIZE | B-SIZE | B-SIZE | B-SIZE | B-SIZE | B-SIZE | B-SIZE |
+--------|--------|--------|--------|--------|--------|--------+
|input:               [addr                    size]           |
+--------------------------------------------------------------+
|backup:          [===][oooooooooooooooooooooooooo][==]        |
+--------------------------------------------------------------+
|erase:           [eraseAddr                  eraseEnd]        |
+--------------------------------------------------------------+
|recovery:        [===][mmmmmmmmmmmmmmmmmmmmmmmmmm][==]        |
+--------------------------------------------------------------+
*/

int ZMD_WRITE(unsigned int addr ,unsigned int size ,void *arg)
{
	char *pbuf=NULL;
	unsigned int eraseAddr = 0;   //block erase start address
	unsigned int eraseEnd = 0;    //block erase end address
	unsigned int relativeAddr =0; //cache start address shoube modify relative to should be backup 
	//unsigned int relativeEnd  =0; //cache end address shoube modify relative to should be backup 
	unsigned int backupSize = 0;
	unsigned int checkSum = 0;
	unsigned int calcheckSum = 0;

	//get erase addr (should be times of block size)
	if(addr%ERASE_FLASH_BLOCK_SIZE == 0){
		eraseAddr = addr;
	}else{
		eraseAddr = (addr/ERASE_FLASH_BLOCK_SIZE)*ERASE_FLASH_BLOCK_SIZE;
		relativeAddr = addr - eraseAddr;
	}

	//get end addr (should be times of block size)
	if((addr+size)%ERASE_FLASH_BLOCK_SIZE == 0){
		eraseEnd = addr+size;
	}else{
		eraseEnd = ((addr+size)/ERASE_FLASH_BLOCK_SIZE + 1)*ERASE_FLASH_BLOCK_SIZE;
	}	

	//relativeEnd = relativeAddr+size;
	
	//get backupSize
	backupSize = eraseEnd - eraseAddr;

	//malloc for buf
	pbuf = (char*)malloc(backupSize);
	
	//read original data
	Zmodo_read(pbuf,eraseAddr,backupSize);

	//modify buffer
	memcpy(pbuf+relativeAddr,(char*)arg,size);

	//check
	checkSum = simpleCheckSum(pbuf,backupSize);
	
	//erase flash
	Zmodo_erase(eraseAddr,backupSize);

	//write to flash
	Zmodo_write(pbuf,eraseAddr,backupSize);

	//re-read
	Zmodo_read(pbuf,eraseAddr,backupSize);

	//check
	calcheckSum = simpleCheckSum(pbuf,backupSize);
	
	free(pbuf);
	pbuf=NULL;

	if(checkSum == calcheckSum){
		return 0;
	}else
	{
		return -1;
	}
}


RWConfig::RWConfig()
{

}

RWConfig::~RWConfig()
{
	
}

void RWConfig::release(void)
{
	pthread_mutex_destroy(&mutex);
}

//when flash do not been inited ,then do 
void RWConfig::Zmodo_Init(void)
{
#ifndef PAGE_CORRECT_IN_USE
	Zmodo_erase(CONFIG_PARAMETER_ADDRESS,CONFIG_PARAMETER_SIZE);
	Zmodo_write(&defaultARR,CONFIG_PARAMETER_ADDRESS,sizeof(SYSTEM_PARAMETER));
#else
	int i = 0;
	CONFIG_PARA t;
	t.method = CONFIG_WRITE;
	CAMERA_PARA t1 = 
	{
		{  
			{
				1,//unsigned char 			m_uChannelValid; 	// 主码流视频通道开关1:on  0:off
				1,//unsigned char				m_uAudioSwitch;	// 主码流音频通道开关1:on  0:off
				1,//unsigned char				m_uSubEncSwitch; // 子码流视频开关1:on 0:off
				1,//unsigned char				m_uSubAudioEncSwitch; // 子码流音频开关1:on 0:off
				"stream2",//char 					m_Title[17];	 	 // 通道标题
				12,//unsigned char 			m_uFrameRate;	 // 帧率	PAL(1-25) NTSC(1-30)
				4,//unsigned char 			m_uResolution;	 // 清晰度  0:D1, 1: HD1, 2:CIF 3:QCIF 4:1080P 5:720P 6:VGA 7:QVGA 8:960H
				0,//unsigned char 			m_uQuality;		 // 画质quality	values:0--4 (最好)(很好)(好)(一般)(差)
				0,//unsigned char 			m_uEncType;	// 1:CBR 0:VBR
				4,//unsigned char			m_uSubRes;  	 // 子码流清晰度0:D1, 1: HD1, 2:CIF 3:QCIF 4:1080P 5:720P 6:VGA 7:QVGA
				4,//unsigned char			m_uSubQuality; 	 // 子码流画质 values:0--4 (最好)(很好)(好)(一般)(差)	
				12,//unsigned char			m_uSubFrameRate; // 子码流帧率 PAL(1-25) NTSC(1-30)
				0,//unsigned char 		m_uSubEncType;// 1:CBR 0:VBR
				5,//unsigned char			m_u16RecDelay;	 // 录像延时
				1,//unsigned char			m_u8PreRecTime;  // 预录时间
				1,//unsigned char			m_TltleSwitch;	 // 通道标题开关0:off 1:on
				1,//unsigned char			m_TimeSwitch;	 // 时间标题开关0:off 1:on
				0,//unsigned char			m_u8Reserved[3];
			},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
			{0},
		},	
		0,
	};
	
	CAMERASENSOR_PARA t2=	
	{
		1,//int	m_colorMode;			// 1:彩色模式2:黑白模式
		1,//int	m_picMode;				// 1:普通2:镜像+翻转
		0,//int	m_picFlip;				
		0,//int	m_picMirrorn;
		3,//int	m_PowerFreq;			// 0:室外抗闪1:50HZ 2:60HZ 3:自动抗闪
	};//CAMERASENSOR_PARA		m_Sensor;			//sensor工作状态

	CAMERA_MD t3=
	{
		{
			{0},//ALARMINDATESET		m_TimeTblSet[8];    //m_TimeTblSet[0]:周日m_TimeTblSet[1]:周一类推
			1,//unsigned char			m_MDMask[20*15];//设置移动区域
			2,//unsigned char			m_uAlarmInterval;//报警间隔的设定值：“1分钟、5分钟、15分钟、30分钟、1小时、无延时”出厂默认值设定为1分钟,分别对应:1,5,15,30,60,0 min
			1,//unsigned int			m_uMDSwitch;		// 移动侦测检查开关 -- 1:开启,0:关闭
			3,//unsigned int			m_uMDSensitive;		// 移动侦测灵敏度  4个等级:0-高，1-较高，2-中，3:低
			1,//unsigned int  			m_uMDAlarm;			// 报警输出每一位代表一个输出
			5,//unsigned int 			m_uOutputDelay;		//报警间隔的设定值：“1分钟、5分钟、15分钟、30分钟、1小时、无延时”出厂默认值设定为1分钟,分别对应:1,5,15,30,60,0 min
			0,//unsigned int			m_uAalarmOutMode;  	// 每一位代表处理方式 bit0   声音报警   bit1  抓拍   bit2  FTP上传bit3 发送邮件bit4 屏幕提示bit5录像bit6 是否上传报警信息
		},
		0,
	};

	MD_Region t4 = 	
	{
		8.0,//float x;      //横坐标比
		4.0,//float y;      //纵坐标比
		3.0,//float width;  //宽比
		2.0,//float height; //高比
#ifdef _BUILD_FOR_NVR_
		1,//int   channel;  //通道号
		1,//char  physical_id[16];  //通道号对应的设备ID
#endif
		0,//int data_type;	//0 是矩形 1 是八边形
		{0},//p2p_point_t octagon[8];
	};

	char t5[resp2_len]={"ZMODO PARAMTER INTERFACE"};
	int t6 = FLASH_HAVE_INIT_VALUE;

	for(;i<CONFIG_METHOD_MAX;i++)
	{
		switch(i)
		{
			case CONFIG_CAMERA_PARA:
				t.category = CONFIG_CAMERA_PARA;
				handle(t,&t1);
				break;
			case CONFIG_CAMERASENSOR_PARA:
				t.category = CONFIG_CAMERASENSOR_PARA;
				handle(t,&t2);
				break;
			case CONFIG_CAMERA_MD:
				t.category = CONFIG_CAMERA_MD;
				handle(t,&t3);
				break;
			case CONFIG_MD_REGION:
				t.category = CONFIG_MD_REGION;
				handle(t,&t4);
				break;
			case CONFIG_RESPONSE_2:
				t.category = CONFIG_MD_REGION;
				handle(t,t5);
				break;
			case CONFIG_FLASH_HAVE_INIT_BIT:
				t.category = CONFIG_FLASH_HAVE_INIT_BIT;
				handle(t,&t6);
				break;
			default:
				break;
		}
	}
#endif
}


int RWConfig::handle(CONFIG_PARA t,void *arg)
{
	int ret = 0;
	pthread_mutex_lock(&mutex);
	switch(t.method)
	{
		case CONFIG_READ:
			hispinor_read(arg,\
					   addrTable[t.category].addr,\
				       addrTable[t.category].size);
			break;
		case CONFIG_WRITE:
			ret = ZMD_WRITE(addrTable[t.category].addr,addrTable[t.category].size,arg);
			break;
	/*	case CONFIG_CLEAR_PART:
			hispinor_erase(addrTable[t.category].addr,\
				       addrTable[t.category].size);
			break;
			*/
		case CONFIG_CLEAR_ALL:
			hispinor_erase(CONFIG_PARAMETER_ADDRESS,\
				        CONFIG_PARAMETER_SIZE);
			break;
		case CONFIG_READ_ALL:
			hispinor_read(arg,\
					   CONFIG_PARAMETER_ADDRESS,\
					   sizeof(SYSTEM_PARAMETER));
			break;
		case CONFIG_INIT_DEFAULT:
			//Zmodo_Init();  //no use thar for avoid of dead-lock
			ret = -1;
			break;
		default:
			ret = -1;
			break;
	}

	pthread_mutex_unlock(&mutex);
	return ret;
}


void RWConfig::init(void)
{
	int ret = -1;
	int retLen=0;
	unsigned int addrStart=0;
	int category=0;

	addrStart = CONFIG_PARAMETER_ADDRESS;
	addrTable[0].addr = CONFIG_PARAMETER_ADDRESS;
	addrTable[0].size = 0;
	
	//init table to record offset from physical address
	while(category<CONFIG_METHOD_MAX)
	{
		//NETWORK_PARA
		if( category==CONFIG_NETWORK_PARA)
		{
			retLen = sizeof(NETWORK_PARA);
			addrStart += sizeof(NETWORK_PARA);
			addrStart = pageAddrCorrect(addrStart);
		}
		//MACHINE_PARA
		else if( category==CONFIG_MACHINE_PARA)
		{
			retLen = sizeof(MACHINE_PARA);
			addrStart += sizeof(MACHINE_PARA);
			addrStart = pageAddrCorrect(addrStart);
		}
		//CAMERA_PARA
		else if( category==CONFIG_CAMERA_PARA)
		{
			retLen = sizeof(CAMERA_PARA);
			addrStart += sizeof(CAMERA_PARA);
			addrStart = pageAddrCorrect(addrStart);
		}
		//CAMERA_ANALOG
		else if( category==CONFIG_CAMERA_ANALOG && ret==-1)
		{
			retLen = sizeof(CAMERA_ANALOG);
			addrStart += sizeof(CAMERA_ANALOG);
			addrStart = pageAddrCorrect(addrStart);
		}
		//CAMERASENSOR_PARA
		else if( category==CONFIG_CAMERASENSOR_PARA)
		{
			retLen = sizeof(CAMERASENSOR_PARA);
			addrStart += sizeof(CAMERASENSOR_PARA);
			addrStart = pageAddrCorrect(addrStart);
		}
		//COMMON_PARA
		else if( category==CONFIG_COMMON_PARA)
		{
			retLen = sizeof(COMMON_PARA);
			addrStart += sizeof(COMMON_PARA);
			addrStart = pageAddrCorrect(addrStart);
		}
		//CAMERA_BLIND
		else if( category==CONFIG_CAMERA_BLIND)
		{
			retLen = sizeof(CAMERA_BLIND);
			addrStart += sizeof(CAMERA_BLIND);
			addrStart = pageAddrCorrect(addrStart);
		}
		//AlarmOutPort
		else if( category==CONFIG_ALARM_OUT_PORT)
		{
			retLen = sizeof(AlarmOutPort);
			addrStart += sizeof(AlarmOutPort);
			addrStart = pageAddrCorrect(addrStart);
		}
		//CAMERA_VideoLoss
		else if( category==CONFIG_CAMERA_VIDEO_LOSS)
		{
			retLen = sizeof(CAMERA_VideoLoss);
			addrStart += sizeof(CAMERA_VideoLoss);
			addrStart = pageAddrCorrect(addrStart);
		}
		//CAMERA_MD
		else if( category==CONFIG_CAMERA_MD)
		{
			retLen = sizeof(CAMERA_MD);
			addrStart += sizeof(CAMERA_MD);
			addrStart = pageAddrCorrect(addrStart);
		}
		//PTZ_PARA
		else if( category==CONFIG_PTZ_PARA)
		{
			retLen = sizeof(PTZ_PARA);
			addrStart += sizeof(PTZ_PARA);
			addrStart = pageAddrCorrect(addrStart);
		}
		//USERGROUPSET
		else if( category==CONFIG_USER_GROUP_SET)
		{
			retLen = sizeof(USERGROUPSET);
			addrStart += sizeof(USERGROUPSET);
			addrStart = pageAddrCorrect(addrStart);
		}
		//SYSTEMMAINETANCE
		else if( category==CONFIG_SYSTEM_MAINETANCE)
		{
			retLen = sizeof(SYSTEMMAINETANCE);
			addrStart += sizeof(SYSTEMMAINETANCE);
			addrStart = pageAddrCorrect(addrStart);
		}
		//VIDEODISPLAYSET
		else if( category==CONFIG_VIDEO_DISPLAY_SET)
		{
			retLen = sizeof(VIDEODISPLAYSET);
			addrStart += sizeof(VIDEODISPLAYSET);
			addrStart = pageAddrCorrect(addrStart);
		}
		//GROUPEXCEPTHANDLE
		else if( category==CONFIG_GROUP_EXCEPT_HANDLE)
		{
			retLen = sizeof(GROUPEXCEPTHANDLE);
			addrStart += sizeof(GROUPEXCEPTHANDLE);
			addrStart = pageAddrCorrect(addrStart);
		}
		//PresetPointSet
		else if( category==CONFIG_PRESET_POINT_SET)
		{
			retLen = sizeof(PresetPointSet);
			addrStart += sizeof(PresetPointSet);
			addrStart = pageAddrCorrect(addrStart);
		}
		//SubDevices
		else if( category==CONFIG_SUB_DEVICE)
		{
			retLen = sizeof(SubDevices);
			addrStart += sizeof(SubDevices);
			addrStart = pageAddrCorrect(addrStart);
		}
		//response 1
		else if( category==CONFIG_RESPONSE_1)
		{
			retLen = resp1_len;
			addrStart += resp1_len;
			addrStart = pageAddrCorrect(addrStart);
		}
		//VIDEOOSDINSERT
		else if( category==CONFIG_VIDEO_OSD_INSET)
		{
			retLen = sizeof(VIDEOOSDINSERT);
			addrStart += sizeof(VIDEOOSDINSERT);
			addrStart = pageAddrCorrect(addrStart);
		}
		//GROUPRECORDTASK
		else if( category==CONFIG_GROUP_RECORD_TASK)
		{
			retLen = sizeof(GROUPRECORDTASK);
			addrStart += sizeof(GROUPRECORDTASK);
			addrStart = pageAddrCorrect(addrStart);
		}
		//ALARMZONEGROUPSET
		else if( category==CONFIG_ALARM_ZONE_GROUP_SET)
		{
			retLen = sizeof(ALARMZONEGROUPSET);
			addrStart += sizeof(ALARMZONEGROUPSET);
			addrStart = pageAddrCorrect(addrStart);
		}	
		//PARAMETEREXTEND
		else if( category==CONFIG_PARA_METER_EXTEND)
		{
			retLen = sizeof(PARAMETEREXTEND);
			addrStart += sizeof(PARAMETEREXTEND);
			addrStart = pageAddrCorrect(addrStart);
		}
		//web_sync_param_t
		else if( category==CONFIG_WEB_SYNC_PARAM)
		{
			retLen = sizeof(web_sync_param_t);
			addrStart += sizeof(web_sync_param_t);
			addrStart = pageAddrCorrect(addrStart);
		}
		//P2P_MD_REGION_CHANNEL
		else if( category==CONFIG_P2P_MD_REGION_CHANNEL)
		{
			retLen = sizeof(P2P_MD_REGION_CHANNEL);
			addrStart += sizeof(P2P_MD_REGION_CHANNEL);
			addrStart = pageAddrCorrect(addrStart);
		}
		//MotAlarmStrategy
		else if( category==CONFIG_MOTOR_ALARM_STRATEGY)
		{
			retLen = sizeof(MotAlarmStrategy);
			addrStart += sizeof(MotAlarmStrategy);
			addrStart = pageAddrCorrect(addrStart);
		}
		//PirAlarmStrategy
		else if( category==CONFIG_PIR_ALARM_STRATEGY)
		{
			retLen = sizeof(PirAlarmStrategy);
			addrStart += sizeof(PirAlarmStrategy);
			addrStart = pageAddrCorrect(addrStart);
		}
		//MD_Region
		else if( category==CONFIG_MD_REGION)
		{
			retLen = sizeof(MD_Region);
			addrStart += sizeof(MD_Region);
			addrStart = pageAddrCorrect(addrStart);
		}
		//response 2
		else if( category==CONFIG_RESPONSE_2)
		{
			retLen = resp2_len;
			addrStart += resp2_len;
			addrStart = pageAddrCorrect(addrStart);
		}
		//CONFIG_FLASH_HAVE_INIT_BIT
		else if( category==CONFIG_FLASH_HAVE_INIT_BIT )
		{
			retLen = sizeof(int);
			addrStart += sizeof(int); 
			addrStart = pageAddrCorrect(addrStart);
		}
		
		addrTable[category].size = retLen;
		category++;
		if(category < CONFIG_METHOD_MAX) //last time address do not need to increase
		{
			addrTable[category].addr = addrStart;
		}
	}
	if(addrTable[category].addr > CONFIG_PARAMETER_ADDRESS+CONFIG_PARAMETER_SIZE)
	{
		RWCERR("write flash run over area\n");
	}
	/*
	for(category=0;category<CONFIG_METHOD_MAX;category++)
	{
		RWCLOG("%d [%d] [%d]\n",category,addrTable[category].addr,addrTable[category].size);
	}
	*/
	pthread_mutex_init(&mutex, NULL); //init mutex

}


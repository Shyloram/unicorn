#include <string.h>
#include "rwconfiginterface.h"
#include "rwconfig.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_RWCONDIF
RwConfigInterface* RwConfigInterface::m_instance = new RwConfigInterface;
#else
RwConfigInterface* RwConfigInterface::m_instance = NULL;
#endif

RwConfigInterface* RwConfigInterface::GetInstance()
{
	return m_instance;
}

#ifdef ZMD_APP_RWCONDIF
RwConfigInterface::RwConfigInterface()
{
}

RwConfigInterface::~RwConfigInterface()
{
}

int RwConfigInterface::init()
{
	return 0;
}

int RwConfigInterface::release()
{
	return 0;
}

int RwConfigInterface::start()
{
	RWConfig* l_instance = RWConfig::GetInstance();
	if(l_instance == NULL)
	{
		return -1;
	}

	l_instance->init();
	
	CONFIG_PARA t;
	int initflag = 0;
	t.method = CONFIG_READ;
	t.category = CONFIG_FLASH_HAVE_INIT_BIT;
	l_instance->handle(t,&initflag);
	
	if(initflag != FLASH_HAVE_INIT_VALUE)
	{
		ITFLOG("===== SYSTEM PARAMER HAVEN NOT BEEN INIT =====\n");
		l_instance->Zmodo_Init();  //init paramter
		
#if 0   //below is for tesing
		CAMERA_PARA t1;
		MACHINE_PARA t2={0,1,2,9,4,5,6,7,8,9,10,11};
		MACHINE_PARA t3;
		char	t4[resp2_len]="zmd testing relative";
		char    t5[resp2_len]={0};
		
		
 		//below is for test
		t.method = CONFIG_READ;
		t.category = CONFIG_FLASH_HAVE_INIT_BIT;
		l_instance->handle(t,&initflag);

		ITFLOG("===== INIT SYSYTEM PARAMER =====\n");
		ITFLOG("get After init flag is %d\n",initflag);

		t.method = CONFIG_READ;
		t.category = CONFIG_CAMERA_PARA;
		l_instance->handle(t,&t1);

		ITFLOG("get streamname is %s\n",t1.m_ChannelPara[0].m_Title);

		char tmp[1200]={0};
		t.method = CONFIG_READ;
		t.category = CONFIG_RESPONSE_2;
		l_instance->handle(t,tmp);
		ITFLOG("total get %s\n",tmp);

		/*
		typedef struct
		{
		unsigned int 			m_uMachinId;		// »úÆ÷ºÅÂë
		unsigned char 			m_uTvSystem;		// ÏµÍ³ÖÆÊ½:1 NTSC, 0  PAL
		unsigned char 			m_uHddOverWrite;	// ×Ô¶¯¸²¸Ç¿ª¹ØÊÇ·ñ´ò¿ª: 0 no, 1 yes
		unsigned char 			m_uHddorSD;			// Ó²ÅÌ»òÕßSD¿¨Â¼Ïñ£¬Èô¸Ä±äÖØÆôÏµÍ³0:HDD 1:SD
		unsigned char			m_uCursorFb;		// ==0 hifb0, !=0 hifb2
		unsigned char			m_uOutputMode;		// 0:VGA  1:HDMI
		unsigned char           m_SyncIpcTime;  //Í¬²½IPCÊ±¼ä
		unsigned char           m_HvrMode;
		unsigned char 			m_Reserved[5];		
		unsigned int 			m_serialId;			// »úÆ÷ÐòÁÐºÅ£¬Êµ¼ÊÊÇ±¨¾¯Ö÷»ú¶Á¹ýÀ´µÄMACµØÖ·
		unsigned int			m_ChipType;			// Ð¾Æ¬ÀàÐÍ:3520 3511 3512
		unsigned int			m_changeinfo;		// ¸üÐÂÖ¸Ê¾ 0ÎÞ¸üÐÂ 1ÓÐ¸üÐÂ
		}MACHINE_PARA;
		*/

		t.method = CONFIG_WRITE;
		t.category = CONFIG_MACHINE_PARA;
		l_instance->handle(t,&t2);

		t.method = CONFIG_READ;
		t.category = CONFIG_MACHINE_PARA;
		l_instance->handle(t,&t3);

		ITFLOG("m_uHddorSD is %d should be 9\n",t3.m_uHddorSD);

		//write response
		t.method = CONFIG_WRITE;
		t.category = CONFIG_RESPONSE_2;
		l_instance->handle(t,t4);

		//write pir
		t.method = CONFIG_WRITE;
		t.category = CONFIG_PIR_ALARM_STRATEGY;
		PirAlarmStrategy t7={
			7,    //int interval;
			6,    //int video_last;
			5,    //int s_time;
			4,    //int cloud_m_video_last;
			3,    //int cloud_s_video_last;
			{0},  //p2p_alarm_stream_level sl[20];
			1,    //int sl_size;
		};
		PirAlarmStrategy t8;
		
		l_instance->handle(t,&t7);

		t.method = CONFIG_READ;
		t.category = CONFIG_RESPONSE_2;
		l_instance->handle(t,t5);

		ITFLOG("config str is %s\n",t5);

		t.method = CONFIG_READ;
		t.category = CONFIG_PIR_ALARM_STRATEGY;
		l_instance->handle(t,&t8);
		ITFLOG("config pir  interval should be 7 ,now %d\n",t8.interval);

#endif
	}
	else
	{
		ITFLOG("===== SYSYTEM PARAMER HAVE BEEN INIT =====\n");
	}
	return 0;
}

int RwConfigInterface::stop()
{
	RWConfig* l_instance = RWConfig::GetInstance();
	if(l_instance == NULL)
	{
		return -1;
	}
	l_instance->release();
	return 0;
}

//for handle interface to handleconfigparam interface ,we need to use special transport.
//para:[CONFIG_PARA][paraDatelocal]

int RwConfigInterface::handle(MFI_CMD cmd,void* para)
{
	int ret = -1;
	CONFIG_PARA cmdlocal;
	char *paraDatelocal=NULL;

	if(para==NULL)
	{
		ITFERR("input error!\n");
		return -1;
	}

	RWConfig* l_instance = RWConfig::GetInstance();
	if(l_instance == NULL)
	{
		return -1;
	}
	
	memcpy(&cmdlocal,para,sizeof(CONFIG_PARA));
	paraDatelocal =  (char*)para;
	paraDatelocal += sizeof(CONFIG_PARA);
	l_instance->handle(cmdlocal,(void*)paraDatelocal);

    return ret;
}
#endif

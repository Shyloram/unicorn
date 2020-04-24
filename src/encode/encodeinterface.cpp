#include "encodeinterface.h"
#include "video.h"
#include "audio.h"
#include "buffermanage.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_ENCODE
EncodeInterface* EncodeInterface::m_instance = new EncodeInterface;
#else
EncodeInterface* EncodeInterface::m_instance = NULL;
#endif

EncodeInterface* EncodeInterface::GetInstance()
{
	return m_instance;
}

#ifdef ZMD_APP_ENCODE
EncodeInterface::EncodeInterface()
{
}

EncodeInterface::~EncodeInterface()
{
}

int EncodeInterface::init()
{
	int ret = 0;
	BufferManageCtrl* pbuffctl = BufferManageCtrl::GetInstance();
	if(pbuffctl != NULL)
	{
		ret = pbuffctl->BufferManageInit();
		if(ret != 0)
		{
			ITFERR("BufferManageInit failed! ret:%d\n",ret);
			return ret;
		}
	}

	Video* pvideo = ZMDVideo::GetInstance();
	if(pvideo != NULL)
	{
		ret = pvideo->VideoInit();
		if(ret != 0)
		{
			ITFERR("VideoInit failed! ret:%d\n",ret);
			return ret;
		}
	}

#ifdef ZMD_APP_ENCODE_AUDIO
	Audio* paudio = ZMDAudio::GetInstance(); 
	if(paudio != NULL)
	{
		ret = paudio->AudioInit();
		if(ret != 0)
		{
			ITFERR("AudioInit failed! ret:%d\n",ret);
			return ret;
		}
	}
#endif

	ITFLOG("EncodeInterface init success!\n");
	return 0;
}

int EncodeInterface::release()
{
	int ret = 0;
#ifdef ZMD_APP_ENCODE_AUDIO
	Audio* paudio = ZMDAudio::GetInstance(); 
	if(paudio != NULL)
	{
		ret = paudio->AudioRelease();
		if(ret != 0)
		{
			ITFERR("AudioRelease failed! ret:%d\n",ret);
			return ret;
		}
	}
#endif

	Video* pvideo = ZMDVideo::GetInstance();
	if(pvideo != NULL)
	{
		ret = pvideo->VideoRelease();
		if(ret != 0)
		{
			ITFERR("VideoRelease failed! ret:%d\n",ret);
			return ret;
		}
	}

	BufferManageCtrl* pbuffctl = BufferManageCtrl::GetInstance();
	if(pbuffctl != NULL)
	{
		ret = pbuffctl->BufferManageRelease();
		if(ret != 0)
		{
			ITFERR("BufferManageRelease failed! ret:%d\n",ret);
			return ret;
		}
	}

	ITFLOG("EncodeInterface release success!\n");
	return 0;
}

int EncodeInterface::start()
{
	int ret = 0;
	Video* pvideo = ZMDVideo::GetInstance();
	if(pvideo != NULL)
	{
		ret = pvideo->VideoStart();
		if(ret != 0)
		{
			ITFERR("VideoStart failed! ret:%d\n",ret);
			return ret;
		}
	}

#ifdef ZMD_APP_ENCODE_AUDIO
	Audio* paudio = ZMDAudio::GetInstance(); 
	if(paudio != NULL)
	{
		ret = paudio->AudioStart();
		if(ret != 0)
		{
			ITFERR("AudioStart failed! ret:%d\n",ret);
			return ret;
		}
	}
#endif

	ITFLOG("EncodeInterface start success!\n");
	return 0;
}

int EncodeInterface::stop()
{
	int ret = 0;
	Video* pvideo = ZMDVideo::GetInstance();
	if(pvideo != NULL)
	{
		ret = pvideo->VideoStop();
		if(ret != 0)
		{
			ITFERR("VideoStop failed! ret:%d\n",ret);
			return ret;
		}
	}

#ifdef ZMD_APP_ENCODE_AUDIO
	Audio* paudio = ZMDAudio::GetInstance(); 
	if(paudio != NULL)
	{
		ret = paudio->AudioStop();
		if(ret != 0)
		{
			ITFERR("AudioStop failed! ret:%d\n",ret);
			return ret;
		}
	}
#endif

	ITFLOG("EncodeInterface stop success!\n");
	return 0;
}

int EncodeInterface::handle(MFI_CMD cmd,void* para)
{
	switch(cmd)
	{
		case CMD_ENC_GETFRAME:
			return BufferManageCtrl::GetInstance() != NULL ? BufferManageCtrl::GetInstance()->GetFrame(para) : -1;
		case CMD_ENC_RESETUSER:
			return BufferManageCtrl::GetInstance() != NULL ? BufferManageCtrl::GetInstance()->ResetUser(para) : -1;
		case CMD_ENC_GETAESKEY:
			return BufferManageCtrl::GetInstance() != NULL ? BufferManageCtrl::GetInstance()->GetAesKey(para) : -1;
#ifdef ZMD_APP_ENCODE_AUDIO
		case CMD_ENC_AUDIODECODE:
			return ZMDAudio::GetInstance() != NULL ? ZMDAudio::GetInstance()->AudioDecHandle(para) : -1;
		case CMD_ENC_AUDIODECODESWON:
			return ZMDAudio::GetInstance() != NULL ? ZMDAudio::GetInstance()->AudioDecSwitch(true) : -1;
		case CMD_ENC_AUDIODECODESWOFF:
			return ZMDAudio::GetInstance() != NULL ? ZMDAudio::GetInstance()->AudioDecSwitch(false) : -1;
#endif
		default:
			return -1;
	}
}

#endif

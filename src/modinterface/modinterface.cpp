#include "modinterface.h"
#include "encodeinterface.h"
#include "meshareinterface.h"
#include "zspinterface.h"
#include "rtspinterface.h"

ModInterface* ModInterface::m_instance = new ModInterface;

ModInterface* ModInterface::GetInstance()
{
	return m_instance;
}

Interface* ModInterface::GetModInstance(MFI_MOD mod)
{
	switch(mod)
	{
		case MOD_ENC:
			return EncodeInterface::GetInstance();
		case MOD_RTSP:
			return RTSPInterface::GetInstance();
		case MOD_MSH:
			return MeshareInterface::GetInstance();
		case MOD_ZSP:
			return ZSPInterface::GetInstance();
		default:
			return NULL;
	}
}

ModInterface::ModInterface()
{
}

ModInterface::~ModInterface()
{
}

int ModInterface::init()
{
	int i;
	Interface* pinterface;
	ITFLOG("ModInterface init launch ----------------------------->\n");
	for(i = 0;i < (int)MOD_MAX;i++)
	{
		pinterface = GetModInstance((MFI_MOD)i);
		if(pinterface != NULL)
		{
			pinterface->init();
		}
	}
	ITFLOG("ModInterface init finish <-----------------------------\n");
	return 0;
}

int ModInterface::release()
{
	int i;
	Interface* pinterface;
	ITFLOG("ModInterface release launch ----------------------------->\n");
	for(i = 0;i < (int)MOD_MAX;i++)
	{
		pinterface = GetModInstance((MFI_MOD)i);
		if(pinterface != NULL)
		{
			pinterface->release();
		}
	}
	ITFLOG("ModInterface release finish <-----------------------------\n");
	return 0;
}

int ModInterface::start()
{
	int i;
	Interface* pinterface;
	ITFLOG("ModInterface start launch ----------------------------->\n");
	for(i = 0;i < (int)MOD_MAX;i++)
	{
		pinterface = GetModInstance((MFI_MOD)i);
		if(pinterface != NULL)
		{
			pinterface->start();
		}
	}
	ITFLOG("ModInterface start finish <-----------------------------\n");
	return 0;
}

int ModInterface::stop()
{
	int i;
	Interface* pinterface;
	ITFLOG("ModInterface stop launch ----------------------------->\n");
	for(i = 0;i < (int)MOD_MAX;i++)
	{
		pinterface = GetModInstance((MFI_MOD)i);
		if(pinterface != NULL)
		{
			pinterface->stop();
		}
	}
	ITFLOG("ModInterface stop finish <-----------------------------\n");
	return 0;
}

int ModInterface::CMD_handle(MFI_MOD mod,MFI_CMD cmd,void* para)
{
	int ret;
	Interface* interface = GetModInstance(mod);
	if(interface == NULL)
	{
		return -1;
	}
	ret = interface->handle(cmd,para);
	return ret;
}

int startmodinterface(void)
{
	ModInterface* modinterface = ModInterface::GetInstance(); 
	if(modinterface != NULL)
	{
		modinterface->init();
		modinterface->start();
	}
	return 0;
}

int stopmodinterface(void)
{
	ModInterface* modinterface = ModInterface::GetInstance(); 
	if(modinterface != NULL)
	{
		modinterface->stop();
		modinterface->release();
	}
	return 0;
}

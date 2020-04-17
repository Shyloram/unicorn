#include "zspinterface.h"
#include "zsp_main.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_ZSP
ZSPInterface* ZSPInterface::m_instance = new ZSPInterface;
#else
ZSPInterface* ZSPInterface::m_instance = NULL;
#endif

ZSPInterface* ZSPInterface::GetInstance()
{
	return m_instance;
}

#ifdef ZMD_APP_ZSP
ZSPInterface::ZSPInterface()
{
}

ZSPInterface::~ZSPInterface()
{
}

int ZSPInterface::init()
{
	return 0;
}

int ZSPInterface::release()
{
	return 0;
}

int ZSPInterface::start()
{
    zsp_main();
	ITFLOG("ZSPInterface start success!\n");
	return 0;
}

int ZSPInterface::stop()
{
	return 0;
}

int ZSPInterface::handle(MFI_CMD cmd,void* para)
{

#if 0
	switch(cmd)
	{
		case CMD_ZSP_FUNC1:
			return ZSP::GetInstance()->ZSP_func1();
		case CMD_ZSP_FUNC2:
			return ZSP::GetInstance()->ZSP_func1();
		case CMD_ZSP_FUNC3:
			return ZSP::GetInstance()->ZSP_func1();
		default:
			return -1;
	}
#endif
    return 0;
}
#endif

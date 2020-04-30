#include "rtspinterface.h"
#include "rtspserver.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_RTSP
RTSPInterface* RTSPInterface::m_instance = new RTSPInterface;
#else
RTSPInterface* RTSPInterface::m_instance = NULL;
#endif

RTSPInterface* RTSPInterface::GetInstance()
{
	return m_instance;
}

#ifdef ZMD_APP_RTSP
RTSPInterface::RTSPInterface()
{
}

RTSPInterface::~RTSPInterface()
{
}

int RTSPInterface::init()
{
	return 0;
}

int RTSPInterface::release()
{
	return 0;
}

int RTSPInterface::start()
{
	StartRtspServer();
	ITFLOG("RTSPInterface start success!\n");
	return 0;
}

int RTSPInterface::stop()
{
	StopRtspServer();
	ITFLOG("RTSPInterface stop success!\n");
	return 0;
}

int RTSPInterface::handle(MFI_CMD cmd,void* para)
{
	return 0;
}

#endif

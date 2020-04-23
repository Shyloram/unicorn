#ifndef __RTSP_INTERFACE_H__
#define __RTSP_INTERFACE_H__

#include "modinterface.h"

class RTSPInterface : public Interface
{
	private:
		static RTSPInterface* m_instance;
		
	private:
		RTSPInterface();
		~RTSPInterface();

	public:
		static RTSPInterface* GetInstance();
		int init();
		int release();
		int start();
		int stop();
		int handle(MFI_CMD cmd,void* para = NULL);
};

#endif //__RTSP_INTERFACE_H__

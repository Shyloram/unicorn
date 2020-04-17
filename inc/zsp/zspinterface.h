#ifndef __ZSP_INTERFACE_H__
#define __ZSP_INTERFACE_H__

#include "modinterface.h"

class ZSPInterface : public Interface
{
	private:
		static ZSPInterface* m_instance;
		
	private:
		ZSPInterface();
		~ZSPInterface();

	public:
		static ZSPInterface* GetInstance();
		int init();
		int release();
		int start();
		int stop();
		int handle(MFI_CMD cmd,void* para = NULL);
};

#endif //__ZSP_INTERFACE_H__

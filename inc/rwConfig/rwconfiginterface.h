#ifndef __RW_CONFIG_INTERFACE_H__
#define __RW_CONFIG_INTERFACE_H__

#include "modinterface.h"

class RwConfigInterface : public Interface
{
	private:
		static RwConfigInterface* m_instance;
		
	private:
		RwConfigInterface();
		~RwConfigInterface();

	public:
		static RwConfigInterface* GetInstance();
		int init();
		int release();
		int start();
		int stop();
		int handle(MFI_CMD cmd,void* para = NULL);
};

#endif //__RW_CONFIG_INTERFACE_H__

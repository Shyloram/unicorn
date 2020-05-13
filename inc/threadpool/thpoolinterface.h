#ifndef __THPOOL_INTERFACE_H__
#define __THPOOL_INTERFACE_H__

#include "modinterface.h"

class TPLInterface : public Interface
{
	private:
		static TPLInterface* m_instance;
		
	private:
		TPLInterface();
		~TPLInterface();

	public:
		static TPLInterface* GetInstance();
		int init();
		int release();
		int start();
		int stop();
		int handle(MFI_CMD cmd,void* para = NULL);
};

#endif //__THPOOL_INTERFACE_H__

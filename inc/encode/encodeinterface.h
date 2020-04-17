#ifndef __ENCODE_INTERFACE_H__
#define __ENCODE_INTERFACE_H__

#include "modinterface.h"

class EncodeInterface : public Interface
{
	private:
		static EncodeInterface* m_instance;
		
	private:
		EncodeInterface();
		~EncodeInterface();

	public:
		static EncodeInterface* GetInstance();
		int init();
		int release();
		int start();
		int stop();
		int handle(MFI_CMD cmd,void* para = NULL);
};

#endif //__ENCODE_INTERFACE_H__

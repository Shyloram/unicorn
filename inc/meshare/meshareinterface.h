#ifndef __MESHARE_INTERFACE_H__
#define __MESHARE_INTERFACE_H__

#include "modinterface.h"

class MeshareInterface : public Interface
{
	private:
		static MeshareInterface* m_instance;
		
	private:
		MeshareInterface();
		~MeshareInterface();

	public:
		static MeshareInterface* GetInstance();
		int init();
		int release();
		int start();
		int stop();
		int handle(MFI_CMD cmd,void* para = NULL);
};

#endif //__MESHARE_INTERFACE_H__

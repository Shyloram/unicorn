#include "meshareinterface.h"
#include "meshare_main.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_MESHARE
MeshareInterface* MeshareInterface::m_instance = new MeshareInterface;
#else
MeshareInterface* MeshareInterface::m_instance = NULL;
#endif

MeshareInterface* MeshareInterface::GetInstance()
{
	return m_instance;
}

#ifdef ZMD_APP_MESHARE
MeshareInterface::MeshareInterface()
{
}

MeshareInterface::~MeshareInterface()
{
}

int MeshareInterface::init()
{
	return 0;
}

int MeshareInterface::release()
{
	return 0;
}

int MeshareInterface::start()
{
    /* meshare 库代码开始运行 */
    meshare_main();

	return 0;
}

int MeshareInterface::stop()
{
	return 0;
}

int MeshareInterface::handle(MFI_CMD cmd,void* para)
{
#if 0
	switch(cmd)
	{
		case CMD_MSH_FUNC1:
			return Meshare::GetInstance()->meshare_func1();
		case CMD_MSH_FUNC2:
			return Meshare::GetInstance()->meshare_func1();
		case CMD_MSH_FUNC3:
			return Meshare::GetInstance()->meshare_func1();
		default:
			return -1;
	}
#endif
    return 0;
}
#endif

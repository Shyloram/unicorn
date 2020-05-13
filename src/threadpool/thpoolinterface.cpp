#include "thpoolinterface.h"
#include "threadpool.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_THREADPOOL
TPLInterface* TPLInterface::m_instance = new TPLInterface;
#else
TPLInterface* TPLInterface::m_instance = NULL;
#endif

TPLInterface* TPLInterface::GetInstance()
{
	return m_instance;
}

#ifdef ZMD_APP_THREADPOOL
TPLInterface::TPLInterface()
{
}

TPLInterface::~TPLInterface()
{
}

int TPLInterface::init()
{
	ThreadPool *tp = ThreadPool::GetInstance();
	tp->InitThreadPool();
	ITFLOG("TPLInterface init success!\n");
	return 0;
}

int TPLInterface::release()
{
	ThreadPool *tp = ThreadPool::GetInstance();
	tp->DestroyThreadPool();
	ITFLOG("TPLInterface release success!\n");
	return 0;
}

int TPLInterface::start()
{
	return 0;
}

int TPLInterface::stop()
{
	return 0;
}

int TPLInterface::handle(MFI_CMD cmd,void* para)
{
	switch(cmd)
	{
		case CMD_TPL_ADDTASK:
			return ThreadPool::GetInstance() != NULL ?ThreadPool::GetInstance()->AddTask(para) : -1;
		default:
			return -1;
	}
}

#endif

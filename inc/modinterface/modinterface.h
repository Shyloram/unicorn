#ifndef __MOD_INTERFACE__
#define __MOD_INTERFACE__

#include <stdio.h>
#include "modinterfacedef.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_DEBUG_ITF
#define ITFLOG(format, ...)     fprintf(stdout, "[ITFLOG][Func:%s][Line:%d], " format, __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define ITFLOG(format, ...)
#endif
#define ITFERR(format, ...)     fprintf(stdout, "\033[31m[ITFERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)

class Interface
{
	public:
		virtual int init()  = 0;
		virtual int release()  = 0;
		virtual int start()  = 0;
		virtual int stop()  = 0;
		virtual int handle(MFI_CMD cmd,void* para) = 0;
};

class ModInterface
{
	private:
		static ModInterface* m_instance;

	private:
		ModInterface();
		~ModInterface();
		Interface* GetModInstance(MFI_MOD mod);

	public:
		static ModInterface* GetInstance();
		int init();
		int release();
		int start();
		int stop();
		int CMD_handle(MFI_MOD mod,MFI_CMD cmd,void* para = NULL);
};

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */
int mainloop(void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //__MOD_INTERFACE__

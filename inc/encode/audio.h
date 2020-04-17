#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <pthread.h>
#include "zmdconfig.h"

#define NumPerFrm       160
#define ACODEC_FILE     "/dev/acodec"

#ifdef ZMD_APP_DEBUG_AUD
#define AUDLOG(format, ...)     fprintf(stdout, "[AUDLOG][Func:%s][Line:%d], " format, __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define AUDLOG(format, ...)
#endif
#define AUDERR(format, ...)     fprintf(stdout, "\033[31m[AUDERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)

typedef struct
{
	unsigned char* buffer;
	int len;
	bool block;
}ParaEncAdecInfo;

class Audio
{
	public:
		virtual int AudioInit() = 0;
		virtual int AudioRelease() = 0;
		virtual int AudioStart() = 0;
		virtual int AudioStop() = 0;
};

class ZMDAudio : public Audio
{
	private:
		static ZMDAudio* m_instance;
		int			  m_ThreadStat;/*线程状态1:running 0:stop*/
		pthread_t	  m_pid;
		unsigned int  m_TalkDelay;

	private:
		ZMDAudio();
		~ZMDAudio();

	public:
		static ZMDAudio* GetInstance(void);
		virtual int AudioInit();
		virtual int AudioRelease();
		virtual int AudioStart();
		virtual int AudioStop();
		int AudioEncStreamThreadBody(void);
		int AudioDecHandle(void* para);
		int AudioDecSwitch(bool onoff);
};

#endif //__AUDIO_H__

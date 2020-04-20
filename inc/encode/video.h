#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <pthread.h>
#include "zmdconfig.h"

#ifdef ZMD_APP_DEBUG_VID
#define VIDLOG(format, ...)     fprintf(stdout, "[VIDLOG][Func:%s][Line:%d], " format, __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define VIDLOG(format, ...)
#endif
#define VIDERR(format, ...)     fprintf(stdout, "\033[31m[VIDERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)

class Video
{
	public:
		virtual int VideoInit() = 0;
		virtual int VideoRelease() = 0;
		virtual int VideoStart() = 0;
		virtual int VideoStop() = 0;
};

class ZMDVideo : public Video
{
	private:
		static ZMDVideo* m_instance;
		int			  m_ThreadStat;/*线程状态1:running 0:stop*/
		pthread_t	  m_pid;

	private:
		ZMDVideo();
		~ZMDVideo();

	public:
		static ZMDVideo* GetInstance(void);
		virtual int VideoInit();
		virtual int VideoRelease();
		virtual int VideoStart();
		virtual int VideoStop();
		int VideoEncStreamThreadBody(void);
		int RequestIFrame(int ch);
};

#endif //__VIDEO_H__

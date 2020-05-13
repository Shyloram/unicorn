#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "zmdconfig.h"

#ifdef ZMD_APP_DEBUG_TPL
#define TPLLOG(format, ...)     fprintf(stdout, "[TPLLOG][Func:%s][Line:%d], " format, __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define TPLLOG(format, ...)
#endif
#define TPLERR(format, ...)     fprintf(stdout, "\033[31m[TPLERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)

typedef struct task
{
	void *(*run)(void *args);
	void *arg;
	struct task *next;
}ptask_t;

class TaskList
{
	private:
		ptask_t *m_first;
		ptask_t *m_last;

	public:
		TaskList();
		~TaskList();
		int PushTask(ptask_t *task);
		ptask_t* PullTask();
};

class ThreadPool
{
	private:
		static ThreadPool* m_instance;
		pthread_mutex_t m_mutex;
		pthread_cond_t m_cond;
		TaskList l_task;
		int m_sum;
		int m_idle;
		int m_maxidle;
		int g_exit;

		ThreadPool();
		~ThreadPool();

	public:
		static ThreadPool* GetInstance(void);
		int InitThreadPool();
		int DestroyThreadPool();
		void AddIdle();
		void DelIdle();
		void AddSum();
		void DelSum();
		int CheckIdle();
		int CheckExit();
		int AddTask(void *arg);
		ptask_t* GetTask();
		int InitCondition();
		int LockCondition();
		int UnlockCondition();
		int WaitCondition();
		int WakeupCondition();
		int WakeupAllCondition();
		int DestroyCondition();
};

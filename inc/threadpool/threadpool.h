#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef struct task
{
	void *(*run)(void *args);
	void *arg;
	struct task *next;
}task_t;

class TaskList
{
	private:
		task_t *m_first;
		task_t *m_last;

	public:
		TaskList();
		~TaskList();
		int PushTask(task_t *task);
		task_t* PullTask();
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
		task_t* GetTask();
		int InitCondition();
		int LockCondition();
		int UnlockCondition();
		int WaitCondition();
		int WakeupCondition();
		int WakeupAllCondition();
		int DestroyCondition();
};

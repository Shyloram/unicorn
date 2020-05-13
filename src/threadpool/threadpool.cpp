#include "threadpool.h"

/****************************************************************
 *           Class TaskList
 ***************************************************************/
TaskList::TaskList()
{
	m_first = NULL;
	m_last = NULL;
}

TaskList::~TaskList()
{
	ptask_t *task = NULL;
	while(m_first != NULL)
	{
		task = m_first;
		m_first = m_first->next;
		delete task;
	}
}

int TaskList::PushTask(ptask_t *task)
{
	if(m_first == NULL)
	{
		m_first = task;
	}
	else
	{
		m_last->next = task;
	}
	m_last = task;
	return 0;
}

ptask_t* TaskList::PullTask()
{
	ptask_t *task = NULL;
	if(m_first != NULL)
	{
		task = m_first;
		m_first = m_first->next;
		return task;
	}
	return NULL;
}
/****************************************************************
 *           Class ThreadPool
 ***************************************************************/
int ThreadPool::InitCondition()
{
	int status;
	if((status = pthread_mutex_init(&m_mutex, NULL)))
		return status;
	if((status = pthread_cond_init(&m_cond, NULL)))
		return status;
	return 0;
}

int ThreadPool::LockCondition()
{
	return pthread_mutex_lock(&m_mutex);
}

int ThreadPool::UnlockCondition()
{
	return pthread_mutex_unlock(&m_mutex);
}

int ThreadPool::WaitCondition()
{
	return pthread_cond_wait(&m_cond, &m_mutex);
}

int ThreadPool::WakeupCondition()
{
	return pthread_cond_signal(&m_cond);
}

int ThreadPool::WakeupAllCondition()
{
	return pthread_cond_broadcast(&m_cond);
}

int ThreadPool::DestroyCondition()
{
	int status;
	if((status = pthread_mutex_destroy(&m_mutex)))
		return status;
	if((status = pthread_cond_destroy(&m_cond)))
		return status;
	return 0;
}

void *thread_handle(void *arg)
{
	TPLLOG("thread %d is starting\n", (int)pthread_self());
	ThreadPool *pool = (ThreadPool*)arg;
	ptask_t *t = NULL;

	while(1)
	{
		pool->LockCondition();
		pool->AddIdle();
		while((t = pool->GetTask()) == NULL && pool->CheckExit() != 1)
		{
			TPLLOG("thread %d is waiting\n", (int)pthread_self());
			pool->WaitCondition();
		}
		pool->DelIdle();
		if(t != NULL)
		{
			pool->UnlockCondition();
			TPLLOG("thread %d is dealing the task\n", (int)pthread_self());
			t->run(t->arg);
			delete t;
			pool->LockCondition();
		}
		if(pool->CheckIdle() || pool->CheckExit())
		{
			pool->DelSum();
			pool->UnlockCondition();
			break;
		}
		pool->UnlockCondition();
	}

	TPLLOG("thread %d is exiting\n", (int)pthread_self());
	return NULL;
}

ThreadPool* ThreadPool::m_instance = new ThreadPool;
ThreadPool* ThreadPool::GetInstance(void)
{
	return m_instance;
}

ThreadPool::ThreadPool()
{
}

ThreadPool::~ThreadPool()
{
}

int ThreadPool::InitThreadPool()
{
	InitCondition();
	m_sum = 0;
	m_idle = 0;
	m_maxidle = 5;
	g_exit = 0;
	return 0;
}

int ThreadPool::DestroyThreadPool()
{
	TPLLOG("destory thread pool\n");
	if(g_exit)
	{
		return 0;
	}
	LockCondition();
	g_exit = 1;
	if(m_sum > 0)
	{
		if(m_idle > 0)
		{
			WakeupAllCondition();
		}
		while(m_sum)
		{
			UnlockCondition();
			TPLLOG("sum:%d\n",m_sum);
			sleep(1);
			LockCondition();
		}
	}
	UnlockCondition();
	DestroyCondition();
	return 0;
}

int ThreadPool::AddTask(void *arg)
{
	pthread_t pid;
	ptask_t *task = new ptask_t;
	ptask_t *input = (ptask_t*)arg;
	task->run = input->run;
	task->arg = input->arg;
	task->next = NULL;

	LockCondition();
	l_task.PushTask(task);

	if(m_idle > 0)
	{
		WakeupCondition();
	}
	else
	{
		if(pthread_create(&pid,NULL,thread_handle,this))
		{
			TPLERR("pthread create failed\n");
		}
		else
		{
			AddSum();
		}
	}
	UnlockCondition();
	return 0;
}

ptask_t* ThreadPool::GetTask()
{
	ptask_t *t = NULL;
	t = l_task.PullTask();
	return t;
}

void ThreadPool::AddIdle()
{
	m_idle++;
}

void ThreadPool::DelIdle()
{
	m_idle--;
}

void ThreadPool::AddSum()
{
	m_sum++;
}

void ThreadPool::DelSum()
{
	m_sum--;
}
int ThreadPool::CheckIdle()
{
	return m_idle >= m_maxidle;
}

int ThreadPool::CheckExit()
{
	return g_exit;
}

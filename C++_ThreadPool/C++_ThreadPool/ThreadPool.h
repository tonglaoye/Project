#pragma once
#include"TaskQueue.h"
#include"TaskQueue.cpp"
#include<iostream>
#include<string>
#include<string.h>
#include<unistd.h>
using namespace std;
template<typename T>
class ThreadPool
{
public:
	ThreadPool(int min,int max);
	~ThreadPool();

	void addTask(Task<T> task);

private:
	static void* worker(void* arg);
	static void* manager(void* arg);
	void exitThread();
private:
	TaskQueue<T>* taskQ; //任务队列
	pthread_mutex_t mutexPool;//锁
	pthread_cond_t notEmpty;//条件变量   任务队列是否为空
	pthread_t* threadIDs;   //线程数组
	pthread_t managerID;    //管理者线程
	int m_min;				//最小线程数
	int m_max;				//最大线程数
	int m_live;				//存活线程数
	int m_exit;				//退出线程数
	int m_busy;				//忙的线程数

	bool shutdown = false;			//是否关闭线程池

	static const int NUMBER = 2; //每次创建或销毁线程数
};

template<typename T>
inline ThreadPool<T>::ThreadPool(int min,int max)
{
	do
	{
		//创建任务队列
		taskQ = new TaskQueue<T>;
		if (taskQ == nullptr)
		{
			cout << "new taskQ failed...." << endl;
			break;
		}
		//创建线程数组
		m_min = min;
		m_max = max;
		m_live = min;
		m_busy = 0;
		m_exit = 0;
		threadIDs = new pthread_t[max];
		if (threadIDs == nullptr)
		{
			cout << "threadIds new failed ...." << endl;
			break;
		}
		//初始化线程数组全部置零
		memset(threadIDs, 0, sizeof(pthread_t) * max);
		//初始化锁和条件变量
		if (pthread_mutex_init(&mutexPool, NULL) != 0 || pthread_cond_init(&notEmpty, NULL) != 0)
		{
			cout << "init mutex or cond failed ...." << endl;
			break;
		}
		//创建线程
		pthread_create(&managerID, NULL, manager, this);
		cout << "create manager ID：" << to_string(managerID) << endl;

		for (int i = 0; i < min; ++i)
		{
			pthread_create(&threadIDs[i], NULL, worker, this);
			cout << "create threads ID：" << to_string(threadIDs[i]) << endl;
		}
	} while (0);
}
template<typename T>
inline ThreadPool<T>::~ThreadPool()
{
	shutdown = true;
	pthread_join(managerID, NULL);
	for (int i = 0; i < m_live; ++i)//唤醒所有的阻塞工作线程
	{
		pthread_cond_signal(&notEmpty);
	}
	if (taskQ)
	{
		delete taskQ;
	}

	if (threadIDs)
	{
		delete[] threadIDs;
	}
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);
}
template<typename T>
inline void ThreadPool<T>::addTask(Task<T> task)
{
	if (shutdown)
	{
		return;
	}
	taskQ->addTask(task);
	pthread_cond_signal(&notEmpty);
}

template<typename T>
inline void* ThreadPool<T>::worker(void* arg)
{
	ThreadPool<T>* pool = static_cast<ThreadPool<T>*>(arg);
	while (1)
	{
		pthread_mutex_lock(&pool->mutexPool);
		while ((pool->taskQ->getsize() == 0) && (!pool->shutdown))
		{
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);//如果任务队列为空 就阻塞
			//解除阻塞时判断是否时要销毁线程
			if (pool->m_exit > 0)
			{	
				pool->m_exit--;
				if (pool->m_live > pool->m_min)
				{
					pool->m_live--;
					pthread_mutex_unlock(&pool->mutexPool);
					pool->exitThread();
				}
			}
		}
		//判断线程池是否关闭
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			pool->exitThread();
		}
		//都不是就开始取出任务工作
		Task<T> task = pool->taskQ->getTask();

		pool->m_busy++;
		pthread_mutex_unlock(&pool->mutexPool);
		//开始工作
		task.f(task.arg);
		cout << "thread: " << to_string(pthread_self()) << "start working....." << endl;
		delete task.arg;
		arg = nullptr;
		cout << "thread: " << to_string(pthread_self()) << "end working....." << endl;

		pthread_mutex_lock(&pool->mutexPool);
		pool->m_busy--;
		pthread_mutex_unlock(&pool->mutexPool);

	}
	return nullptr;
}

template<typename T>
inline void* ThreadPool<T>::manager(void *arg)
{
	ThreadPool<T>* pool = static_cast<ThreadPool<T>*>(arg);
	while (!pool->shutdown)
	{
		sleep(3);//每隔三秒检测一次
		pthread_mutex_lock(&pool->mutexPool);
		int tasksize = pool->taskQ->getsize();
		int livenum = pool->m_live;
		int busynum = pool->m_busy;
		pthread_mutex_unlock(&pool->mutexPool);
		//判断是否需要改变线程数量
		if (tasksize > livenum && livenum < pool->m_max)
		{
			
			pthread_mutex_lock(&pool->mutexPool);
			int num = 0;
			for (int i = 0; i < pool->m_max && pool->m_live < pool->m_max && num < NUMBER; ++i)
			{
				if (pool->threadIDs[i] == 0)
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					++num;
					pool->m_live++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		if (busynum * 2 < livenum && livenum > pool->m_min)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->m_exit = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			for (int i = 0; i < NUMBER; ++i)
			{
				pthread_cond_signal(&pool->notEmpty);//让空闲线程自杀
			}

		}
	}
	return nullptr;
}

template<typename T>
inline void ThreadPool<T>::exitThread()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < m_max; ++i)
	{
		if (threadIDs[i] == tid)
		{
			cout << "exitThread( ) func thread :" << to_string(tid) << "exiting ...." << endl;
			threadIDs[i] = 0;
			break;
		}
	}
	//pthread_detach(tid);
	pthread_exit(NULL);
}

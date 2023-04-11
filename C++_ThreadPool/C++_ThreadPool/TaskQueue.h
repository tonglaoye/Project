#pragma once
#include<pthread.h>
#include<queue>
using namespace std;
using funcation = void(*)(void*);

template<typename T>
struct Task
{
	Task():f(nullptr),arg(nullptr){ }
	Task(funcation fun, void* a) : f(fun), arg((T*)a){ }
	funcation f;
	T* arg;
};

template<typename T>

class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	void addTask(Task<T> task);//向队列中添加任务
	void addTask(funcation f, void* arg);
	Task<T> getTask();        //从队列中取出任务

	inline int getsize()//得到队列中任务数量
	{
		return m_queue.size();
	}

private:
	pthread_mutex_t m_mutex;
	queue<Task<T>> m_queue;
};



template<typename T>
TaskQueue<T>::TaskQueue()
{
	pthread_mutex_init(&m_mutex, NULL);//创建锁
}
template<typename T>
TaskQueue<T>::~TaskQueue()
{
	pthread_mutex_destroy(&m_mutex);
}
template<typename T>
void TaskQueue<T>::addTask(Task<T> task)
{
	pthread_mutex_lock(&m_mutex);
	m_queue.push(task);
	pthread_mutex_unlock(&m_mutex);
}
template<typename T>
void TaskQueue<T>::addTask(funcation f, void* arg)
{
	pthread_mutex_lock(&m_mutex);
	m_queue.push(Task<T>(f, arg));
	pthread_mutex_unlock(&m_mutex);
}
template<typename T>
Task<T> TaskQueue<T>::getTask()
{
	Task<T> task;
	pthread_mutex_lock(&m_mutex);
	task = m_queue.front();
	m_queue.pop();
	pthread_mutex_unlock(&m_mutex);
	return task;
}



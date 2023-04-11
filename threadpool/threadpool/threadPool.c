#define _CRT_SECURE_NO_WARNINGS 1
#include"threadPool.h"

//static int COUNT = 0;
const int NUMBER = 2;
//为了生成库是不被看到如何实现的所以在.c文件中实现

//任务
typedef struct Task
{
	void (*function)(void* arg);//做任务
	void* arg;//任务参数
}Task;

//线程池结构体
typedef struct ThreadPool
{
	//任务队列
	Task* taskQ;
	int qcapacity; //容量
	int qsize;     //当前任务数量
	int qfront;    //队头
	int qback;     //队尾

	pthread_t managerID;//管理者线程
	pthread_t* threadIDs;//工作线程
	int minNum;      //最小线程数
	int maxNum;      //最大线程数
	int busyNum;     //忙的线程数
	int liveNum;     //存活的线程
	int exitNum;     //要销毁的线程

	pthread_mutex_t mutexPool;//锁整个线程池
	pthread_mutex_t mutexBusy;//锁忙线程数的锁   因为busyNum经常变化所以给他加个锁
	pthread_cond_t notFull;  //任务队列是不是满了
	pthread_cond_t notEmpty;//任务队列是不是空了
	
	int shutdown;  // 要不要销毁线程池  要1  不要0
}ThreadPool;

ThreadPool* threadPoolCreate(int min, int max, int size)
{
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	do
	{
		if (pool == NULL)
		{
			printf("malloc pool failed...\n");
			break;
		}
		//去为线程数组申请空间
		pool->threadIDs = (pthread_t *)malloc(sizeof(pthread_t) * max);
		if (pool->threadIDs == NULL)
		{
			printf("malloc pthreadIDs failed...\n");
			break;
		}
		//将申请来的线程数组全部置零  表示没有申请过任何一个线程
		memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
		pool->minNum = min;
		pool->maxNum = max;
		pool->busyNum = 0;
		pool->exitNum = 0;
		pool->liveNum = min; //和最小数量相等
		//创建锁和条件变量 并判断是否创建成功
		if (pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0)
		{
			printf("mutex or cond init failed...\n");
			break;
		}
		//给任务队列申请空间
		pool->taskQ = (Task*)malloc(sizeof(Task) * size);
		if (pool->taskQ == NULL)
		{
			printf("malloc taskQ failed...");
			break;
		}
		pool->qcapacity = size;
		pool->qsize = 0;
		pool->qback = 0;
		pool->qfront = 0;

		pool->shutdown = 0;

		//创建管理者线程
		pthread_create(&pool->managerID, NULL, manager, pool);
		//printf("%d------------------------------------------------------------", COUNT++);

		//pthread_detach(pool->managerID);
		//创建工作线程
		for (int i = 0; i < min; ++i)
		{
			pthread_create(&pool->threadIDs[i], NULL, worker, pool);
			//printf("%d------------------------------------------------------------", COUNT++);


			//pthread_detach(pool->threadIDs[i]);
		}
		//全部成功返回线程池的地址
		return pool;

	} while (0);
	//过程中有失败的就会跳出来释放资源和返回空指针
	if (pool && pool->threadIDs)
	{
		printf("free threadIDs.............................\n");
		free(pool->threadIDs);
	}
	
	if (pool && pool->taskQ)
		free(pool->taskQ);
	if (pool)
		free(pool);

	return NULL;
}

int threadPoolDestroy(ThreadPool* pool)
{
	//if (pool == NULL)
	//	return -1;
	//pool->shutdown = 1;//关闭线程池
	//pthread_join(pool->managerID, NULL);//阻塞回收管理者线程
	////唤醒阻塞的消费线程  让其自杀
	//for (int i = 0; i < pool->liveNum; ++i)
	//{
	//	pthread_cond_signal(&pool->notEmpty);
	//}
	//printf("555555555555555555555555555555555555555\n");
	//printf("%ld.............................\n", pool->threadIDs[0]);

	
	//释放堆内存
	//printf("come on111111 .............................\n");

	if (pool->taskQ)
	{
		free(pool->taskQ);
		pool->taskQ = NULL;
	}
		
	//printf("come on 222222.............................\n");

	if (pool && pool->threadIDs )
	{
		//printf("free threadIDs.............................\n");
		free(pool->threadIDs);
		pool->threadIDs = NULL;
	}
	//销毁锁和条件变量
	//if (!pool->liveNum)
	//{
		//printf("come on .............................\n");
	pthread_mutex_lock(&pool->mutexBusy);
	pthread_mutex_destroy(&pool->mutexBusy);
	pthread_mutex_lock(&pool->mutexPool);
	pthread_mutex_destroy(&pool->mutexPool);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);
		//printf("ok.............................\n");
	//}
	if (pool)
	{
		free(pool);
		pool = NULL;
		//printf("pool ok.............................\n");
	}
	//printf("pool ok.............................\n");

	return 0;
}

void thread_Des(ThreadPool* pool)
{
	if (pool == NULL)
		return -1;
	pool->shutdown = 1;//关闭线程池
	pthread_join(pool->managerID, NULL);//阻塞回收管理者线程
	//pthread_detach(pool->managerID);
	//唤醒阻塞的消费线程  让其自杀
	for (int i = 0; i < pool->liveNum; ++i)
	{
		pthread_cond_signal(&pool->notEmpty);
	}
	//sleep(2);
	
	threadPoolDestroy(pool);
}


void pthreadPoolAdd(ThreadPool* pool, void(*func)(void* ), void* arg)
{
	pthread_mutex_lock(&pool->mutexPool);
	while (pool->qcapacity == pool->qsize && !pool->shutdown)
	{
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);//阻塞生产线程
	}
	if (pool->shutdown)
	{
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}
	//添加任务
	pool->taskQ[pool->qback].function = func;
	pool->taskQ[pool->qback].arg = arg;
	pool->qback = (pool->qback + 1) % pool->qcapacity;
	pool->qsize++;

	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->mutexPool);

}


//工作函数
void *worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown)
	{
		//先上锁
		pthread_mutex_lock(&pool->mutexPool);
		//判断任务队列是否为空 并且 销不销毁线程池
		while (pool->qsize == 0 && !pool->shutdown)
		{
			//printf("work listen.............................\n");
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);//如果为空并且不销毁线程池就阻塞等待条件发生
			//printf("work waiting.....................\n"); 
			//判断是不是要销毁线程
			if (pool->exitNum > 0)
			{
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->mutexPool);
					//printf("1111111111111111111111111111111111\n");
					//printf("%ld.............................\n", pool->threadIDs[0]);
					pthreadExit(pool);
				}
			}
		}
		//判断线程池是不是关闭了
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			//printf("2222222222222222222222222222222222222222222222\n");
			//printf("%ld.............................\n", pool->threadIDs[0]);
			pthreadExit(pool);
		}
		//从任务队列取一个函数
		Task task;
		task.function = pool->taskQ[pool->qfront].function;
		task.arg = pool->taskQ[pool->qfront].arg;
		//移动头节点
		pool->qfront = (pool->qfront + 1) % pool->qcapacity;
		pool->qsize--;

		//向生产出发出信号 
		pthread_cond_signal(&pool->notFull);
		//解锁
		pthread_mutex_unlock(&pool->mutexPool);

		printf("thread %ld start working...\n" ,pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexBusy);
		task.function(task.arg);
		//free(task.arg);
		task.arg = NULL;
		printf("thread %ld end working...\n", pthread_self());

		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexBusy);
	}

	return NULL;
}

//管理函数
void* manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown)
	{
		sleep(3);//每三秒检测一次
		//取出线程池中的任务数量 和当前线程数量
		pthread_mutex_lock(&pool->mutexPool);
		int qsize = pool->qsize;
		int livenum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);
		pthread_mutex_lock(&pool->mutexBusy);//取出在忙的线程数
		int busynum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);
		//添加线程
		if (qsize > livenum && livenum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			int count = 0;
			for (int i = 0; i < pool->maxNum && livenum < pool->maxNum && count < NUMBER; ++i)
			{
				if (pool->threadIDs[i] == 0)
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					//printf("%d------------------------------------------------------------", COUNT++);

					//pthread_detach(pool->threadIDs[i]);
					++count;
					pool->liveNum++;
					pool->liveNum++;
					pool->liveNum++;
					pool->liveNum++;
					pool->liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}
		//销毁线程
		if (busynum * 2 < livenum && livenum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			for (int i = 0; i < NUMBER && livenum > pool->minNum; ++i)//让线程自杀
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}	
	}

	return NULL;
}
//退出函数
void pthreadExit(ThreadPool* pool)
{
	//printf("4444444444444444444444444444444444444444444444\n");
	//printf("%ld.............................\n", pool->threadIDs[0]);
	pthread_t tid = pthread_self();
	//pthread_mutex_lock(&pool->mutexPool);
	for (int i = 0; i < pool->maxNum; ++i)
	{
		if (pool->threadIDs[i] == tid)
		{
			pool->threadIDs[i] = 0;//将要退出的线程所占用的线程数组中的线程号清零
			printf("threadExit called , %ld exiting...\n", tid);
			break;
		}
	}
	//pthread_mutex_unlock(&pool->mutexPool);
	//printf("3333333333333333333333333333333333333333\n");
	//printf("%ld.............................\n", pool->threadIDs[0]);
	pthread_exit(NULL);//让这个线程退出
	//pthread_detach(tid);
}

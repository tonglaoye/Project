#define _CRT_SECURE_NO_WARNINGS 1
#include"threadPool.h"

//static int COUNT = 0;
const int NUMBER = 2;
//Ϊ�����ɿ��ǲ����������ʵ�ֵ�������.c�ļ���ʵ��

//����
typedef struct Task
{
	void (*function)(void* arg);//������
	void* arg;//�������
}Task;

//�̳߳ؽṹ��
typedef struct ThreadPool
{
	//�������
	Task* taskQ;
	int qcapacity; //����
	int qsize;     //��ǰ��������
	int qfront;    //��ͷ
	int qback;     //��β

	pthread_t managerID;//�������߳�
	pthread_t* threadIDs;//�����߳�
	int minNum;      //��С�߳���
	int maxNum;      //����߳���
	int busyNum;     //æ���߳���
	int liveNum;     //�����߳�
	int exitNum;     //Ҫ���ٵ��߳�

	pthread_mutex_t mutexPool;//�������̳߳�
	pthread_mutex_t mutexBusy;//��æ�߳�������   ��ΪbusyNum�����仯���Ը����Ӹ���
	pthread_cond_t notFull;  //��������ǲ�������
	pthread_cond_t notEmpty;//��������ǲ��ǿ���
	
	int shutdown;  // Ҫ��Ҫ�����̳߳�  Ҫ1  ��Ҫ0
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
		//ȥΪ�߳���������ռ�
		pool->threadIDs = (pthread_t *)malloc(sizeof(pthread_t) * max);
		if (pool->threadIDs == NULL)
		{
			printf("malloc pthreadIDs failed...\n");
			break;
		}
		//�����������߳�����ȫ������  ��ʾû��������κ�һ���߳�
		memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
		pool->minNum = min;
		pool->maxNum = max;
		pool->busyNum = 0;
		pool->exitNum = 0;
		pool->liveNum = min; //����С�������
		//���������������� ���ж��Ƿ񴴽��ɹ�
		if (pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0)
		{
			printf("mutex or cond init failed...\n");
			break;
		}
		//�������������ռ�
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

		//�����������߳�
		pthread_create(&pool->managerID, NULL, manager, pool);
		//printf("%d------------------------------------------------------------", COUNT++);

		//pthread_detach(pool->managerID);
		//���������߳�
		for (int i = 0; i < min; ++i)
		{
			pthread_create(&pool->threadIDs[i], NULL, worker, pool);
			//printf("%d------------------------------------------------------------", COUNT++);


			//pthread_detach(pool->threadIDs[i]);
		}
		//ȫ���ɹ������̳߳صĵ�ַ
		return pool;

	} while (0);
	//��������ʧ�ܵľͻ��������ͷ���Դ�ͷ��ؿ�ָ��
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
	//pool->shutdown = 1;//�ر��̳߳�
	//pthread_join(pool->managerID, NULL);//�������չ������߳�
	////���������������߳�  ������ɱ
	//for (int i = 0; i < pool->liveNum; ++i)
	//{
	//	pthread_cond_signal(&pool->notEmpty);
	//}
	//printf("555555555555555555555555555555555555555\n");
	//printf("%ld.............................\n", pool->threadIDs[0]);

	
	//�ͷŶ��ڴ�
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
	//����������������
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
	pool->shutdown = 1;//�ر��̳߳�
	pthread_join(pool->managerID, NULL);//�������չ������߳�
	//pthread_detach(pool->managerID);
	//���������������߳�  ������ɱ
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
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);//���������߳�
	}
	if (pool->shutdown)
	{
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}
	//�������
	pool->taskQ[pool->qback].function = func;
	pool->taskQ[pool->qback].arg = arg;
	pool->qback = (pool->qback + 1) % pool->qcapacity;
	pool->qsize++;

	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->mutexPool);

}


//��������
void *worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown)
	{
		//������
		pthread_mutex_lock(&pool->mutexPool);
		//�ж���������Ƿ�Ϊ�� ���� ���������̳߳�
		while (pool->qsize == 0 && !pool->shutdown)
		{
			//printf("work listen.............................\n");
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);//���Ϊ�ղ��Ҳ������̳߳ؾ������ȴ���������
			//printf("work waiting.....................\n"); 
			//�ж��ǲ���Ҫ�����߳�
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
		//�ж��̳߳��ǲ��ǹر���
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			//printf("2222222222222222222222222222222222222222222222\n");
			//printf("%ld.............................\n", pool->threadIDs[0]);
			pthreadExit(pool);
		}
		//���������ȡһ������
		Task task;
		task.function = pool->taskQ[pool->qfront].function;
		task.arg = pool->taskQ[pool->qfront].arg;
		//�ƶ�ͷ�ڵ�
		pool->qfront = (pool->qfront + 1) % pool->qcapacity;
		pool->qsize--;

		//�������������ź� 
		pthread_cond_signal(&pool->notFull);
		//����
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

//������
void* manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown)
	{
		sleep(3);//ÿ������һ��
		//ȡ���̳߳��е��������� �͵�ǰ�߳�����
		pthread_mutex_lock(&pool->mutexPool);
		int qsize = pool->qsize;
		int livenum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);
		pthread_mutex_lock(&pool->mutexBusy);//ȡ����æ���߳���
		int busynum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);
		//����߳�
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
		//�����߳�
		if (busynum * 2 < livenum && livenum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			for (int i = 0; i < NUMBER && livenum > pool->minNum; ++i)//���߳���ɱ
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}	
	}

	return NULL;
}
//�˳�����
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
			pool->threadIDs[i] = 0;//��Ҫ�˳����߳���ռ�õ��߳������е��̺߳�����
			printf("threadExit called , %ld exiting...\n", tid);
			break;
		}
	}
	//pthread_mutex_unlock(&pool->mutexPool);
	//printf("3333333333333333333333333333333333333333\n");
	//printf("%ld.............................\n", pool->threadIDs[0]);
	pthread_exit(NULL);//������߳��˳�
	//pthread_detach(tid);
}

#pragma once
#include<pthread.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/wait.h>
typedef struct ThreadPool ThreadPool;
//�����̳߳�
ThreadPool* threadPoolCreate(int min, int max, int size);
//�����̳߳�
int threadPoolDestroy(ThreadPool*);
//���̳߳��м�������
void pthreadPoolAdd(ThreadPool* , void (*func)(void *),void *);

//��������
void *worker(void* arg);
//�����߳�
void thread_Des(ThreadPool* pool);


//������
void* manager(void *arg);

//�˳�����
void pthreadExit(ThreadPool* pool);

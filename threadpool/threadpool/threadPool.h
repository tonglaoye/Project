#pragma once
#include<pthread.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/wait.h>
typedef struct ThreadPool ThreadPool;
//创建线程池
ThreadPool* threadPoolCreate(int min, int max, int size);
//销毁线程池
int threadPoolDestroy(ThreadPool*);
//向线程池中加入任务
void pthreadPoolAdd(ThreadPool* , void (*func)(void *),void *);

//工作函数
void *worker(void* arg);
//销毁线程
void thread_Des(ThreadPool* pool);


//管理函数
void* manager(void *arg);

//退出函数
void pthreadExit(ThreadPool* pool);

#include <stdio.h>
#include"threadPool.h"
#include<pthread.h>
void print(void* arg)
{
    int num = *(int*)arg;
    printf("我是 %d 号。。。\n", num);
    sleep(1);
}


int main()
{
    ThreadPool* pool = threadPoolCreate(2, 10, 100);
    for (int i = 0; i < 100; ++i)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        pthreadPoolAdd(pool, print, num);
    }
    sleep(30);
    printf("开始销毁线程池。。。。。。。。。。。。。。。。。\n");
    //threadPoolDestroy(pool);
    thread_Des(pool);
    
    return 0;
}
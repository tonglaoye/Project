#define _CRT_SECURE_NO_WARNINGS 1
#include"ThreadPool.h"
#include<sys/wait.h>
void print(void *arg)
{
	int num = *static_cast<int*>(arg);
	cout << "working................................" << num << endl;
	sleep(1);
}

int main()
{
	ThreadPool<int> pool(2, 10);
	for (int i = 0; i < 100; ++i)
	{
		int* num = new int(i);
		pool.addTask(Task<int>(print, num));
	}
	sleep(20);
	cout << "end................." << endl;
	wait(NULL);
	cout << "end.....ok............" << endl;

	return 0;
}
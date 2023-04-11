#pragma once
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<strings.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<dirent.h>
#include<assert.h>
#include<ctype.h>
#include<pthread.h>
//����һ�������߳���Ϣ�ṹ��
typedef struct Info
{
	int fd;
	int epfd;
	pthread_t tid;
}Info;

#define EVS_SIZE 1024//�����洢�����¼��Ľڵ�������С
//��ʼ���׽���
int initListenFd(unsigned short port);
//����epoll
int epollrun(int lfd);
//�Ϳͻ��˽�������
//int acceptClient(int epfd, int lfd);
void* acceptClient(void* arg);
//����http������Ϣ
void* recvHttpRequest(void* arg);
//������������Ϣ
int parseRequestLine(const char* line, int cfd);
//�����ļ�
int sendFile(const char* fileName, int cfd);
//������Ӧͷ��״̬�� + ��Ӧͷ��
int sendHeadMsg(int cfd, int status, const char* descr, const char *type, int length);
//�õ��ļ����ͷ�
const char* getFileType(const char* filename);
//����Ŀ¼
int sendDir(const char* dirname, int cfd);
//��ʮ�����Ƶ���ת��������
int hexToInt(char c);
//����
//to �洢����֮������ݣ���������   from Ϊԭʼ���� Ϊ�������
void decodeMsg(char* to, char* from);
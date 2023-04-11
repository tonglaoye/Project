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
//定义一个传进线程信息结构体
typedef struct Info
{
	int fd;
	int epfd;
	pthread_t tid;
}Info;

#define EVS_SIZE 1024//用来存储触发事件的节点的数组大小
//初始化套接字
int initListenFd(unsigned short port);
//启动epoll
int epollrun(int lfd);
//和客户端建立连接
//int acceptClient(int epfd, int lfd);
void* acceptClient(void* arg);
//接收http请求信息
void* recvHttpRequest(void* arg);
//解析请求行信息
int parseRequestLine(const char* line, int cfd);
//发送文件
int sendFile(const char* fileName, int cfd);
//发送响应头（状态行 + 响应头）
int sendHeadMsg(int cfd, int status, const char* descr, const char *type, int length);
//得到文件类型符
const char* getFileType(const char* filename);
//发送目录
int sendDir(const char* dirname, int cfd);
//将十六进制的数转换成整形
int hexToInt(char c);
//解码
//to 存储解码之后的数据，传出参数   from 为原始数据 为传入参数
void decodeMsg(char* to, char* from);
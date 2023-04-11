#define _CRT_SECURE_NO_WARNINGS 1
#include "Server.h"



int initListenFd(unsigned short port)
{ 
	//创建套接字
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket");
		return -1;
	}
	//设置端口复用
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1)
	{
		perror("setsockopt");
		return -1;
	}
	//绑定
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1)
	{
		perror("bind");
		return -1;
	}
	//监听
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	printf("start listen.....\n");
	return lfd;
}

int epollrun(int lfd)
{
	printf("start epollrun.....\n");

	//创建epoll实例
	int epfd = epoll_create(1);
	if (epfd == -1)
	{
		perror("epoll_create");
		return -1;
	}
	//上树
	struct epoll_event ev;
	ev.data.fd = lfd;
	ev.events = EPOLLIN;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	//创建一个用来存储触发事件的节点的数组
	struct epoll_event evs[EVS_SIZE];
	//循环监听
	while(1)
	{
		
		int num = epoll_wait(epfd, evs, EVS_SIZE, -1);
		for (int i = 0; i < num; ++i)
		{
			Info* info = (Info*)malloc(sizeof(Info));
			int fd = evs[i].data.fd;
			info->epfd = epfd;
			info->fd = fd;
			if (fd == lfd)
			{
				printf("come accept.....\n");

				//提取连接  建立新连接
				//acceptClient(epfd, lfd);
				pthread_create(&info->tid, NULL, acceptClient, info);
				int ret = pthread_detach(info->tid);//设置线程分离属性
				if (ret != 0)
				{
					perror("pthread_detach");
				}
				//printf("set pthread_detach success....\n");
			}
			else
			{
				//接收数据
				//recvHttpRequest(epfd, fd);
				pthread_create(&info->tid, NULL, recvHttpRequest, info);
				int ret = pthread_detach(info->tid);
				if (ret != 0)
				{
					perror("pthread_detach");
				}
				//printf("set pthread_detach success....\n");

			}
		}
	}
	return 0;
}

//int acceptClient(int epfd, int lfd)
void *acceptClient(void *arg)
{
	Info* info = (Info*)arg;
	int cfd = accept(info->fd, NULL, NULL);
	if (cfd == -1)
	{
		perror("accept");
		return -1;
	}
	//将cfd 设置成非阻塞
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	struct epoll_event ev;
	ev.data.fd = cfd;
	ev.events = EPOLLIN | EPOLLET;//监听读事件并设置成边缘触发
	//上树
	int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	printf("start accept....\n");
	free(info);
	return 0;
}

//int recvHttpRequest(int epfd, int cfd)
void* recvHttpRequest(void* arg)
{
	Info* info = (Info*)arg;
	//printf("start recv data.....\n");
	int len = 0;
	int total = 0;
	char tmp[1024] = "";
	char buf[4096] = "";
	while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0)
	{
		if (total + len < sizeof buf)
		{
			memcpy(buf + total, tmp, len);
		}
		total += len;
	}
	if (len == -1 && errno == EAGAIN)
	{
		//printf("parseRequestLine.....\n");

		//解析请求行
		char* ptr = strstr(buf, "\r\n");//找到第一个\r\n 便是第一行的截至位置
		int reqLen = ptr - buf;// 求出第一行到\r 的长度
		buf[reqLen] = '\0';// 给第一行\r前加\0
		parseRequestLine(buf, info->fd);
	}
	else if (len == 0)
	{
		//客户端断开连接
		epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
		close(info->fd);
	}
	else
	{
		perror("recv");
	}
	free(info);
	return 0;
}

int parseRequestLine(const char* line, int cfd)
{
	//printf(" start parseRequestLine.....\n");

	char method[12] = "";
	char path[1024] = "";
	//printf("%s\n", line);
	sscanf(line, "%[^ ] %[^ ]", method, path);
	printf("%s\n%s\n", method, path);
	//printf("success parse.....\n");

	if (strcasecmp(method, "get") != 0)
	{
		return -1;
	}
	decodeMsg(path, path);//解码 
	printf("%s\n%s\n", method, path);

	char* file = NULL;
	if (strcmp(path, "/") == 0)
	{
		//printf("file = ./.....\n");

		file = "./";
	}
	else
	{
		//printf("file = path + 1.....\n");

		file = path + 1;
	}
	//获得文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		//printf("404.....\n");

		//文件不存在 返回 404 界面
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//printf("send head...\n");

		sendFile("404.html", cfd);
		//printf("send file404...\n");
		return 0;
	}
	if (S_ISDIR(st.st_mode))
	{
		//printf("send dir.....\n");

		//把本地目录的内容返回给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		//printf("send head...\n");
		sendDir(file, cfd);
		
	}
	else
	{
		printf("send file.....\n");

		// 把文件的内容返回给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}
	//printf("end parseRequestLine.....\n");

	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	//打开文件
	int fd = open(fileName, O_RDONLY);
	assert(fd > 0);
	//求文件大小
	int len = lseek(fd, 0, SEEK_END);//将文件指针移动到了文件末尾
	lseek(fd, 0, SEEK_SET);// 将文件指针在移动到头
	off_t offset = 0;
	//发送文件
	while (offset < len)
	{
		int ret = sendfile(cfd, fd, &offset, len);
		printf("ret = %d", ret);
		usleep(10);//让浏览器歇会
	}
	close(fd);
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	//printf("start send head...\n");

	char buf[4096] = "";
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);//状态行  版本 状态  描述
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type);// 数据类型
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);// 数据长度
	printf("come send head...\n");

	int n = send(cfd, buf, strlen(buf), 0);
	if (n < 0)
	{
		printf("send error...\n");

	}
	write(STDOUT_FILENO, buf, strlen(buf));
	//printf("end send head... %d\n",n);

	return 0;
}

const char* getFileType(const char* filename)
{
	const char* tmp = strrchr(filename, '.');
	if (tmp == NULL)
		return "text/plain; charset=utf-8";//纯文本
	if (strcasecmp(tmp, ".html") == 0 || strcasecmp(tmp, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcasecmp(tmp, ".jpg") == 0 || strcasecmp(tmp, ".jpeg") == 0)
		return "image/jpeg";
	if (strcasecmp(tmp, ".gif") == 0)
		return "image/gif";
	if (strcasecmp(tmp, ".png") == 0)
		return "image/png";
	if (strcasecmp(tmp, ".css") == 0)
		return "text/css";
	if (strcasecmp(tmp, ".au") == 0)
		return "audio/basic";
	if (strcasecmp(tmp, ".wav") == 0)
		return "audio/wav";
	if (strcasecmp(tmp, ".avi") == 0)
		return "video/x-msvideo";
	if (strcasecmp(tmp, ".mov") == 0)
		return "video/quicktime";
	if (strcasecmp(tmp, ".mpeg") == 0)
		return "video/mpeg";
	if (strcasecmp(tmp, ".vrml") == 0)
		return "model/vrml";
	if (strcasecmp(tmp, ".midi") == 0)
		return "audio/midi";
	if (strcasecmp(tmp, ".mp3") == 0)
		return "audio/mpeg";
	if (strcasecmp(tmp, ".ogg") == 0)
		return "application/ogg";
	if (strcasecmp(tmp, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";

	return NULL;
}

int sendDir(const char* dirname, int cfd)
{
	char buf[4096] = "";
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "<html><title>%s</title><body><table>", dirname);
	struct dirent **namelist;
	int num = scandir(dirname, &namelist, NULL, alphasort);
	for (int i = 0; i < num; ++i)
	{
		char* name = namelist[i]->d_name;
		struct stat st;
		char subPath[1024] = "";
		sprintf(subPath, "%s/%s", dirname, name);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode))//判断是不是目录
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	free(namelist);
	return 0;
}

int hexToInt(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		///%E5%AE%9E%E9%AA%8C /实验
		//isxdigit 判断字符是不是16禁止格式
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			//将十六进制转换为十进制
			*to = hexToInt(from[1]) * 16 + hexToInt(from[2]);
			from += 2;
		}
		else
		{
			*to = *from;
		}
	}
	*to = '\0';
}

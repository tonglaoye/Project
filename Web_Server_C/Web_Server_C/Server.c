#define _CRT_SECURE_NO_WARNINGS 1
#include "Server.h"



int initListenFd(unsigned short port)
{ 
	//�����׽���
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket");
		return -1;
	}
	//���ö˿ڸ���
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1)
	{
		perror("setsockopt");
		return -1;
	}
	//��
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
	//����
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

	//����epollʵ��
	int epfd = epoll_create(1);
	if (epfd == -1)
	{
		perror("epoll_create");
		return -1;
	}
	//����
	struct epoll_event ev;
	ev.data.fd = lfd;
	ev.events = EPOLLIN;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	//����һ�������洢�����¼��Ľڵ������
	struct epoll_event evs[EVS_SIZE];
	//ѭ������
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

				//��ȡ����  ����������
				//acceptClient(epfd, lfd);
				pthread_create(&info->tid, NULL, acceptClient, info);
				int ret = pthread_detach(info->tid);//�����̷߳�������
				if (ret != 0)
				{
					perror("pthread_detach");
				}
				//printf("set pthread_detach success....\n");
			}
			else
			{
				//��������
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
	//��cfd ���óɷ�����
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	struct epoll_event ev;
	ev.data.fd = cfd;
	ev.events = EPOLLIN | EPOLLET;//�������¼������óɱ�Ե����
	//����
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

		//����������
		char* ptr = strstr(buf, "\r\n");//�ҵ���һ��\r\n ���ǵ�һ�еĽ���λ��
		int reqLen = ptr - buf;// �����һ�е�\r �ĳ���
		buf[reqLen] = '\0';// ����һ��\rǰ��\0
		parseRequestLine(buf, info->fd);
	}
	else if (len == 0)
	{
		//�ͻ��˶Ͽ�����
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
	decodeMsg(path, path);//���� 
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
	//����ļ�����
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		//printf("404.....\n");

		//�ļ������� ���� 404 ����
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//printf("send head...\n");

		sendFile("404.html", cfd);
		//printf("send file404...\n");
		return 0;
	}
	if (S_ISDIR(st.st_mode))
	{
		//printf("send dir.....\n");

		//�ѱ���Ŀ¼�����ݷ��ظ��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		//printf("send head...\n");
		sendDir(file, cfd);
		
	}
	else
	{
		printf("send file.....\n");

		// ���ļ������ݷ��ظ��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}
	//printf("end parseRequestLine.....\n");

	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	//���ļ�
	int fd = open(fileName, O_RDONLY);
	assert(fd > 0);
	//���ļ���С
	int len = lseek(fd, 0, SEEK_END);//���ļ�ָ���ƶ������ļ�ĩβ
	lseek(fd, 0, SEEK_SET);// ���ļ�ָ�����ƶ���ͷ
	off_t offset = 0;
	//�����ļ�
	while (offset < len)
	{
		int ret = sendfile(cfd, fd, &offset, len);
		printf("ret = %d", ret);
		usleep(10);//�������Ъ��
	}
	close(fd);
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	//printf("start send head...\n");

	char buf[4096] = "";
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);//״̬��  �汾 ״̬  ����
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type);// ��������
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);// ���ݳ���
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
		return "text/plain; charset=utf-8";//���ı�
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
		if (S_ISDIR(st.st_mode))//�ж��ǲ���Ŀ¼
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
		///%E5%AE%9E%E9%AA%8C /ʵ��
		//isxdigit �ж��ַ��ǲ���16��ֹ��ʽ
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			//��ʮ������ת��Ϊʮ����
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

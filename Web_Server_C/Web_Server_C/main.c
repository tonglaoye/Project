#include"Server.h"



int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("./a.out port path");
    }
    unsigned short port = atoi(argv[1]);
    //切换服务器工作目录
    chdir(argv[2]);
    //初始化监听套接字
    int lfd = initListenFd(port);
    //启动服务器
    epollrun(lfd);
    close(lfd);
    return 0;
}
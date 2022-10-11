#include "Server.hpp"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

void Server::setListenFd()
{
    //1.创建用于监听的套接字
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd==-1)
    {
        perror("socket");
        exit(0);
    }
    //2.设置端口复用
    int opt = 1;
    int res = setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof opt);
    if(res==-1)
    {
        perror("setsockopt");
        exit(0);
    }
    //3.绑定 bind
    sockaddr_in addr;   //sockaddr_in为网络地址的结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   //协议源地址  随机ip
    res = bind(lfd,(struct sockaddr*)&addr,sizeof addr);
    if(res==-1)
    {
        perror("bind");
        exit(0);
    }
    std::cout<<"bind port success"<<std::endl;
    //4.设置监听
    res = listen(lfd,5);  //5:  socket的未完成连接队列长度
    if(res<0)
    {
        perror("listen");
        exit(0);
    }
    // init  lfd
    m_lfd = lfd;
}

Server::Server(int port)
{
    setListenFd();  // m_lfd = lfd
    dispatcher = new PollDispatcher(m_lfd);
}

void Server::run()
{
    std::cout<<"server is running ...\n";
    while(true)
    {   
        dispatcher->dispatch();
    }
}


#include "PollDispatcher.hpp"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>

Node* PollDispatcher::head = new Node;
Node* PollDispatcher::tail = new Node;
pollfd* PollDispatcher::m_fds = (struct pollfd*)malloc(sizeof(struct pollfd)*1024);
pthread_mutex_t PollDispatcher::m_mutex;

PollDispatcher::PollDispatcher(int lfd) : m_lfd(lfd)   
{
    pthread_mutex_init(&m_mutex,nullptr);
    head->next = tail;
    tail->next = nullptr;
    m_maxfd = 0;
    for (int i = 0; i < m_count; ++i)
    {
        m_fds[i].fd = -1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    pollAdd(m_lfd);
}

void PollDispatcher::acceptClient()
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int cfd = accept(m_lfd,(sockaddr*)&addr,&len);
    Info* info = new Info;      //创建一个新 client Info
    info->addr = addr;
    info->fd = cfd;
    char clientIP[16];
    inet_ntop(AF_INET, &addr.sin_addr.s_addr, clientIP, sizeof(clientIP));
    unsigned short clientPort = ntohs(addr.sin_port);
    std::cout<<"new client ip:  "<<clientIP<<"  port:  "<<clientPort<<std::endl;

    addToList(info);    //用于循环发送数据给其他clients
    pollAdd(cfd);       //用于 poll监听
    sendMsgNewClient(cfd,info);      //告知其他clients 有新用户来了
}

void PollDispatcher::addToList(Info* info)
{
    pthread_mutex_lock(&PollDispatcher::m_mutex);
    Node* node = new Node(info);
    node->next = head->next;
    head->next = node;
    // std::cout<<"addToList"<<std::endl;
    pthread_mutex_unlock(&PollDispatcher::m_mutex);
}

Info* PollDispatcher::getInfoFromList(int fd)
{
    pthread_mutex_lock(&PollDispatcher::m_mutex);
    Info* res = nullptr;
    Node* node = head->next;
    while(node!=tail)
    {
        if(node->info->fd==fd)
        {
            res =  node->info;
            break;
        }
        node = node->next;
    }
    pthread_mutex_unlock(&PollDispatcher::m_mutex);
    return res;
}

void PollDispatcher::dispatch()
{
    // sleep(1);
    int count = poll(m_fds,m_maxfd+1,-1);
    // std::cout<<"count: "<<count<<'\t'<<"m_maxfd: "<<m_maxfd<<std::endl;
    if(count==-1)
    {
        perror("poll");
        exit(0);
    }
    for(int i = 0;i<=PollDispatcher::m_maxfd;++i)
    {
        if(m_fds[i].fd==-1)
            continue;
        if(m_fds[i].fd==m_lfd )  //由主线程来接受 new client
        {
            if(m_fds[i].revents & POLLIN)
            {
                acceptClient();     //阻塞在 accept这里 
            }
        }
        else
        {
                //A client sent Msg             
            if(m_fds[i].revents & POLLIN)       //由子线程来处理   clients 之间的通信
            {
                Info* info = getInfoFromList(m_fds[i].fd);  //在list 通过fd找到 info* 
                pthread_create(&info->pid,nullptr,readMsg,(void*)info);
            }
            // if(m_fds[i].revents & POLLOUT)  //实际cfd上没有写的 事件
        }
    }  
}

void PollDispatcher::pollAdd(int fd)
{
    pthread_mutex_lock(&PollDispatcher::m_mutex);
    int event = 0;
    event |= POLLIN;
    for(int i = 0;i<m_count;++i)
    {
        if(m_fds[i].fd==-1)
        {
            m_fds[i].fd = fd;
            m_fds[i].events = event;
            m_maxfd = i > m_maxfd ? i : m_maxfd;
            // std::cout<<"pollAdd sucess !\n";
            break;
        }
    }
    pthread_mutex_unlock(&PollDispatcher::m_mutex);
}

void sendMsgNewClient(int cfd,Info* info)
{
    char tmpBuf[64];
    char clientIP[16];
    inet_ntop(AF_INET, &info->addr.sin_addr.s_addr, clientIP, sizeof(clientIP));
    unsigned short clientPort = ntohs(info->addr.sin_port);
    sprintf(tmpBuf,"ip:%s  port:%d joined!!\n",clientIP,clientPort);
    std::cout<<tmpBuf<<std::endl;
    pthread_mutex_lock(&PollDispatcher::m_mutex);
    Node* node = PollDispatcher::head->next;
    while(node!=PollDispatcher::tail)
    {
        if(node->info->fd!=cfd) 
        {
            send(node->info->fd,tmpBuf,strlen(tmpBuf),0);
        }
        node = node->next;
    }
    pthread_mutex_unlock(&PollDispatcher::m_mutex);
}

void sendMsgClientClosed()
{
    char tmpBuf[32];
    // char clientIP[16];
    // inet_ntop(AF_INET, &addr.sin_addr.s_addr, clientIP, sizeof(clientIP));
    // unsigned short clientPort = ntohs(addr.sin_port);
    // sprintf(tmpBuf,"one  disconnected!! ip: %s  port: %d\n",clientIP,clientPort);
    sprintf(tmpBuf,"one  disconnected!!\n");
    // std::cout<<tmpBuf<<std::endl;
    pthread_mutex_lock(&PollDispatcher::m_mutex);
    Node* node = PollDispatcher::head->next;
    while(node!=PollDispatcher::tail)
    {
        // if(info->fd!=node->info->fd)    //此时还没有 delete info ，delete node
        send(node->info->fd,tmpBuf,strlen(tmpBuf),0);
        node = node->next;
    }
    pthread_mutex_unlock(&PollDispatcher::m_mutex);
}

void removeFromList(int fd)
{
    pthread_mutex_lock(&PollDispatcher::m_mutex);
    Node* node = PollDispatcher::head->next;
    Node* pre = PollDispatcher::head;
    while(node!=PollDispatcher::tail)
    {
        if(node->info->fd==fd)
        {
            // std::cout<<"one client disconnected!! :"<<node->info->addr.sin_port<<"\n";
            pre->next = node->next;
            delete node->info;
            delete node;
            std::cout<<"delete sucess\n";
            break;
        }
        pre = node;
        node = node->next;
    }
    pthread_mutex_unlock(&PollDispatcher::m_mutex);
}

void* readMsg(void* arg)
{
    Info* info = static_cast<Info*>(arg);
    char readBuf[1024];
    int len = read(info->fd,readBuf,sizeof(readBuf));
    if(len==-1){    //client结束了
        perror("read");
        exit(0);
    }
    else if(len==0){    
    // For TCP sockets   return value 0 means the peer has closed its half
    //  side of the connection.
        
        // Info* tmpinfo = new Info(*info);
        // sockaddr_in addr = info->addr;
        pollRemove(info->fd);
        removeFromList(info->fd);
        close(info->fd);
        // sendMsgClientClosed();
        // std::cout<<"len==0\n"<<info;
    }
    else
    {
        char tmpBuf[1024] = {0};
        char clientIP[16];
        char clientPort1[16];
        inet_ntop(AF_INET, &info->addr.sin_addr.s_addr, clientIP, sizeof(clientIP));
        int clientPort = ntohs(info->addr.sin_port);
        sprintf(clientPort1,"%d  :",clientPort);
        memcpy(tmpBuf,clientPort1,sizeof(clientPort1));
        // memcpy(tmpBuf,clientIP,sizeof(clientIP));
        // memcpy(tmpBuf+strlen(tmpBuf),);
        Node* node = PollDispatcher::head->next;
        memcpy(tmpBuf+strlen(tmpBuf),readBuf,sizeof(readBuf));
        char sendBuf[1024] = {0};
        memcpy(sendBuf,tmpBuf,sizeof(tmpBuf));
        while(node!=PollDispatcher::tail)
        {
            if(node->info->fd!=info->fd)
                write(node->info->fd,sendBuf,strlen(sendBuf));
            node = node->next;
        }
    }
    return nullptr;
}

void pollRemove(int fd)
{
    pthread_mutex_lock(&PollDispatcher::m_mutex);
    for(int i = 0;i<PollDispatcher::m_count;++i)
    {
        if(PollDispatcher::m_fds[i].fd==fd)
        {
            PollDispatcher::m_fds[i].fd = -1;
            PollDispatcher::m_fds[i].events = 0;
            PollDispatcher::m_fds[i].revents = 0;
            std::cout<<"pollRemove sucess\n"<<std::endl;
            pthread_mutex_unlock(&PollDispatcher::m_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&PollDispatcher::m_mutex);
}

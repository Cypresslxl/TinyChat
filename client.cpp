#include <iostream>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

void* sendMsg(void*);

bool flag = true;
int main(int argc,char** argv)
{
    // if(argc<3)
    // {
    //     std::cout<<"please input ./c port ip\n";
    //     exit(0);
    // }
    //1. socket
    int cfd = socket(AF_INET,SOCK_STREAM,0);
    if(cfd<0)
    {
        perror("socket");
        exit(0);
    }

    //2. connect
    sockaddr_in addr ={};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10000);
    int res = inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr.s_addr);
    if(res <=0)
    {
        perror("inet_pton");
        exit(0);
    }
    res = connect(cfd,(struct sockaddr*)&addr,sizeof(addr));
    if(res<0)
    {
        perror("connect");
        exit(0);
    }
    std::cout<<"connected !!\n";
    pthread_t pid;
    pthread_create(&pid,nullptr,sendMsg,(void*)&cfd);
    //3. recv
    char recvBuf[1024];
    while(flag)     
    {
        memset(recvBuf,0,sizeof recvBuf);
        int len = recv(cfd,&recvBuf,sizeof recvBuf,0);
        recvBuf[len] = '\0';
        printf("\t\t\t\t%s\n",recvBuf);
    }
    //主线程
    std::cout<<"cliend closed\n";
    close(cfd);
    return 0;
}

void* sendMsg(void* arg)
{
    std::cout<<"sendMsg"<<std::endl;
    int cfd = *(int*)arg;
    char sendBuf[1024];
    const char EXIT[5] = "exit";
    while(flag)
    {
        memset(sendBuf,0,sizeof sendBuf);
        std::cout<<"please input sendMsg\n";
        std::cin>>sendBuf;
        if(strncasecmp(sendBuf,EXIT,strlen(EXIT))==0)
            flag = false;
        else{
            send(cfd,sendBuf,strlen(sendBuf),0);
            sleep(1);   
        }
    }
    std::cout<<"exit sucess\n";
    sleep(2);//??
    close(cfd);
    sleep(2);//??
    exit(0);
    return nullptr;
}
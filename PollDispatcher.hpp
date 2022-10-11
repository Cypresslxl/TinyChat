#pragma once
#include <poll.h>
#include <pthread.h>
#include <arpa/inet.h>

struct Info
{
    pthread_t pid;
    int fd;
    sockaddr_in addr;
};

struct Node
{
    Info* info;
    struct Node* next;
    Node() : info(NULL),next(NULL)   {}
    Node(Info* info) : info(info),next(NULL)    {}
};

void pollRemove(int fd);
void* readMsg(void* fd);
void removeFromList(int fd);
void sendMsgNewClient(int cfd,Info* info);
void sendMsgClientClosed();

class PollDispatcher
{
public:
     //poll
    PollDispatcher(int lfd);
    void pollAdd(int fd);
    void dispatch();

    Info* getInfoFromList(int fd);
    void addToList(Info* info);
    void acceptClient();
    static Node* head;
    static Node* tail;
    static struct pollfd* m_fds;
    static const int m_count = 1024;
    static pthread_mutex_t m_mutex;
private:
    int m_maxfd;
    int m_lfd;
};
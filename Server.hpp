#pragma once

#include <iostream>
#include <arpa/inet.h>
#include <stdlib.h>
#include "PollDispatcher.hpp"

class Server
{
public:
    Server(int port = 10000);
    void run();

private:
    void setListenFd();

    PollDispatcher* dispatcher;
    int m_lfd;
};
#include "Server.hpp"


int main(int argc,char** argv)
{
    if(argc < 2)
    {
        std::cout<<"./s port"<<std::endl;
        exit(0);
    }

    Server server(atoi(argv[1]));   //setListenFd
    server.run();
    return 0;
}
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h> 
#include <arpa/inet.h> 

class UDPConnection{
private:
    int socketfd;
    sockaddr_in clientAddress, serverAddress;
    socklen_t s_socklen;
    socklen_t c_socklen;
    char buf[1024];
    void createSocket();
    void bindSocket();

public:
    UDPConnection(std::string clientAddr, int serverPort, int clientPort);
    ~UDPConnection();
    int send(const void* message, int msglen, int flags);
    int receive(char* buffer, int bufferSize = 1024, int flags = 0);
};

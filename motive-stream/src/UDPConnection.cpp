#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include "UDPConnection.hpp"


void UDPConnection::createSocket(){
    this->socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd < 0) {
        throw std::runtime_error("Socket creation failed");
    }
}
    
void UDPConnection::bindSocket(){
    if (bind(this->socketfd, (struct sockaddr*)&(this->serverAddress), this->s_socklen) < 0){
        throw std::runtime_error("Bind failed");
    }
}

UDPConnection::UDPConnection(std::string clientAddr, int serverPort, int clientPort){
    memset(this->clientAddress.sin_zero, 0, sizeof(this->clientAddress.sin_zero));
    memset(this->serverAddress.sin_zero, 0, sizeof(this->serverAddress.sin_zero));

    createSocket();

    this->clientAddress.sin_family = AF_INET;
    this->clientAddress.sin_port = htons(clientPort);
    inet_pton(AF_INET, clientAddr.c_str(), &(this->clientAddress.sin_addr));

    this->serverAddress.sin_family = AF_INET;
    this->serverAddress.sin_port = htons(serverPort);
    this->serverAddress.sin_addr.s_addr = INADDR_ANY;

    this->c_socklen = sizeof(this->clientAddress);
    this->s_socklen = sizeof(this->serverAddress);
    bindSocket();
}

UDPConnection::~UDPConnection(){
    if(this->socketfd >= 0){
        close(this->socketfd);
    }
}

int UDPConnection::send(const void* message, int msglen, int flags){
    int bytes_sent = sendto(this->socketfd, message, msglen, flags, (struct sockaddr*)&(this->clientAddress), this->c_socklen);
    if(bytes_sent < 0){
        std::cerr << "Message was not sent" << std::endl;
    } else{
        std::cout << "Sent Packet" << std::endl;
    }
    return bytes_sent;
}

int UDPConnection::receive(char* buffer, int bufferSize, int flags){

    int bytes_recv = recvfrom(socketfd, buffer,  bufferSize, flags,
            (struct sockaddr*)(&this->clientAddress), &this->c_socklen);
    
    if (bytes_recv < 0) {
        std::cerr << "Failed to receive message." << std::endl;
        return -1;
    }

    std::cout << "Received Message" << std::endl;
    buffer[bytes_recv] = '\0';
    return bytes_recv;
}

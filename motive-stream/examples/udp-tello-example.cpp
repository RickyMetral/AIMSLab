// C++ program to show the example of server application in
// socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/stdc++.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <arpa/inet.h> 



int main()
{
    // creating socket
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    char buf[1024] = {0};

    // specifying the address
    sockaddr_in telloAddress, localAddress;
    telloAddress.sin_family = AF_INET;
    telloAddress.sin_port = htons(8889);
    inet_pton(AF_INET, "192.168.10.1", &telloAddress.sin_addr);

    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(8889);
    localAddress.sin_addr.s_addr = INADDR_ANY;

    memset(telloAddress.sin_zero, 0, sizeof(telloAddress.sin_zero));
    memset(localAddress.sin_zero, 0, sizeof(localAddress.sin_zero));


    socklen_t socklen = sizeof(telloAddress);


    bind(socketfd, (struct sockaddr*)&localAddress, sizeof(localAddress));
    int bytes_sent = sendto(socketfd, "command", 7, 0, (struct sockaddr*)&telloAddress, socklen);
    if(bytes_sent < 0){
        std::cerr << "Message was not sent" << std::endl;
    } else{
        std::cout << "Sent: " << "command" << std::endl;
    }
    std::string message;
    while (true) {
        int bytes_recv = recvfrom(socketfd, buf, 1024, MSG_WAITALL, (struct sockaddr*)&telloAddress, &socklen);
        if(bytes_recv > 0){
            buf[bytes_recv] = '\0';
            std::cout << "Received: "  << buf << std::endl;
        } else{
            std::cout << "No Message was Received" << std::endl;
        }
        
        std::cout << "Enter Instruction: ";
        std::getline(std::cin, message);
        bytes_sent = sendto(socketfd, message.c_str(), message.size(), 0,
               (struct sockaddr*)&telloAddress, socklen);
        if(bytes_sent < 0){
            std::cerr << "Message was not sent" << std::endl;
        } else{
            std::cout << "Sent: " << message << std::endl;
        }

        if(message == "exit"){
            break;
        }
    }
    close(socketfd);
    return 0;

}
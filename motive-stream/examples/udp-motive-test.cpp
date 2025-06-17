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
    sockaddr_in localAddress, motiveAddress;
    localAddress.sin_family = AF_INET;
    motiveAddress.sin_port = htons(3883);
    inet_pton(AF_INET, "239.255.42.99", &motiveAddress.sin_addr);

    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(8889);
    localAddress.sin_addr.s_addr = INADDR_ANY;

    memset(motiveAddress.sin_zero, 0, sizeof(localAddress.sin_zero));
    memset(localAddress.sin_zero, 0, sizeof(localAddress.sin_zero));


    socklen_t socklen = sizeof(motiveAddress);


    bind(socketfd, (struct sockaddr*)&motiveAddress, sizeof(motiveAddress));
    std::string message;
    while (true) {
        int bytes_recv = recvfrom(socketfd, buf, 1024, MSG_WAITALL, (struct sockaddr*)&motiveAddress, &socklen);
        if(bytes_recv > 0){
            buf[bytes_recv] = '\0';
            std::cout << "Received: "  << buf << std::endl;
        } else{
            std::cout << "No Message was Received" << std::endl;
        }
        
        if(message == "exit"){
            break;
        }
    }
    close(socketfd);
    return 0;

}

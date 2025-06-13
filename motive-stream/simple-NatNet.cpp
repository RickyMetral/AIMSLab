#include "NatNetTypes.h"
#include "NatNetClient.h"
#include "NatNetCAPI.h"
#include <iostream>

NatNetClient* motive_client = NULL;
sNatNetClientConnectParams motive_params;
sServerDescription motive_serverDescription;


int main(){
    motive_client = new NatNetClient();
    // const int kMaxDescriptions = 10; // Get info for, at most, the first 10 servers to respond.
    // sNatNetDiscoveredServer servers[kMaxDescriptions];
    // int actualNumDescriptions = kMaxDescriptions;
    // NatNet_BroadcastServerDiscovery( servers, &actualNumDescriptions );
    // for(int i = 0; i  < kMaxDescriptions; i++){
    //     sNatNetDiscoveredServer* pDiscoveredServer = &servers[i];
    //     printf( "%s %d.%d.%d at %s \n",
    //         pDiscoveredServer->serverDescription.szHostApp,
    //         pDiscoveredServer->serverDescription.HostAppVersion[0],
    //         pDiscoveredServer->serverDescription.HostAppVersion[1],
    //         pDiscoveredServer->serverDescription.HostAppVersion[2],
    //         pDiscoveredServer->serverAddress);
    //     }
    motive_params.connectionType = ConnectionType_Multicast;
    motive_params.serverCommandPort = (uint16_t) 1510;
    motive_params.serverDataPort= (uint16_t) 1511;
    motive_params.serverAddress = "192.168.1.14";
    motive_params.localAddress= "127.0.0.1";
    motive_params.multicastAddress= "255.255.255.255";
    motive_client->Disconnect();
    int ret = motive_client->Connect(motive_params);
    if(ret){
        std::cout << "Client Initialized" << std::endl;
    } else{
        std::cout << "Error Initializing Client" << std::endl;
        return 1;
    }

    // print server info
     memset( &motive_serverDescription, 0, sizeof( motive_serverDescription ) );
     ret = motive_client->GetServerDescription( &motive_serverDescription );
     if ( ret != ErrorCode_OK || ! motive_serverDescription.HostPresent )
     {
        printf("Unable to connect to server. Host not present. Exiting.\n");
        return 1;
     }
     printf("[SampleClient] Server application info:\n");

     printf("Application: %s (ver. %d.%d.%d.%d)\n", motive_serverDescription.szHostApp, motive_serverDescription.HostAppVersion[0],
             motive_serverDescription.HostAppVersion[1],motive_serverDescription.HostAppVersion[2],
             motive_serverDescription.HostAppVersion[3]);

     printf("NatNet Version: %d.%d.%d.%d\n", motive_serverDescription.NatNetVersion[0], motive_serverDescription.NatNetVersion[1],
             motive_serverDescription.NatNetVersion[2], motive_serverDescription.NatNetVersion[3]);

     printf( "Client IP:%s\n", motive_params.localAddress );
     printf( "Server IP:%s\n", motive_params.serverAddress );
     printf("Server Name:%s\n\n", motive_serverDescription.szHostComputerName);

    motive_client->Disconnect();

    return 0;
}
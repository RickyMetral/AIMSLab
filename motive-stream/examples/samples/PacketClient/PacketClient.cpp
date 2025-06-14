//=============================================================================
// Copyright © 2025 NaturalPoint, Inc. All Rights Reserved.
// 
// THIS SOFTWARE IS GOVERNED BY THE OPTITRACK PLUGINS EULA AVAILABLE AT https://www.optitrack.com/about/legal/eula.html 
// AND/OR FOR DOWNLOAD WITH THE APPLICABLE SOFTWARE FILE(S) (“PLUGINS EULA”). BY DOWNLOADING, INSTALLING, ACTIVATING 
// AND/OR OTHERWISE USING THE SOFTWARE, YOU ARE AGREEING THAT YOU HAVE READ, AND THAT YOU AGREE TO COMPLY WITH AND ARE
// BOUND BY, THE PLUGINS EULA AND ALL APPLICABLE LAWS AND REGULATIONS. IF YOU DO NOT AGREE TO BE BOUND BY THE PLUGINS
// EULA, THEN YOU MAY NOT DOWNLOAD, INSTALL, ACTIVATE OR OTHERWISE USE THE SOFTWARE AND YOU MUST PROMPTLY DELETE OR
// RETURN IT. IF YOU ARE DOWNLOADING, INSTALLING, ACTIVATING AND/OR OTHERWISE USING THE SOFTWARE ON BEHALF OF AN ENTITY,
// THEN BY DOING SO YOU REPRESENT AND WARRANT THAT YOU HAVE THE APPROPRIATE AUTHORITY TO ACCEPT THE PLUGINS EULA ON
// BEHALF OF SUCH ENTITY. See license file in root directory for additional governing terms and information.
//=============================================================================

/**
 * \page   PacketClient.cpp
 * \file   PacketClient.cpp
 * \brief  Example of how to decode NatNet packets directly. 
 * Decodes NatNet packets directly.
 * Usage [optional]:
 *  PacketClient [ServerIP] [LocalIP]
 *     [ServerIP]			IP address of server ( defaults to local machine)
 *     [LocalIP]			IP address of client ( defaults to local machine)
 */


#include <cstdio>
#include <cinttypes>
#include <tchar.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <map>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>


#pragma warning( disable : 4996 )

#ifdef VDEBUG
#undef VDEBUG
#endif
// #define VDEBUG
#ifdef VDEBUG
const int kValidDataTypes = 6;
#endif

#define MAX_NAMELENGTH              256
#define MAX_ANALOG_CHANNELS          32

// NATNET message ids
#define NAT_CONNECT                 0 
#define NAT_SERVERINFO              1
#define NAT_REQUEST                 2
#define NAT_RESPONSE                3
#define NAT_REQUEST_MODELDEF        4
#define NAT_MODELDEF                5
#define NAT_REQUEST_FRAMEOFDATA     6
#define NAT_FRAMEOFDATA             7
#define NAT_MESSAGESTRING           8
#define NAT_DISCONNECT              9
#define NAT_KEEPALIVE               10
#define NAT_UNRECOGNIZED_REQUEST    100
#define UNDEFINED                   999999.9999


#define MAX_PACKETSIZE				100000	// max size of packet (actual packet size is dynamic)

// This should match the multicast address listed in Motive's streaming settings.
#define MULTICAST_ADDRESS		"239.255.42.99"

// Requested size for socket
#define OPTVAL_REQUEST_SIZE 0x10000

// NatNet Command channel
#define PORT_COMMAND            1510

// NatNet Data channel
#define PORT_DATA  			    1511                

SOCKET gCommandSocket;
SOCKET gDataSocket;
//in_addr gServerAddress;
sockaddr_in gHostAddr;

int gNatNetVersion[4] = { 0,0,0,0 };
int gNatNetVersionServer[4] = { 0,0,0,0 };
int gServerVersion[4] = { 0,0,0,0 };
char gServerName[MAX_NAMELENGTH] = { 0 };
bool gCanChangeBitstream = false;
bool gBitstreamVersionChanged = false;
bool gBitstreamChangePending = false;

// Compiletime flag for unicast/multicast
//gUseMulticast = true  : Use Multicast
//gUseMulticast = false : Use Unicast
bool gUseMulticast = false;
bool gPausePlayback = false;
int gCommandResponse = 0;
int gCommandResponseSize = 0;
unsigned char gCommandResponseString[MAX_PATH];
int gCommandResponseCode = 0;
static std::string kTabStr( "  " );

struct sParsedArgs
{
    char    szMyIPAddress[128] = "127.0.0.1";
    char    szServerIPAddress[128] = "127.0.0.1";
    in_addr myAddress;
    in_addr serverAddress;

    in_addr multiCastAddress;
    bool    useMulticast = false;
};

// sender
struct sSender
{
    char szName[MAX_NAMELENGTH];            // sending app's name
    unsigned char Version[4];               // sending app's version [major.minor.build.revision]
    unsigned char NatNetVersion[4];         // sending app's NatNet version [major.minor.build.revision]
};

struct sPacket
{
    unsigned short iMessage;                // message ID (e.g. NAT_FRAMEOFDATA)
    unsigned short nDataBytes;              // Num bytes in payload
    union
    {
        unsigned char  cData[MAX_PACKETSIZE];
        char           szData[MAX_PACKETSIZE];
        unsigned long  lData[MAX_PACKETSIZE / 4];
        float          fData[MAX_PACKETSIZE / 4];
        sSender        Sender;
    } Data;                                 // Payload incoming from NatNet Server
};

struct sConnectionOptions
{
    bool subscribedDataOnly;
    uint8_t BitstreamVersion[4];
#if defined(__cplusplus)
    sConnectionOptions() : subscribedDataOnly( false ), BitstreamVersion{ 0,0,0,0 } {}
#endif
};

// Utility functions
std::string GetTabString( std::string tabStr, unsigned int level );

// Communications functions
bool IPAddress_StringToAddr( char* szNameOrAddress, struct in_addr* Address );
int GetLocalIPAddresses( unsigned long Addresses[], int nMax );
int SendCommand( char* szCOmmand );

// Packet unpacking functions
char* Unpack( char* pPacketIn, unsigned int level=0 );
char* UnpackPacketHeader( char* ptr, int& messageID, int& nBytes, int& nBytesTotal, unsigned int level );
char* UnpackDataSize(char* ptr, int major, int minor, int& nBytes, unsigned int level, bool skip = false );

// Frame data
char* UnpackFrameData( char* inptr, int nBytes, int major, int minor, unsigned int level );
char* UnpackFramePrefixData( char* ptr, int major, int minor, unsigned int level );
char* UnpackMarkersetData( char* ptr, int major, int minor, unsigned int level );
char* UnpackRigidBodyData( char* ptr, int major, int minor, unsigned int level );
char* UnpackSkeletonData( char* ptr, int major, int minor, unsigned int level );
char* UnpackLabeledMarkerData( char* ptr, int major, int minor, unsigned int level );
char* UnpackForcePlateData( char* ptr, int major, int minor, unsigned int level );
char* UnpackDeviceData( char* ptr, int major, int minor, unsigned int level );
char* UnpackFrameSuffixData( char* ptr, int major, int minor, unsigned int level );
char* UnpackAssetData( char* ptr, int major, int minor, unsigned int level=0 );
char* UnpackAssetMarkerData( char* ptr, int major, int minor, unsigned int level );
char* UnpackAssetRigidBodyData( char* ptr, int major, int minor, unsigned int level );
char* UnpackLegacyOtherMarkers( char* ptr, int major, int minor, unsigned int level );

// Descriptions
char* UnpackDescription( char* inptr, int nBytes, int major, int minor, unsigned int level );
char* UnpackMarkersetDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );
char* UnpackRigidBodyDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );
char* UnpackSkeletonDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );
char* UnpackForcePlateDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );
char* UnpackDeviceDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );
char* UnpackCameraDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );
char* UnpackAssetDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );
char* UnpackMarkerDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level );


/**
* \brief WSA Error codes:
* https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
*/
std::map<int, std::string> wsaErrors = {
    { 10004, " WSAEINTR: Interrupted function call."},
    { 10009, " WSAEBADF: File handle is not valid."},
    { 10013, " WSAEACCESS: Permission denied."},
    { 10014, " WSAEFAULT: Bad address."},
    { 10022, " WSAEINVAL: Invalid argument."},
    { 10024, " WSAEMFILE: Too many open files."},
    { 10035, " WSAEWOULDBLOCK: Resource temporarily unavailable."},
    { 10036, " WSAEINPROGRESS: Operation now in progress."},
    { 10037, " WSAEALREADY: Operation already in progress."},
    { 10038, " WSAENOTSOCK: Socket operation on nonsocket."},
    { 10039, " WSAEDESTADDRREQ Destination address required."},
    { 10040, " WSAEMSGSIZE: Message too long."},
    { 10041, " WSAEPROTOTYPE: Protocol wrong type for socket."},
    { 10047, " WSAEAFNOSUPPORT: Address family not supported by protocol family."},
    { 10048, " WSAEADDRINUSE: Address already in use."},
    { 10049, " WSAEADDRNOTAVAIL: Cannot assign requested address."},
    { 10050, " WSAENETDOWN: Network is down."},
    { 10051, " WSAEWSAENETUNREACH: Network is unreachable."},
    { 10052, " WSAENETRESET: Network dropped connection on reset."},
    { 10053, " WSAECONNABORTED: Software caused connection abort."},
    { 10054, " WSAECONNRESET: Connection reset by peer."},
    { 10060, " WSAETIMEDOUT: Connection timed out."},
    { 10093, " WSANOTINITIALIZED: Successful WSAStartup not yet performed."}
};


/**
 * \brief - Generate a formatting string based on the input tabStr and the level.
 * \return - String to use for formatting.
*/
std::string GetTabString( std::string tabStr, unsigned int level )
{
    std::string outTabStr = "";
    unsigned int levelNum = 0;
    for( levelNum = 0; levelNum < level; ++levelNum )
    {
        outTabStr += tabStr;
    }
    return outTabStr;
}

/**
 * \brief - Send command to get bitream version.
 * \return - Success or failure.
*/
bool GetBitstreamVersion()
{
    int result = SendCommand( "Bitstream" );
    if( result != 0 )
    {
        printf( "Error getting Bitstream Version" );
        return false;
    }
    return true;
}

/**
 * \brief - Request bitstream version from Motive
 * \param major - Major version
 * \param minor - Minor Version
 * \param revision - Revision
 * \return 
*/

/**
 * .
 * 
 * \param major
 * \param minor
 * \param revision
 * \return 
 */
bool SetBitstreamVersion( int major, int minor, int revision )
{
    gBitstreamChangePending = true;
    char szRequest[512];
    sprintf( szRequest, "Bitstream,%1.1d.%1.1d.%1.1d", major, minor, revision );
    int result = SendCommand( szRequest );
    if( result != 0 )
    {
        printf( "Error setting Bitstream Version" );
        gBitstreamChangePending = false;
        return false;
    }

    // query to confirm
    GetBitstreamVersion();

    return true;
}

/**
 * \brief - Get Windows Sockets error codes as a string.
 * \param errorValue - input error code
 * \return - returns error as a string.
*/
std::string GetWSAErrorString( int errorValue )
{
    // Additional values can be found in Winsock2.h or
    // https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2

    std::string errorString = std::to_string( errorValue );
    // loop over entries in map
    auto mapItr = wsaErrors.begin();
    for( ; mapItr != wsaErrors.end(); ++mapItr )
    {
        if( mapItr->first == errorValue )
        {
            errorString += mapItr->second;
            return errorString;
        }
    }

    // If it gets here, the code is unknown, so show the reference link.																		
    errorString += std::string( " Please see: https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2" );
    return errorString;
}

/**
 * \brief - make sure the string is printable ascii
 * \param szName - input string
 * \param len - string length
*/
void MakeAlnum( char* szName, int len )
{
    int i = 0, i_max = len;
    szName[len - 1] = 0;
    while( ( i < len ) && ( szName[i] != 0 ) )
    {
        if( szName[i] == 0 )
        {
            break;
        }
        if( isalnum( szName[i] ) == 0 )
        {
            szName[i] = ' ';
        }
        ++i;
    }
}

/**
 * \brief - Manage the command channel and print status.
 * \param dummy - unused parameter
 * \return - 0 = Success, 1 = CommandListenThread Start FAILURE
*/
DWORD WINAPI CommandListenThread( void* dummy )
{
    DWORD retValue = 0;
    int addr_len;
    int nDataBytesReceived;
    sockaddr_in TheirAddress;
    sPacket* PacketIn = new sPacket();
    sPacket* PacketOut = new sPacket();
    addr_len = sizeof( struct sockaddr );

    if( PacketIn && PacketOut )
    {
        printf( "[PacketClient CLTh] CommandListenThread Started\n" );
        while( true )
        {
            // Send a Keep Alive message to Motive (required for Unicast transmission only)
            if( !gUseMulticast )
            {
                PacketOut->iMessage = NAT_KEEPALIVE;
                PacketOut->nDataBytes = 0;
                int iRet = sendto( gCommandSocket, (char*) PacketOut, 4 + PacketOut->nDataBytes, 0, (sockaddr*) &gHostAddr, sizeof( gHostAddr ) );
                if( iRet == SOCKET_ERROR )
                {
                    printf( "[PacketClient CLTh] sendto failure   (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
                }
            }

            // blocking with timeout
            nDataBytesReceived = recvfrom( gCommandSocket, (char*) PacketIn, sizeof( sPacket ),
                0, (struct sockaddr*) &TheirAddress, &addr_len );

            if( ( nDataBytesReceived == 0 ) )
            {
                continue;
            }
            else if( nDataBytesReceived == SOCKET_ERROR )
            {
                if( WSAGetLastError() != 10060 )// Ignore normal timeout failures
                {
                    printf( "[PacketClient CLTh] recvfrom failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
                }
                continue;
            }

            /*
            // debug - print message
            char str[MAX_NAMELENGTH];
            sprintf(str, "[PacketClient CLTh] Received command from %d.%d.%d.%d: Command=%d, nDataBytes=%d",
                TheirAddress.sin_addr.S_un.S_un_b.s_b1, TheirAddress.sin_addr.S_un.S_un_b.s_b2,
                TheirAddress.sin_addr.S_un.S_un_b.s_b3, TheirAddress.sin_addr.S_un.S_un_b.s_b4,
                (int)PacketIn->iMessage, (int)PacketIn->nDataBytes);
            printf("%s\n", str);
            */

            // handle command
            switch( PacketIn->iMessage )
            {
            case NAT_SERVERINFO: // 1
                strcpy_s( gServerName, PacketIn->Data.Sender.szName );
                for( int i = 0; i < 4; i++ )
                {
                    gNatNetVersionServer[i] = (int) PacketIn->Data.Sender.NatNetVersion[i];
                    gServerVersion[i] = (int) PacketIn->Data.Sender.Version[i];
                }
                if( ( gNatNetVersion[0] == 0 ) && ( gNatNetVersion[1] == 0 ) )
                {
                    for( int i = 0; i < 4; i++ )
                    {
                        gNatNetVersion[i] = gNatNetVersionServer[i];
                    }
                }

                if( ( gNatNetVersionServer[0] >= 3 ) && ( !gUseMulticast ) )     // Requires Motive 3.x or greater and Unicast
                {
                    gCanChangeBitstream = true;
                }

                printf( "[PacketClient CLTh]  NatNet Server Info\n" );
                printf( "[PacketClient CLTh]    Sending Application Name: %s\n", gServerName );
                printf( "[PacketClient CLTh]    %s Version %d %d %d %d\n", gServerName,
                    gServerVersion[0], gServerVersion[1], gServerVersion[2], gServerVersion[3] );
                printf( "[PacketClient CLTh]    NatNet Version %d %d %d %d\n",
                    gNatNetVersion[0], gNatNetVersion[1], gNatNetVersion[2], gNatNetVersion[3] );
                break;
            case NAT_RESPONSE: // 3
                gCommandResponseSize = PacketIn->nDataBytes;
                if( gCommandResponseSize == 4 )
                {
                    memcpy( &gCommandResponse, &PacketIn->Data.lData[0], gCommandResponseSize );
                }
                else
                {
                    memcpy( &gCommandResponseString[0], &PacketIn->Data.cData[0], gCommandResponseSize );
                    printf( "[PacketClient CLTh]    Response : %s\n", gCommandResponseString );
                    gCommandResponse = 0;   // ok
                }

                // handle GetBitstreamVersion command
                if( _strnicmp( (char*) gCommandResponseString, "Bitstream", strlen( "Bitstream" ) ) == 0 )
                {
                    char* value = strchr( (char*) gCommandResponseString, ',' );
                    if( value )
                    {
                        value++;
                        char* token = strtok( value, "." );
                        int i = 0;
                        while( token != nullptr )
                        {
                            gNatNetVersion[i] = atoi( token );
                            token = strtok( nullptr, "." );
                            i++;
                        }
                        printf( "[PacketClient CLTh]    NatNet Bitstream Version : %d.%d.%d\n", gNatNetVersion[0], gNatNetVersion[1], gNatNetVersion[2] );
                    }
                }
                break;
            case NAT_MODELDEF: //5
                Unpack( (char*) PacketIn );
                break;
            case NAT_FRAMEOFDATA: // 7
                Unpack( (char*) PacketIn );
                break;
            case NAT_UNRECOGNIZED_REQUEST: //100
                printf( "[PacketClient CLTh]    Received iMessage 100 = 'unrecognized request'\n" );
                gCommandResponseSize = 0;
                gCommandResponse = 1;       // err
                break;
            case NAT_MESSAGESTRING: //8
            {
                printf( "[PacketClient CLTh]    Received message: %s\n", PacketIn->Data.szData );
                break;
            }
            default:
                printf( "[PacketClient CLTh]    Received unknown command %d\n",
                    PacketIn->iMessage );
            }
        }// end of while
    }
    else
    {
        printf( "[PacketClient CLTh] CommandListenThread Start FAILURE\n" );
        retValue = 1;
    }
    if( PacketIn )
    {
        delete PacketIn;
        PacketIn = nullptr;
    }
    if( PacketOut )
    {
        delete PacketOut;
        PacketOut = nullptr;
    }
    return retValue;
}

/**
 * \brief - Data listener thread. Listens for incoming bytes from NatNet
 * \param dummy - unused parameter 
 * \return - 0 = Success
*/
DWORD WINAPI DataListenThread( void* dummy )
{
    unsigned int level = 0;
    const int baseDataBytes = 48 * 1024;
    char* szData = nullptr;
    int nDataBytes = 0;
    szData = new char[baseDataBytes];
    if( szData )
    {
        nDataBytes = baseDataBytes;
    }
    else
    {
        printf( "[PacketClient DLTh] DataListenThread Start FAILURE memory allocation\n" );
    }
    int addr_len = sizeof( struct sockaddr );
    sockaddr_in TheirAddress;
    printf( "[PacketClient DLTh] DataListenThread Started\n" );

    while( true )
    {
        // Block until we receive a datagram from the network (from anyone including ourselves)
        int nDataBytesReceived = recvfrom( gDataSocket, szData, nDataBytes, 0, (sockaddr*) &TheirAddress, &addr_len );
        // Once we have bytes recieved Unpack organizes all the data
        if( nDataBytesReceived > 0 )
        {
            Unpack( szData );
        }
        else if( nDataBytesReceived < 0 )
        {
            int wsaLastError = WSAGetLastError();
            printf( "[PacketClient DLTh] gDataSocket failure (error: %d)\n", nDataBytesReceived );
            printf( "[PacketClient DLTh] WSAError (error: %s)\n", GetWSAErrorString( wsaLastError ).c_str() );
            if( wsaLastError == 10040 )
            {
                // peek at truncated data, determine better buffer size
                int messageID = 0;
                int nBytes = 0;
                int nBytesTotal = 0;
                UnpackPacketHeader( szData, messageID, nBytes, nBytesTotal, level);
                printf( "[PacketClient DLTh] messageID %d nBytes %d nBytesTotal %d\n",
                    messageID, nBytes, nBytesTotal );
                if( nBytesTotal <= MAX_PACKETSIZE )
                {
                    int newSize = nBytesTotal + 10000;
                    newSize = min( newSize, (int) MAX_PACKETSIZE );
                    char* szDataNew = new char[newSize];
                    if( szDataNew )
                    {
                        printf( "[PacketClient DLTh] Resizing data buffer from %d bytes to %d bytes",
                            nDataBytes, newSize );
                        if( szData )
                        {
                            delete[] szData;
                        }
                        szData = szDataNew;
                        nDataBytes = newSize;
                        szDataNew = nullptr;
                        newSize = 0;
                    }
                    else
                    {
                        printf( "PacketClient DLTh] Data buffer size failure have %d bytes but need %d bytes", nDataBytes, nBytesTotal );
                    }
                }
                else
                {
                    printf( "PacketClient DLTh] Data buffer size failure have %d bytes but need %d bytes", nDataBytes, nBytesTotal );
                }

            }
        }
    }
    if( szData )
    {
        delete[] szData;
        szData = nullptr;
        nDataBytes = 0;
    }
    return 0;
}

/**
 * \brief - Create Command Socket
 * \param IP_Address - IP address of Motive/NatNet Server
 * \param uPort - port on Motive/NatNet Server
 * \param optval - buffer size
 * \param useMulticast - true = use Multicast false = use Unicast.
 * \return - socket file descriptor
*/
SOCKET CreateCommandSocket( unsigned long IP_Address, unsigned short uPort, int optval,
    bool useMulticast )
{
    int retval = SOCKET_ERROR;
    struct sockaddr_in my_addr;
    static unsigned long ivalue = 0x0;
    static unsigned long bFlag = 0x0;
    int nlengthofsztemp = 64;
    SOCKET sockfd = -1;
    int optval_size = sizeof( int );
    ivalue = 1;
    int bufSize = optval;

    int protocol = 0;

    if( !useMulticast )
    {
        protocol = IPPROTO_UDP;
    }
    // Create a datagram socket
    if( ( sockfd = socket( AF_INET, SOCK_DGRAM, protocol ) ) == INVALID_SOCKET )
    {
        printf( "[PacketClient Main] gCommandSocket create failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        return -1;
    }

    // bind socket
    memset( &my_addr, 0, sizeof( my_addr ) );
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons( uPort );
    my_addr.sin_addr.S_un.S_addr = IP_Address;

    if( bind( sockfd, (struct sockaddr*) &my_addr, sizeof( struct sockaddr ) ) == SOCKET_ERROR )
    {
        printf( "[PacketClient Main] gCommandSocket bind failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        closesocket( sockfd );
        return -1;
    }

    if( useMulticast )
    {
        // set to broadcast mode
        if( setsockopt( sockfd, SOL_SOCKET, SO_BROADCAST, (char*) &ivalue, sizeof( ivalue ) ) == SOCKET_ERROR )
        {
            // error - should show setsockopt error.
            closesocket( sockfd );
            return -1;
        }

        // set a read timeout to allow for sending keep_alive message
        int timeout = 2000;
        setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof timeout );
    }
    retval = getsockopt( sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &optval, &optval_size );
    if( retval == SOCKET_ERROR )
    {
        // error
        printf( "[PacketClient Main] gCommandSocket get options  SO_RCVBUF failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        closesocket( sockfd );
        return -1;
    }
    if( optval != OPTVAL_REQUEST_SIZE )
    {
        // err - actual size...
        printf( "[PacketClient Main] gCommandSocket Receive Buffer size = %d requested %d\n",
            optval, OPTVAL_REQUEST_SIZE );
    }
    if( useMulticast )
    {
        // [optional] set to non-blocking
        //u_long iMode=1;
        //ioctlsocket(gCommandSocket,FIONBIO,&iMode); 
        // set buffer
        retval = setsockopt( sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &optval, 4 );
        if( retval == SOCKET_ERROR )
        {
            // error
            printf( "[PacketClient Main] gCommandSocket set options SO_RCVBUF failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
            closesocket( sockfd );
            return -1;
        }
    }
    // Unicast case
    else
    {
        // set a read timeout to allow for sending keep_alive message for unicast clients
        int timeout = 2000;
        setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof timeout );

        // allow multiple clients on same machine to use multicast group address/port
        int value = 1;
        int retval = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &value, sizeof( value ) );
        if( retval == -1 )
        {
            printf( "[PacketClient Main] gCommandSocket setsockopt failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
            closesocket( sockfd );
            return -1;
        }


        // set user-definable send buffer size
        int defaultBufferSize = 0;
        socklen_t optval_size = 4;
        retval = getsockopt( sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &defaultBufferSize, &optval_size );
        retval = setsockopt( sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &bufSize, sizeof( bufSize ) );
        if( retval == -1 )
        {
            printf( "[PacketClient Main] gCommandSocket user send buffer failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
            closesocket( sockfd );
            return -1;
        }
        int confirmValue = 0;
        getsockopt( sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &confirmValue, &optval_size );
        if( confirmValue != bufSize )
        {
            // not fatal, but notify user requested size is not valid
            printf( "[PacketClient Main] gCommandSocket buffer smaller than expected %d instead of %d\n",
                confirmValue, bufSize );
        }

        // Set "Don't Fragment" bit in IP header to false (0).
        // note : we want fragmentation support since our packets are over the standard ethernet MTU (~1500 bytes).
        int optval2;
        socklen_t optlen = sizeof( int );
        int iRet = getsockopt( sockfd, IPPROTO_IP, IP_DONTFRAGMENT, (char*) &optval2, &optlen );
        optval2 = 0;
        iRet = setsockopt( sockfd, IPPROTO_IP, IP_DONTFRAGMENT, (char*) &optval2, sizeof( optval2 ) );
        if( iRet == -1 )
        {
            printf( "[PacketClient Main] gCommandSocket Don't fragment request failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
            closesocket( sockfd );
            return -1;
        }
        iRet = getsockopt( sockfd, IPPROTO_IP, IP_DONTFRAGMENT, (char*) &optval2, &optlen );

    }


    return sockfd;
}

/**
 * \brief - Create Data Socket to communicate with Motive/NatNet server
 * \param socketIPAddress - IP address of Motive/NatNet server
 * \param uUnicastPort - unicast port of Motive/NatNet server
 * \param optval - buffer size
 * \param useMulticast - true = use Multicast false = use Unicast
 * \param multicastIPAddress - Multicast Motive/NatNet server IP address
 * \param uMulticastPort - Multicast Motive/NatNet server port
 * \return - socket file descriptor
*/
SOCKET CreateDataSocket( unsigned long socketIPAddress, unsigned short uUnicastPort, int optval,
    bool useMulticast, unsigned long multicastIPAddress, unsigned short uMulticastPort )
{
    int retval = SOCKET_ERROR;
    static unsigned long ivalue = 0x0;
    static unsigned long bFlag = 0x0;
    int nlengthofsztemp = 64;
    SOCKET sockfd = -1;
    int optval_size = sizeof( int );
    int value = 1;
    // create the socket
    sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sockfd == SOCKET_ERROR )
    {
        printf( "[PacketClient Main] gDataSocket socket allocation failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        return -1;
    }
    // allow multiple clients on same machine to use address/port
    retval = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &value, sizeof( value ) );
    if( retval == SOCKET_ERROR )
    {
        printf( "[PacketClient Main] gDataSocket SO_REUSEADDR setsockopt failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        closesocket( sockfd );
        return -1;
    }

    if( useMulticast )
    {
        // Bind socket to address/port								  
        struct sockaddr_in MySocketAddress;
        memset( &MySocketAddress, 0, sizeof( MySocketAddress ) );
        MySocketAddress.sin_family = AF_INET;
        MySocketAddress.sin_port = htons( uMulticastPort );
        MySocketAddress.sin_addr.S_un.S_addr = socketIPAddress;
        if( bind( sockfd, (struct sockaddr*) &MySocketAddress, sizeof( struct sockaddr ) ) == SOCKET_ERROR )
        {
            printf( "[PacketClient Main] gDataSocket bind failed (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
            closesocket( sockfd );
            return -1;
        }

        // If Motive is transmitting data in Multicast, must join multicast group
        in_addr MyAddress, MultiCastAddress;
        MyAddress.S_un.S_addr = socketIPAddress;
        MultiCastAddress.S_un.S_addr = multicastIPAddress;

        struct ip_mreq Mreq;
        Mreq.imr_multiaddr = MultiCastAddress;
        Mreq.imr_interface = MyAddress;
        retval = setsockopt( sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &Mreq, sizeof( Mreq ) );
        if( retval == SOCKET_ERROR )
        {
            printf( "[PacketClient Main] gDataSocket join failed (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
            WSACleanup();
            return -1;
        }
    }
    //Unicast case
    else
    {
        // bind it
        struct sockaddr_in MyAddr;
        memset( &MyAddr, 0, sizeof( MyAddr ) );
        MyAddr.sin_family = AF_INET;
        MyAddr.sin_port = htons( uUnicastPort );
        MyAddr.sin_addr.S_un.S_addr = socketIPAddress;

        if( bind( sockfd, (struct sockaddr*) &MyAddr, sizeof( sockaddr_in ) ) == -1 )
        {
            printf( "[PacketClient Main] gDataSocket bind failed (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
            closesocket( sockfd );
            return -1;
        }
    }

    // create a 1MB buffer
    retval = setsockopt( sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &optval, 4 );
    if( retval == SOCKET_ERROR )
    {
        printf( "[PacketClient Main] gDataSocket setsockopt failed (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
    }
    retval = getsockopt( sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &optval, &optval_size );
    if( retval == SOCKET_ERROR )
    {
        printf( "[PacketClient Main] CreateDataSocket getsockopt failed (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
    }
    if( optval != OPTVAL_REQUEST_SIZE )
    {
        printf( "[PacketClient Main] gDataSocket ReceiveBuffer size = %d requested %d\n",
            optval, OPTVAL_REQUEST_SIZE );
    }

    // return working gDataSocket
    return sockfd;
}

/**
 * \brief - Prints configuration.
 * \param szMyIPAddress
 * \param szServerIPAddress 
 * \param useMulticast 
*/
void PrintConfiguration( const char* szMyIPAddress, const char* szServerIPAddress, bool useMulticast )
{
    printf( "Connection Configuration:\n" );
    printf( "  Client:          %s\n", szMyIPAddress );
    printf( "  Server:          %s\n", szServerIPAddress );
    printf( "  Command Port:    %d\n", PORT_COMMAND );
    printf( "  Data Port:       %d\n", PORT_DATA );

    if( useMulticast )
    {
        printf( "  Using Multicast\n" );
        printf( "  Multicast Group: %s\n", MULTICAST_ADDRESS );
    }
    else
    {
        printf( "  Using Unicast\n" );
    }
    printf( "  NatNet Server Info\n" );
    printf( "    Application Name %s\n", gServerName );
    printf( "    %s Version  %d %d %d %d\n", gServerName,
        gServerVersion[0], gServerVersion[1],
        gServerVersion[2], gServerVersion[3] );
    printf( "    NatNetVersion  %d %d %d %d\n",
        gNatNetVersionServer[0], gNatNetVersionServer[1],
        gNatNetVersionServer[2], gNatNetVersionServer[3] );
    printf( "  NatNet Bitstream Requested\n" );
    printf( "    NatNetVersion  %d %d %d %d\n",
        gNatNetVersion[0], gNatNetVersion[1],
        gNatNetVersion[2], gNatNetVersion[3] );
    printf( "    Can Change Bitstream Version = %s\n", ( gCanChangeBitstream ) ? "true" : "false" );
}

/**
 * \brief - Print Commands
 * \param canChangeBitstream - Unused parameter
*/
void PrintCommands( bool canChangeBitstream )
{
    printf( "Commands:\n"
        "Return Data from Motive\n"
        "  s  send data descriptions\n"
        "  r  resume/start frame playback\n"
        "  p  pause frame playback\n"
        "     pause may require several seconds\n"
        "     depending on the frame data size\n"
        "Change Working Range\n"
        "  o  reset Working Range to: start/current/end frame 0/0/end of take\n"
        "  w  set Working Range to: start/current/end frame 1/100/1500\n"
        "Change NatNet data stream version (Unicast only)\n"
        "  3 Request NatNet 3.1 data stream (Unicast only)\n"
        "  4 Request NatNet 4.0 data stream (Unicast only)\n"
        "  v Get the version of the currently streaming bitstream (Unicast Only)\n"
        "c  print configuration\n"
        "h  print commands\n"
        "q  quit\n"
        "\n"
        "NOTE: Motive frame playback will respond differently in\n"
        "       Endpoint, Loop, and Bounce playback modes.\n"
        "\n"
        "EXAMPLE: PacketClient [serverIP [ clientIP [ Multicast/Unicast]]]\n"
        "         PacketClient \"192.168.10.14\" \"192.168.10.14\" Multicast\n"
        "         PacketClient \"127.0.0.1\" \"127.0.0.1\" u\n"
        "\n"
    );
}

/**
 * \brief - Parse the command line arguments.
 * \param argc 
 * \param argv 
 * \param parsedArgs 
 * \return - Return Success or Failure
*/
bool MyParseArgs( int argc, char* argv[], sParsedArgs& parsedArgs )
{
    bool retval = true;

    // Process arguments
    // server address
    if( argc > 1 )
    {
        strcpy_s( parsedArgs.szServerIPAddress, argv[1] );	// specified on command line
        retval = IPAddress_StringToAddr( parsedArgs.szServerIPAddress, &parsedArgs.serverAddress );
    }
    // pull IP address from local IP addy
    else
    {
        // default to loopback
        retval = IPAddress_StringToAddr( parsedArgs.szServerIPAddress, &parsedArgs.serverAddress );
        // attempt to get address from local environment
        //GetLocalIPAddresses((unsigned long*)&parsedArgs.serverAddress, 1);
        // formatted print back to parsedArgs
        sprintf_s( parsedArgs.szServerIPAddress, "%d.%d.%d.%d",
            parsedArgs.serverAddress.S_un.S_un_b.s_b1,
            parsedArgs.serverAddress.S_un.S_un_b.s_b2,
            parsedArgs.serverAddress.S_un.S_un_b.s_b3,
            parsedArgs.serverAddress.S_un.S_un_b.s_b4 );
    }

    if( retval == false )
        return retval;

    // client address
    if( argc > 2 )
    {
        strcpy_s( parsedArgs.szMyIPAddress, argv[2] );	// specified on command line
        retval = IPAddress_StringToAddr( parsedArgs.szMyIPAddress, &parsedArgs.myAddress );
    }
    // pull IP address from local IP addy
    else
    {
        // default to loopback
        retval = IPAddress_StringToAddr( parsedArgs.szMyIPAddress, &parsedArgs.myAddress );
        // attempt to get IP from environment
        //GetLocalIPAddresses((unsigned long*)&parsedArgs.myAddress, 1);
        // print back to szMyIPAddress
        sprintf_s( parsedArgs.szMyIPAddress, "%d.%d.%d.%d",
            parsedArgs.myAddress.S_un.S_un_b.s_b1,
            parsedArgs.myAddress.S_un.S_un_b.s_b2,
            parsedArgs.myAddress.S_un.S_un_b.s_b3,
            parsedArgs.myAddress.S_un.S_un_b.s_b4 );
    }
    if( retval == false )
        return retval;


    // unicast/multicast
    if( ( argc > 3 ) && strlen( argv[3] ) )
    {
        char firstChar = toupper( argv[3][0] );
        switch( firstChar )
        {
        case 'M':
            parsedArgs.useMulticast = true;
            break;
        case 'U':
            parsedArgs.useMulticast = false;
            break;
        default:
            parsedArgs.useMulticast = true;
            break;
        }
    }
    return retval;
}

int main( int argc, char* argv[] )
{
    int retval = SOCKET_ERROR;
    sParsedArgs parsedArgs;

    WSADATA wsaData;
    int optval = OPTVAL_REQUEST_SIZE;
    int optval_size = 4;

    // Command Listener Attributes
    SECURITY_ATTRIBUTES commandListenSecurityAttribs;
    commandListenSecurityAttribs.nLength = sizeof( SECURITY_ATTRIBUTES );
    commandListenSecurityAttribs.lpSecurityDescriptor = nullptr;
    commandListenSecurityAttribs.bInheritHandle = TRUE;
    DWORD commandListenThreadID;
    HANDLE commandListenThreadHandle;

    // Data Listener Attributes
    SECURITY_ATTRIBUTES dataListenThreadSecurityAttribs;
    dataListenThreadSecurityAttribs.nLength = sizeof( SECURITY_ATTRIBUTES );
    dataListenThreadSecurityAttribs.lpSecurityDescriptor = nullptr;
    dataListenThreadSecurityAttribs.bInheritHandle = TRUE;
    DWORD dataListenThread_ID;
    HANDLE dataListenThread_Handle;

    // Start up winsock
    if( WSAStartup( 0x202, &wsaData ) == SOCKET_ERROR )
    {
        printf( "[PacketClient Main] WSAStartup failed (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        WSACleanup();
        return 0;
    }


    if( MyParseArgs( argc, argv, parsedArgs ) == false )
    {
        return -1;
    }
    gUseMulticast = parsedArgs.useMulticast;

    // multicast address - hard coded to MULTICAST_ADDRESS define above.
    parsedArgs.multiCastAddress.S_un.S_addr = inet_addr( MULTICAST_ADDRESS );

    // create "Command" socket
    int commandPort = 0;
    gCommandSocket = CreateCommandSocket( parsedArgs.myAddress.S_un.S_addr, commandPort, optval, gUseMulticast );
    if( gCommandSocket == -1 )
    {
        // error
        printf( "[PacketClient Main] gCommandSocket create failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        WSACleanup();
        return -1;
    }
    printf( "[PacketClient Main] gCommandSocket started\n" );

    // create the gDataSocket
    int dataPort = 0;
    gDataSocket = CreateDataSocket( parsedArgs.myAddress.S_un.S_addr, dataPort, optval,
        parsedArgs.useMulticast, parsedArgs.multiCastAddress.S_un.S_addr, PORT_DATA );
    if( gDataSocket == -1 )
    {
        printf( "[PacketClient Main] gDataSocket create failure (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        closesocket( gCommandSocket );
        WSACleanup();
        return -1;
    }
    printf( "[PacketClient Main] gDataSocket started\n" );

    // startup our "Command Listener" thread
    commandListenThreadHandle = CreateThread( &commandListenSecurityAttribs, 0, CommandListenThread, nullptr, 0, &commandListenThreadID );
    printf( "[PacketClient Main] CommandListenThread started\n" );

    // startup our "Data Listener" thread
    dataListenThread_Handle = CreateThread( &dataListenThreadSecurityAttribs, 0, DataListenThread, nullptr, 0, &dataListenThread_ID );
    printf( "[PacketClient Main] DataListenThread started\n" );

    // server address for commands
    memset( &gHostAddr, 0, sizeof( gHostAddr ) );
    gHostAddr.sin_family = AF_INET;
    gHostAddr.sin_port = htons( PORT_COMMAND );
    gHostAddr.sin_addr = parsedArgs.serverAddress;

    // send initial connect request
    sPacket* PacketOut = new sPacket;
    sSender sender;
    sConnectionOptions connectOptions;
    PacketOut->iMessage = NAT_CONNECT;
    PacketOut->nDataBytes = sizeof( sSender ) + sizeof( connectOptions ) + 4;
    memset( &sender, 0, sizeof( sender ) );
    memcpy( &PacketOut->Data, &sender, (int) sizeof( sSender ) );

    // [optional] Custom connection options
    // only send subscribed data
    connectOptions.subscribedDataOnly = false;
    // request a specific bit stream version.
    // Note : If not specified, Motive will send data in the most current version
    // Note : This is the NatNet version, not the Motive version
    gNatNetVersion[0] = 0;
    gNatNetVersion[1] = 0;
    gNatNetVersion[2] = 0;
    gNatNetVersion[3] = 0;
    connectOptions.BitstreamVersion[0] = gNatNetVersion[0];
    connectOptions.BitstreamVersion[1] = gNatNetVersion[1];
    connectOptions.BitstreamVersion[2] = gNatNetVersion[2];
    connectOptions.BitstreamVersion[3] = gNatNetVersion[3];
    memcpy( &PacketOut->Data.cData[(int) sizeof( sSender )], &connectOptions, sizeof( connectOptions ) );

    int nTries = 3;
    int iRet = SOCKET_ERROR;
    while( nTries-- )
    {
        iRet = sendto( gCommandSocket, (char*) PacketOut, 4 + PacketOut->nDataBytes, 0, (sockaddr*) &gHostAddr, sizeof( gHostAddr ) );
        if( iRet != SOCKET_ERROR )
            break;
    }
    if( iRet == SOCKET_ERROR )
    {
        printf( "[PacketClient Main] gCommandSocket sendto error (error: %s)\n", GetWSAErrorString( WSAGetLastError() ).c_str() );
        return -1;
    }

    // just to make things look more orderly on startup
    std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
    printf( "[PacketClient Main] Started\n\n" );
    PrintConfiguration( parsedArgs.szMyIPAddress, parsedArgs.szServerIPAddress, parsedArgs.useMulticast );
    PrintCommands( gCanChangeBitstream );

    int c;
    char szRequest[512] = { 0 };
    bool bExit = false;
    nTries = 3;
    iRet = SOCKET_ERROR;
    std::string errorString;
    while( !bExit )
    {
        c = _getch();
        switch( c )
        {
        case 's':
            // send NAT_REQUEST_MODELDEF command to server (will respond on the "Command Listener" thread)
            PacketOut->iMessage = NAT_REQUEST_MODELDEF;
            PacketOut->nDataBytes = 0;
            nTries = 3;
            iRet = SOCKET_ERROR;
            while( nTries-- )
            {
                iRet = sendto( gCommandSocket, (char*) PacketOut, 4 + PacketOut->nDataBytes, 0, (sockaddr*) &gHostAddr, sizeof( gHostAddr ) );
                if( iRet != SOCKET_ERROR )
                    break;
            }
            printf( "Command: NAT_REQUEST_MODELDEF returned value: %d%s\n", iRet, ( iRet == -1 ) ? " SOCKET_ERROR" : "" );
            break;
        case 'p':
        {
            char szCommand[512];
            sprintf( szCommand, "TimelineStop" );
            printf( "Command: %s - ", szCommand );
            int returnCode = SendCommand( szCommand );
            printf( " returnCode: %d\n", returnCode );
        }

        break;
        case 'r':
        {
            char szCommand[512];
            sprintf( szCommand, "TimelinePlay" );
            int returnCode = SendCommand( szCommand );
            printf( "Command: %s -  returnCode: %d\n", szCommand, returnCode );
        }

        break;
        case 'h':
            PrintCommands( gCanChangeBitstream );
            break;
        case 'c':
            PrintConfiguration( parsedArgs.szMyIPAddress, parsedArgs.szServerIPAddress, parsedArgs.useMulticast );
            break;
        case 'o':
        {
            char szCommand[512];
            long startFrameNum = 0;
            long endFrameNum = 100000;
            int returnCode;
            std::vector<std::string> commandVec{
                "TimelineStop",
                "SetPlaybackStartFrame,0",
                "SetPlaybackCurrentFrame,0",
                "SetPlaybackStopFrame,1000000",
                "SetPlaybackLooping,0",
                "TimelineStop"
            };

            for( const std::string& command : commandVec )
            {
                strcpy_s( szCommand, command.c_str() );
                returnCode = SendCommand( szCommand );
                printf( "Command: %s -  returnCode: %d\n", szCommand, returnCode );
            }
        }
        break;
        case 'w':
        {
            char szCommand[512];
            int returnCode;
            std::vector<std::string> commandVec{
                "TimelineStop",
                "SetPlaybackStartFrame,10",
                "SetPlaybackCurrentFrame,100",
                "SetPlaybackStopFrame,1500",
                "SetPlaybackLooping,0",
                "TimelineStop"
            };

            for( const std::string& command : commandVec )
            {
                strcpy_s( szCommand, command.c_str() );
                returnCode = SendCommand( szCommand );
                printf( "Command: %s -  returnCode: %d\n", szCommand, returnCode );
            }
        }
        break;
        case 'v':
        {
            printf( "Retrieving current bitstream version...\n" );
            GetBitstreamVersion();
        }
        break;
        case '3':
        {
            if( gCanChangeBitstream )
            {
                SetBitstreamVersion( 3, 1, 0 );
            }
            else
            {
                printf( "Bitstream changes allowed for Unicast with Motive >= 3 only\n" );
            }
        }
        break;
        case '4':
        {
            if( gCanChangeBitstream )
            {
                SetBitstreamVersion( 4, 0, 0 );
            }
            else
            {
                printf( "Bitstream changes allowed for Unicast with Motive >= 3 only\n" );
            }
        }
        break;
        case 'q':
            bExit = true;
            break;
        default:
            break;
        }
    }

    return 0;
}

// Send a command to Motive.  

/**
 * \brief - Send a command to Motive/NatNet server
 * \param szCommand 
 * \return - Command response
*/
int SendCommand( char* szCommand )
{
    // reset global result
    gCommandResponse = -1;

    // format command packet
    sPacket* commandPacket = new sPacket();
    strcpy( commandPacket->Data.szData, szCommand );
    commandPacket->iMessage = NAT_REQUEST;
    commandPacket->nDataBytes = (short) strlen( commandPacket->Data.szData ) + 1;

    // send command, and wait (a bit) for command response to set global response var in CommandListenThread
    int iRet = sendto( gCommandSocket, (char*) commandPacket, 4 + commandPacket->nDataBytes, 0, (sockaddr*) &gHostAddr, sizeof( gHostAddr ) );
    if( iRet == SOCKET_ERROR )
    {
        printf( "Socket error sending command\n" );
    }
    else
    {
        int waitTries = 5;
        while( waitTries-- )
        {
            if( gCommandResponse != -1 )
                break;
            Sleep( 30 );
        }

        if( gCommandResponse == -1 )
        {
            printf( "Command response not received (timeout)\n" );
        }
        else if( gCommandResponse == 0 )
        {
            printf( "Command response received with success\n" );
        }
        else if( gCommandResponse > 0 )
        {
            printf( "Command response received with errors\n" );
        }
        else
        {
            printf( "Command response unknown value=%d\n", gCommandResponse );
        }
    }

    return gCommandResponse;
}

// 


/**
 * \brief - Convert IP address string to address
 * \param szNameOrAddress - server name or address
 * \param Address - IP address response
 * \return success or failure
*/
bool IPAddress_StringToAddr( char* szNameOrAddress, struct in_addr* Address )
{
    int retVal;
    struct sockaddr_in saGNI;
    char hostName[MAX_NAMELENGTH];
    char servInfo[MAX_NAMELENGTH];
    u_short port;
    port = 0;

    // Set up sockaddr_in structure which is passed to the getnameinfo function
    saGNI.sin_family = AF_INET;
    saGNI.sin_addr.s_addr = inet_addr( szNameOrAddress );
    saGNI.sin_port = htons( port );

    // getnameinfo in WS2tcpip is protocol independent and resolves address to ANSI host name
    if( ( retVal = getnameinfo( (SOCKADDR*) &saGNI, sizeof( sockaddr ), hostName, MAX_NAMELENGTH, servInfo, MAX_NAMELENGTH, NI_NUMERICSERV ) ) != 0 )
    {
        // Returns error if getnameinfo failed
        printf( "[PacketClient Main] GetHostByAddr failed. Error #: %ld\n", WSAGetLastError() );
        return false;
    }

    Address->S_un.S_addr = saGNI.sin_addr.S_un.S_addr;

    return true;
}

// 

/**
 * \brief - get ip addresses on local host
 * \param Addresses - returned local IP address 
 * \param nMax - length of Addresses array
 * \return - number of Addresses returned.
*/
int GetLocalIPAddresses( unsigned long Addresses[], int nMax )
{
    unsigned long  NameLength = 128;
    char szMyName[1024];
    struct addrinfo aiHints;
    struct addrinfo* aiList = nullptr;
    struct sockaddr_in addr;
    int retVal = 0;
    char* port = "0";

    if( GetComputerName( szMyName, &NameLength ) != TRUE )
    {
        printf( "[PacketClient Main] get computer name  failed. Error #: %ld\n", WSAGetLastError() );
        return 0;
    };

    memset( &aiHints, 0, sizeof( aiHints ) );
    aiHints.ai_family = AF_INET;
    aiHints.ai_socktype = SOCK_DGRAM;
    aiHints.ai_protocol = IPPROTO_UDP;

    // Take ANSI host name and translates it to an address
    if( ( retVal = getaddrinfo( szMyName, port, &aiHints, &aiList ) ) != 0 )
    {
        printf( "[PacketClient Main] getaddrinfo failed. Error #: %ld\n", WSAGetLastError() );
        return 0;
    }

    memcpy( &addr, aiList->ai_addr, aiList->ai_addrlen );
    freeaddrinfo( aiList );
    Addresses[0] = addr.sin_addr.S_un.S_addr;

    return 1;
}

/**
 * \brief Funtion that assigns a time code values to 5 variables passed as arguments
 * Requires an integer from the packet as the timecode and timecodeSubframe
 * \param inTimecode - input time code
 * \param inTimecodeSubframe - input time code sub frame
 * \param hour - output hour
 * \param minute - output minute
 * \param second - output second
 * \param frame - output frame number 0 to 255
 * \param subframe - output subframe number
 * \return - true
*/
bool DecodeTimecode( unsigned int inTimecode, unsigned int inTimecodeSubframe, int* hour, int* minute, int* second, int* frame, int* subframe )
{
    bool bValid = true;

    *hour = ( inTimecode >> 24 ) & 255;
    *minute = ( inTimecode >> 16 ) & 255;
    *second = ( inTimecode >> 8 ) & 255;
    *frame = inTimecode & 255;
    *subframe = inTimecodeSubframe;

    return bValid;
}

/**
 * \brief Takes timecode and assigns it to a string
 * \param inTimecode  - input time code
 * \param inTimecodeSubframe - input time code subframe
 * \param Buffer - output buffer
 * \param BufferSize - output buffer size
 * \return 
*/
bool TimecodeStringify( unsigned int inTimecode, unsigned int inTimecodeSubframe, char* Buffer, int BufferSize )
{
    bool bValid;
    int hour, minute, second, frame, subframe;
    bValid = DecodeTimecode( inTimecode, inTimecodeSubframe, &hour, &minute, &second, &frame, &subframe );

    sprintf_s( Buffer, BufferSize, "%2d:%2d:%2d:%2d.%d", hour, minute, second, frame, subframe );
    for( unsigned int i = 0; i < strlen( Buffer ); i++ )
        if( Buffer[i] == ' ' )
            Buffer[i] = '0';

    return bValid;
}

/**
 * \brief Decode marker ID
 * \param sourceID - input source ID
 * \param pOutEntityID - output entity ID
 * \param pOutMemberID - output member ID
*/
void DecodeMarkerID( int sourceID, int& modelID, int& markerID )
{
        modelID  = sourceID >> 16;
        markerID = sourceID & 0x0000ffff;
}

/**
 * \brief Receives pointer to byes of a data description and decodes based on major/minor version
 * \param inptr - input 
 * \param nBytes - input buffer size 
 * \param major - NatNet Major version
 * \param minor - NatNet Minor version
 * \return - pointer to after decoded object
*/
char* UnpackDescription( char* inptr, int nBytes, int major, int minor, unsigned int level)
{
    std::string outTabStr = GetTabString( kTabStr, level );
    char* ptr = inptr;
    char* targetPtr = ptr + nBytes;
    long long nBytesProcessed = (long long) ptr - (long long) inptr;
    // number of datasets
    int nDatasets = 0; memcpy( &nDatasets, ptr, 4 ); ptr += 4;
    printf( "%sDataset Count : %d\n", outTabStr.c_str(), nDatasets );
#ifdef VDEBUG
    int datasetCounts[kValidDataTypes+1] = { 0,0,0,0,0,0,0 };
#endif
    bool errorDetected = false;
    for( int i = 0; i < nDatasets; i++ )
    {
        printf( "%sDataset %d\n", outTabStr.c_str(), i );
#ifdef VDEBUG
        int nBytesUsed = (long long) ptr - (long long) inptr;
        int nBytesRemaining = nBytes - nBytesUsed;
        printf( "%sBytes Decoded: %d Bytes Remaining: %d)\n", outTabStr.c_str(),
            nBytesUsed, nBytesRemaining );
#endif

        // Determine type and advance
        // The next type entry is inaccurate 
        // if data descriptions are out of date
        int type = 0;
        memcpy( &type, ptr, 4 ); ptr += 4;
        
        // size of data description (in bytes)
        // Unlike frame data, in which all data for a particular type
        // is bundled together, descriptions are not guaranteed to be so,
        // so the size here is per description, not for 'all data of a type'
        int sizeInBytes = 0;
        memcpy(&sizeInBytes, ptr, 4); ptr += 4;

#ifdef VDEBUG
        if( ( 0 <= type ) && ( type <= kValidDataTypes ) )
        {
            datasetCounts[type] += 1;
        }
        else
        {
            datasetCounts[kValidDataTypes+1] += 1;
        }
#endif

        switch( type )
        {
        case 0: // Markerset
        {
            printf( "%sType: 0 Markerset\n", outTabStr.c_str() );
            ptr = UnpackMarkersetDescription( ptr, targetPtr, major, minor, level+1 );
        }
        break;
        case 1: // rigid body
            printf( "%sType: 1 Rigid Body\n", outTabStr.c_str() );
            ptr = UnpackRigidBodyDescription( ptr, targetPtr, major, minor, level + 4 );
            break;
        case 2: // skeleton
            printf( "%sType: 2 Skeleton\n", outTabStr.c_str() );
            ptr = UnpackSkeletonDescription( ptr, targetPtr, major, minor, level + 4 );
            break;
        case 3: // force plate
            printf( "%sType: 3 Force Plate\n", outTabStr.c_str() );
            ptr = UnpackForcePlateDescription( ptr, targetPtr, major, minor, level + 4 );
            break;
        case 4: // device
            printf( "%sType: 4 Device\n", outTabStr.c_str() );
            ptr = UnpackDeviceDescription( ptr, targetPtr, major, minor, level + 4 );
            break;
        case 5: // camera
            printf( "%sType: 5 Camera\n", outTabStr.c_str() );
            ptr = UnpackCameraDescription( ptr, targetPtr, major, minor, level + 4 );
            break;
        case 6: // asset
            printf( "%sType: 6 Asset\n", outTabStr.c_str() );
            ptr = UnpackAssetDescription(ptr, targetPtr, major, minor, level + 4 );
            break;
        default: // unknown type
            printf( "%sType: %d UNKNOWN\n", outTabStr.c_str(), type );
            printf( "%sERROR: Type decode failure\n", outTabStr.c_str() );
            errorDetected = true;
            break;
        }
        if( errorDetected )
        {
            printf( "%sERROR: Stopping decode\n", outTabStr.c_str() );
            break;
        }
        if( ptr > targetPtr )
        {
            printf( "%sUnpackDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }
        printf( "%s\t%d datasets processed of %d\n", outTabStr.c_str(), ( i + 1 ), nDatasets );
        printf( "%s\t%lld bytes processed of %d\n", outTabStr.c_str(), ( (long long) ptr - (long long) inptr ), nBytes );
    }   // next dataset

#ifdef VDEBUG
    printf( "%sCnt Type    Description\n", outTabStr.c_str() );
    for( int i = 0; i < kValidDataTypes+1; ++i )
    {
        printf( "%s%3.3d ", outTabStr.c_str(), datasetCounts[i] );
        switch( i )
        {
        case 0: // Markerset
            printf( "Type: 0 Markerset\n" );
            break;
        case 1: // rigid body
            printf( "Type: 1 rigid body\n" );
            break;
        case 2: // skeleton
            printf( "Type: 2 skeleton\n" );
            break;
        case 3: // force plate
            printf( "Type: 3 force plate\n" );
            break;
        case 4: // device
            printf( "Type: 4 device\n" );
            break;
        case 5: // camera
            printf("Type: 5 camera\n" );
            break;
        case 6: // asset
            printf("Type: 6 asset\n" );
            break;
        default:
            printf( "Type: %d UNKNOWN\n", i );
            break;
        }
    }
#endif
    return ptr;
}


/**
 * \brief Unpack markerset description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackMarkersetDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );

    // name
    char szName[MAX_NAMELENGTH];
    strcpy_s( szName, ptr );
    int nDataBytes = (int) strlen( szName ) + 1;
    ptr += nDataBytes;
    MakeAlnum( szName, MAX_NAMELENGTH );
    printf( "%sMarkerset Name: %s\n", outTabStr.c_str(), szName );

    // marker data
    int nMarkers = 0; memcpy( &nMarkers, ptr, 4 ); ptr += 4;
    printf( "%sMarker Count : %d\n", outTabStr.c_str(), nMarkers );

    for( int j = 0; j < nMarkers; j++ )
    {
        char szName[MAX_NAMELENGTH];
        strcpy_s( szName, ptr );
        int nDataBytes = (int) strlen( ptr ) + 1;
        ptr += nDataBytes;
        MakeAlnum( szName, MAX_NAMELENGTH );
        printf( "%s  %3.1d Marker Name: %s\n", outTabStr.c_str(), j, szName );
        if( ptr > targetPtr )
        {
            printf( "%sUnpackMarkersetDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }
    }

    return ptr;
}


/**
 * \brief Unpack Rigid Body description and print it.
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackRigidBodyDescription( char* inptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );

    char* ptr = inptr;
    int nBytes = 0; // common scratch variable
    if( ( major >= 2 ) || ( major == 0 ) )
    {
        // RB name
        char szName[MAX_NAMELENGTH];
        strcpy_s( szName, ptr );
        ptr += strlen( ptr ) + 1;
        MakeAlnum( szName, MAX_NAMELENGTH );
        printf( "%sRigid Body Name: %s\n", outTabStr.c_str(), szName );
    }

    int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;
    printf( "%sRigidBody ID   : %d\n", outTabStr.c_str(), ID );

    int parentID = 0; memcpy( &parentID, ptr, 4 ); ptr += 4;
    printf( "%sParent ID      : %d\n", outTabStr.c_str(), parentID );

    // Position Offsets
    float xoffset = 0; memcpy( &xoffset, ptr, 4 ); ptr += 4;
    float yoffset = 0; memcpy( &yoffset, ptr, 4 ); ptr += 4;
    float zoffset = 0; memcpy( &zoffset, ptr, 4 ); ptr += 4;
    printf( "%sPosition       : [%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), xoffset, yoffset, zoffset );

    if( ptr > targetPtr )
    {
        printf( "%sUnpackRigidBodyDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
        return ptr;
    }

    if( major > 4 || ( major == 4 && minor >= 2 ) || major == 0 )
    {
        // Rotation Offsets
        float qxoffset = 0; memcpy( &qxoffset, ptr, 4 ); ptr += 4;
        float qyoffset = 0; memcpy( &qyoffset, ptr, 4 ); ptr += 4;
        float qzoffset = 0; memcpy( &qzoffset, ptr, 4 ); ptr += 4;
        float qwoffset = 0; memcpy( &qwoffset, ptr, 4 ); ptr += 4;
        printf( "%sRotation       : [%3.2f, %3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), qxoffset, qyoffset, qzoffset, qwoffset );

        if( ptr > targetPtr )
        {
            printf( "%sUnpackRigidBodyDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }
    }

    if( ( major >= 3 ) || ( major == 0 ) )
    {
        int nMarkers = 0; memcpy( &nMarkers, ptr, 4 ); ptr += 4;
        printf( "%sNumber of Markers : %d\n", outTabStr.c_str(), nMarkers );
        if( nMarkers > 16000 )
        {
            int nBytesProcessed = (int) ( targetPtr - ptr );
            printf( "%sUnpackRigidBodyDescription: UNPACK ERROR DETECTED: STOPPING DECODE at %d processed\n", outTabStr.c_str(),
                nBytesProcessed );
            printf( "%s  Unreasonable number of markers\n", outTabStr.c_str() );
            return targetPtr + 4;
        }

        if( nMarkers > 0 )
        {

            printf( "%sMarker Positions:\n", outTabStr.c_str() );
            char* ptr2 = ptr + ( nMarkers * sizeof( float ) * 3 );
            char* ptr3 = ptr2 + ( nMarkers * sizeof( int ) );
            for( int markerIdx = 0; markerIdx < nMarkers; ++markerIdx )
            {
                float xpos, ypos, zpos;
                int32_t label;
                char szMarkerNameUTF8[MAX_NAMELENGTH] = { 0 };
                char szMarkerName[MAX_NAMELENGTH] = { 0 };
                // marker positions
                memcpy( &xpos, ptr, 4 ); ptr += 4;
                memcpy( &ypos, ptr, 4 ); ptr += 4;
                memcpy( &zpos, ptr, 4 ); ptr += 4;

                // Marker Required activeLabels
                memcpy( &label, ptr2, 4 ); ptr2 += 4;

                // Marker Name
                szMarkerName[0] = 0;
                if( ( major >= 4 ) || ( major == 0 ) )
                {
                    strcpy_s( szMarkerName, ptr3 );
                    ptr3 += strlen( ptr3 ) + 1;
                }

                printf( "%s%3.1d Marker Label: %3.1d Position: [%3.2f %3.2f %3.2f] %s\n",
                    outTabStr.c_str(),
                    markerIdx, label, xpos, ypos, zpos, szMarkerName );
                if( ptr3 > targetPtr )
                {
                    printf( "%sUnpackRigidBodyDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
                    return ptr3;
                }
            }
            ptr = ptr3; // advance to the end of the labels & marker names
        }
    }

    if( ptr > targetPtr )
    {
        printf( "%sUnpackRigidBodyDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
        return ptr;
    }
    printf( "%s  UnpackRigidBodyDescription processed %lld bytes\n", outTabStr.c_str(), ( (long long) ptr - (long long) inptr ) );
    return ptr;
}


/**
 * \brief Unpack skeleton description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackSkeletonDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );

    char szName[MAX_NAMELENGTH];
    // Name
    strcpy_s( szName, ptr );
    ptr += strlen( ptr ) + 1;
    MakeAlnum( szName, MAX_NAMELENGTH );
    printf( "%sName: %s\n", outTabStr.c_str(), szName );

    // ID
    int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;
    printf( "%sID : %d\n", outTabStr.c_str(), ID );

    // # of RigidBodies
    int nRigidBodies = 0; memcpy( &nRigidBodies, ptr, 4 ); ptr += 4;
    printf( "%sRigidBody (Bone) Count : %d\n", outTabStr.c_str(), nRigidBodies );

    if( ptr > targetPtr )
    {
        printf( "%sUnpackSkeletonDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
        return ptr;
    }

    for( int i = 0; i < nRigidBodies; i++ )
    {
        printf( "%sRigid Body (Bone) %d:\n", outTabStr.c_str(), i );
        ptr = UnpackRigidBodyDescription( ptr, targetPtr, major, minor, level + 1 );
        if( ptr > targetPtr )
        {
            printf( "%sUnpackSkeletonDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }
    }
    return ptr;
}


/**
 * \brief Unpack force plate description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackForcePlateDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );

    if( ( major >= 3 ) || ( major == 0 ) )
    {
        // ID
        int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;
        printf( "%sID : %d\n", outTabStr.c_str(), ID );

        // Serial Number
        char strSerialNo[128];
        strcpy_s( strSerialNo, ptr );
        ptr += strlen( ptr ) + 1;
        printf( "%sSerial Number : %s\n", outTabStr.c_str(), strSerialNo );

        // Dimensions
        float fWidth = 0; memcpy( &fWidth, ptr, 4 ); ptr += 4;
        printf( "%sWidth : %3.2f\n", outTabStr.c_str(), fWidth );

        float fLength = 0; memcpy( &fLength, ptr, 4 ); ptr += 4;
        printf( "%sLength : %3.2f\n", outTabStr.c_str(), fLength );

        // Origin
        float fOriginX = 0; memcpy( &fOriginX, ptr, 4 ); ptr += 4;
        float fOriginY = 0; memcpy( &fOriginY, ptr, 4 ); ptr += 4;
        float fOriginZ = 0; memcpy( &fOriginZ, ptr, 4 ); ptr += 4;
        printf( "%sOrigin : [%3.2f,  %3.2f,  %3.2f]\n", outTabStr.c_str(), fOriginX, fOriginY, fOriginZ );

        // Calibration Matrix
        const int kCalMatX = 12;
        const int kCalMatY = 12;
        float fCalMat[kCalMatX][kCalMatY];
        printf( "%sCal Matrix:\n", outTabStr.c_str() );
        int rowCount = 0;
        for( auto& calMatX : fCalMat )
        {
            printf( "%s  ", outTabStr.c_str() );
            printf( "%3.1d ", rowCount );
            for( float& calMatY : calMatX )
            {
                memcpy( &calMatY, ptr, 4 ); ptr += 4;
                printf( "%3.3e ", calMatY );
            }
            printf( "\n" );
            ++rowCount;
        }

        // Corners
        const int kCornerX = 4;
        const int kCornerY = 3;
        float fCorners[kCornerX][kCornerY] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} };
        rowCount = 0;
        printf( "%sCorners:\n", outTabStr.c_str() );
        for( auto& fCorner : fCorners )
        {
            printf( "%s  ", outTabStr.c_str() );
            printf( "%3.1d ", rowCount );
            for( float& cornerY : fCorner )
            {
                memcpy( &cornerY, ptr, 4 ); ptr += 4;
                printf( "%3.3e ", cornerY );
            }
            printf( "\n" );
            ++rowCount;
        }

        // Plate Type
        int iPlateType = 0; memcpy( &iPlateType, ptr, 4 ); ptr += 4;
        printf( "%sPlate Type : %d\n", outTabStr.c_str(), iPlateType );

        // Channel Data Type
        int iChannelDataType = 0; memcpy( &iChannelDataType, ptr, 4 ); ptr += 4;
        printf( "%sChannel Data Type : %d\n", outTabStr.c_str(), iChannelDataType );

        // Number of Channels
        int nChannels = 0; memcpy( &nChannels, ptr, 4 ); ptr += 4;
        printf( "%sNumber of Channels : %d\n", outTabStr.c_str(), nChannels );
        if( ptr > targetPtr )
        {
            printf( "%sUnpackSkeletonDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }

        for( int chNum = 0; chNum < nChannels; ++chNum )
        {
            char szName[MAX_NAMELENGTH];
            strcpy_s( szName, ptr );
            int nDataBytes = (int) strlen( szName ) + 1;
            ptr += nDataBytes;
            printf( "%sChannel Name %d: %s\n", outTabStr.c_str(), chNum, szName );
            if( ptr > targetPtr )
            {
                printf( "%sUnpackSkeletonDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
                return ptr;
            }
        }
    }
    return ptr;
}


/**
 * \brief Unpack device description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackDeviceDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    if( ( major >= 3 ) || ( major == 0 ) )
    {
        int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;
        printf( "%sID                 : %d\n", outTabStr.c_str(), ID );

        // Name
        char strName[128];
        strcpy_s( strName, ptr );
        ptr += strlen( ptr ) + 1;
        printf( "%sName               : %s\n", outTabStr.c_str(), strName );

        // Serial Number
        char strSerialNo[128];
        strcpy_s( strSerialNo, ptr );
        ptr += strlen( ptr ) + 1;
        printf( "%sSerial Number      : %s\n", outTabStr.c_str(), strSerialNo );

        int iDeviceType = 0; memcpy( &iDeviceType, ptr, 4 ); ptr += 4;
        printf( "%sDevice Type        : %d\n", outTabStr.c_str(), iDeviceType );

        int iChannelDataType = 0; memcpy( &iChannelDataType, ptr, 4 ); ptr += 4;
        printf( "%sChannel Data Type  : %d\n", outTabStr.c_str(), iChannelDataType );

        int nChannels = 0; memcpy( &nChannels, ptr, 4 ); ptr += 4;
        printf( "%sNumber of Channels : %d\n", outTabStr.c_str(), nChannels );
        char szChannelName[MAX_NAMELENGTH];

        if( ptr > targetPtr )
        {
            printf( "%sUnpackDeviceDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }

        for( int chNum = 0; chNum < nChannels; ++chNum )
        {
            strcpy_s( szChannelName, ptr );
            ptr += strlen( ptr ) + 1;
            printf( "%s  Channel %d Name  : %s\n", outTabStr.c_str(), chNum, szChannelName );
            if( ptr > targetPtr )
            {
                printf( "%sUnpackDeviceDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
                return ptr;
            }
        }
    }

    return ptr;
}

/**
 * \brief Unpack camera description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackCameraDescription( char* ptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Name
    char szName[MAX_NAMELENGTH];
    strcpy_s( szName, ptr );
    ptr += strlen( ptr ) + 1;
    MakeAlnum( szName, MAX_NAMELENGTH );
    printf( "%sName  : %s\n", outTabStr.c_str(), szName );

    // Pos
    float cameraPosition[3];
    memcpy( cameraPosition + 0, ptr, 4 ); ptr += 4;
    memcpy( cameraPosition + 1, ptr, 4 ); ptr += 4;
    memcpy( cameraPosition + 2, ptr, 4 ); ptr += 4;
    printf( "%s  Position   : [%3.2f, %3.2f, %3.2f]\n",
        outTabStr.c_str(),
        cameraPosition[0], cameraPosition[1],
        cameraPosition[2] );

    // Ori
    float cameraOriQuat[4]; // x, y, z, w
    memcpy( cameraOriQuat + 0, ptr, 4 ); ptr += 4;
    memcpy( cameraOriQuat + 1, ptr, 4 ); ptr += 4;
    memcpy( cameraOriQuat + 2, ptr, 4 ); ptr += 4;
    memcpy( cameraOriQuat + 3, ptr, 4 ); ptr += 4;
    printf( "%s  Orientation: [%3.2f, %3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(),
        cameraOriQuat[0], cameraOriQuat[1],
        cameraOriQuat[2], cameraOriQuat[3] );

    return ptr;
}

/**
 * \brief Unpack marker description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackMarkerDescription(char* ptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Name
    char szName[MAX_NAMELENGTH];
    strcpy_s(szName, ptr);
    ptr += strlen(ptr) + 1;
    MakeAlnum(szName, MAX_NAMELENGTH);
    printf("%sName : %s\n", outTabStr.c_str(), szName);

    // ID
    int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
    printf("%sID : %d\n", outTabStr.c_str(), ID);

    // initial position
    float pos[3];
    memcpy(pos + 0, ptr, 4); ptr += 4;
    memcpy(pos + 1, ptr, 4); ptr += 4;
    memcpy(pos + 2, ptr, 4); ptr += 4;
    printf("%sPosition   : [%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(),
        pos[0], pos[1], pos[2]);

    // size
    float size = 0;
    memcpy(&size, ptr, 4); ptr += 4;
    printf("%sSize : %.2f\n", outTabStr.c_str(), size);

    // params
    int16_t params = 0;
    memcpy(&params, ptr, 2); ptr += 2;
    printf("%sParams : %d\n", outTabStr.c_str(), params);

    return ptr;
}

/**
 * \brief Unpack asset description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackAssetDescription(char* ptr, char* targetPtr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    char szName[MAX_NAMELENGTH];
    // Name
    strcpy_s(szName, ptr);
    ptr += strlen(ptr) + 1;
    MakeAlnum(szName, MAX_NAMELENGTH);
    printf("%sName       : %s\n",outTabStr.c_str(), szName);

    // asset type
    int type = 0; memcpy(&type, ptr, 4); ptr += 4;
    printf("%sType       : %d\n", outTabStr.c_str(), type);

    // ID
    int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
    printf("%sID         : %d\n", outTabStr.c_str(), ID);

    // # of RigidBodies
    int nRigidBodies = 0; memcpy(&nRigidBodies, ptr, 4); ptr += 4;
    printf("%sRigidBody (Bone) Count : %d\n", outTabStr.c_str(), nRigidBodies);

    if (ptr > targetPtr)
    {
        printf("%sUnpackAssetDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
        return ptr;
    }

    for (int i = 0; i < nRigidBodies; i++)
    {
        printf("%sRigid Body (Bone) %d:\n", outTabStr.c_str(), i);
        ptr = UnpackRigidBodyDescription(ptr, targetPtr, major, minor, level + 1);
        if (ptr > targetPtr)
        {
            printf("%sUnpackAssetDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }
    }

    // # of Markers
    int nMarkers = 0; memcpy(&nMarkers, ptr, 4); ptr += 4;
    printf("%sMarker Count : %d\n", outTabStr.c_str(), nMarkers);
    for (int i = 0; i < nMarkers; i++)
    {
        printf("%sMarker %d:\n", outTabStr.c_str(), i);
        ptr = UnpackMarkerDescription(ptr, targetPtr, major, minor, level + 1);
        if (ptr > targetPtr)
        {
            printf("%sUnpackAssetDescription: UNPACK ERROR DETECTED: STOPPING DECODE\n", outTabStr.c_str() );
            return ptr;
        }
    }

    return ptr;
}

/**
 * \brief Unpack frame description and print contents
 * \param ptr - input data stream pointer
 * \param targetPtr - pointer to maximum input memory location
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackFrameData( char* inptr, int nBytes, int major, int minor, unsigned int level )
{
    char* ptr = inptr;
    ptr = UnpackFramePrefixData( ptr, major, minor, level );

    ptr = UnpackMarkersetData( ptr, major, minor, level );

    ptr = UnpackLegacyOtherMarkers( ptr, major, minor, level );

    ptr = UnpackRigidBodyData( ptr, major, minor, level );

    ptr = UnpackSkeletonData( ptr, major, minor, level );

    // Assets ( Motive 3.1 / NatNet 4.1 and greater)
    if (((major == 4) && (minor > 0)) || (major > 4))
    {
        ptr = UnpackAssetData(ptr, major, minor, level );
    }

    ptr = UnpackLabeledMarkerData( ptr, major, minor, level );

    ptr = UnpackForcePlateData( ptr, major, minor, level );

    ptr = UnpackDeviceData( ptr, major, minor, level );

    ptr = UnpackFrameSuffixData( ptr, major, minor, level );

    return ptr;
}

/**
 * \brief Unpack frame prefix data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackFramePrefixData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Next 4 Bytes is the frame number
    int frameNumber = 0; memcpy( &frameNumber, ptr, 4 ); ptr += 4;
    printf( "%sFrame #: %3.1d\n", outTabStr.c_str(), frameNumber );
    return ptr;
}

/**
 * \brief legacy 'other' unlabeled marker and print contents (will be deprecated)
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackLegacyOtherMarkers(char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // First 4 Bytes is the number of Other markers
    int nOtherMarkers = 0; memcpy(&nOtherMarkers, ptr, 4); ptr += 4;
    printf("%sUnlabeled Marker Count : %3.1d\n", outTabStr.c_str(), nOtherMarkers);
    int nBytes;
    ptr = UnpackDataSize(ptr, major, minor,nBytes,level+1);

    for (int j = 0; j < nOtherMarkers; j++)
    {
        float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
        float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
        float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
        printf("%s  Marker %3.1d pos : [x=%3.2f,y=%3.2f,z=%3.2f]\n", outTabStr.c_str(), j, x, y, z);
    }

    return ptr;
}

/**
 * \brief Unpack markerset data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackMarkersetData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // First 4 Bytes is the number of data sets (markersets, rigidbodies, etc)
    int nMarkerSets = 0; memcpy( &nMarkerSets, ptr, 4 ); ptr += 4;
    printf( "%sMarkerset Count : %3.1d\n", outTabStr.c_str(), nMarkerSets );

    int nBytes=0;
    ptr = UnpackDataSize(ptr, major, minor,nBytes, level+1);

    // Loop through number of marker sets and get name and data
    for( int i = 0; i < nMarkerSets; i++ )
    {
        // Markerset name
        char szName[MAX_NAMELENGTH];
        strcpy_s( szName, ptr );
        int nDataBytes = (int) strlen( szName ) + 1;
        ptr += nDataBytes;
        MakeAlnum( szName, MAX_NAMELENGTH );
        printf( "%sMarkerData:\n", outTabStr.c_str() );
        printf( "%sModel Name       : %s\n", outTabStr.c_str(), szName );

        // marker data
        int nMarkers = 0; memcpy( &nMarkers, ptr, 4 ); ptr += 4;
        printf( "%sMarker Count     : %3.1d\n", outTabStr.c_str(), nMarkers );

        for( int j = 0; j < nMarkers; j++ )
        {
            float x = 0; memcpy( &x, ptr, 4 ); ptr += 4;
            float y = 0; memcpy( &y, ptr, 4 ); ptr += 4;
            float z = 0; memcpy( &z, ptr, 4 ); ptr += 4;
            printf( "%s  Marker %3.1d pos : [x=%3.2f,y=%3.2f,z=%3.2f]\n", outTabStr.c_str(), j, x, y, z );
        }
    }

    return ptr;
}


/**
 * \brief Unpack rigid body data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackRigidBodyData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Loop through rigidbodies
    int nRigidBodies = 0;
    memcpy( &nRigidBodies, ptr, 4 ); ptr += 4;
    printf( "%sRigid Body Count : %3.1d\n", outTabStr.c_str(), nRigidBodies );

    int nBytes=0;
    ptr = UnpackDataSize(ptr, major, minor,nBytes, level + 1);

    for( int j = 0; j < nRigidBodies; j++ )
    {
        // Rigid body position and orientation 
        int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;
        float x = 0.0f; memcpy( &x, ptr, 4 ); ptr += 4;
        float y = 0.0f; memcpy( &y, ptr, 4 ); ptr += 4;
        float z = 0.0f; memcpy( &z, ptr, 4 ); ptr += 4;
        float qx = 0; memcpy( &qx, ptr, 4 ); ptr += 4;
        float qy = 0; memcpy( &qy, ptr, 4 ); ptr += 4;
        float qz = 0; memcpy( &qz, ptr, 4 ); ptr += 4;
        float qw = 0; memcpy( &qw, ptr, 4 ); ptr += 4;
        printf( "%s  Rigid Body      : %3.1d\n", outTabStr.c_str(), j );
        printf( "%s    ID            : %3.1d\n", outTabStr.c_str(), ID );
        printf( "%s    Position      : [%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), x, y, z );
        printf( "%s    Orientation   : [%3.2f, %3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), qx, qy, qz, qw );

        // Marker positions removed as redundant (since they can be derived from RB Pos/Ori plus initial offset) in NatNet 3.0 and later to optimize packet size
        if( major < 3 )
        {
            // Associated marker positions
            int nRigidMarkers = 0;  memcpy( &nRigidMarkers, ptr, 4 ); ptr += 4;
            printf( "%sMarker Count: %d\n", outTabStr.c_str(), nRigidMarkers );
            int nBytes = nRigidMarkers * 3 * sizeof( float );
            float* markerData = (float*) malloc( nBytes );
            memcpy( markerData, ptr, nBytes );
            ptr += nBytes;

            // NatNet Version 2.0 and later
            if( major >= 2 )
            {
                // Associated marker IDs
                nBytes = nRigidMarkers * sizeof( int );
                int* markerIDs = (int*) malloc( nBytes );
                memcpy( markerIDs, ptr, nBytes );
                ptr += nBytes;

                // Associated marker sizes
                nBytes = nRigidMarkers * sizeof( float );
                float* markerSizes = (float*) malloc( nBytes );
                memcpy( markerSizes, ptr, nBytes );
                ptr += nBytes;

                for( int k = 0; k < nRigidMarkers; k++ )
                {
                    printf( "%s  Marker %d: id=%d  size=%3.1f  pos=[%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(),
                        k, markerIDs[k], markerSizes[k],
                        markerData[k * 3], markerData[k * 3 + 1], markerData[k * 3 + 2] );
                }

                if( markerIDs )
                    free( markerIDs );
                if( markerSizes )
                    free( markerSizes );

            }
            // Print marker positions for all rigid bodies
            else
            {
                int k3;
                for( int k = 0; k < nRigidMarkers; k++ )
                {
                    k3 = k * 3;
                    printf( "%s  Marker %d: pos : [%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(),
                        k, markerData[k3], markerData[k3 + 1], markerData[k3 + 2] );
                }
            }

            if( markerData )
                free( markerData );
        }

        // NatNet version 2.0 and later
        if( ( major >= 2 ) || ( major == 0 ) )
        {
            // Mean marker error
            float fError = 0.0f; memcpy( &fError, ptr, 4 ); ptr += 4;
            printf( "%s  Marker Error: %3.2f\n", outTabStr.c_str(), fError );
        }

        // NatNet version 2.6 and later
        if( ( ( major == 2 ) && ( minor >= 6 ) ) || ( major > 2 ) || ( major == 0 ) )
        {
            // params
            short params = 0; memcpy( &params, ptr, 2 ); ptr += 2;
            bool bTrackingValid = params & 0x01; // 0x01 : rigid body was successfully tracked in this frame
            printf( "%s  Tracking Valid: %s\n", outTabStr.c_str(), ( bTrackingValid ) ? "True" : "False" );
        }

    } // Go to next rigid body


    return ptr;
}


/**
 * \brief Unpack skeleton data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackSkeletonData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Skeletons (NatNet version 2.1 and later)
    if( ( ( major == 2 ) && ( minor > 0 ) ) || ( major > 2 ) )
    {
        int nSkeletons = 0;
        memcpy( &nSkeletons, ptr, 4 ); ptr += 4;
        printf( "%sSkeleton Count : %d\n", outTabStr.c_str(), nSkeletons );

        int nBytes=0;
        ptr = UnpackDataSize(ptr, major, minor,nBytes, level);

        // Loop through skeletons
        for( int j = 0; j < nSkeletons; j++ )
        {
            // skeleton id
            int skeletonID = 0;
            memcpy( &skeletonID, ptr, 4 ); ptr += 4;
            printf( "%s  Skeleton %3.1d\n", outTabStr.c_str(), j );
            printf( "%s    ID: %3.1d\n", outTabStr.c_str(), skeletonID );

            // Number of rigid bodies (bones) in skeleton
            int nRigidBodies = 0;
            memcpy( &nRigidBodies, ptr, 4 ); ptr += 4;
            printf( "%s  Rigid Body Count : %d\n", outTabStr.c_str(), nRigidBodies );

            // Loop through rigid bodies (bones) in skeleton
            for( int k = 0; k < nRigidBodies; k++ )
            {
                // Rigid body position and orientation
                int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;
                float x = 0.0f; memcpy( &x, ptr, 4 ); ptr += 4;
                float y = 0.0f; memcpy( &y, ptr, 4 ); ptr += 4;
                float z = 0.0f; memcpy( &z, ptr, 4 ); ptr += 4;
                float qx = 0; memcpy( &qx, ptr, 4 ); ptr += 4;
                float qy = 0; memcpy( &qy, ptr, 4 ); ptr += 4;
                float qz = 0; memcpy( &qz, ptr, 4 ); ptr += 4;
                float qw = 0; memcpy( &qw, ptr, 4 ); ptr += 4;
                printf( "%s    Rigid Body      : %3.1d\n", outTabStr.c_str(), k );
                printf( "%s      ID            : %3.1d\n", outTabStr.c_str(), ID );
                printf( "%s      Position      : [%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), x, y, z );
                printf( "%s      Orientation   : [%3.2f, %3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), qx, qy, qz, qw );

                // Mean marker error (NatNet version 2.0 and later)
                if( major >= 2 )
                {
                    float fError = 0.0f; memcpy( &fError, ptr, 4 ); ptr += 4;
                    printf( "%s      Marker Error  : %3.2f\n", outTabStr.c_str(), fError );
                }

                // Tracking flags (NatNet version 2.6 and later)
                if( ( ( major == 2 ) && ( minor >= 6 ) ) || ( major > 2 ) || ( major == 0 ) )
                {
                    // params
                    short params = 0; memcpy( &params, ptr, 2 ); ptr += 2;
                    bool bTrackingValid = params & 0x01; // 0x01 : rigid body was successfully tracked in this frame
                    printf( "%s      Tracking Valid: %s\n", outTabStr.c_str(), (bTrackingValid)?"True":"False");
                }
            } // next rigid body
            //printf( "%s  Skeleton %d ID=%d : END\n", outTabStr.c_str(), j, skeletonID );

        } // next skeleton
    }

    return ptr;
}

/**
 * \brief Unpack Asset data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackAssetData(char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Assets ( Motive 3.1 / NatNet 4.1 and greater)
    if (((major == 4) && (minor > 0)) || (major > 4))
    {
        int nAssets = 0;
        memcpy(&nAssets, ptr, 4); ptr += 4;
        printf("%sAsset Count : %d\n", outTabStr.c_str(), nAssets);

        int nBytes=0;
        ptr = UnpackDataSize(ptr, major, minor,nBytes, level);

        for (int i = 0; i < nAssets; i++)
        {
            // asset id
            int assetID = 0;
            memcpy(&assetID, ptr, 4); ptr += 4;
            printf("%sAsset ID: %d\n", outTabStr.c_str(), assetID);

            // # of Rigid Bodies
            int nRigidBodies = 0;
            memcpy(&nRigidBodies, ptr, 4); ptr += 4;
            printf("%sRigid Body Count: %3.1d\n", outTabStr.c_str(), nRigidBodies);

            // Rigid Body data
            for (int j = 0; j < nRigidBodies; j++)
            {
                printf( "%s  Rigid Body : %d\n", outTabStr.c_str(), j );
                ptr = UnpackAssetRigidBodyData(ptr, major, minor, level);
            }

            // # of Markers
            int nMarkers = 0;
            memcpy(&nMarkers, ptr, 4); ptr += 4;
            printf("%sMarker Count: %3.1d\n", outTabStr.c_str(), nMarkers);

            // Marker data
            for (int j = 0; j < nMarkers; j++)
            {
                printf( "%s%3.1d ", outTabStr.c_str(), j );
                ptr = UnpackAssetMarkerData(ptr, major, minor, 0);
            }
        }
    }

    return ptr;
}

/**
 * \brief Asset Rigid Body data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackAssetRigidBodyData(char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Rigid body position and orientation 
    int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
    float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
    float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
    float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
    float qx = 0; memcpy(&qx, ptr, 4); ptr += 4;
    float qy = 0; memcpy(&qy, ptr, 4); ptr += 4;
    float qz = 0; memcpy(&qz, ptr, 4); ptr += 4;
    float qw = 0; memcpy(&qw, ptr, 4); ptr += 4;
    printf( "%s    ID : %d\n", outTabStr.c_str(), ID );
    printf( "%s    Position    : [%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), x, y, z );
    printf( "%s    Orientation : [%3.2f, %3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), qx, qy, qz, qw );

    // Mean error
    float fError = 0.0f; memcpy(&fError, ptr, 4); ptr += 4;
    printf("%s    Mean Error: %3.2f\n", outTabStr.c_str(), fError);

    // params
    short params = 0; memcpy(&params, ptr, 2); ptr += 2;
    printf("%s    Params : %d\n", outTabStr.c_str(), params);

    return ptr;
}

/**
 * \brief Asset marker data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackAssetMarkerData(char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // ID
    int ID = 0;
    memcpy(&ID, ptr, 4); ptr += 4;

    // X
    float x = 0.0f;
    memcpy(&x, ptr, 4); ptr += 4;

    // Y
    float y = 0.0f;
    memcpy(&y, ptr, 4); ptr += 4;

    // Z
    float z = 0.0f;
    memcpy(&z, ptr, 4); ptr += 4;

    // size
    float size = 0.0f;
    memcpy(&size, ptr, 4); ptr += 4;

    // params
    int16_t params = 0;
    memcpy(&params, ptr, 2); ptr += 2;

    // residual
    float residual = 0.0f;
    memcpy(&residual, ptr, 4); ptr += 4;

    printf("%s  Marker %d\tpos : [%3.2f, %3.2f, %3.2f]\tsize=%3.2f\terr=%3.2f\tparams=%d\n", outTabStr.c_str(),
                ID, x, y, z, size, residual, params);

    return ptr;
}

/**
 * \brief Unpack labeled marker data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackLabeledMarkerData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // labeled markers (NatNet version 2.3 and later)
    // labeled markers - this includes all markers: Active, Passive, and 'unlabeled' (markers with no asset but a PointCloud ID)
    if( ( ( major == 2 ) && ( minor >= 3 ) ) || ( major > 2 ) )
    {
        int nLabeledMarkers = 0;
        memcpy( &nLabeledMarkers, ptr, 4 ); ptr += 4;
        printf( "%sLabeled Marker Count : %d\n", outTabStr.c_str(), nLabeledMarkers );

        int nBytes=0;
        ptr = UnpackDataSize(ptr, major, minor,nBytes, level);

        // Loop through labeled markers
        for( int j = 0; j < nLabeledMarkers; j++ )
        {
            // id
            // Marker ID Scheme:
            // Active Markers:
            //   ID = ActiveID, correlates to RB ActiveLabels list
            // Passive Markers: 
            //   If Asset with Legacy Labels
            //      AssetID 	(Hi Word)
            //      MemberID	(Lo Word)
            //   Else
            //      PointCloud ID
            int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;
            int modelID, markerID;
            DecodeMarkerID( ID, modelID, markerID );


            // x
            float x = 0.0f; memcpy( &x, ptr, 4 ); ptr += 4;
            // y
            float y = 0.0f; memcpy( &y, ptr, 4 ); ptr += 4;
            // z
            float z = 0.0f; memcpy( &z, ptr, 4 ); ptr += 4;
            // size
            float size = 0.0f; memcpy( &size, ptr, 4 ); ptr += 4;

            // NatNet version 2.6 and later
            short params = 0;
            bool bOccluded = false;     // marker was not visible (occluded) in this frame
            bool bPCSolved = false;     // position provided by point cloud solve
            bool bModelSolved = false;  // position provided by model solve
            bool bHasModel = false;     // marker has an associated asset in the data stream
            bool bUnlabeled = false;    // marker is 'unlabeled', but has a point cloud ID
            bool bActiveMarker = false; // marker is an actively labeled LED marker
            if( ( ( major == 2 ) && ( minor >= 6 ) ) || ( major > 2 ) || ( major == 0 ) )
            {
                // marker params
                params = 0; memcpy( &params, ptr, 2 ); ptr += 2;
                bOccluded = ( params & 0x01 ) != 0;     // marker was not visible (occluded) in this frame
                bPCSolved = ( params & 0x02 ) != 0;     // position provided by point cloud solve
                bModelSolved = ( params & 0x04 ) != 0;  // position provided by model solve
                if( ( major >= 3 ) || ( major == 0 ) )
                {
                    bHasModel = ( params & 0x08 ) != 0;     // marker has an associated asset in the data stream
                    bUnlabeled = ( params & 0x10 ) != 0;    // marker is 'unlabeled', but has a point cloud ID
                    bActiveMarker = ( params & 0x20 ) != 0; // marker is an actively labeled LED marker
                }

            }

            // NatNet version 3.0 and later
            float residual = 0.0f;
            if( ( major >= 3 ) || ( major == 0 ) )
            {
                // Marker residual
                memcpy( &residual, ptr, 4 ); ptr += 4;
                residual *= 1000.0;
            }

            printf( "%sLabeled Marker %3.1d:\n", outTabStr.c_str(), j);
            printf( "%s    ID                 : [MarkerID: %d] [ModelID: %d]\n", outTabStr.c_str(), markerID, modelID );
            printf( "%s    pos                : [%3.2f, %3.2f, %3.2f]\n", outTabStr.c_str(), x, y, z );
            printf( "%s    size               : [%3.2f]\n", outTabStr.c_str(), size );
            printf( "%s    err                : [%3.2f]\n", outTabStr.c_str(), residual );
            printf( "%s    occluded           : [%3.1d]\n", outTabStr.c_str(), bOccluded );
            printf( "%s    point_cloud_solved : [%3.1d]\n", outTabStr.c_str(), bPCSolved);
            printf( "%s    model_solved       : [%3.1d]\n", outTabStr.c_str(), bModelSolved );
        }
    }
    return ptr;
}

/**
 * \brief Unpack number of bytes of data for a given data type. 
 * Useful if you want to skip this type of data. 
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackDataSize(char* ptr, int major, int minor, int& nBytes, unsigned int level, bool skip /*= false*/ )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    nBytes = 0;

    // size of all data for this data type (in bytes);
    if (((major == 4) && (minor > 0)) || (major > 4))
    {
        memcpy(&nBytes, ptr, 4); ptr += 4;
        printf("%sByte Count: %d\n", outTabStr.c_str(), nBytes);
        if (skip)
        {
            ptr += nBytes;
        }
    }
    return ptr;
}

/**
 * \brief Unpack force plate data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackForcePlateData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Force Plate data (NatNet version 2.9 and later)
    if( ( ( major == 2 ) && ( minor >= 9 ) ) || ( major > 2 ) )
    {
        int nForcePlates;
        const int kNFramesShowMax = 4;
        memcpy( &nForcePlates, ptr, 4 ); ptr += 4;
        printf( "%sForce Plate Count: %d\n", outTabStr.c_str(), nForcePlates );

        int nBytes=0;
        ptr = UnpackDataSize(ptr, major, minor,nBytes, level);

        for( int iForcePlate = 0; iForcePlate < nForcePlates; iForcePlate++ )
        {
            // ID
            int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;

            // Channel Count
            int nChannels = 0; memcpy( &nChannels, ptr, 4 ); ptr += 4;

            printf( "%sForce Plate %3.1d\n", outTabStr.c_str(), iForcePlate );
            printf( "%s  ID           : %3.1d  Channel Count: %3.1d\n", outTabStr.c_str(), ID, nChannels );

            // Channel Data
            for( int i = 0; i < nChannels; i++ )
            {
                printf( "%s", outTabStr.c_str() );
                printf( "  Channel %d : ", i );
                int nFrames = 0; memcpy( &nFrames, ptr, 4 ); ptr += 4;
                printf( "  %3.1d Frames - Frame Data: ", nFrames );

                // Force plate frames
                int nFramesShow = min( nFrames, kNFramesShowMax );
                for( int j = 0; j < nFrames; j++ )
                {
                    float val = 0.0f;  memcpy( &val, ptr, 4 ); ptr += 4;
                    if( j < nFramesShow )
                        printf( "%3.2f   ", val );
                }
                if( nFramesShow < nFrames )
                {
                    printf( " - Showing %3.1d of %3.1d frames", nFramesShow, nFrames );
                }
                printf( "\n" );
            }
        }
    }
    return ptr;
}


/**
 * \brief Unpack device data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackDeviceData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Device data (NatNet version 3.0 and later)
    if( ( ( major == 2 ) && ( minor >= 11 ) ) || ( major > 2 ) )
    {
        const int kNFramesShowMax = 4;
        int nDevices;
        memcpy( &nDevices, ptr, 4 ); ptr += 4;
        printf( "%s", outTabStr.c_str() );
        printf( "Device Count: %d\n", nDevices );

        int nBytes=0;
        ptr = UnpackDataSize(ptr, major, minor,nBytes, level+1);

        for( int iDevice = 0; iDevice < nDevices; iDevice++ )
        {
            // ID
            int ID = 0; memcpy( &ID, ptr, 4 ); ptr += 4;

            // Channel Count
            int nChannels = 0; memcpy( &nChannels, ptr, 4 ); ptr += 4;

            printf( "%s", outTabStr.c_str() );
            printf( "Device %3.1d      ID: %3.1d Num Channels: %3.1d\n", iDevice, ID, nChannels );

            // Channel Data
            for( int i = 0; i < nChannels; i++ )
            {
                printf( "%s", outTabStr.c_str() );
                printf( "  Channel %d : ", i );
                int nFrames = 0; memcpy( &nFrames, ptr, 4 ); ptr += 4;
                printf( "  %3.1d Frames - Frame Data: ", nFrames );
                // Device frames
                int nFramesShow = min( nFrames, kNFramesShowMax );
                for( int j = 0; j < nFrames; j++ )
                {
                    float val = 0.0f;  memcpy( &val, ptr, 4 ); ptr += 4;
                    if( j < nFramesShow )
                        printf( "%3.2f   ", val );
                }
                if( nFramesShow < nFrames )
                {
                    printf( " showing %3.1d of %3.1d frames", nFramesShow, nFrames );
                }
                printf( "\n" );
            }
        }
    }

    return ptr;
}

/**
 * \brief Unpack suffix data and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackFrameSuffixData( char* ptr, int major, int minor, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );

    // software latency (removed in version 3.0)
    if( major < 3 )
    {
        float softwareLatency = 0.0f; memcpy( &softwareLatency, ptr, 4 );	ptr += 4;
        printf( "%s", outTabStr.c_str() );
        printf( "software latency : %3.3f\n", softwareLatency );
    }

    // timecode
    unsigned int timecode = 0; 	memcpy( &timecode, ptr, 4 );	ptr += 4;
    unsigned int timecodeSub = 0; memcpy( &timecodeSub, ptr, 4 ); ptr += 4;
    char szTimecode[128] = "";
    TimecodeStringify( timecode, timecodeSub, szTimecode, 128 );

    // timestamp
    double timestamp = 0.0f;

    // NatNet version 2.7 and later - increased from single to double precision
    if( ( ( major == 2 ) && ( minor >= 7 ) ) || ( major > 2 ) )
    {
        memcpy( &timestamp, ptr, 8 ); ptr += 8;
    }
    else
    {
        float fTemp = 0.0f;
        memcpy( &fTemp, ptr, 4 ); ptr += 4;
        timestamp = (double) fTemp;
    }
    printf( "%s", outTabStr.c_str() );
    printf( "Timestamp : %3.3f\n", timestamp );

    // high res timestamps (version 3.0 and later)
    if( ( major >= 3 ) || ( major == 0 ) )
    {
        uint64_t cameraMidExposureTimestamp = 0;
        memcpy( &cameraMidExposureTimestamp, ptr, 8 ); ptr += 8;
        printf( "%s", outTabStr.c_str() );
        printf( "Mid-exposure timestamp         : %" PRIu64"\n", cameraMidExposureTimestamp );

        uint64_t cameraDataReceivedTimestamp = 0;
        memcpy( &cameraDataReceivedTimestamp, ptr, 8 ); ptr += 8;
        printf( "%s", outTabStr.c_str() );
        printf( "Camera data received timestamp : %" PRIu64"\n", cameraDataReceivedTimestamp );

        uint64_t transmitTimestamp = 0;
        memcpy( &transmitTimestamp, ptr, 8 ); ptr += 8;
        printf( "%s", outTabStr.c_str() );
        printf( "Transmit timestamp             : %" PRIu64"\n", transmitTimestamp );
    }

    // precision timestamps (optionally present) (NatNet 4.1 and later)
    if (((major == 4) && (minor > 0)) || (major > 4) || (major == 0))
    {
        uint32_t PrecisionTimestampSecs = 0;
        memcpy(&PrecisionTimestampSecs, ptr, 4); ptr += 4;
        printf( "%s", outTabStr.c_str() );
        printf("Precision timestamp (seconds) : %d\n", PrecisionTimestampSecs);

        uint32_t PrecisionTimestampFractionalSecs = 0;
        memcpy(&PrecisionTimestampFractionalSecs, ptr, 4); ptr += 4;
        printf( "%s", outTabStr.c_str() );
        printf("Precision timestamp (fractional seconds) : %d\n", PrecisionTimestampFractionalSecs);
    }

    // frame params
    short params = 0;  memcpy( &params, ptr, 2 ); ptr += 2;
    bool bIsRecording = ( params & 0x01 ) != 0;                  // 0x01 Motive is recording
    bool bTrackedModelsChanged = ( params & 0x02 ) != 0;         // 0x02 Actively tracked model list has changed
    bool bLiveMode = ( params & 0x03 ) != 0;                     // 0x03 Live or Edit mode
    gBitstreamVersionChanged = ( params & 0x04 ) != 0;           // 0x04 Bitstream syntax version has changed
    if( gBitstreamVersionChanged )
        gBitstreamChangePending = false;

    // end of data tag
    int eod = 0; memcpy( &eod, ptr, 4 ); ptr += 4;
    /*End Packet*/

    return ptr;
}

/**
 * \brief Unpack packet header and print contents
 * \param ptr - input data stream pointer
 * \param major - NatNet major version
 * \param minor - NatNet minor version
 * \return - pointer after decoded object
*/
char* UnpackPacketHeader( char* ptr, int& messageID, int& nBytes, int& nBytesTotal, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // First 2 Bytes is message ID
    memcpy( &messageID, ptr, 2 ); ptr += 2;

    // Second 2 Bytes is the size of the packet
    memcpy( &nBytes, ptr, 2 ); ptr += 2;
    nBytesTotal = nBytes + 4;
    return ptr;
}


/**
 *      Receives pointer to bytes that represent a packet of data
 *
 *      There are lots of print statements that show what
 *      data is being stored
 *
 *      Most memcpy functions will assign the data to a variable.
 *      Use this variable at your descretion.
 *      Variables created for storing data do not exceed the
 *      scope of this function.
 * 
 * \brief Unpack data stream and print contents
 * \param ptr - input data stream pointer
 * \return - pointer after decoded object
*/
char* Unpack( char* pData, unsigned int level )
{
    std::string outTabStr = GetTabString( kTabStr, level );
    // Checks for NatNet Version number. Used later in function. 
    // Packets may be different depending on NatNet version.
    int major = gNatNetVersion[0];
    int minor = gNatNetVersion[1];
    bool packetProcessed = true;
    char* ptr = pData;

    printf( "%s", outTabStr.c_str() );
    printf( "MoCap Frame Begin\n" );
    printf( "-----------------\n" );
    printf( "%s", outTabStr.c_str() );
    printf( "NatNetVersion %d %d %d %d\n",
        gNatNetVersion[0], gNatNetVersion[1],
        gNatNetVersion[2], gNatNetVersion[3] );

    int messageID = 0;
    int nBytes = 0;
    int nBytesTotal = 0;
    ptr = UnpackPacketHeader( ptr, messageID, nBytes, nBytesTotal, level+1 );

    switch( messageID )
    {
    case NAT_CONNECT:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_CONNECT\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_SERVERINFO:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_SERVERINFO\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_REQUEST:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_REQUEST\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_RESPONSE:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_RESPONSE\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_REQUEST_MODELDEF:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_REQUEST_MODELDEF\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_MODELDEF:
        // Data Descriptions
    {
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_MODELDEF\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        ptr = UnpackDescription( ptr, nBytes, major, minor, level );
    }
    break;
    case NAT_REQUEST_FRAMEOFDATA:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_REQUEST_FRAMEOFDATA\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_FRAMEOFDATA:
    {
        /*
            // FRAME OF MOCAP DATA packet
                printf( "%s", outTabStr.c_str() );
            printf("Message ID  : %d NAT_FRAMEOFDATA\n", messageID);
                printf( "%s", outTabStr.c_str() );
            printf("Packet Size : %d\n", nBytes);
        */

        // Extract frame data flags (last 2 bytes in packet)
        uint16_t params;
        char* ptrToParams = ptr + ( nBytes - 6 );                     // 4 bytes for terminating 0 + 2 bytes for params
        memcpy( &params, ptrToParams, 2 );
        bool bIsRecording = ( params & 0x01 ) != 0;                   // 0x01 Motive is recording
        bool bTrackedModelsChanged = ( params & 0x02 ) != 0;          // 0x02 Actively tracked model list has changed
        bool bLiveMode = ( params & 0x04 ) != 0;                      // 0x03 Live or Edit mode
        gBitstreamVersionChanged = ( params & 0x08 ) != 0;            // 0x04 Bitstream syntax version has changed
        if( gBitstreamChangePending )
        {
            printf( "%s", outTabStr.c_str() );
            printf( "========================================================================================\n" );
            printf( "%s", outTabStr.c_str() );
            printf( " BITSTREAM CHANGE IN - PROGRESS\n" );
            if( gBitstreamVersionChanged )
            {
                gBitstreamChangePending = false;
                printf( "%s", outTabStr.c_str() );
                printf( "  -> Bitstream Changed\n" );
            }
            else
            {
                printf( "%s", outTabStr.c_str() );
                printf( "   -> Skipping Frame\n" );
                packetProcessed = false;
            }
        }
        if( !gBitstreamChangePending )
        {
            ptr = UnpackFrameData( ptr, nBytes, major, minor, level );
            packetProcessed = true;
        }
    }
    break;
    case NAT_MESSAGESTRING:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_MESSAGESTRING\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_DISCONNECT:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_DISCONNECT\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_KEEPALIVE:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_KEEPALIVE\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    case NAT_UNRECOGNIZED_REQUEST:
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d NAT_UNRECOGNIZED_REQUEST\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
        break;
    default:
    {
        printf( "%s", outTabStr.c_str() );
        printf( "Unrecognized Packet Type.\n" );
        printf( "%s", outTabStr.c_str() );
        printf( "Message ID  : %d\n", messageID );
        printf( "%s", outTabStr.c_str() );
        printf( "Packet Size : %d\n", nBytes );
    }
    break;
    }

    printf( "%s", outTabStr.c_str() );
    printf( "MoCap Frame End\n" );
    printf( "-----------------\n" );


    // check for full packet processing
    if( packetProcessed )
    {
        long long nBytesProcessed = (long long) ptr - (long long) pData;
        if( nBytesTotal != nBytesProcessed )
        {
            printf( "%s", outTabStr.c_str() );
            printf( "WARNING: %d expected but %lld bytes processed\n",
                nBytesTotal, nBytesProcessed );
            if( nBytesTotal > nBytesProcessed )
            {
                int count = 0, countLimit = 8 * 25;// put on 8 byte boundary
                printf( "%s", outTabStr.c_str() );
                printf( "Sample of remaining bytes:\n" );
                char* ptr_start = ptr;
                int nCount = (int) nBytesProcessed;
                char tmpChars[9] = { "        " };
                int charPos = ( (long long) ptr % 8 );
                char tmpChar;
                // add spaces for first row
                if( charPos > 0 )
                {
                    for( int i = 0; i < charPos; ++i )
                    {
                        printf( "   " );
                        if( i == 4 )
                        {
                            printf( "    " );
                        }
                    }
                }
                countLimit = countLimit - ( charPos + 1 );
                while( nCount < nBytesTotal )
                {
                    printf( "%s", outTabStr.c_str() );
                    tmpChar = ' ';
                    if( isalnum( *ptr ) )
                    {
                        tmpChar = *ptr;
                    }
                    tmpChars[charPos] = tmpChar;
                    printf( "%2.2x ", (unsigned char) *ptr );
                    ptr += 1;
                    charPos = (long long) ptr % 8;
                    if( charPos == 0 )
                    {
                        printf( "    " );
                        for( int i = 0; i < 8; ++i )
                        {
                            printf( "%c", tmpChars[i] );
                        }
                        printf( "\n" );
                    }
                    else if( charPos == 4 )
                    {
                        printf( "    " );
                    }
                    if( ++count > countLimit )
                    {
                        break;
                    }
                    ++nCount;
                }
                if( (long long) ptr % 8 )
                {
                    printf( "\n" );
                }
            }
        }
    }

    // return the beginning of the possible next packet
    // assuming no additional termination
    ptr = pData + nBytesTotal;
    return ptr;
}

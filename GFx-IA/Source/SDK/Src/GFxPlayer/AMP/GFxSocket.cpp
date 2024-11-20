/**********************************************************************

Filename    :   GFxSocket.cpp
Content     :   Socket wrapper class

Created     :   July 03, 2009
Authors     :   Boris Rayskiy, Alex Mantzaris

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#include "GFxSocket.h"

#ifdef GFX_USE_SOCKETS

#if defined(GFC_OS_WIN32)

    #include <winsock2.h>
    #include <errno.h>
    #include <ws2tcpip.h>


    #define GWSAECONNRESET WSAECONNRESET
    #define GWSAEWOULDBLOCK WSAEWOULDBLOCK

    typedef int socklen_t;

    #define GINVALIDSOCKET(a)   (INVALID_SOCKET == (a))
    #define GSOCKETERROR(a)     (SOCKET_ERROR == (a))

    #define GIPPROTOCOL_TCP IPPROTO_TCP
    #define GIPPROTOCOL_UDP IPPROTO_UDP

#elif defined(GFC_OS_XBOX360)

    #include <Xtl.h>
    #include <errno.h>

    #define GWSAECONNRESET WSAECONNRESET
    #define GWSAEWOULDBLOCK WSAEWOULDBLOCK

    typedef int socklen_t;

    #define GINVALIDSOCKET(a)   (INVALID_SOCKET == (a))
    #define GSOCKETERROR(a)     (SOCKET_ERROR == (a))

    #define GIPPROTOCOL_TCP IPPROTO_TCP
    #define GIPPROTOCOL_UDP IPPROTO_UDP

#elif defined(GFC_OS_PS3)

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <string.h>
    #include <sys/time.h>
    #include <netex/libnetctl.h>
    #include <netex/errno.h>

    #define GINVALIDSOCKET(a)   ((a) < 0)
    #define GSOCKETERROR(a)     ((a) < 0)
    #define INVALID_SOCKET      -1

    #define GWSAECONNRESET 54L
    #define GWSAEWOULDBLOCK 35L

    typedef int SOCKET;
    typedef struct sockaddr* LPSOCKADDR;

    #include <netex/net.h>
    #include <netex/ifctl.h>
    #include <cell/sysmodule.h>
    #include <sys/process.h>

    #define GIPPROTOCOL_TCP 0
    #define GIPPROTOCOL_UDP 0

#elif defined(GFC_OS_LINUX) || defined(GFC_OS_MAC)

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <string.h>
    #include <sys/errno.h>
    #include <unistd.h>
    #include <fcntl.h>

    #define GINVALIDSOCKET(a)   ((a) < 0)
    #define GSOCKETERROR(a)     ((a) < 0)
    #define INVALID_SOCKET      -1

    #define GWSAECONNRESET ECONNRESET
    #define GWSAEWOULDBLOCK EWOULDBLOCK

    typedef int SOCKET;
    typedef struct sockaddr* LPSOCKADDR;

    #define GIPPROTOCOL_TCP IPPROTO_TCP
    #define GIPPROTOCOL_UDP 0

#elif defined(GFC_OS_WII)

    #include <revolution/hio2.h>
    //#define GFX_SOCKETS_NOT_SUPPORTED

#else

    #define GFX_SOCKETS_NOT_SUPPORTED

#endif

#include "GString.h"
#include "GMsgFormat.h"

#ifdef GFX_SOCKETS_NOT_SUPPORTED

class GFxSocketImpl : public GNewOverrideBase<GStat_Default_Mem>, public GFxSocketInterface
{
public:
    bool CreateStream() { return false; }
    bool CreateDatagram() { return false; }
    bool Bind() { return false; }
    bool Listen(int) { return false; }
    bool Connect() { return false; }
    bool Accept(int) { return false; }
    int Send(const char*, UPInt) const { return -1; }
    int Receive(char*, int) const { return -1; }
    int SendBroadcast(const char*, UPInt) const { return -1; }
    int ReceiveBroadcast(char*, int) const { return -1; }
    void SetListenPort(UInt32) { }
    void SetBroadcastPort(UInt32) { }
    void SetAddress(UInt32, const char*) { }
    void SetBlocking(bool) { }
    void SetBroadcast(bool) { }
    void GetName(UInt32*, UInt32*, char*, UInt32) { }
    bool Shutdown() { return false; }
    bool Startup() { return false; }
    void Cleanup() { }
    int GetLastError() const { return 0; }
    bool IsValid() const { return false; }
    bool CheckAbort() const { return false; }
};

#elif defined(GFC_OS_WII)
// Wii does not support sockets
// We create a pseudo-socket based on HIO2

class GFxSocketImpl  : public GNewOverrideBase<GStat_Default_Mem>, public GFxSocketInterface
{
public:
    GFxSocketImpl()
    { 
        memset(BytesReceived, 0, sizeof(BytesReceived));
    }
    ~GFxSocketImpl()
    {
    }
    bool CreateStream() 
    { 
        return true; 
    }
    bool CreateDatagram() 
    { 
        // Not supported for the Wii
        return false; 
    }
    bool Bind() 
    { 
        return true; 
    }
    bool Listen(int) 
    { 
        if (!Open())
        {
            return false;
        }
        return true;
    }
    bool Connect() 
    {
        return false;  // Server only
    }
    bool Accept(int) 
    { 
        return true;
    }
    int Send(const char* buffer, UPInt bufferSize) const 
    { 
        if (!IsValid() || Sending)
        {
            return 0;
        }

        if (!IsSendPossible())
        {
            return 0;
        }

        memcpy(SendBuffer, buffer, bufferSize);

        int msgSize = static_cast<int>(bufferSize);
        for (int i = 3; i >= 0; --i)
        {
            BytesSent[i] = msgSize % 256;
            msgSize /= 256;
        }
        DCFlushRange(&BytesSent, sizeof(BytesSent));
        if (!HIO2Write(Hio2Handle, WriteAddress, BytesSent, sizeof(BytesSent)))
        {
            return -1;
        }
        DCFlushRange(&SendBuffer, sizeof(SendBuffer));
        if (!HIO2Write(Hio2Handle, WriteAddress + sizeof(BytesSent), SendBuffer, sizeof(SendBuffer)))
        {
            return -1;
        }
        if (!HIO2WriteMailbox(Hio2Handle, AMP_Mailbox_Recv))
        {
            return -1;
        }
        Sending = true;
        return bufferSize;
    }
    int Receive(char* buffer, int bufferSize) const 
    {
        if (!IsSendPossible())
        {
            return 0;
        }

        int msgSize = 0;
        for (int i = 0; i < 4; ++i)
        {
            msgSize <<= 8;
            msgSize += BytesReceived[i];
        }

        if (msgSize > 0)
        {
            memcpy(buffer, static_cast<void*>(RecvBuffer), msgSize);
            memset(BytesReceived, 0, sizeof(BytesReceived));

            HIO2WriteMailbox(Hio2Handle, AMP_Mailbox_RecvAck);
        }

        return msgSize;
    }
    int SendBroadcast(const char*, UPInt) const { return -1; }
    int ReceiveBroadcast(char*, int) const { return -1; }
    void SetListenPort(UInt32) { }
    void SetBroadcastPort(UInt32) { }
    void SetAddress(UInt32, const char*) { }
    void SetBlocking(bool) { }
    void SetBroadcast(bool) { }
    void GetName(UInt32*, UInt32*, char*, UInt32) { }
    bool Shutdown() 
    { 
        if (Hio2Handle != HIO2_INVALID_HANDLE_VALUE)
        {
            HIO2WriteMailbox(Hio2Handle, AMP_Mailbox_Exit);
            HIO2Close(Hio2Handle);
        }
        HIO2Exit();
        Hio2Handle = HIO2_INVALID_HANDLE_VALUE;
        Connected = false;
        Sending = false;
        Exiting = false;
        return true; 
    }
    bool Startup() 
    { 
        return true; 
    }
    void Cleanup() 
    { 
    }
    int GetLastError() const 
    { 
        return HIO2GetLastError(); 
    }
    bool IsValid() const 
    { 
        return (Connected && Hio2Handle != HIO2_INVALID_HANDLE_VALUE); 
    }
    bool CheckAbort() const 
    { 
        if (!Connected)
        {
            if (IsSendPossible())
            {
                if (!HIO2WriteMailbox(Hio2Handle, AMP_Mailbox_Open))
                {
                    return true;
                }
            }
        }
        if (Exiting)
        {
            return true;
        }
        return false;
    }
private:

    enum MailboxValue
    {
        AMP_Mailbox_Open = 0,
        AMP_Mailbox_OpenResult,
        AMP_Mailbox_Recv,
        AMP_Mailbox_RecvAck,
        AMP_Mailbox_Exit,
    };

    static HIO2Handle       Hio2Handle;

    // HIO2 API buffer
    static u8               SendBuffer[512] ATTRIBUTE_ALIGN(32);
    static u8               RecvBuffer[512] ATTRIBUTE_ALIGN(32);

    static HIO2DeviceType   ExiDev;     // EXI device
    static bool             Connected;  // PC connection state
    static bool             Sending;    // Send flag
    static bool             Exiting;
    static u8               BytesReceived[32] ATTRIBUTE_ALIGN(32);
    static u8               BytesSent[32] ATTRIBUTE_ALIGN(32);
    static UInt32           ReadAddress;
    static UInt32           WriteAddress;

    bool Open()
    {
        Exiting = false;

        // Initialization
        if (!HIO2Init())
        {
            return false;
        }

        // Device list
        if (!HIO2EnumDevices(EnumCallback))
        {
            return false;
        }

        // When the device could not be found
        if (ExiDev == HIO2_DEVICE_INVALID)
        {
            return false;
        }

        // Device open
        Hio2Handle = HIO2Open(ExiDev, ReceiveCallback, NULL);
        if (Hio2Handle == HIO2_INVALID_HANDLE_VALUE)
        {
            return false;
        }
        return true;
    }

    bool IsSendPossible() const
    {
        // Get HIO2 status
        u32 status;
        if (!HIO2ReadStatus(Hio2Handle, &status))
        {
            return false;
        }

        return ((status & HIO2_STATUS_RX) == 0);
    }

    // enum devices
    static BOOL EnumCallback(HIO2DeviceType type)
    {
        // Use the first device that was detected
        ExiDev = type;
        return FALSE;
    }

    static void ReceiveCallback(HIO2Handle h)
    {
        u32 mail;

        if (HIO2ReadMailbox(h, &mail))
        {
            Connected = true;
            switch (mail)
            {
            case AMP_Mailbox_Recv:          // Receive data from connection target
                if (!HIO2Read(h, ReadAddress, BytesReceived, sizeof(BytesReceived)))
                {
                    memset(BytesReceived, 0, sizeof(BytesReceived));
                }
                else if (!HIO2Read(h, ReadAddress + sizeof(BytesReceived), RecvBuffer, sizeof(RecvBuffer)))
                {
                    memset(BytesReceived, 0, sizeof(BytesReceived));
                }
                DCInvalidateRange(&BytesReceived, sizeof(BytesReceived));
                DCInvalidateRange(&RecvBuffer, sizeof(RecvBuffer));
                break;
            case AMP_Mailbox_RecvAck:
                Sending = false; 
                break;
            case AMP_Mailbox_Exit:
                Exiting = true;
                break;
            }
        }
    }
};

HIO2Handle GFxSocketImpl::Hio2Handle = HIO2_INVALID_HANDLE_VALUE;
u8 GFxSocketImpl::SendBuffer[512] ATTRIBUTE_ALIGN(32);
u8 GFxSocketImpl::RecvBuffer[512] ATTRIBUTE_ALIGN(32);
HIO2DeviceType GFxSocketImpl::ExiDev = HIO2_DEVICE_INVALID;
bool GFxSocketImpl::Connected = false;
bool GFxSocketImpl::Sending = false;
bool GFxSocketImpl::Exiting = false;
u8 GFxSocketImpl::BytesReceived[32]  ATTRIBUTE_ALIGN(32);
u8 GFxSocketImpl::BytesSent[32]  ATTRIBUTE_ALIGN(32);
UInt32 GFxSocketImpl::ReadAddress = 0x0000;
UInt32 GFxSocketImpl::WriteAddress = 0x1000;

#else

// Platform-specific socket implementation
class GFxSocketImpl : public GNewOverrideBase<GStat_Default_Mem>, public GFxSocketInterface
{
public:
    GFxSocketImpl() : Socket(INVALID_SOCKET), LocalHostAddress(0) {}

    // Create the socket
    bool CreateStream()
    {
        Socket = ::socket(AF_INET, SOCK_STREAM, GIPPROTOCOL_TCP);
        return (!GINVALIDSOCKET(Socket));
    }

    bool CreateDatagram()
    {
        Socket = ::socket(AF_INET, SOCK_DGRAM, GIPPROTOCOL_UDP);
        return (!GINVALIDSOCKET(Socket));
    }

    bool Bind()
    {
        return (!GSOCKETERROR(::bind(Socket, (LPSOCKADDR) &SocketAddress, sizeof(SocketAddress))));
    }

    bool Listen(int numConnections)
    {
        return (!GSOCKETERROR(::listen(Socket, numConnections)));
    }

    bool Connect()
    {
        return (!GSOCKETERROR(::connect(Socket, (LPSOCKADDR) &SocketAddress, sizeof(SocketAddress))));
    }

    bool Accept(int timeout)
    {
        bool doSelect = false;
#ifdef GFC_OS_LINUX
        doSelect = (timeout >= 0);
#endif
        if (doSelect)
        {
            timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(Socket, &readfds);

#ifdef GFC_OS_PS3
            ::socketselect((int)Socket+1, &readfds, NULL, NULL, &tv);
#else
            ::select((int)Socket+1, &readfds, NULL, NULL, &tv);
#endif
            if (!FD_ISSET(Socket, &readfds))
            {
                return false;
            }
        }

        socklen_t iSize = sizeof(SocketAddress);
        SOCKET hSocket = ::accept(Socket, (LPSOCKADDR) &SocketAddress, &iSize);
        if (!GINVALIDSOCKET(hSocket))
        {
            Shutdown(); // we no longer need the listen socket
            Socket = hSocket;
            return true;
        }

        return false;
    }
    int Send(const char* dataBuffer, UPInt dataBufferSize) const
    {
        if (GINVALIDSOCKET(Socket))
        {
            return -1;
        }

        int bytesSent = ::send(Socket, dataBuffer, static_cast<int>(dataBufferSize), 0);
        if (GSOCKETERROR(bytesSent))
        {
            return (GetLastError() == GWSAEWOULDBLOCK ? 0 : -1);
        }
        return bytesSent;
    }

    int Receive(char* dataBuffer, int dataBufferSize) const
    {
        if (GINVALIDSOCKET(Socket))
        {
            return -1;
        }

        return ::recv(Socket, dataBuffer, dataBufferSize, 0);
    }

    int SendBroadcast(const char* dataBuffer, UPInt dataBufferSize) const
    {
        if (GINVALIDSOCKET(Socket))
        {
            return -1;
        }

        return ::sendto(Socket, dataBuffer, static_cast<int>(dataBufferSize), 0, 
            (const sockaddr *) &SocketAddress, sizeof(SocketAddress));
    }

    int ReceiveBroadcast(char* dataBuffer, int dataSize) const
    {
        if (GINVALIDSOCKET(Socket))
        {
            return -1;
        }

        socklen_t addrLength = sizeof(SocketAddress);
        return ::recvfrom(Socket, dataBuffer, dataSize, 0, (sockaddr*)&SocketAddress, &addrLength); 
    }

    void SetListenPort(UInt32 port)
    {
        memset(&SocketAddress, 0, sizeof(sockaddr_in));
        SocketAddress.sin_family = AF_INET;
        SocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        SocketAddress.sin_port = htons((UInt16) port);
    }

    void SetBroadcastPort(UInt32 port)
    {
        memset(&SocketAddress, 0, sizeof(sockaddr_in));
        SocketAddress.sin_family = AF_INET;
        SocketAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        SocketAddress.sin_port = htons((UInt16) port);
    }

    void SetAddress(UInt32 port, const char* address)
    {
        memset(&SocketAddress, 0, sizeof(sockaddr_in));
        SocketAddress.sin_family = AF_INET;
        SocketAddress.sin_addr.s_addr = inet_addr(address);
        SocketAddress.sin_port = htons((UInt16) port);
    }

    // Blocking means that some socket operations block the thread until completed
    // Non-blocking do not block, returning GWSAEWOULDBLOCK instead
    void SetBlocking(bool blocking)
    {
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360)        
        u_long iMode = blocking ? 0 : 1;
        ::ioctlsocket(Socket, FIONBIO, &iMode);
#elif defined (GFC_OS_PS3)
        int nValue = blocking ? 0 : 1;
        setsockopt(Socket, SOL_SOCKET, SO_NBIO, &nValue, sizeof(int));
#else
        int cmd = fcntl(Socket, F_GETFL, 0);
        if (blocking)
        {
            cmd &= ~O_NONBLOCK;
        }
        else
        {
            cmd |= O_NONBLOCK;
        }
        fcntl(Socket, F_SETFL, cmd);
#endif
    }

    void SetBroadcast(bool broadcast)
    {
        int iBroadcast = (broadcast ? 1 : 0);
        ::setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (char*)&iBroadcast, sizeof(iBroadcast));
    }

    // Port and IP address of connected socket
    // Returns true if successful
    void GetName(UInt32* port, UInt32* address, char* name, UInt32 nameSize)
    {
        *port = ntohs(SocketAddress.sin_port);
        *address = ntohl(SocketAddress.sin_addr.s_addr);
        if (name != NULL)
        {
#ifdef GFC_OS_WIN32
            GHash<UInt32, GString>::Iterator it = AddressMap.Find(*address);
            if (it == AddressMap.End())
            {
                GASSERT(nameSize <= NI_MAXHOST);
                char servInfo[NI_MAXSERV];
                if (getnameinfo((sockaddr*) &SocketAddress, sizeof(sockaddr), name, nameSize, servInfo, NI_MAXSERV, NI_NOFQDN) != 0)
                {
                    *name = '\0';
                }
                AddressMap.Add(*address, name);
            }
            else
            {
                G_strcpy(name, nameSize, it->Second);
            }

#else
            *name = '\0';
#endif
        }

#ifdef GFC_OS_WIN32
        if (LocalHostAddress == 0)
        {
            char hostName[NI_MAXHOST];
            if (gethostname(hostName, NI_MAXHOST) == 0)
            {
                hostent *result = gethostbyname(hostName);
                if (result != NULL)
                {
                    for (int i = 0; result->h_addr_list[i] != 0; ++i) 
                    {
                        in_addr addr;
                        addr.s_addr = *(u_long *) result->h_addr_list[i];
                        if (addr.s_addr == SocketAddress.sin_addr.s_addr)
                        {
                            LocalHostAddress = *address;
                            break;
                        }
                    }
                }
            }
        }
#endif

        if (LocalHostAddress == *address)
        {
            *address = 0x7F000001;  // localhost
        }
    }

    bool Shutdown()
    {
        if (!GINVALIDSOCKET(Socket))
        {
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360)
            ::closesocket(Socket);
#elif defined(GFC_OS_PS3)
            ::socketclose(Socket); 
#else
            ::shutdown(Socket, 2);
            ::close(Socket);
#endif
            Socket = INVALID_SOCKET;
        }
        return true;
    }

    // Initialize the socket library
    bool Startup()
    {
        GLock::Locker locker(&LibRefLock);
        if (LibRefs == 0)
        {
#if defined(GFC_OS_WIN32)
            WSADATA wsaData;
            WORD version = MAKEWORD(2, 0);
            if (::WSAStartup(version, &wsaData) != 0)
            {
                GFC_DEBUG_MESSAGE(1, "\nCreate: Error occured. Could not create socket\n");
                return false;
            }
#elif defined(GFC_OS_XBOX360)
            XNetStartupParams xnsp;
            memset(&xnsp, 0, sizeof(xnsp));
            xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
            xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
            if (::XNetStartup(&xnsp) != 0)
            {
                return false;
            }

            WSADATA wsaData;
            WORD version = MAKEWORD(2, 0);
            if (::WSAStartup(version, &wsaData) != 0)
            {
                return false;
            }
#elif defined(GFC_OS_PS3)
            if (::cellSysmoduleLoadModule(CELL_SYSMODULE_NET) < 0)
            {
                return false;
            }

            if (sys_net_initialize_network() < 0) 
            {
                return false;
            }

            int err = ::cellNetCtlInit();
            if (err < 0 && err != (int)CELL_NET_CTL_ERROR_NOT_TERMINATED)
            {
                return false;
            }
#endif
        }
        ++LibRefs;
        return true;
    }

    // Terminate the socket library
    void Cleanup()
    {
        GLock::Locker locker(&LibRefLock);
        if (LibRefs > 0)
        {
            --LibRefs;
            if (LibRefs == 0)
            {
#if defined(GFC_OS_WIN32)
                ::WSACleanup();
#elif defined(GFC_OS_XBOX360)
                ::WSACleanup();
                ::XNetCleanup();
#elif defined(GFC_OS_PS3)
                // Don't clean up the socket library in case someone else is using it
                // Unlike windows, the PS3 releases the library even if it has been initialized 
                // more times than it has been released
                //::cellNetCtlTerm();
                //::sys_net_finalize_network();
                //::cellSysmoduleUnloadModule(CELL_SYSMODULE_NET);
#endif
            }
        }
    }

    // Get error code
    int GetLastError() const
    {
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360)
        return ::WSAGetLastError();
#elif defined(GFC_OS_PS3)
        return sys_net_errno;
#else
        return errno;
#endif
    }

    bool IsValid() const 
    { 
        return (!GINVALIDSOCKET(Socket)); 
    }

    bool CheckAbort() const 
    { 
        return false; 
    }

    sockaddr_in                 SocketAddress;
    SOCKET                      Socket;

    mutable GHash<UInt32, GString>      AddressMap;
    mutable UInt32                      LocalHostAddress;                     

    static UInt32               LibRefs;
    static GLock                LibRefLock;
};

UInt32 GFxSocketImpl::LibRefs = 0;
GLock GFxSocketImpl::LibRefLock;

#endif  // GFX_SOCKETS_NOT_SUPPORTED


////////////////////////////////////////////////////////////////////

#ifndef GFC_NO_THREADSUPPORT
class GFxSocketLocker
{
public:     
    GFxSocketLocker(GLock* lock) : Lock(lock)
    {
        if (Lock != NULL)
        {
            Lock->Lock();
        }
    }
    ~GFxSocketLocker()
    {
        if (Lock != NULL)
        {
            Lock->Unlock();
        }
    }
private:
    GLock *Lock;
};
#endif

class GFxDefaultSocketFactory : public GNewOverrideBase<GStat_Default_Mem>, public GFxSocketImplFactory
{
public:
    virtual ~GFxDefaultSocketFactory() { }
    virtual GFxSocketInterface* Create()
    {
        return GNEW GFxSocketImpl();
    }
    virtual void Destroy(GFxSocketInterface* socketImpl)
    {
        delete socketImpl;
    }
};

static GFxDefaultSocketFactory defaultSocketFactory;

///////////////////////////////////////////////////////////////////

// Constructor
GFxSocket::GFxSocket(bool initLib, GFxSocketImplFactory* socketImplFactory) : 
    SocketImplFactory(socketImplFactory),
    SocketImpl(NULL),
    IsServer(false), 
    InitLib(initLib)
#ifndef GFC_NO_THREADSUPPORT
    , CreateLock(NULL)
#endif
{
    if (SocketImplFactory == NULL)
    {
        SocketImplFactory = &defaultSocketFactory;
    }
    SocketImpl = SocketImplFactory->Create();

    // Socket library initialization
    if (InitLib)
    {
        if (!SocketImpl->Startup()) 
        {
            GFC_DEBUG_MESSAGE(1, "GFxSocket: Library initialization failed");
        }
    }
}

// Destructor
GFxSocket::~GFxSocket()
{
    Destroy();

    if (InitLib)
    {
        Cleanup();
    }
    SocketImplFactory->Destroy(SocketImpl);
}

// Create a socket connection
// An IP address of NULL means that this is a server socket
// Returns true if successful
bool GFxSocket::Create(const char* ipAddress, UInt32 port, GString* errorMsg)
{
#ifndef GFC_NO_THREADSUPPORT
    GFxSocketLocker locker(CreateLock);
#endif

    IsServer = (ipAddress == NULL);

    if (!SocketImpl->CreateStream())
    {
        if (errorMsg != NULL)
        {
            G_SPrintF(*errorMsg, "Could not create listener socket. Error %d", SocketImpl->GetLastError());
        }
        GFC_DEBUG_MESSAGE1(1, "GFxSocket: Could not create listener socket. Error %d", 
            SocketImpl->GetLastError());
        Cleanup();
        return false;
    }

    if (IsServer)
    {
        SocketImpl->SetListenPort(port);

        // Associate local address with socket
        if (!SocketImpl->Bind())
        {
            if (errorMsg != NULL)
            {
                G_SPrintF(*errorMsg, "Could not associate local address with listener socket. Error %d", 
                    SocketImpl->GetLastError());
            }
            GFC_DEBUG_MESSAGE1(1, 
                "GFxSocket: Could not associate local address with listener socket. Error %d", 
                SocketImpl->GetLastError());
            Destroy();
            return false;
        }

        if (!SocketImpl->Listen(1))
        {
            if (errorMsg != NULL)
            {
                G_SPrintF(*errorMsg, "Could not place socket in listening state. Error %d", 
                    SocketImpl->GetLastError());
            }
            GFC_DEBUG_MESSAGE1(1, "GFxSocket: could not place socket in listening state. Error %d", 
                SocketImpl->GetLastError());
            Destroy();
            return false;
        }
    }
    else
    {
        SocketImpl->SetAddress(port, ipAddress);

        // Establish connection to specified socket 
        if (!SocketImpl->Connect())
        {
            if (errorMsg != NULL)
            {
                G_SPrintF(*errorMsg, "Could not connect to server. Error %d", SocketImpl->GetLastError());
            }
            GFC_DEBUG_MESSAGE1(1, "GFxSocket: could not connect to server. Error %d", SocketImpl->GetLastError());
            Destroy();
            return false;
        }
    }

    if (errorMsg != NULL)
    {
        G_SPrintF(*errorMsg, "Socket connection established on port %d", port);
    }
    //GFC_DEBUG_MESSAGE1(1, "GFxSocket: socket created on port %d", port);

    return true;
}

// Shuts down and performs cleanup
void GFxSocket::Destroy()
{
    if (IsValid())
    {
        Shutdown();
    }
}

// Permits an incoming connection attempt on a socket
// Blocks the thread until connection has been established
// Returns true if connection has been established
bool GFxSocket::Accept(int timeout)
{
    if (IsServer)
    {
        // Only relevant for server sockets
        if (!SocketImpl->Accept(timeout))
        {
//             GFC_DEBUG_MESSAGE1(1, "GFxSocket: Incoming connection rejected. Error %d", 
//                 SocketImpl->GetLastError());
            Destroy();
            return false;
        }

        GFC_DEBUG_MESSAGE(1, "GFxSocket: Incoming connection accepted.");
    }

    return true;
}

// Sends data on the connected socket
// Returns true if successful
int GFxSocket::Send(const char* dataBuffer, UPInt dataBufferSize) const
{
    int bytesTransferred = SocketImpl->Send(dataBuffer, dataBufferSize);
    if (bytesTransferred < 0)
    {
        GFC_DEBUG_MESSAGE1(1, "GFxSocket: Send Error %d", SocketImpl->GetLastError());
        return -1;
    }

    return bytesTransferred;
}

// Receives data from a connected socket 
// Blocks the thread until data have been received (or shutdown)
// Returns the number of bytes transmitted
int GFxSocket::Receive(char* dataBuffer, int dataBufferSize) const
{
    int iBytesTransferred = SocketImpl->Receive(dataBuffer, dataBufferSize);
    if (iBytesTransferred < 0)
    {
        return 0;
    }
    return iBytesTransferred;
}

// Tests the outgoing or incoming connection
bool GFxSocket::IsConnected() const
{
    // We determine the connection status based on 
    // whether or not an attempt to send is successful
    char strBuf[1];
    return (Send(strBuf, 0) >= 0);
}


// Blocking means that some socket operations block the thread until completed
// Non-blocking do not block, returning GWSAEWOULDBLOCK instead
void GFxSocket::SetBlocking(bool blocking)
{
    SocketImpl->SetBlocking(blocking);
}

#ifndef GFC_NO_THREADSUPPORT
// Set lock for multithreaded access
void GFxSocket::SetLock(GLock* lock)
{
    CreateLock = lock;
}
#endif

bool GFxSocket::IsValid() const
{
    return SocketImpl->IsValid();
}

bool GFxSocket::CheckAbort() const
{
    return SocketImpl->CheckAbort();
}


// Shut down the socket
bool GFxSocket::Shutdown()
{
#ifndef GFC_NO_THREADSUPPORT
    GFxSocketLocker locker(CreateLock);
#endif

    return SocketImpl->Shutdown();
}

// Initialize the socket library
// Heavily platform-specific
bool GFxSocket::Startup()
{
#ifndef GFC_NO_THREADSUPPORT
    GFxSocketLocker locker(CreateLock);
#endif

    if (!SocketImpl->Startup())
    {
        return false;
    }

    return true;
}

// Terminate the socket library
// Heavily platform-specific
void GFxSocket::Cleanup()
{
#ifndef GFC_NO_THREADSUPPORT
    GFxSocketLocker locker(CreateLock);
#endif

    SocketImpl->Cleanup();
}


//////////////////////////////////////////////////////////////////////////////////

GFxBroadcastSocket::GFxBroadcastSocket(bool initLib, GFxSocketImplFactory* socketImplFactory) 
    : SocketImplFactory(socketImplFactory), InitLib(initLib)
{
    if (SocketImplFactory == NULL)
    {
        SocketImplFactory = &defaultSocketFactory;
    }
    SocketImpl = SocketImplFactory->Create();

    if (InitLib)
    {
        if (!SocketImpl->Startup()) 
        {
            GFC_DEBUG_MESSAGE(1, "GFxSocket: Startup failed");
        }
    }
}

GFxBroadcastSocket::~GFxBroadcastSocket()
{
    Destroy();
    if (InitLib)
    {
        SocketImpl->Cleanup();
    }
    SocketImplFactory->Destroy(SocketImpl);
}

bool GFxBroadcastSocket::Create(UInt32 port, bool broadcast)
{
    if (!SocketImpl->CreateDatagram())
    {
        GFC_DEBUG_MESSAGE1(1, "GFxBroadcastSocket: Could not create socket. Error %d", 
            SocketImpl->GetLastError());
        return false;
    }

    if (broadcast)
    {
        SocketImpl->SetBroadcastPort(port);
        SocketImpl->SetBroadcast(true);
    }
    else
    {
        SocketImpl->SetListenPort(port);

        if (!SocketImpl->Bind())
        {
            GFC_DEBUG_MESSAGE1(1, 
                "GFxBroadcastSocket: Could not bind UDP socket. Error %d", SocketImpl->GetLastError());
            Destroy();
            return false;
        }

        SocketImpl->SetBlocking(false);
    }

    return true;
}

// Shut down the socket
void GFxBroadcastSocket::Destroy()
{
    if (SocketImpl->IsValid())
    {
        SocketImpl->Shutdown();
    }
}


int GFxBroadcastSocket::Broadcast(const char* dataBuffer, UPInt dataBufferSize) const
{
    int bytesTransferred = SocketImpl->SendBroadcast(dataBuffer, dataBufferSize);
    if (bytesTransferred < 0)
    {
        GFC_DEBUG_MESSAGE1(true, "GFxBroadcastSocket: Send Error %d", SocketImpl->GetLastError());
    }

    return bytesTransferred;
}

int GFxBroadcastSocket::Receive(char* dataBuffer, int dataSize) const
{
    int bytesReceived = SocketImpl->ReceiveBroadcast(dataBuffer, dataSize);
    if (bytesReceived < 0)
    {
        return 0;
    }
    return bytesReceived;
}

void GFxBroadcastSocket::GetName(UInt32* port, UInt32* address, char* name, UInt32 nameSize) const
{
    SocketImpl->GetName(port, address, name, nameSize);
}

#endif

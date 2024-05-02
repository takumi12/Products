/**********************************************************************

Filename    :   GFxAmpThread.h
Content     :   Manages the socket threads
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXAMP_H
#define INC_GFXAMP_H

#include "GConfig.h"

#ifdef GFX_USE_SOCKETS

#include "GFxSocket.h"
#include "GRefCount.h"
#include "GString.h"
#include "GArray.h"
#include "GAtomic.h"
#include "GThreads.h"
#include "GFxLog.h"
#include "GFxAmpInterfaces.h"

// Forward declarations
class GThread;
class GFxAmpMessage;
class GFxAmpStream;
class GFxAmpMsgHandler;

// Manages the socket connection threads for AMP
// An instance of this class is contained both in the AMP server singleton and in the AMP client
// The caller can pass in callback objects for notification
// The GFxAmpSendInterface::OnSendLoop gets called from the socket thread for app-specific processing
// The GFxAmpConnStatusInterface::OnStatusChanged gets called whenever the connection status is updated
// A GFxAmpMsgHandler can be specified for immediate message handling, bypassing the received queue (for AS debugger)
class GFxAmpThreadMgr : public GRefCountBase<GFxAmpThreadMgr, GStat_Default_Mem>
{
public:
    GFxAmpThreadMgr(GFxAmpMsgHandler* msgHandler, GFxAmpSendInterface* sendCallback, 
                    GFxAmpConnStatusInterface* connectionCallback, GEvent* sendQueueWaitEvent,
                    GFxSocketImplFactory* socketImplFactory);
    virtual ~GFxAmpThreadMgr();

    // Initialize AMP - Starts the threads for sending and receiving messages
    bool                InitAmp(const char* ipAddress, UInt32 port, UInt32 broadcastPort);
    // Uninitialize AMP - Performs thread cleanup
    void                UninitAmp();
    // Initialize the broadcast receive thread
    void                StartBroadcastRecv(UInt32 port);
    // Set the connected app
    void                SetBroadcastInfo(const char* connectedApp, const char* connectedFile);
    // Heartbeat interval
    void                SetHeartbeatInterval(UInt32 heartbeatInterval);

    // Accessors
    const GStringLH&                        GetAddress() const  { return IpAddress; }
    UInt32                                  GetPort() const     { return Port; }
    GFxAmpConnStatusInterface::StatusType   GetConnectionStatus() const;
    const char*                             GetConnectionMessage() const;
    UInt32                                  GetReceivedQueueSize() const { return MsgReceivedQueue.GetSize(); }
    UInt32                                  GetSendQueueSize() const { return MsgSendQueue.GetSize(); }
    UInt32                                  GetSendRate() const { return SendRate; }
    UInt32                                  GetReceiveRate() const { return ReceiveRate; }

    // Has a socket been successfully created? Thread-safe
    bool                IsValidSocket();
    // Has a message been received from the other end recently? Thread-safe
    bool                IsValidConnection();
    // Place a message on the queue to be sent.  Thread-safe
    void                SendAmpMessage(GFxAmpMessage* msg);
    // Send a log message.  Thread-safe
    void                SendLog(const GString& logMessage, GFxLogConstants::LogMessageType messageType);
    // Retrieve the next received message from the queue.  Thread-safe
    GPtr<GFxAmpMessage> GetNextReceivedMessage();
    // Clear all messages of the specified type from the received queue
    void                ClearReceivedMessages(const GFxAmpMessage* msg);

private:
    
    enum 
    {
        // maximum packet size - larger messages are broken up
        BufferSize = 512,  

        // If no message has been sent for this long, we send a heartbeat message
#ifdef GFC_OS_WII
        DefaultHeartbeatIntervalMillisecs = 0, // 0 means no heartbeat
#else
        DefaultHeartbeatIntervalMillisecs = 1000,
#endif
    };

    // GFxAmpMsgQueue encapsulates a message queue
    // It is a thread-safe list that keeps track of its size
    class GFxAmpMsgQueue
    {
    public:
        GFxAmpMsgQueue(GEvent* sizeEvent, UInt32 sizeCheckHysterisisPercent);
        void            PushBack(GFxAmpMessage* msg);
        GFxAmpMessage*  PopFront();
        void            Clear();
        void            ClearMsgType(const GFxAmpMessage* msg);
        UInt32          GetSize() const;

    private:

        GLock                   QueueLock;
        GList<GFxAmpMessage>    Queue;
        GAtomicInt<UInt32>      Size;
        GEvent*                 SizeEvent;
        UInt32                  SizeCheckHysterisisPercent;

        void                    CheckSize(GMemoryHeap* heap);
    };


    GPtr<GThread>       SocketThread;  // Takes messages from the send queue and sends them
    GPtr<GThread>       BroadcastThread;  // Broadcasts listener socket IP address and port 
    GPtr<GThread>       BroadcastRecvThread;  // Listens for broadcast messages 
    GPtr<GThread>       CompressThread;   // Uncompresses/compresses received/sending messages
    UInt32              Port;
    GLock               BroadcastInfoLock;
    UInt32              BroadcastPort;
    GStringLH           BroadcastApp;
    GStringLH           BroadcastFile;
    UInt32              BroadcastRecvPort;
    GStringLH           IpAddress;
    bool                Server;  // Server or Client?
    GLock               SocketLock;
    GFxSocket           Socket;
    UInt32              HeartbeatIntervalMillisecs;

    // Status is checked from send and received threads
    mutable GLock                       InitLock;
    mutable GLock                       StatusLock;
    bool                                Exiting;
    UInt64                              LastSendHeartbeat;
    UInt64                              LastRcvdHeartbeat;
    GFxAmpConnStatusInterface::StatusType   ConnectionStatus;
    bool                                LastConnected;
    GAtomicInt<UInt32>                  SendRate;
    GAtomicInt<UInt32>                  ReceiveRate;

    // Message queues
    GFxAmpMsgQueue                      MsgReceivedQueue;
    GFxAmpMsgQueue                      MsgSendQueue;
    GFxAmpMsgQueue                      MsgUncompressedQueue;
    GFxAmpMsgQueue                      MsgCompressedQueue;
    GEvent*                             SendQueueWaitEvent;
    GAtomicInt<UInt32>                  MsgVersion;
    UByte                               LastGFxVersion;

    // Callback interfaces
    GFxAmpSendInterface*                SendCallback;         // send thread callback
    GFxAmpConnStatusInterface*          ConnectionChangedCallback; // connection changed callback
    GFxAmpMsgHandler*                   MsgHandler;           // message handler for bypassing the received queue
    GFxSocketImplFactory*               SocketImplFactory;

    // Thread functions
    static SInt         SocketThreadLoop(GThread* sendThread, void* param);
    static SInt         BroadcastThreadLoop(GThread* broadcastThread, void* param);
    static SInt         BroadcastRecvThreadLoop(GThread* broadcastThread, void* param);
    static int          CompressThreadLoop(GThread* compressThread, void* param);

    // Private helper functions
    bool                IsServer() const;
    bool                IsRunning() const;
    bool                IsExiting() const;
    void                SetExiting();
    void                UpdateLastReceivedTime(bool updateStatus);
    bool                SocketConnect(GString* errorMsg = NULL);
    GFxAmpMessage*      RetrieveMessageForSending();
    bool                SendReceiveLoop();
    bool                BroadcastLoop();
    bool                BroadcastRecvLoop();
    bool                CompressLoop();
    void                UpdateStatus(GFxAmpConnStatusInterface::StatusType eStatus, const char* pcMessage);
};

#endif  // GFX_USE_SOCKETS

#endif // INC_GFXAMP_H

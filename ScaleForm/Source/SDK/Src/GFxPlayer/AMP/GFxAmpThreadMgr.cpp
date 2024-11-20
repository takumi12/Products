#include "GFxAmpThreadMgr.h"

#if !defined(GFC_NO_THREADSUPPORT) && defined(GFX_USE_SOCKETS)

#include "GFxAmpStream.h"
#include "GFxAmpMessage.h"
#include "GFxAmpInterfaces.h"
#include "GFxAmpProfileFrame.h"
#include "GTimer.h"
#include "GHeapNew.h"
#include "GMsgFormat.h"
#include "GFxLog.h"
#include <time.h>

// Static 
// Send thread function
SInt GFxAmpThreadMgr::SocketThreadLoop(GThread* sendThread, void* param)
{
    GUNUSED(sendThread);

    GFxAmpThreadMgr* threadMgr = static_cast<GFxAmpThreadMgr*>(param);
    if (threadMgr == NULL)
    {
        return 1;
    }

    while (threadMgr->SendReceiveLoop())
    {
        // SendLoop exited but we are not shutting down
        // re-try to establish connection

        GThread::Sleep(1);  // seconds
    }

    return 0;
}

SInt GFxAmpThreadMgr::BroadcastThreadLoop(GThread*, void* param)
{
    GFxAmpThreadMgr* threadMgr = static_cast<GFxAmpThreadMgr*>(param);
    if (threadMgr == NULL)
    {
        return 1;
    }

    threadMgr->BroadcastLoop();

    return 0;
}

SInt GFxAmpThreadMgr::BroadcastRecvThreadLoop(GThread*, void* param)
{
    GFxAmpThreadMgr* threadMgr = static_cast<GFxAmpThreadMgr*>(param);
    if (threadMgr == NULL)
    {
        return 1;
    }

    threadMgr->BroadcastRecvLoop();

    return 0;
}

int GFxAmpThreadMgr::CompressThreadLoop(GThread*, void* param)
{
    GFxAmpThreadMgr* threadMgr = static_cast<GFxAmpThreadMgr*>(param);
    if (threadMgr == NULL)
    {
        return 1;
    }

    threadMgr->CompressLoop();

    return 0;
}

// Constructor
GFxAmpThreadMgr::GFxAmpThreadMgr(GFxAmpMsgHandler* msgHandler, 
                                 GFxAmpSendInterface* sendCallback, 
                                 GFxAmpConnStatusInterface* connectionCallback,
                                 GEvent* sendQueueWaitEvent,
                                 GFxSocketImplFactory* socketImplFactory) : 
    SocketThread(NULL),
    BroadcastThread(NULL),
    BroadcastRecvThread(NULL),
    CompressThread(NULL),
    Port(0),
    BroadcastPort(0),
    BroadcastRecvPort(0),
    Server(true),
    Socket(msgHandler->IsInitSocketLib(), socketImplFactory),
    HeartbeatIntervalMillisecs(DefaultHeartbeatIntervalMillisecs),
    Exiting(true),
    LastSendHeartbeat(0),
    LastRcvdHeartbeat(0),
    ConnectionStatus(GFxAmpConnStatusInterface::CS_Idle),
    LastConnected(false),
    SendRate(0),
    ReceiveRate(0),
    MsgReceivedQueue(NULL, 0),
    MsgSendQueue(sendQueueWaitEvent, 90),
    MsgUncompressedQueue(NULL, 0),
    MsgCompressedQueue(NULL, 0),
    SendQueueWaitEvent(sendQueueWaitEvent),
    MsgVersion(GFxAmpMessage::GetLatestVersion()),
    LastGFxVersion(0),
    SendCallback(sendCallback),
    ConnectionChangedCallback(connectionCallback),
    MsgHandler(msgHandler),
    SocketImplFactory(socketImplFactory)
{
    Socket.SetLock(&SocketLock);
}

// Destructor
GFxAmpThreadMgr::~GFxAmpThreadMgr()
{
    UninitAmp();
}

// Initialization
// Returns true if successful
// A NULL IP address means that we are going to be a server
bool GFxAmpThreadMgr::InitAmp(const char* address, UInt32 port, UInt32 broadcastPort)
{
    GLock::Locker locker(&InitLock);

    // Have we already been initialized?
    if (IsRunning())
    {
        if ((IsServer() ? address == NULL : IpAddress == address) && port == Port)
        {
            // Same connection information - done
            return true;
        }
        else
        {
            // Different connection information - start over
            UninitAmp();
        }
    }

    Exiting = false;
    Port = port;
    BroadcastPort = broadcastPort;
    Server = (address == NULL);
    if (!IsServer())
    {
        IpAddress = address;
    }

    // Start the send thread
    if (Port != 0)
    {
        SocketThread = *GHEAP_AUTO_NEW(this) GThread(GFxAmpThreadMgr::SocketThreadLoop, (void*) this);
        if (!SocketThread || !SocketThread->Start())
        {
            return false;
        }
    }

    StartBroadcastRecv(BroadcastRecvPort);

    return true;
}

// Cleanup
void GFxAmpThreadMgr::UninitAmp()
{
    GLock::Locker locker(&InitLock);

    SetExiting();

    // wait until threads are done
    if (BroadcastThread)
    {
        BroadcastThread->Wait();
        BroadcastThread = NULL;
    }

    if (BroadcastRecvThread)
    {
        BroadcastRecvThread->Wait(); 
        BroadcastRecvThread = NULL;
    }

    if (SocketThread)
    {
        SocketThread->Wait();
        SocketThread = NULL;
    }

    if (CompressThread)
    {
        CompressThread->Wait();
        CompressThread = NULL;
    }

    MsgSendQueue.Clear();
    MsgReceivedQueue.Clear();
    MsgUncompressedQueue.Clear();
    MsgCompressedQueue.Clear();
}

// Initialize the broadcast thread
void GFxAmpThreadMgr::StartBroadcastRecv(UInt32 port)
{
    BroadcastRecvPort = port;
    if (BroadcastRecvPort != 0 && !BroadcastRecvThread)
    {
        BroadcastRecvThread = *GHEAP_AUTO_NEW(this) GThread(
                                        GFxAmpThreadMgr::BroadcastRecvThreadLoop, (void*) this);
        if (BroadcastRecvThread)
        {
            GLock::Locker locker(&StatusLock);
            Exiting = false;

            BroadcastRecvThread->Start();
        }
    }
}

void GFxAmpThreadMgr::SetBroadcastInfo(const char* connectedApp, const char* connectedFile)
{
    GLock::Locker locker(&BroadcastInfoLock);
    BroadcastApp = connectedApp;
    BroadcastFile = connectedFile;
}

void GFxAmpThreadMgr::SetHeartbeatInterval(UInt32 heartbeatInterval)
{
    HeartbeatIntervalMillisecs = heartbeatInterval;
}

// Returns true when there is a thread still running
bool GFxAmpThreadMgr::IsRunning() const
{
    if (SocketThread && !SocketThread->IsFinished())
    {
        return true;
    }

    return false;
}

// Thread-safe status accessor
bool GFxAmpThreadMgr::IsExiting() const
{
    GLock::Locker locker(&StatusLock);
    return Exiting;
}

// Signals the threads that we are shutting down
void GFxAmpThreadMgr::SetExiting()
{
    GLock::Locker locker(&StatusLock);
    Exiting = true;
    Socket.Destroy();
}

// Keep track of last message received for connection state
void GFxAmpThreadMgr::UpdateLastReceivedTime(bool updateStatus)
{
    if (updateStatus)
    {
        UpdateStatus(GFxAmpConnStatusInterface::CS_OK, "");
    }

    GLock::Locker locker(&StatusLock);
    LastRcvdHeartbeat = GTimer::GetTicks();
}

// Create the socket
// Returns true on success
bool GFxAmpThreadMgr::SocketConnect(GString* errorMsg)
{
    GLock::Locker kLocker(&StatusLock);

    // Check for exit
    // This needs to be inside SocketConnect for thread safety
    // Otherwise we could exit and destroy the socket after entering this function
    if (IsExiting())
    {
        Socket.Destroy();
        return false;
    }

    UpdateStatus(GFxAmpConnStatusInterface::CS_Connecting, "");

    if (IsServer())
    {
        if (!Socket.Create(NULL, Port, errorMsg))
        {
            SetExiting();  // Something is terribly wrong, don't keep trying
            return false;
        }
    }
    else
    {
        if (!Socket.Create(IpAddress.ToCStr(), Port, errorMsg))
        {
            //
            // PPS: Remove setting CS_Failed since SendLoop tries to reconnect forever. 
            //      Semantics for CS_Failed should be defined, since the UI is expected
            //      to handle this status differently than CS_Connecting.
            //
            // if (errorMsg != NULL)
            // {
            //    UpdateStatus(GFxAmpConnStatusInterface::CS_Failed, *errorMsg);
            // }
            return false;
        }

        // If the client succeeds in creating the socket, it means we have a valid connection
        // Update the received time even though we have not yet received a message

        // We don't do this for the server until a connection has been accepted
        UpdateLastReceivedTime(false);
    }

    if (BroadcastPort != 0)
    {
        if (!BroadcastThread)
        {
            // Start the broadcast thread
            BroadcastThread = *GHEAP_AUTO_NEW(this) GThread(GFxAmpThreadMgr::BroadcastThreadLoop, (void*) this);
            if (BroadcastThread)
            {
                BroadcastThread->Start();
            }
        }
    }

    return true;
}

GFxAmpConnStatusInterface::StatusType GFxAmpThreadMgr::GetConnectionStatus() const
{
    GLock::Locker locker(&StatusLock);
    return ConnectionStatus;
}

bool GFxAmpThreadMgr::IsValidSocket()
{
    GLock::Locker locker(&StatusLock);
    return Socket.IsValid();
}

// Socket status
// Notifies connection interface on loss of connection
// Sends a log message on connection
// We are considered connected if we have received a message 
// in the last 2 * HeartbeatIntervalMillisecs
bool GFxAmpThreadMgr::IsValidConnection()
{
    GLock::Locker locker(&StatusLock);

    UInt64 ticks = GTimer::GetTicks();

    bool connected = (LastRcvdHeartbeat != 0);
    if (HeartbeatIntervalMillisecs > 0)
    {
        UInt64 deltaTicks = ticks - LastRcvdHeartbeat;
        connected = (deltaTicks < 2 * HeartbeatIntervalMillisecs * 1000);
    }

    //GFC_DEBUG_MESSAGE1(!connected, "Lost connection after %d milliseconds", (ticks - LastRcvdHeartbeat) / 1000);

    if (!connected)
    {
        // Notify that we have lost connection
        UpdateStatus(GFxAmpConnStatusInterface::CS_Connecting, NULL);
    }

    if (!LastConnected && connected)
    {
        // Send a log message to announce that we have connected
        GString strMessage;
        G_SPrintF(strMessage, "Established AMP connection on port %d\n", Port);
        SendLog(strMessage, GFxLogConstants::Log_Message);
    }
    LastConnected = connected;

    return connected;
}

// For different behavior between server and client sockets
bool GFxAmpThreadMgr::IsServer() const
{
    return Server;
}

// Reads a message from the queue
// Returns true if a message was retrieved
GFxAmpMessage* GFxAmpThreadMgr::RetrieveMessageForSending()
{
    UInt64 ticks = GTimer::GetTicks();

    // Recover the next message, if any
    GFxAmpMessage* msg = MsgCompressedQueue.PopFront();

    if (msg == NULL)
    {
        // Nothing to send. Should we send a heartbeat?
        UInt64 deltaTicks = ticks - LastSendHeartbeat;
        if (HeartbeatIntervalMillisecs > 0 && deltaTicks >  HeartbeatIntervalMillisecs * 1000)
        {
            msg = GHEAP_AUTO_NEW(this) GFxAmpMessageHeartbeat();
        }
    }

    if (msg != NULL)
    {
        msg->SetVersion(MsgVersion);
        LastSendHeartbeat = ticks;  // update the last message send time
    }

    return msg;
}

// Socket thread loop
// Returns true if exited with a broken connection
// Returns false if shutting down
bool GFxAmpThreadMgr::SendReceiveLoop()
{
    if (SendQueueWaitEvent != NULL)
    {
        SendQueueWaitEvent->SetEvent();
    }

    // Create a socket
    while (SocketConnect())
    {
        // wait until a connection has been established with the client
        if (Socket.Accept(1))
        {
            Socket.SetBlocking(false);

            // Send a heartbeat message first for version checking
            MsgVersion = GFxAmpMessage::GetLatestVersion();
            SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageHeartbeat());

            GPtr<GFxAmpStream> strmReceived = *GHEAP_AUTO_NEW(this) GFxAmpStream();
            char bufferReceived[BufferSize];

            // Signal to the connection status that we are good to go
            UpdateLastReceivedTime(true);

            // Start the thread that uncompresses the received messages
            if (!CompressThread)
            {
                CompressThread = *GHEAP_AUTO_NEW(this) GThread(GFxAmpThreadMgr::CompressThreadLoop, (void*) this);
                CompressThread->Start();
            }

            UInt64 lastSampleTime = GTimer::GetProfileTicks();
            UInt32 bytesSent = 0;
            UInt32 bytesReceived = 0;
            GPtr<GFxAmpStream> streamSend = *GHEAP_AUTO_NEW(this) GFxAmpStream();
            UPInt streamSendDataLeft = streamSend->GetBufferSize();
            const char* sendBuffer = NULL;

            // Keep sending messages from the send queue
            while (!IsExiting() && IsValidConnection() && !Socket.CheckAbort())
            {
                bool actionPerformed = false;

                // Check for a callback
                if (SendCallback != NULL)
                {
                    if (SendCallback->OnSendLoop())
                    {
                        actionPerformed = true;
                    }
                }

                if (streamSendDataLeft == 0)
                {
                    // Retrieve the next message from the send queue
                    GPtr<GFxAmpMessage> msg = *RetrieveMessageForSending();
                    if (msg)
                    {
                        streamSend = *GHEAP_AUTO_NEW(this) GFxAmpStream();
                        msg->Write(*streamSend);
                        streamSendDataLeft = streamSend->GetBufferSize();
                        sendBuffer = reinterpret_cast<const char*>(streamSend->GetBuffer());
                    }
                }

                if (streamSendDataLeft > 0)
                {
                    UPInt nextSendSize;
                    if (streamSendDataLeft <= BufferSize)
                    {
                        // The message fits in one packet
                        nextSendSize = streamSendDataLeft;
                    }
                    else
                    {
                        // The message does not fit. 
                        // Send the first BUFFER_SIZE bytes in this packet, 
                        // and the rest in the next one
                        nextSendSize = BufferSize;
                    }

                    // Send the packet
                    int sentBytes = Socket.Send(sendBuffer, nextSendSize);
                    if (sentBytes > 0)
                    {
                        //GFC_DEBUG_MESSAGE3(true, "Sent %d/%d of %s", sentBytes, streamDataSizeLeft, msg->ToString().ToCStr());
                        bytesSent += sentBytes;
                        sendBuffer += sentBytes;
                        streamSendDataLeft -= sentBytes;
                        actionPerformed = true;
                    }
                    else if (sentBytes < 0)
                    {
                        // Failed to send. Retry
                    }
                }

                // Check for incoming messages and place on the received queue
                int packetSize = Socket.Receive(bufferReceived, BufferSize);
                if (packetSize > 0)
                {
                    bytesReceived += packetSize;
                    actionPerformed = true;

                    // add packet to previously-received incomplete data
                    strmReceived->Append(reinterpret_cast<UByte*>(bufferReceived), packetSize);

                    //GFC_DEBUG_MESSAGE3(true, "Received %u/%u bytes (%u)", packetSize, strmReceived->FirstMessageSize(), strmReceived->GetBufferSize());
                    UpdateLastReceivedTime(false);
                }

                UPInt readBufferSize = strmReceived->GetBufferSize();
                if (readBufferSize > 0 && readBufferSize >= strmReceived->FirstMessageSize())
                {
                    // Read the stream and create a message
                    GFxAmpMessage* msg = GFxAmpMessage::CreateAndReadMessage(*strmReceived, GMemory::GetHeapByAddress(this));

                    // Update the buffer to take into account what we already read
                    strmReceived->PopFirstMessage();

                    if (msg != NULL)
                    {
                        MsgReceivedQueue.PushBack(msg);
                    }
                    else
                    {
                        GFC_DEBUG_MESSAGE(1, "Corrupt message received");
                    }
                }

                if (!actionPerformed)
                {
                    GThread::MSleep(10);  // Don't hog the CPU
                }

                // Update the byte rates
                UInt64 nextTicks = GTimer::GetProfileTicks();
                UInt32 deltaTicks = static_cast<UInt32>(nextTicks - lastSampleTime);
                if (deltaTicks > GFX_MICROSECONDS_PER_SECOND) // one second
                {
                    SendRate = bytesSent * GFX_MICROSECONDS_PER_SECOND / deltaTicks;
                    ReceiveRate = bytesReceived * GFX_MICROSECONDS_PER_SECOND / deltaTicks;
                    lastSampleTime = nextTicks;
                    bytesSent = 0;
                    bytesReceived = 0;
                    //GFC_DEBUG_MESSAGE2(true, "Send: %d bytes/s, Receive: %d bytes/s", SendRate, ReceiveRate);
                }                
            }
        }

        Socket.Destroy();
    }

    return !IsExiting();
}

// UDP socket broadcast loop
bool GFxAmpThreadMgr::BroadcastLoop()
{
    GFxBroadcastSocket broadcastSocket(MsgHandler->IsInitSocketLib(), SocketImplFactory);

    if (!broadcastSocket.Create(BroadcastPort, true))
    {
        return false;
    }

    while (!IsExiting())
    {
        if (!IsValidConnection())
        {
            GPtr<GFxAmpStream> stream = *GHEAP_AUTO_NEW(this) GFxAmpStream();
            BroadcastInfoLock.Lock();
            GPtr<GFxAmpMessagePort> msg = *GHEAP_AUTO_NEW(this) GFxAmpMessagePort(Port, BroadcastApp, BroadcastFile);
            BroadcastInfoLock.Unlock();

            msg->Write(*stream);

            broadcastSocket.Broadcast(
                reinterpret_cast<const char*>(stream->GetBuffer()), 
                stream->GetBufferSize());
        }

        GThread::Sleep(1);
    }

    return true;
}

// UDP socket broadcast receive loop
bool GFxAmpThreadMgr::BroadcastRecvLoop()
{
    GFxBroadcastSocket broadcastSocket(MsgHandler->IsInitSocketLib(), SocketImplFactory);
    char bufferReceived[BufferSize];

    if (!broadcastSocket.Create(BroadcastRecvPort, false))
    {
        return false;
    }

    while (!IsExiting())
    {
        int packetSize = broadcastSocket.Receive(bufferReceived, BufferSize);
        if (packetSize > 0)
        {
            GPtr<GFxAmpStream> strmReceived = *GHEAP_AUTO_NEW(this) GFxAmpStream(
                                            reinterpret_cast<UByte*>(bufferReceived), packetSize);
            GASSERT(strmReceived->FirstMessageSize() == static_cast<UPInt>(packetSize));
            if (strmReceived->FirstMessageSize() == static_cast<UPInt>(packetSize))
            {
                UInt32 port;
                UInt32 address;
                char name[256];
                broadcastSocket.GetName(&port, &address, name, 256);
                MsgHandler->SetRecvAddress(port, address, name);

                // Read the stream and create a message
                GPtr<GFxAmpMessage> msg = *GFxAmpMessage::CreateAndReadMessage(
                                                *strmReceived, GMemory::GetHeapByAddress(this));
                if (msg)
                {
                    // Uncompress the message, if necessary
                    GFxAmpMessage* uncompressed = msg->Uncompress();
                    if (uncompressed != NULL)
                    {
                        msg = *uncompressed;
                    }
                }

                if (msg)
                {
                    msg->AcceptHandler(MsgHandler);
                }
            }
        }
        else
        {
            GThread::MSleep(100);
        }
    }

    return true;
}

bool GFxAmpThreadMgr::CompressLoop()
{
    while (!IsExiting())
    {
        bool actionTaken = false;
        GFxAmpMessage* msg = MsgReceivedQueue.PopFront();
        if (msg != NULL)
        {
            // Uncompress the message, if necessary
            GFxAmpMessage* uncompressed = msg->Uncompress();
            if (uncompressed != NULL)
            {
                msg->Release();
                msg = uncompressed;
            }

            if (msg->GetGFxVersion() != LastGFxVersion)
            {
                LastGFxVersion = static_cast<UByte>(msg->GetGFxVersion());
                if (ConnectionChangedCallback != NULL)
                {
                    ConnectionChangedCallback->OnMsgGFxVersionChanged(msg->GetGFxVersion());
                }
            }

            if (msg->GetVersion() < MsgVersion)
            {
                MsgVersion = msg->GetVersion();
                if (ConnectionChangedCallback != NULL)
                {
                    ConnectionChangedCallback->OnMsgVersionMismatch(msg->GetVersion());
                }
            }

            if (msg->ShouldQueue() || MsgHandler == NULL)
            {
                // Place the new message on the received queue
                MsgUncompressedQueue.PushBack(msg);
            }
            else
            {
                // Handle immediately
                // bypass the queue
                msg->AcceptHandler(MsgHandler);
                msg->Release();
            }
            actionTaken = true;
        }

        // Compress the messages to be sent
        msg = MsgSendQueue.PopFront();
        if (msg != NULL)
        {
            if (MsgVersion >= 18)
            {
                msg->SetVersion(MsgVersion);
                GFxAmpMessage* msgCompressed = msg->Compress();
                if (msgCompressed != NULL)
                {
                    msg->Release();
                    msg = msgCompressed;
                }
            }
            MsgCompressedQueue.PushBack(msg);
            actionTaken = true;
        }

        if (!actionTaken)
        {
            GThread::MSleep(100);
        }

    }

    return true;
}

// Places a message on the send queue
// after serializing it into a stream
void GFxAmpThreadMgr::SendAmpMessage(GFxAmpMessage* msg)
{
    MsgSendQueue.PushBack(msg);
}

// Sends a log message
// This is a convenience function that just calls SendAmpMessage
void GFxAmpThreadMgr::SendLog(const GString& messageText, GFxLogConstants::LogMessageType messageType)
{
    // create time stamp for log message
    time_t rawTime;
    time(&rawTime);

    // Send a message with the appropriate category for filtering
    SendAmpMessage(GHEAP_AUTO_NEW(this) GFxAmpMessageLog(messageText, messageType, rawTime));
}

// Retrieves a message from the received queue
// Returns NULL if there is no message to retrieve
GPtr<GFxAmpMessage> GFxAmpThreadMgr::GetNextReceivedMessage()
{
    return *MsgUncompressedQueue.PopFront();
}

// Clears all messages of the specified type from the received queue
// If the input parameter is NULL, the it clears all the messages
void GFxAmpThreadMgr::ClearReceivedMessages(const GFxAmpMessage* msg)
{
    MsgReceivedQueue.ClearMsgType(msg);
}

// Notify the callback object that the connection status has changed
void GFxAmpThreadMgr::UpdateStatus(GFxAmpConnStatusInterface::StatusType status, 
                                   const char* statusMsg)
{
    GLock::Locker lock(&StatusLock);

    if (ConnectionStatus != status)
    {
        GFxAmpConnStatusInterface::StatusType oldStatus = ConnectionStatus;
        ConnectionStatus = status;

        if (ConnectionChangedCallback != NULL)
        {
            ConnectionChangedCallback->OnStatusChanged(status, oldStatus, statusMsg);
        }
    }
}

///////////////////////////////////////////////////////////////////

GFxAmpThreadMgr::GFxAmpMsgQueue::GFxAmpMsgQueue(GEvent* sizeEvent, UInt32 sizeCheckHysterisisPercent) : 
    Size(0), 
    SizeEvent(sizeEvent),
    SizeCheckHysterisisPercent(sizeCheckHysterisisPercent)
{
    if (SizeEvent != NULL)
    {
        SizeEvent->SetEvent();
    }
}


void GFxAmpThreadMgr::GFxAmpMsgQueue::PushBack(GFxAmpMessage* msg)
{
    GLock::Locker locker(&QueueLock);
    Queue.PushBack(msg);
    ++Size;
    CheckSize(GMemory::GetHeapByAddress(msg));
}

GFxAmpMessage* GFxAmpThreadMgr::GFxAmpMsgQueue::PopFront()
{
    GFxAmpMessage* msg = NULL;
    GLock::Locker locker(&QueueLock);
    if (!Queue.IsEmpty())
    {
        msg = Queue.GetFirst();
        GMemoryHeap* heap = GMemory::GetHeapByAddress(msg);
        Queue.Remove(msg);
        --Size;
        CheckSize(heap);
    }
    return msg;
}

void GFxAmpThreadMgr::GFxAmpMsgQueue::Clear()
{
    GLock::Locker locker(&QueueLock);
    while (!Queue.IsEmpty())
    {
        GFxAmpMessage* msg = Queue.GetFirst();
        Queue.Remove(msg);
        msg->Release();
    }
    Size = 0;
    if (SizeEvent != NULL)
    {
        SizeEvent->SetEvent();
    }
}

void GFxAmpThreadMgr::GFxAmpMsgQueue::ClearMsgType(const GFxAmpMessage* msg)
{
    GLock::Locker locker(&QueueLock);
    GFxAmpMessage* msgQueued = Queue.GetFirst();
    UPInt queueInitSize = Size;
    for (UPInt i = 0; i < queueInitSize; ++i)
    {
        if (msg == NULL || msgQueued->IsSameType(*msg))
        {
            GFxAmpMessage* msgNext = Queue.GetNext(msgQueued);
            GMemoryHeap* heap = GMemory::GetHeapByAddress(msgNext);
            Queue.Remove(msgQueued);
            msgQueued->Release();
            msgQueued = msgNext;
            --Size;
            CheckSize(heap);
        }
    }    
}

UInt32 GFxAmpThreadMgr::GFxAmpMsgQueue::GetSize() const
{
    return Size;
}

void GFxAmpThreadMgr::GFxAmpMsgQueue::CheckSize(GMemoryHeap* heap)
{
    if (SizeEvent != NULL && heap->GetLimit() > 0)
    {
        if (100 * heap->GetFootprint() < heap->GetLimit() * SizeCheckHysterisisPercent)
        {
            SizeEvent->SetEvent();
        }
        else if (heap->GetFootprint() > heap->GetLimit())
        {
            SizeEvent->ResetEvent();
            GFC_DEBUG_MESSAGE1(true, "Waiting for AMP... Queue size=%d\n", static_cast<UInt32>(Size));
            if (Size < 2)
            {
                // Increase heap limit because barely fits one AMP message
                heap->SetLimit(2 * heap->GetLimit());
            }
        }
    }
}


#endif  // GFC_NO_THREADSUPPORT

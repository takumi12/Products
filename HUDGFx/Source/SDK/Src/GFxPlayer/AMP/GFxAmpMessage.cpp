/**********************************************************************

Filename    :   GFxAmpMessage.cpp
Content     :   Messages sent back and forth to AMP
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxAmpMessage.h"
#include "GFxAmpStream.h"
#include "GFxAmpInterfaces.h"
#include "GMemoryHeap.h"
#include "GHeapNew.h"
#include "GMsgFormat.h"
#include "GFxAmpProfileFrame.h"
#include "GFxAmpServer.h"

#ifdef GFC_USE_ZLIB
#include "zlib.h"
#endif

// static factory method
GFxAmpMessage* GFxAmpMessage::CreateMessage(MessageType msgType, GMemoryHeap* heap)
{
    GFxAmpMessage* message = NULL;
    switch (msgType)
    {
    case Msg_Heartbeat:
        message = GHEAP_NEW(heap) GFxAmpMessageHeartbeat();
        break;
    case Msg_Log:
        message = GHEAP_NEW(heap) GFxAmpMessageLog();
        break;
    case Msg_CurrentState:
        message = GHEAP_NEW(heap) GFxAmpMessageCurrentState();
        break;
    case Msg_ProfileFrame:
        message = GHEAP_NEW(heap) GFxAmpMessageProfileFrame();
        break;
    case Msg_SwdFile:
        message = GHEAP_NEW(heap) GFxAmpMessageSwdFile();
        break;
    case Msg_SourceFile:
        message = GHEAP_NEW(heap) GFxAmpMessageSourceFile();
        break;
    case Msg_SwdRequest:
        message = GHEAP_NEW(heap) GFxAmpMessageSwdRequest();
        break;
    case Msg_SourceRequest:
        message = GHEAP_NEW(heap) GFxAmpMessageSourceRequest();
        break;
    case Msg_AppControl:
        message = GHEAP_NEW(heap) GFxAmpMessageAppControl();
        break;
    case Msg_Port:
        message = GHEAP_NEW(heap) GFxAmpMessagePort();
        break;
    case Msg_ImageRequest:
        message = GHEAP_NEW(heap) GFxAmpMessageImageRequest();
        break;
    case Msg_ImageData:
        message = GHEAP_NEW(heap) GFxAmpMessageImageData();
        break;
    case Msg_FontRequest:
        message = GHEAP_NEW(heap) GFxAmpMessageFontRequest();
        break;
    case Msg_FontData:
        message = GHEAP_NEW(heap) GFxAmpMessageFontData();
        break;
    case Msg_Compressed:
        message = GHEAP_NEW(heap) GFxAmpMessageCompressed();
        break;    
    default:
        break;
    }
    return message;
}

// static factory method that extracts data from the stream and creates a new message
GFxAmpMessage* GFxAmpMessage::CreateAndReadMessage(GFxAmpStream& str, GMemoryHeap* heap)
{
    MessageType msgType = static_cast<MessageType>(str.ReadUByte());
    UInt32 msgVersion = str.ReadUInt32();
    if (msgVersion > Version_Latest)
    {
        return NULL;
    }

    str.Rewind();

    GFxAmpMessage* message = CreateMessage(msgType, heap);
    if (message != NULL)
    {
        message->Read(str);
    }

    return message;
}

// Constructor
GFxAmpMessage::GFxAmpMessage(MessageType msgType) :
    MsgType(msgType),
    Version(Version_Latest),
    GFxVersion(GFx_Version_3)
{
}

// Virtual method called by the message handler when a message of unknown type needs to be handled
// AcceptHandler in turn calls a message-specific method of the message handler
// This paradigm allows the message handler to process messages in an object-oriented way
// Returns true if the client handles this message
bool GFxAmpMessage::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    GUNUSED(handler);
    return false;
}

// Serialization
void GFxAmpMessage::Read(GFxAmpStream& str)
{
    MsgType = static_cast<MessageType>(str.ReadUByte());
    Version = str.ReadUInt32();
    if (Version >= 22)
    {
        GFxVersion = static_cast<GFxVersionType>(str.ReadUByte());
    }
}

// Serialization
void GFxAmpMessage::Write(GFxAmpStream& str) const
{
    str.WriteUByte(static_cast<UByte>(MsgType));
    str.WriteUInt32(Version);
    if (Version >= 22)
    {
        str.WriteUByte(static_cast<UByte>(GFxVersion));
    }
}

#ifdef GFC_USE_ZLIB
// Memory allocation functions for ZLib
static voidpf ZLibAllocFunc(voidpf opaque, uInt items, uInt size)
{
    return GHEAP_AUTO_ALLOC(opaque, size * items);
}

static void ZLibFreeFunc(voidpf, voidpf address)
{
    GFREE(address);
}
#endif

GFxAmpMessage* GFxAmpMessage::Compress() const
{
#ifdef GFC_USE_ZLIB
    static const unsigned zlibOutBufferSize = 1024;
    UByte zlibOutBuffer[zlibOutBufferSize];
    z_stream strm;
    strm.zalloc = ZLibAllocFunc;
    strm.zfree = ZLibFreeFunc;
    strm.opaque = (voidpf) this;

    int ret = deflateInit(&strm, Z_BEST_SPEED);
    GFC_DEBUG_MESSAGE1(ret != Z_OK, "Zlib deflateInit failed with code {0}", ret);
    if (ret != Z_OK)
    {
        return NULL;
    }

    GPtr<GFxAmpStream> serializedMsg = *GHEAP_AUTO_NEW(this) GFxAmpStream();
    Write(*serializedMsg);
    strm.avail_in = static_cast<uInt>(serializedMsg->GetBufferSize());
    strm.next_in = const_cast<UByte*>(serializedMsg->GetBuffer());

    GFxAmpMessageCompressed* msgCompressed = GHEAP_AUTO_NEW(this) GFxAmpMessageCompressed();
    msgCompressed->SetVersion(Version);

    do 
    {
        strm.avail_out = zlibOutBufferSize;
        strm.next_out = zlibOutBuffer;

        ret = deflate(&strm, Z_FINISH);
        GASSERT(ret != Z_STREAM_ERROR);

        msgCompressed->AddCompressedData(zlibOutBuffer, zlibOutBufferSize - strm.avail_out);

    } while (strm.avail_out == 0);
    GASSERT(ret == Z_STREAM_END);
    deflateEnd(&strm);

    return msgCompressed;
#else
    return NULL;
#endif
}


//////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageText::GFxAmpMessageText(MessageType msgType, const GString& textValue) : 
    GFxAmpMessage(msgType),
    TextValue(textValue)
{
}

// Serialization
void GFxAmpMessageText::Read(GFxAmpStream& str)
{
    GFxAmpMessage::Read(str);
    str.ReadString(&TextValue);
}

// Serialization
void GFxAmpMessageText::Write(GFxAmpStream& str) const
{
    GFxAmpMessage::Write(str);
    str.WriteString(TextValue);
}

// Text field accessor
const GStringLH& GFxAmpMessageText::GetText() const
{
    return TextValue;
}

//////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageInt::GFxAmpMessageInt(MessageType msgType, UInt32 intValue) : 
    GFxAmpMessage(msgType),
    BaseValue(intValue)
{
}

// Serialization
void GFxAmpMessageInt::Read(GFxAmpStream& str)
{
    GFxAmpMessage::Read(str);
    BaseValue = str.ReadUInt32();
}

// Serialization
void GFxAmpMessageInt::Write(GFxAmpStream& str) const
{
    GFxAmpMessage::Write(str);
    str.WriteUInt32(BaseValue);
}

// Text field accessor
UInt32 GFxAmpMessageInt::GetValue() const
{
    return BaseValue;
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageHeartbeat::GFxAmpMessageHeartbeat() : GFxAmpMessage(Msg_Heartbeat)
{
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageLog::GFxAmpMessageLog(const GString& logText, UInt32 logCategory, const UInt64 timeStamp) : 
    GFxAmpMessageText(Msg_Log, logText),
    LogCategory(logCategory)
{
    static const int textSize = 9;
    char timeStampText[textSize];
    UInt32 seconds = static_cast<UInt32>(timeStamp % 60);
    UInt32 minutes = static_cast<UInt32>((timeStamp / 60) % 60);
    UInt32 hours = static_cast<UInt32>((timeStamp / 3600) % 24);
    G_sprintf(timeStampText, textSize, "%02u:%02u:%02u", hours, minutes, seconds);
    TimeStamp = timeStampText;
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageLog::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleLog(this);
}

// Serialization
void GFxAmpMessageLog::Read(GFxAmpStream& str)
{
    GFxAmpMessageText::Read(str);
    LogCategory = str.ReadUInt32();
    str.ReadString(&TimeStamp);

    if (Version <= 2)
    {
        for (int i = 0; i < 128; ++i)
        {
            str.ReadUInt32();
        }
    }
}

// Serialization
void GFxAmpMessageLog::Write(GFxAmpStream& str) const
{
    GFxAmpMessageText::Write(str);
    str.WriteUInt32(LogCategory);
    str.WriteString(TimeStamp);

    if (Version <= 2)
    {
        for (int i = 0; i < 128; ++i)
        {
            str.WriteUInt32(0);
        }
    }
}

// Accessor
// Category of the log message. Error, warning, informational, etc
UInt32 GFxAmpMessageLog::GetLogCategory() const
{
    return LogCategory;
}

// Accessor
const GStringLH& GFxAmpMessageLog::GetTimeStamp() const
{
    return TimeStamp;
}

/////////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageCurrentState::GFxAmpMessageCurrentState(const GFxAmpCurrentState* state) : 
    GFxAmpMessage(Msg_CurrentState)
{
    State = *GHEAP_AUTO_NEW(this) GFxAmpCurrentState();
    if (state != NULL)
    {
        *State = *state;
    }
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageCurrentState::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleCurrentState(this);
}

// Serialization
void GFxAmpMessageCurrentState::Read(GFxAmpStream& str)
{
    GFxAmpMessage::Read(str);
    State->Read(str, Version);
}

// Serialization
void GFxAmpMessageCurrentState::Write(GFxAmpStream& str) const
{
    GFxAmpMessage::Write(str);
    State->Write(str, Version);
}

/////////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageProfileFrame::GFxAmpMessageProfileFrame(GPtr<GFxAmpProfileFrame> frameInfo) : 
    GFxAmpMessage(Msg_ProfileFrame), 
    FrameInfo(frameInfo)
{
}

GFxAmpMessageProfileFrame::~GFxAmpMessageProfileFrame()
{
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageProfileFrame::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleProfileFrame(this);
}

// Serialization
void GFxAmpMessageProfileFrame::Read(GFxAmpStream& str)
{
    GFxAmpMessage::Read(str);
    FrameInfo = *GHEAP_AUTO_NEW(this) GFxAmpProfileFrame();
    FrameInfo->Read(str, Version);
}

// Serialization
void GFxAmpMessageProfileFrame::Write(GFxAmpStream& str) const
{
    GFxAmpMessage::Write(str);
    GASSERT(FrameInfo);
    FrameInfo->Write(str, Version);
}

// Accessor
const GFxAmpProfileFrame* GFxAmpMessageProfileFrame::GetFrameInfo() const
{
    return FrameInfo;
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageSwdFile::GFxAmpMessageSwdFile(UInt32 swfHandle, UByte* bufferData, 
                                           UInt bufferSize, const char* filename) : 
    GFxAmpMessageInt(Msg_SwdFile, swfHandle), 
    Filename(filename)
{
    FileData.Resize(bufferSize);
    for (UPInt i = 0; i < bufferSize; ++i)
    {
        FileData[i] = *(bufferData + i);
    }
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageSwdFile::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleSwdFile(this);
}

// Serialization
void GFxAmpMessageSwdFile::Read(GFxAmpStream& str)
{
    GFxAmpMessageInt::Read(str);
    UInt32 dataSize = str.ReadUInt32();
    FileData.Resize(dataSize);
    for (UInt32 i = 0; i < dataSize; ++i)
    {
        FileData[i] = str.ReadUByte();
    }
    str.ReadString(&Filename);
}

// Serialization
void GFxAmpMessageSwdFile::Write(GFxAmpStream& str) const
{
    GFxAmpMessageInt::Write(str);

    str.WriteUInt32(static_cast<UInt32>(FileData.GetSize()));
    for (UInt32 i = 0; i < FileData.GetSize(); ++i)
    {
        str.WriteUByte(FileData[i]);
    }
    str.WriteString(Filename);
}


// Accessor
const GArrayLH<UByte>& GFxAmpMessageSwdFile::GetFileData() const
{
    return FileData;
}

// Accessor
const char* GFxAmpMessageSwdFile::GetFilename() const
{
    return Filename;
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageSourceFile::GFxAmpMessageSourceFile(UInt64 fileHandle, UByte* bufferData, 
                                                 UInt bufferSize, const char* filename) : 
    GFxAmpMessage(Msg_SourceFile), 
    FileHandle(fileHandle),
    Filename(filename)
{
    FileData.Resize(bufferSize);
    for (UPInt i = 0; i < bufferSize; ++i)
    {
        FileData[i] = *(bufferData + i);
    }
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageSourceFile::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleSourceFile(this);
}

// Serialization
void GFxAmpMessageSourceFile::Read(GFxAmpStream& str)
{
    GFxAmpMessage::Read(str);
    FileHandle = str.ReadUInt64();
    UInt32 dataSize = str.ReadUInt32();
    FileData.Resize(dataSize);
    for (UInt32 i = 0; i < dataSize; ++i)
    {
        FileData[i] = str.ReadUByte();
    }
    str.ReadString(&Filename);
}

// Serialization
void GFxAmpMessageSourceFile::Write(GFxAmpStream& str) const
{
    GFxAmpMessage::Write(str);

    str.WriteUInt64(FileHandle);
    str.WriteUInt32(static_cast<UInt32>(FileData.GetSize()));
    for (UInt32 i = 0; i < FileData.GetSize(); ++i)
    {
        str.WriteUByte(FileData[i]);
    }
    str.WriteString(Filename);
}


// Accessor
UInt64 GFxAmpMessageSourceFile::GetFileHandle() const
{
    return FileHandle;
}

// Accessor
const GArrayLH<UByte>& GFxAmpMessageSourceFile::GetFileData() const
{
    return FileData;
}

// Accessor
const char* GFxAmpMessageSourceFile::GetFilename() const
{
    return Filename;
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageSwdRequest::GFxAmpMessageSwdRequest(UInt32 swfHandle, bool requestContents) : 
    GFxAmpMessageInt(Msg_SwdRequest, swfHandle), 
    RequestContents(requestContents)
{
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageSwdRequest::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleSwdRequest(this);
}

// Serialization
void GFxAmpMessageSwdRequest::Read(GFxAmpStream& str)
{
    GFxAmpMessageInt::Read(str);
    RequestContents = (str.ReadUByte() != 0);
}

// Serialization
void GFxAmpMessageSwdRequest::Write(GFxAmpStream& str) const
{
    GFxAmpMessageInt::Write(str);
    str.WriteUByte(RequestContents ? 1 : 0);
}

// Accessor
bool GFxAmpMessageSwdRequest::IsRequestContents() const
{
    return RequestContents;
}


////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageSourceRequest::GFxAmpMessageSourceRequest(UInt64 handle, bool requestContents) : 
    GFxAmpMessage(Msg_SourceRequest), 
    FileHandle(handle),
    RequestContents(requestContents)
{
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageSourceRequest::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleSourceRequest(this);
}

// Serialization
void GFxAmpMessageSourceRequest::Read(GFxAmpStream& str)
{
    GFxAmpMessage::Read(str);
    FileHandle = str.ReadUInt64();
    RequestContents = (str.ReadUByte() != 0);
}

// Serialization
void GFxAmpMessageSourceRequest::Write(GFxAmpStream& str) const
{
    GFxAmpMessage::Write(str);
    str.WriteUInt64(FileHandle);
    str.WriteUByte(RequestContents ? 1 : 0);
}

// Accessor
UInt64 GFxAmpMessageSourceRequest::GetFileHandle() const
{
    return FileHandle;
}

// Accessor
bool GFxAmpMessageSourceRequest::IsRequestContents() const
{
    return RequestContents;
}


////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageAppControl::GFxAmpMessageAppControl(UInt32 flags) : 
    GFxAmpMessageInt(Msg_AppControl, flags), ProfileLevel(-1)
{
}

// Virtual method called from GFxAmpServer::HandleNextMessage
// Returns true if the client handles this message
bool GFxAmpMessageAppControl::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleAppControl(this);
}

// Serialization
void GFxAmpMessageAppControl::Read(GFxAmpStream& str)
{
    GFxAmpMessageInt::Read(str);
    str.ReadString(&LoadMovieFile);
    if (Version >= 20)
    {
        ProfileLevel = str.ReadSInt32();
    }
}

// Serialization
void GFxAmpMessageAppControl::Write(GFxAmpStream& str) const
{
    GFxAmpMessageInt::Write(str);
    str.WriteString(LoadMovieFile);
    if (Version >= 20)
    {
        str.WriteSInt32(ProfileLevel);
    }
}

// Setting this flag toggles wireframe mode on the app
bool GFxAmpMessageAppControl::IsToggleWireframe() const
{
    return (BaseValue & OF_ToggleWireframe) != 0;
}

// Setting this flag toggles wireframe mode on the app
void GFxAmpMessageAppControl::SetToggleWireframe(bool wireframeToggle)
{
    if (wireframeToggle)
    {
        BaseValue |= OF_ToggleWireframe;
    }
    else
    {
        BaseValue &= (~OF_ToggleWireframe);
    }
}

// Setting this flag toggles pause on the movie
bool GFxAmpMessageAppControl::IsTogglePause() const
{
    return (BaseValue & OF_TogglePause) != 0;
}

// Setting this flag toggles pause on the movie
void GFxAmpMessageAppControl::SetTogglePause(bool pauseToggle)
{
    if (pauseToggle)
    {
        BaseValue |= OF_TogglePause;
    }
    else
    {
        BaseValue &= (~OF_TogglePause);
    }
}

// Setting this flag toggles fast forward on the movie
bool GFxAmpMessageAppControl::IsToggleFastForward() const
{
    return (BaseValue & OF_ToggleFastForward) != 0;
}

// Setting this flag toggles fast forward on the movie
void GFxAmpMessageAppControl::SetToggleFastForward(bool ffToggle)
{
    if (ffToggle)
    {
        BaseValue |= OF_ToggleFastForward;
    }
    else
    {
        BaseValue &= (~OF_ToggleFastForward);
    }
}

// Setting this flag toggles between edge anti-aliasing modes
bool GFxAmpMessageAppControl::IsToggleAaMode() const
{
    return (BaseValue & OF_ToggleAaMode) != 0;
}

// Setting this flag toggles between edge anti-aliasing modes
void GFxAmpMessageAppControl::SetToggleAaMode(bool aaToggle)
{
    if (aaToggle)
    {
        BaseValue |= OF_ToggleAaMode;
    }
    else
    {
        BaseValue &= (~OF_ToggleAaMode);
    }
}

// Setting this flag toggles between stroke types
bool GFxAmpMessageAppControl::IsToggleStrokeType() const
{
    return (BaseValue & OF_ToggleStrokeType) != 0;
}

// Setting this flag toggles between stroke types
void GFxAmpMessageAppControl::SetToggleStrokeType(bool strokeToggle)
{
    if (strokeToggle)
    {
        BaseValue |= OF_ToggleStrokeType;
    }
    else
    {
        BaseValue &= (~OF_ToggleStrokeType);
    }
}

// Setting this flag restarts the flash movie on the app
bool GFxAmpMessageAppControl::IsRestartMovie() const
{
    return (BaseValue & OF_RestartMovie) != 0;
}

// Setting this flag restarts the flash movie on the app
void GFxAmpMessageAppControl::SetRestartMovie(bool restart)
{
    if (restart)
    {
        BaseValue |= OF_RestartMovie;
    }
    else
    {
        BaseValue &= (~OF_RestartMovie);
    }
}

// Setting this flag cycles through the font configurations 
bool GFxAmpMessageAppControl::IsNextFont() const
{
    return (BaseValue & OF_NextFont) != 0;
}

// Setting this flag cycles through the font configurations 
void GFxAmpMessageAppControl::SetNextFont(bool next)
{
    if (next)
    {
        BaseValue |= OF_NextFont;
    }
    else
    {
        BaseValue &= (~OF_NextFont);
    }
}

// Setting this flag increases the curve tolerance
bool GFxAmpMessageAppControl::IsCurveToleranceUp() const
{
    return (BaseValue & OF_CurveToleranceUp) != 0;
}

// Setting this flag increases the curve tolerance
void GFxAmpMessageAppControl::SetCurveToleranceUp(bool up)
{
    if (up)
    {
        BaseValue |= OF_CurveToleranceUp;
    }
    else
    {
        BaseValue &= (~OF_CurveToleranceUp);
    }
}

// Setting this flag decreases the curve tolerance
bool GFxAmpMessageAppControl::IsCurveToleranceDown() const
{
    return (BaseValue & OF_CurveToleranceDown) != 0;
}

// Setting this flag decreases the curve tolerance
void GFxAmpMessageAppControl::SetCurveToleranceDown(bool down)
{
    if (down)
    {
        BaseValue |= OF_CurveToleranceDown;
    }
    else
    {
        BaseValue &= (~OF_CurveToleranceDown);
    }
}

bool GFxAmpMessageAppControl::IsForceInstructionProfile() const
{
    return (BaseValue & OF_CurveToleranceDown) != 0;
}

void GFxAmpMessageAppControl::SetForceInstructionProfile(bool instProf)
{
    if (instProf)
    {
        BaseValue |= OF_ForceInstructionProfile;
    }
    else
    {
        BaseValue &= (~OF_ForceInstructionProfile);
    }
}


// Setting this flag toggles amp sending profile messages every frame
bool GFxAmpMessageAppControl::IsToggleAmpRecording() const
{
    return (BaseValue & OF_ToggleAmpRecording) != 0;
}

// Setting this flag toggles amp sending profile messages every frame
void GFxAmpMessageAppControl::SetToggleAmpRecording(bool recordingToggle)
{
    if (recordingToggle)
    {
        BaseValue |= OF_ToggleAmpRecording;
    }
    else
    {
        BaseValue &= (~OF_ToggleAmpRecording);
    }
}

// Setting this flag toggles amp renderer to draw hot spots
bool GFxAmpMessageAppControl::IsToggleOverdraw() const
{
    return (BaseValue & OF_ToggleOverdraw) != 0;
}

// Setting this flag toggles amp renderer to draw hot spots
void GFxAmpMessageAppControl::SetToggleOverdraw(bool overdrawToggle)
{
    if (overdrawToggle)
    {
        BaseValue |= OF_ToggleOverdraw;
    }
    else
    {
        BaseValue &= (~OF_ToggleOverdraw);
    }
}

// Setting this flag toggles amp renderer to draw hot spots
bool GFxAmpMessageAppControl::IsToggleBatch() const
{
    return (BaseValue & OF_ToggleBatch) != 0;
}

// Setting this flag toggles amp renderer to draw hot spots
void GFxAmpMessageAppControl::SetToggleBatch(bool overdrawBatch)
{
    if (overdrawBatch)
    {
        BaseValue |= OF_ToggleBatch;
    }
    else
    {
        BaseValue &= (~OF_ToggleBatch);
    }
}

// Setting this flag toggles instruction profiling for per-line timings
bool GFxAmpMessageAppControl::IsToggleInstructionProfile() const
{
    return (BaseValue & OF_ToggleInstructionProfile) != 0;
}

// Setting this flag toggles instruction profiling for per-line timings
void GFxAmpMessageAppControl::SetToggleInstructionProfile(bool instToggle)
{
    if (instToggle)
    {
        BaseValue |= OF_ToggleInstructionProfile;
    }
    else
    {
        BaseValue &= (~OF_ToggleInstructionProfile);
    }
}

bool GFxAmpMessageAppControl::IsDebugPause() const
{
    return (BaseValue & OF_DebugPause) != 0;
}

void GFxAmpMessageAppControl::SetDebugPause(bool debug)
{
    if (debug)
    {
        BaseValue |= OF_DebugPause;
    }
    else
    {
        BaseValue &= (~OF_DebugPause);
    }
}

bool GFxAmpMessageAppControl::IsDebugStep() const
{
    return (BaseValue & OF_DebugStep) != 0;
}

void GFxAmpMessageAppControl::SetDebugStep(bool debug)
{
    if (debug)
    {
        BaseValue |= OF_DebugStep;
    }
    else
    {
        BaseValue &= (~OF_DebugStep);
    }
}

bool GFxAmpMessageAppControl::IsDebugStepIn() const
{
    return (BaseValue & OF_DebugStepIn) != 0;
}

void GFxAmpMessageAppControl::SetDebugStepIn(bool debug)
{
    if (debug)
    {
        BaseValue |= OF_DebugStepIn;
    }
    else
    {
        BaseValue &= (~OF_DebugStepIn);
    }
}

bool GFxAmpMessageAppControl::IsDebugStepOut() const
{
    return (BaseValue & OF_DebugStepOut) != 0;
}

void GFxAmpMessageAppControl::SetDebugStepOut(bool debug)
{
    if (debug)
    {
        BaseValue |= OF_DebugStepOut;
    }
    else
    {
        BaseValue &= (~OF_DebugStepOut);
    }
}

bool GFxAmpMessageAppControl::IsDebugNextMovie() const
{
    return (BaseValue & OF_DebugNextMovie) != 0;
}

void GFxAmpMessageAppControl::SetDebugNextMovie(bool debug)
{
    if (debug)
    {
        BaseValue |= OF_DebugNextMovie;
    }
    else
    {
        BaseValue &= (~OF_DebugNextMovie);
    }
}

bool GFxAmpMessageAppControl::IsToggleMemReport() const
{
    return (BaseValue & OF_ToggleMemReport) != 0;

}

void GFxAmpMessageAppControl::SetToggleMemReport(bool toggle)
{
    if (toggle)
    {
        BaseValue |= OF_ToggleMemReport;
    }
    else
    {
        BaseValue &= (~OF_ToggleMemReport);
    }
}

bool GFxAmpMessageAppControl::IsToggleProfileFunctions() const
{
    return (BaseValue & OF_ToggleProfileFunctions) != 0;

}

void GFxAmpMessageAppControl::SetToggleProfileFunctions(bool toggle)
{
    if (toggle)
    {
        BaseValue |= OF_ToggleProfileFunctions;
    }
    else
    {
        BaseValue &= (~OF_ToggleProfileFunctions);
    }
}

SInt32 GFxAmpMessageAppControl::GetProfileLevel() const
{
    return ProfileLevel;
}

void GFxAmpMessageAppControl::SetProfileLevel(SInt32 profileLevel)
{
    ProfileLevel = profileLevel;
}


// Accessor
const GStringLH& GFxAmpMessageAppControl::GetLoadMovieFile() const
{
    return LoadMovieFile;
}

// Accessor
void GFxAmpMessageAppControl::SetLoadMovieFile(const char* pcFileName)
{
    LoadMovieFile = pcFileName;
}

///////////////////////////////////////////////////////////////////////////////////

GFxAmpMessagePort::GFxAmpMessagePort(UInt32 port, const char* appName, const char* fileName) :
    GFxAmpMessageInt(Msg_Port, port)
{
    if (appName != NULL)
    {
        AppName = appName;
    }
    if (fileName != NULL)
    {
        FileName = fileName;
    }
#if defined(GFC_OS_WIN32)
    Platform = PlatformWindows;
#elif defined(GFC_OS_XBOX360)
    Platform = PlatformXbox360;
#elif defined(GFC_OS_XBOX)
    Platform = PlatformXbox;
#elif defined(GFC_OS_PS3)
    Platform = PlatformPs3;
#elif defined(GFC_OS_PS2)
    Platform = PlatformPs2;
#elif defined(GFC_OS_PSP)
    Platform = PlatformPsp;
#elif defined(GFC_OS_WII)
    Platform = PlatformWii;
#elif defined(GFC_OS_GAMECUBE)
    Platform = PlatformGamecube;
#elif defined(GFC_OS_LINUX)
    Platform = PlatformLinux;
#elif defined(GFC_OS_MAC)
    Platform = PlatformMac;
#elif defined(GFC_OS_IPHONE)
    Platform = PlatformIphone;
#elif defined(GFC_OS_QNX)
    Platform = PlatformQnx;
#elif defined(GFC_OS_SYMBIAN)
    Platform = PlatformSymbian;
#else
    Platform = PlatformOther;
#endif
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessagePort::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandlePort(this);
}

// Serialization
void GFxAmpMessagePort::Read(GFxAmpStream& str)
{
    GFxAmpMessageInt::Read(str);
    str.ReadString(&AppName);
    if (Version >= 5)
    {
        Platform = static_cast<PlatformType>(str.ReadUInt32());
        str.ReadString(&FileName);
    }
}

// Serialization
void GFxAmpMessagePort::Write(GFxAmpStream& str) const
{
    GFxAmpMessageInt::Write(str);
    str.WriteString(AppName);
    if (Version >= 5)
    {
        str.WriteUInt32(Platform);
        str.WriteString(FileName);
    }
}

// Accessor
const GStringLH& GFxAmpMessagePort::GetAppName() const
{
    return AppName;
}

// Accessor
const GStringLH& GFxAmpMessagePort::GetFileName() const
{
    return FileName;
}

// Accessor
GFxAmpMessagePort::PlatformType GFxAmpMessagePort::GetPlatform() const
{
    return Platform;
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageImageRequest::GFxAmpMessageImageRequest(UInt32 imageId) : 
    GFxAmpMessageInt(Msg_ImageRequest, imageId)
{
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageImageRequest::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleImageRequest(this);
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageImageData::GFxAmpMessageImageData(UInt32 imageId) : 
    GFxAmpMessageInt(Msg_ImageData, imageId), PngFormat(true)
{
    ImageData = GHEAP_AUTO_NEW(this) GFxAmpStream();
}

// Destructor
GFxAmpMessageImageData::~GFxAmpMessageImageData()
{
    ImageData->Release();
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageImageData::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleImageData(this);
}

// Serialization
void GFxAmpMessageImageData::Read(GFxAmpStream& str)
{
    GFxAmpMessageInt::Read(str);
    str.ReadStream(ImageData);
    if (Version >= 26)
    {
        PngFormat = (str.ReadUByte() != 0);
    }
}

// Serialization
void GFxAmpMessageImageData::Write(GFxAmpStream& str) const
{
    GFxAmpMessageInt::Write(str);
    if (Version >= 26 || PngFormat)
    {
        str.WriteStream(*ImageData);
    }
    else
    {
        str.WriteUInt32(0);
    }
    if (Version >= 26)
    {
        str.WriteUByte(PngFormat ? 1 : 0);
    }
}


// Accessor
GFxAmpStream* GFxAmpMessageImageData::GetImageData() const
{
    return ImageData;
}

void GFxAmpMessageImageData::SetImageData(GFxAmpStream* imageData)
{
    ImageData->Release();
    ImageData = imageData;
    ImageData->AddRef();
}

bool GFxAmpMessageImageData::IsPngFormat() const
{
    return PngFormat;
}

void GFxAmpMessageImageData::SetPngFormat(bool pngFormat)
{
    PngFormat = pngFormat;
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageFontRequest::GFxAmpMessageFontRequest(UInt32 fontId) : 
    GFxAmpMessageInt(Msg_FontRequest, fontId)
{
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageFontRequest::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleFontRequest(this);
}

////////////////////////////////////////////////////////////////////

// Constructor
GFxAmpMessageFontData::GFxAmpMessageFontData(UInt32 fontId) : 
    GFxAmpMessageInt(Msg_FontData, fontId)
{
    ImageData = GHEAP_AUTO_NEW(this) GFxAmpStream();
}

// Destructor
GFxAmpMessageFontData::~GFxAmpMessageFontData()
{
    ImageData->Release();
}

// Virtual method called from the message handler
// Returns true if the client handles this message
bool GFxAmpMessageFontData::AcceptHandler(GFxAmpMsgHandler* handler) const
{
    return handler->HandleFontData(this);
}

// Serialization
void GFxAmpMessageFontData::Read(GFxAmpStream& str)
{
    GFxAmpMessageInt::Read(str);
    str.ReadStream(ImageData);
}

// Serialization
void GFxAmpMessageFontData::Write(GFxAmpStream& str) const
{
    GFxAmpMessageInt::Write(str);
    str.WriteStream(*ImageData);
}


// Accessor
GFxAmpStream* GFxAmpMessageFontData::GetImageData() const
{
    return ImageData;
}

void GFxAmpMessageFontData::SetImageData(GFxAmpStream* imageData)
{
    ImageData->Release();
    ImageData = imageData;
    ImageData->AddRef();
}

/////////////////////////////////////////////////////////////////////

GFxAmpMessageCompressed::GFxAmpMessageCompressed() : GFxAmpMessage(Msg_Compressed)
{
}

GFxAmpMessageCompressed::~GFxAmpMessageCompressed()
{
}

void GFxAmpMessageCompressed::Read(GFxAmpStream& str)
{
    GFxAmpMessage::Read(str);
    UInt32 dataSize = str.ReadUInt32();
    CompressedData.Resize(dataSize);
    for (UInt32 i = 0; i < dataSize; ++i)
    {
        CompressedData[i] = str.ReadUByte();
    }
}

void GFxAmpMessageCompressed::Write(GFxAmpStream& str) const
{
    GFxAmpMessage::Write(str);
    str.WriteUInt32(static_cast<UInt32>(CompressedData.GetSize()));
    for (UInt32 i = 0; i < CompressedData.GetSize(); ++i)
    {
        str.WriteUByte(CompressedData[i]);
    }
}

GFxAmpMessage* GFxAmpMessageCompressed::Uncompress()
{
#ifdef GFC_USE_ZLIB
    z_stream strm;
    strm.zalloc = ZLibAllocFunc;
    strm.zfree = ZLibFreeFunc;
    strm.opaque = (voidpf) this;
    strm.avail_in = static_cast<uInt>(CompressedData.GetSize());
    strm.next_in = &CompressedData[0];
    int ret = inflateInit(&strm);
    GFC_DEBUG_MESSAGE1(ret != Z_OK, "Zlib inflateInit failed with code {0}", ret);
    if (ret != Z_OK)
    {
        return NULL;
    }

    unsigned currentPosition = 0;
    static const unsigned chunkSize = 1024;
    GArray<UByte> uncompressedData;

    do 
    {
        uncompressedData.Resize(currentPosition + chunkSize);
        strm.avail_out = chunkSize;
        strm.next_out = (&uncompressedData[0] + currentPosition);

        ret = inflate(&strm, Z_NO_FLUSH);
        GASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */

        currentPosition += chunkSize - strm.avail_out;

    } while (strm.avail_out == 0);

    uncompressedData.Resize(currentPosition);
    inflateEnd(&strm);

    GPtr<GFxAmpStream> ampStream = *GHEAP_AUTO_NEW(this) GFxAmpStream(&uncompressedData[0], uncompressedData.GetSize());
    return CreateAndReadMessage(*ampStream, GMemory::GetHeapByAddress(this));
#else
    return NULL;
#endif
}

void GFxAmpMessageCompressed::AddCompressedData(UByte* data, UPInt dataSize)
{
    for (UPInt i = 0; i < dataSize; ++i)
    {
        CompressedData.PushBack(data[i]);
    }
}

const GArrayLH<UByte>& GFxAmpMessageCompressed::GetCompressedData() const
{
    return CompressedData;
}


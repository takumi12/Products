/**********************************************************************

Filename    :   GFxAmpViewStats.cpp
Content     :   Performance statistics for a MovieView
Created     :   February, 2010
Authors     :   Alex Mantzaris

Copyright   :   (c) 2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxAmpViewStats.h"
#include "GHeapNew.h"
#include "GFxPlayerImpl.h"
#include "GMsgFormat.h"
#include "GFxAmpProfileFrame.h"
#include "GHeapNew.h"
#include "GFxAmpServer.h"

// Constructor
GFxAmpViewStats::GFxAmpViewStats() : 
    NextTreeItemId(0),
    ViewHandle(0), 
    CurrentFrame(0), 
    Version(0),
    Width(0),
    Height(0),
    FrameRate(0),
    FrameCount(0),
    SkipSamples(0), 
    LastTimer(0)
{
    // Initialize map of native function names
    for (UInt32 i = 0; i < Func_NumNativeFunctionIds; ++i)
    {
        RegisterScriptFunction(NativeCodeSwdHandle, i, GetNativeFunctionName((AmpNativeFunctionId)i), 0);
    }

    // Generate unique handle so that view can be identified by AMP
    GLock::Locker locker(&ViewLock);
    static UInt32 nextHandle = 0;
    ViewHandle = nextHandle;
    ++nextHandle;
}

GFxAmpViewStats::~GFxAmpViewStats()
{
    FunctionRoots.Clear();
}

// Registers a script function so that AMP can refer to it by name
void GFxAmpViewStats::RegisterScriptFunction(UInt32 swdHandle, UInt32 swfOffset, const char* name, 
                                             UInt byteCodeLength)
{
    GASSERT(swdHandle != 0);
    UInt64 iKey = MakeSwdOffsetKey(swdHandle, swfOffset);
    GFxAmpFunctionDescMap::Iterator it = FunctionInfoMap.Find(iKey);
    if (it == FunctionInfoMap.End())
    {
        // Register only if not done so already
        GFxAmpFunctionDesc* pDesc = GHEAP_AUTO_NEW(this) GFxAmpFunctionDesc();
        pDesc->Name = name;
        pDesc->Length = byteCodeLength;
        pDesc->FileId = 0;
        pDesc->FileLine = 0;
        pDesc->ASVersion = 2;  //  AS3 not supported
        //GFC_DEBUG_MESSAGE3(1, "Registering function %s %u %u", name, swdHandle, swfOffset);
        FunctionInfoMap.Set(iKey, *pDesc);
    }
    else
    {
        // Make sure the duplicate registration refers to the same function
        GASSERT(it->Second->Length == byteCodeLength);
    }
}

// Registers a source file so that AMP can refer to it by name
void GFxAmpViewStats::RegisterSourceFile(UInt32 swdHandle, UInt32 index, const char* name)
{
    GASSERT(swdHandle != 0);
    UInt64 iKey = MakeSwdOffsetKey(swdHandle, index);
    SourceFileDescMap::Iterator it = SourceLineInfoMap.Find(iKey);
    if (it == SourceLineInfoMap.End())
    {
        // Register only if not done so already
        SourceLineInfoMap.Set(iKey, name);
#ifdef GFX_AMP_SERVER
        GFxAmpServer::GetInstance().AddSourceFile(iKey, name);
#endif
    }
}

// Pushes a function onto the call stack
// This information is used to determine statistics for each called function within a function
void GFxAmpViewStats::PushCallstack(UInt32 swdHandle, UInt32 swfOffset, UInt64 funcTime)
{
    GLock::Locker locker(&ViewLock);

    // Add a virtual root to user calls
    if (swdHandle == NativeCodeSwdHandle
     && swfOffset > Func_Begin_GFxValue_ObjectInterface 
     && swfOffset < Func_End_GFxValue_ObjectInterface)
    {
        if (Callstack.GetSize() == 0)
        {
            PushCallstack(NativeCodeSwdHandle, Func_Begin_GFxValue_ObjectInterface, funcTime);
        }
    }

    GPtr<FuncTreeItem> newCall = *GHEAP_AUTO_NEW(this) FuncTreeItem();
    newCall->BeginTime = funcTime;
    newCall->FunctionId = MakeSwdOffsetKey(swdHandle, swfOffset);
    newCall->TreeItemId = ++NextTreeItemId;
    Callstack.PushBack(newCall);
}

// Pops a function from the call stack
// Records the time measured for a given function
// Thread-safe because function calls can be made from the renderer thread
void GFxAmpViewStats::PopCallstack(UInt32 swdHandle, UInt32 swfOffset, UInt64 funcTime)
{
    GLock::Locker locker(&ViewLock);

    // Make sure we are popping the function we think we are
    GASSERT(swdHandle != 0);
    GASSERT(MakeSwdOffsetKey(swdHandle, swfOffset) == Callstack.Back()->FunctionId);

    GPtr<FuncTreeItem> poppedCall = Callstack.Back();
    poppedCall->EndTime = poppedCall->BeginTime + funcTime;
    Callstack.PopBack();

    // Record the function time
    ParentChildFunctionPair keyParentChild;
    keyParentChild.FunctionId = MakeSwdOffsetKey(swdHandle, swfOffset);
    if (Callstack.GetSize() == 0)
    {
        // Top-level function
        keyParentChild.CallerId = 0;
    }
    else
    {
        // Record the Caller
        keyParentChild.CallerId = Callstack.Back()->FunctionId;
    }

    if (!IsFunctionAggregation())
    {
        if (Callstack.GetSize() > 0)
        {
            Callstack.Back()->Children.PushBack(poppedCall);
        }
        else
        {
            FunctionRoots.PushBack(poppedCall);
        }
    }
    else
    {
        FunctionStatMap::Iterator it = FunctionTimingMap.Find(keyParentChild);
        if (it == FunctionTimingMap.End())
        {
            // This caller-callee combination has not been recorded before
            AmpFunctionStats stats;
            stats.TimesCalled = 0;
            stats.TotalTime = 0; 
            FunctionTimingMap.Set(keyParentChild, stats);
            it = FunctionTimingMap.Find(keyParentChild);
            GASSERT(it != FunctionTimingMap.End());
        }

        // Update statistics
        AmpFunctionStats& funcStats = it->Second;
        ++funcStats.TimesCalled;

        // Update total time only if the same caller-callee pair 
        // is not already being timed further up in the stack. 
        bool recursion = false;
        for (UPInt i = 1; i < Callstack.GetSize(); ++i)
        {
            if (static_cast<UInt32>(Callstack[i - 1]->FunctionId) == keyParentChild.CallerId 
                && static_cast<UInt32>(Callstack[i]->FunctionId) == keyParentChild.FunctionId)
            {
                recursion = true;
                break;
            }
        }
        if (!recursion)
        {
            funcStats.TotalTime += funcTime;
        }
    }

    // Pop the user virtual root
    if (Callstack.GetSize() == 1)
    {
        if (keyParentChild.CallerId == MakeSwdOffsetKey(NativeCodeSwdHandle, 
                                                    Func_Begin_GFxValue_ObjectInterface))
        {
            PopCallstack(NativeCodeSwdHandle, Func_Begin_GFxValue_ObjectInterface, funcTime);
        }
    }
}

// Locks and returns the array of times for a given ActionScript buffer
// Gives direct access to the array for fast modification
// There is one entry in the array for each byte offset, even though each instruction
// is several bytes long. This is done for fast access.
// Must call ReleaseBufferInstructionTimes when done modifying the buffer times
GArrayLH<UInt64>& GFxAmpViewStats::LockBufferInstructionTimes(UInt32 swdHandle, UInt32 swfBufferOffset, 
                                                              UInt bufferLength)
{
    ViewLock.Lock();

    GASSERT(swdHandle != 0);
    UInt64 key = MakeSwdOffsetKey(swdHandle, swfBufferOffset);
    InstructionTimingMap::Iterator it = InstructionTimingsMap.Find(key);
    if (it == InstructionTimingsMap.End())
    {
        // profilertodo: In the case where the corresponding GASActionBufferData is freed, 
        // free this buffer after it is flushed to the network
        GFxAmpBufferInstructionTimes* pBuffer = 
            GHEAP_AUTO_NEW(this) GFxAmpBufferInstructionTimes(bufferLength);
        InstructionTimingsMap.Set(key, *pBuffer);
        it = InstructionTimingsMap.Find(key);
    }
    GASSERT(bufferLength <= it->Second->Times.GetSize());

    return it->Second->Times;
}

// Unlocks the view stats
// Called to match LockBufferInstructionTimes
void GFxAmpViewStats::ReleaseBufferInstructionTimes()
{
    ViewLock.Unlock();
}

// Updates timing for given file and line
void GFxAmpViewStats::RecordSourceLineTime(UInt64 fileId, UInt32 lineNumber, UInt64 lineTime)
{
    FileLinePair fileLine;
    fileLine.FileId = fileId;
    fileLine.LineNumber = lineNumber;

    SourceLineStatMap::Iterator it = SourceLineTimingsMap.Find(fileLine);
    if (it == SourceLineTimingsMap.End())
    {
        SourceLineTimingsMap.Set(fileLine, 0);
        it = SourceLineTimingsMap.Find(fileLine);
        GASSERT(it != SourceLineTimingsMap.End());
    }

    // Update statistics
    it->Second += lineTime;
}

// static
bool GFxAmpViewStats::IsFunctionAggregation()
{
#ifdef GFX_AMP_SERVER
    return GFxAmpServer::GetInstance().IsFunctionAggregation();
#else
    return false;
#endif
}

const char*  GFxAmpViewStats::GetNativeFunctionName(AmpNativeFunctionId functionId)
{
    switch (functionId)
    {
    case Func_GFxMovieRoot_Display:
        return "GFxMovieRoot::Display";
    case Func_GFxVideoProviderDisplay:
        return "GFxVideoProvider::Display";
    case Func_GFxMeshSet_Display:
        return "GFxMeshSet::Display";
    case Func_GFxTexture9Grid_Display:
        return "GFxTexture9Grid::Display";
    case Func_GFxMesh_Display:
        return "GFxMesh::Display";
    case Func_GFxFontCacheManagerImpl_CreateBatchPackage:
        return "GFxFontCacheManagerImpl::CreateBatchPackage";
    case Func_GFxMovieRoot_Advance:
        return "GFxMovieRoot::Advance";
    case Func_GFxMovieRoot_DoActions:
        return "GFxMovieRoot::DoActions";
    case Func_GFxMovieRoot_ProcessInput:
        return "GFxMovieRoot::ProcessInput";
    case Func_GFxMovieRoot_ProcessKeyboard:
        return "GFxMovieRoot::ProcessKeyboard";
    case Func_GFxMovieRoot_ProcessMouse:
        return "GFxMovieRoot::ProcessMouse";
    case Func_GFxMovieRoot_GetTopMostEntity:
        return "GFxMovieRoot::GetTopMostEntity";
    case Func_GFxMovieRoot_AdvanceFrame:
        return "GFxMovieRoot::AdvanceFrame";
    case Func_GFxShapeBase_TessellateImpl:
        return "GFxShapeBase::TessellateImpl";
    case Func_GFxMovieRoot_Invoke:
        return "GFxMovieRoot::Invoke";
    case Func_GFxMovieRoot_InvokeArgs:
        return "GFxMovieRoot::InvokeArgs";
    case Func_GFxMovieRoot_InvokeAlias:
        return "GFxMovieRoot::InvokeAlias";
    case Func_GFxMovieRoot_InvokeAliasArgs:
        return "GFxMovieRoot::InvokeAliasArgs";
    case Func_GFxMovieRoot_SetVariable:
        return "GFxMovieRoot::SetVariable";
    case Func_GFxMovieRoot_SetVariableArray:
        return "GFxMovieRoot::SetVariableArray";
    case Func_GFxMovieRoot_SetVariableArraySize:
        return "GFxMovieRoot::SetVariableArraySize";
    case Func_GFxMovieRoot_GetVariable:
        return "GFxMovieRoot::GetVariable";
    case Func_GFxMovieRoot_GetVariableArray:
        return "GFxMovieRoot::GetVariableArray";
    case Func_GFxMovieRoot_GetVariableArraySize:
        return "GFxMovieRoot::GetVariableArraySize";
    case Func_GFxMovieView_Invoke:
        return "GFxMovieView::Invoke";
    case Func_GFxMovieView_InvokeArgs:
        return "GFxMovieView::InvokeArgs";
    case Func_GFxFillStyle_GetGradientFillTexture:
        return "GFxFillStyle::GetGradientFillTexture";
    case Func_GFxFillStyle_GetImageFillTexture:
        return "GFxFillStyle::GetImageFillTexture";
    case Func_GFxExternalInterface_Callback:
        return "GFxExternalInterface::Callback";
    case Func_GFxFSCommandHandler_Callback:
        return "GFxFSCommandHandler::Callback";
    case Func_Begin_GFxValue_ObjectInterface:
        return "Direct Access API";
    case Func_GFxValue_ObjectInterface_HasMember:
        return "GFxValue::ObjectInterface::HasMember";
    case Func_GFxValue_ObjectInterface_GetMember:
        return "GFxValue::ObjectInterface::GetMember";
    case Func_GFxValue_ObjectInterface_SetMember:
        return "GFxValue::ObjectInterface::SetMember";
    case Func_GFxValue_ObjectInterface_Invoke:
        return "GFxValue::ObjectInterface::Invoke";
    case Func_GFxValue_ObjectInterface_DeleteMember:
        return "GFxValue::ObjectInterface::DeleteMember";
    case Func_GFxValue_ObjectInterface_VisitMembers:
        return "GFxValue::ObjectInterface::VisitMembers";
    case Func_GFxValue_ObjectInterface_GetArraySize:
        return "GFxValue::ObjectInterface::GetArraySize";
    case Func_GFxValue_ObjectInterface_SetArraySize:
        return "GFxValue::ObjectInterface::SetArraySize";
    case Func_GFxValue_ObjectInterface_GetElement:
        return "GFxValue::ObjectInterface::GetElement";
    case Func_GFxValue_ObjectInterface_SetElement:
        return "GFxValue::ObjectInterface::SetElement";
    case Func_GFxValue_ObjectInterface_VisitElements:
        return "GFxValue::ObjectInterface::VisitElements";
    case Func_GFxValue_ObjectInterface_PushBack:
        return "GFxValue::ObjectInterface::PushBack";
    case Func_GFxValue_ObjectInterface_PopBack:
        return "GFxValue::ObjectInterface::PopBack";
    case Func_GFxValue_ObjectInterface_RemoveElements:
        return "GFxValue::ObjectInterface::RemoveElements";
    case Func_GFxValue_ObjectInterface_GetDisplayMatrix:
        return "GFxValue::ObjectInterface::GetDisplayMatrix";
    case Func_GFxValue_ObjectInterface_SetDisplayMatrix:
        return "GFxValue::ObjectInterface::SetDisplayMatrix";
    case Func_GFxValue_ObjectInterface_GetMatrix3D:
        return "GFxValue::ObjectInterface::GetMatrix3D";
    case Func_GFxValue_ObjectInterface_SetMatrix3D:
        return "GFxValue::ObjectInterface::SetMatrix3D";
    case Func_GFxValue_ObjectInterface_IsDisplayObjectActive:
        return "GFxValue::ObjectInterface::IsDisplayObjectActive";
    case Func_GFxValue_ObjectInterface_GetDisplayInfo:
        return "GFxValue::ObjectInterface::GetDisplayInfo";
    case Func_GFxValue_ObjectInterface_SetDisplayInfo:
        return "GFxValue::ObjectInterface::SetDisplayInfo";
    case Func_GFxValue_ObjectInterface_SetText:
        return "GFxValue::ObjectInterface::SetText";
    case Func_GFxValue_ObjectInterface_GetText:
        return "GFxValue::ObjectInterface::GetText";
    case Func_GFxValue_ObjectInterface_GotoAndPlay:
        return "GFxValue::ObjectInterface::GotoAndPlay";
    case Func_GFxValue_ObjectInterface_GetCxform:
        return "GFxValue::ObjectInterface::GetCxform";
    case Func_GFxValue_ObjectInterface_SetCxform:
        return "GFxValue::ObjectInterface::SetCxform";
    case Func_GFxValue_ObjectInterface_CreateEmptyMovieClip:
        return "GFxValue::ObjectInterface::CreateEmptyMovieClip";
    case Func_GFxValue_ObjectInterface_AttachMovie:
        return "GFxValue::ObjectInterface::AttachMovie";
    case Func_GFxAmpServer_WaitForAmpThread:
        return "GFxAmpServer::WaitForAmpThread";
    default:
        break;
    }
    return "Unknown function";
}

// Obtains statistics for the movie view.
void GFxAmpViewStats::GetStats(GStatBag* bag, bool reset)
{
    if (bag != NULL)
    {
        GPtr<GFxAmpProfileFrame> stats = *GHEAP_AUTO_NEW(this) GFxAmpProfileFrame();
        CollectTimingStats(stats);

        GTimerStat timerStat;
        timerStat.Reset();
        timerStat.AddTicks(stats->AdvanceTime);
        bag->Add(GFxStatMV_Advance_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->TimelineTime);
        bag->Add(GFxStatMV_Timeline_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->ActionTime);
        bag->Add(GFxStatMV_Action_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->InputTime);
        bag->Add(GFxStatMV_Input_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->MouseTime);
        bag->Add(GFxStatMV_Mouse_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->GetVariableTime);
        bag->Add(GFxStatMV_GetVariable_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->SetVariableTime);
        bag->Add(GFxStatMV_SetVariable_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->InvokeTime);
        bag->Add(GFxStatMV_Invoke_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->DisplayTime);
        bag->Add(GFxStatMV_Display_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->TesselationTime);
        bag->Add(GFxStatMV_Tessellate_Tks, &timerStat);
        timerStat.Reset();
        timerStat.AddTicks(stats->GradientGenTime);
        bag->Add(GFxStatMV_GradientGen_Tks, &timerStat);
    }

    if (reset)
    {
        ClearAmpFunctionStats();
    }
}

// Sets the friendly name for the view
void GFxAmpViewStats::SetName(const char* name)
{
    ViewName = name;

    // Remove the path, if any
    UPInt nameLength = ViewName.GetLength();
    for (UPInt i = 0; i < nameLength; ++i)
    {
        UPInt index = nameLength - i - 1;
        if (ViewName[index] == '/' || ViewName[index] == '\\')
        {
            ViewName = ViewName.Substring(index + 1, nameLength);
            break;
        }
    }

}

// Gets the friendly name for the view
const GString& GFxAmpViewStats::GetName() const
{
    return ViewName;
}


// Recovers the unique handle for the view
UInt32 GFxAmpViewStats::GetViewHandle() const 
{ 
    GLock::Locker locker(&ViewLock);
    return ViewHandle; 
}

// Assigns the flash file properties
void GFxAmpViewStats::SetMovieDef(GFxMovieDef* movieDef)
{
    GLock::Locker locker(&ViewLock);
    if (movieDef != NULL)
    {
        Version = movieDef->GetVersion();
        Width = movieDef->GetWidth();
        Height = movieDef->GetHeight();
        FrameRate = movieDef->GetFrameRate();
        FrameCount = movieDef->GetFrameCount();
    }
}

// Flash version
UInt32 GFxAmpViewStats::GetVersion() const
{
    GLock::Locker locker(&ViewLock);
    return Version;
}

// Flash display width
float GFxAmpViewStats::GetWidth() const
{
    GLock::Locker locker(&ViewLock);
    return Width;
}

// Flash display height
float GFxAmpViewStats::GetHeight() const
{
    GLock::Locker locker(&ViewLock);
    return Height;
}

// Flash file frame rate
float GFxAmpViewStats::GetFrameRate() const
{
    GLock::Locker locker(&ViewLock);
    return FrameRate;
}

// Total number of frames in the flash file
UInt32 GFxAmpViewStats::GetFrameCount() const
{
    GLock::Locker locker(&ViewLock);
    return FrameCount;
}

// Returns the current Flash frame for this movie
// Called by AMP
UInt32 GFxAmpViewStats::GetCurrentFrame() const 
{ 
    GLock::Locker lock(&ViewLock);
    return CurrentFrame; 
}

// Sets the current Flash frame for this movie
// Called by GFx
void GFxAmpViewStats::SetCurrentFrame(UInt32 frame)
{
    GLock::Locker lock(&ViewLock);
    CurrentFrame = frame;
}

// Returns a time for instruction sampling
// samplePeriod is the average number of samples skipped
// samplePeriod = 0 means timing every instruction
UInt64 GFxAmpViewStats::GetInstructionTime(UInt32 samplePeriod)
{
    UInt64 timeDiff = 0;

    if (samplePeriod == 0)
    {
        UInt64 nextTimer = GTimer::GetRawTicks();
        timeDiff = nextTimer - LastTimer;
        LastTimer = nextTimer;
    }
    else
    {
        if (LastTimer != 0)
        {
            timeDiff = (GTimer::GetRawTicks() - LastTimer) * samplePeriod;
        }

        if (SkipSamples == 0)
        {
            LastTimer = GTimer::GetRawTicks();
            UInt64 random = RandomGen.NextRandom();
            random *= 2 * samplePeriod;
            random /= 0xFFFFFFFF;
            SkipSamples = static_cast<UInt32>(random);
        }
        else
        {
            LastTimer = 0;
            --SkipSamples;
        }
    }

    return timeDiff;
}

// Adds a marker to be displayed by AMP
void GFxAmpViewStats::AddMarker(const char* markerType)
{
    GStringHashLH<UInt32>::Iterator it = Markers.Find(markerType);

    if (it == Markers.End())
    {
        Markers.Add(markerType, 1);
    }
    else
    {
        ++it->Second;
    }
}

// Collects all the instruction statistics for the frame and 
// puts them into the the ProfileFrame structure for sending to the AMP client.
void GFxAmpViewStats::CollectAmpInstructionStats(GFxMovieStats* movieProfile)
{
    GLock::Locker locker(&ViewLock);

    InstructionTimingMap::Iterator it;
    for (it = InstructionTimingsMap.Begin(); it != InstructionTimingsMap.End(); ++it)
    {
        GPtr<GFxAmpBufferInstructionTimes>& buffer = it->Second;

        // Count non-zero entries so we can size the packed array
        UInt nonZeroTimes = 0;
        for (UPInt i = 0; i < buffer->Times.GetSize(); i++)
        {
            if (buffer->Times[i] != 0)
            {
                ++nonZeroTimes;
            }
        }

        if (nonZeroTimes > 0)
        {
            GFxAmpMovieInstructionStats::ScriptBufferStats* nonZeroValues = GHEAP_AUTO_NEW(movieProfile) 
                GFxAmpMovieInstructionStats::ScriptBufferStats();

            nonZeroValues->SwdHandle = static_cast<UInt32>(it->First >> 32);
            nonZeroValues->BufferOffset = static_cast<UInt32>(it->First);
            nonZeroValues->BufferLength = static_cast<UInt32>(buffer->Times.GetSize());
            nonZeroValues->InstructionTimesArray.Resize(nonZeroTimes);

            UInt nonZeroCount = 0;
            for (UPInt i = 0; i < buffer->Times.GetSize(); i++)
            {
                if (buffer->Times[i] != 0)
                {
                    GFxAmpMovieInstructionStats::InstructionTimePair& pair = 
                        nonZeroValues->InstructionTimesArray[nonZeroCount];
                    pair.Offset = static_cast<UInt32>(i);
                    // convert to microseconds
                    pair.Time = buffer->Times[i] * 1000000 / GTimer::GetRawFrequency(); 
                    ++nonZeroCount;
                }
            }

            movieProfile->InstructionStats->BufferStatsArray.PushBack(*nonZeroValues);
        }
    }
}

// Clears the ActionScript buffer times for recording the next frame
void GFxAmpViewStats::ClearAmpInstructionStats()
{
    GLock::Locker locker(&ViewLock);

    InstructionTimingMap::Iterator it;
    for (it = InstructionTimingsMap.Begin(); it != InstructionTimingsMap.End(); ++it)
    {
        GArrayLH<UInt64>& buffer = it->Second->Times;
        memset(&buffer[0], 0, buffer.GetSize() * sizeof(buffer[0]));
    }
}

// Collects the function statistics for the frame and
// puts them into the the ProfileFrame structure for sending to the AMP client.
void GFxAmpViewStats::CollectAmpFunctionStats(GFxMovieStats* movieProfile)
{
    GLock::Locker locker(&ViewLock);

    // Copy over all the non-zero function timings
    FunctionStatMap::Iterator funcTimeIter;
    for (funcTimeIter = FunctionTimingMap.Begin(); funcTimeIter != FunctionTimingMap.End(); ++funcTimeIter)
    {
        AmpFunctionStats& funcTime = funcTimeIter->Second;
        if (funcTime.TimesCalled > 0 || funcTime.TotalTime > 0)
        {
            GFxAmpMovieFunctionStats::FuncStats funcStats;
            funcStats.FunctionId = funcTimeIter->First.FunctionId;
            funcStats.CallerId = funcTimeIter->First.CallerId;
            funcStats.TimesCalled = funcTime.TimesCalled;
            funcStats.TotalTime = funcTime.TotalTime; // microseconds
            movieProfile->FunctionStats->FunctionTimings.PushBack(funcStats);

            // Send the friendly name along with the function Id
            UInt64 swfHandleAndOffset = funcTimeIter->First.FunctionId;
            GFxAmpFunctionDescMap::Iterator funcNameIter = FunctionInfoMap.Find(swfHandleAndOffset);
            if (funcNameIter != FunctionInfoMap.End())
            {
                GFxAmpFunctionDescMap& descMap = movieProfile->FunctionStats->FunctionInfo;
                if (descMap.Find(swfHandleAndOffset) == descMap.End())
                {
                    descMap.Set(swfHandleAndOffset, funcNameIter->Second);
                }
            }
        }
    }

    GHashSet<UInt64> functionIds;
    if (!IsFunctionAggregation())
    {
        for (UPInt i = 0; i < FunctionRoots.GetSize(); ++i)
        {
            movieProfile->FunctionTreeStats->FunctionRoots.PushBack(FunctionRoots[i]);
            FunctionRoots[i]->GetAllFunctions(&functionIds);
        }
    }

    for (GHashSet<UInt64>::Iterator it = functionIds.Begin(); it != functionIds.End(); ++it)
    {
        GFxAmpFunctionDescMap::Iterator funcNameIter = FunctionInfoMap.Find(*it);
        if (funcNameIter != FunctionInfoMap.End())
        {
            GFxAmpFunctionDescMap& descMap = movieProfile->FunctionTreeStats->FunctionInfo;
            if (descMap.Find(*it) == descMap.End())
            {
                descMap.Set(*it, funcNameIter->Second);
            }
        }
    }
}

// Clears the function times stored in the view for recording the next frame
void GFxAmpViewStats::ClearAmpFunctionStats()
{
    GLock::Locker locker(&ViewLock);

    FunctionTimingMap.Clear();
    FunctionRoots.Clear();
    NextTreeItemId = 0;
}

// Collects the source line statistics for the frame and
// puts them into the the ProfileFrame structure for sending to the AMP client.
void GFxAmpViewStats::CollectAmpSourceLineStats(GFxMovieStats* movieProfile)
{
    GLock::Locker locker(&ViewLock);

    // Copy over all the non-zero function timings
    SourceLineStatMap::Iterator sourceTimeIter;
    for (sourceTimeIter = SourceLineTimingsMap.Begin(); sourceTimeIter != SourceLineTimingsMap.End(); ++sourceTimeIter)
    {
        if (sourceTimeIter->Second > 0)
        {
            MovieSourceLineStats::SourceStats sourceStats;
            sourceStats.FileId = sourceTimeIter->First.FileId;
            sourceStats.LineNumber = sourceTimeIter->First.LineNumber;
            sourceStats.TotalTime = sourceTimeIter->Second * 1000000 / GTimer::GetRawFrequency(); // microseconds
            movieProfile->SourceLineStats->SourceLineTimings.PushBack(sourceStats);


            // Send the friendly name along with the function Id
            SourceFileDescMap::Iterator fileNameIter = SourceLineInfoMap.Find(sourceStats.FileId);
            if (fileNameIter != SourceLineInfoMap.End())
            {
                SourceFileDescMap& descMap = movieProfile->SourceLineStats->SourceFileInfo;
                if (descMap.Find(sourceStats.FileId) == descMap.End())
                {
                    descMap.Set(sourceStats.FileId, fileNameIter->Second);
                }
            }
        }
    }
}

// Clears the source line times stored in the view for recording the next frame
void GFxAmpViewStats::ClearAmpSourceLineStats()
{
    GLock::Locker locker(&ViewLock);

    SourceLineStatMap::Iterator it;
    for (it = SourceLineTimingsMap.Begin(); it != SourceLineTimingsMap.End(); ++it)
    {
        it->Second = 0;
    }
}

class FuncStatsVisitor
{
public:
    FuncStatsVisitor(GFxAmpProfileFrame* profileFrame, GFxAmpViewStats* callingView) : FrameInfo(profileFrame), CallingView(callingView) { }
    void operator()(const FuncTreeItem* treeItem)
    {
        CallingView->UpdateStats(treeItem->FunctionId, static_cast<UInt32>(treeItem->EndTime - treeItem->BeginTime), FrameInfo);
    }
private:
    GFxAmpProfileFrame* FrameInfo;
    GFxAmpViewStats*    CallingView;
};


// Collects the performance statistics for the view and
// puts them into the the ProfileFrame structure for sending to the AMP client.
void GFxAmpViewStats::CollectTimingStats(GFxAmpProfileFrame* pFrameInfo)
{
    GLock::Locker locker(&ViewLock);

    FuncStatsVisitor visitor(pFrameInfo, this);
    for (UPInt i = 0; i < FunctionRoots.GetSize(); ++i)
    {
        const FuncTreeItem* rootItem = FunctionRoots[i];
        rootItem->Visit(visitor);
    }

    for (FunctionStatMap::Iterator it = FunctionTimingMap.Begin(); it != FunctionTimingMap.End(); ++it)
    {
        UpdateStats(it->First.FunctionId, static_cast<UInt32>(it->Second.TotalTime), pFrameInfo);
    }
}

// Collects the markers for the view and
// puts them into the the ProfileFrame structure for sending to the AMP client.
void GFxAmpViewStats::CollectMarkers(GFxMovieStats* movieProfile)
{
    GLock::Locker locker(&ViewLock);
    for (GStringHashLH<UInt32>::Iterator it = Markers.Begin(); it != Markers.End(); ++it)
    {
        GPtr<GFxMovieStats::MarkerInfo> info = *GHEAP_AUTO_NEW(movieProfile) GFxMovieStats::MarkerInfo();
        info->Name = it->First;
        info->Number = it->Second;
        movieProfile->Markers.PushBack(info);
    }
}

// Clear all markers
void GFxAmpViewStats::ClearMarkers()
{
    GLock::Locker locker(&ViewLock);
    Markers.Clear();
}

void GFxAmpViewStats::UpdateStats(UInt64 functionId, UInt32 functionTime, GFxAmpProfileFrame* frameProfile)
{
    UInt32 funcOffset = static_cast<UInt32>(functionId);
    UInt32 funcSwdHandle = static_cast<UInt32>(functionId >> 32);
    if (funcSwdHandle == NativeCodeSwdHandle)
    {
        switch (funcOffset)
        {
        case Func_GFxMovieRoot_Advance:
            frameProfile->AdvanceTime += functionTime;
            break;
        case Func_GFxMovieRoot_AdvanceFrame:
            frameProfile->TimelineTime += functionTime;
            break;
        case Func_GFxMovieRoot_DoActions:
            frameProfile->ActionTime += functionTime;
            break;
        case Func_GFxMovieRoot_ProcessInput:
            frameProfile->InputTime += functionTime;
            break;
        case Func_GFxMovieRoot_ProcessMouse:
            frameProfile->MouseTime += functionTime;
            break;
        case Func_GFxMovieRoot_GetVariable:
        case Func_GFxMovieRoot_GetVariableArray:
        case Func_GFxMovieRoot_GetVariableArraySize:
        case Func_GFxValue_ObjectInterface_GetMember:
        case Func_GFxValue_ObjectInterface_GetArraySize:
        case Func_GFxValue_ObjectInterface_GetElement:
        case Func_GFxValue_ObjectInterface_GetText:
            frameProfile->GetVariableTime += functionTime;
            break;
        case Func_GFxMovieRoot_SetVariable:
        case Func_GFxMovieRoot_SetVariableArray:
        case Func_GFxMovieRoot_SetVariableArraySize:
        case Func_GFxValue_ObjectInterface_SetMember:
        case Func_GFxValue_ObjectInterface_SetArraySize:
        case Func_GFxValue_ObjectInterface_SetElement:
        case Func_GFxValue_ObjectInterface_SetText:
            frameProfile->SetVariableTime += functionTime;
            break;
        case Func_GFxMovieRoot_Invoke:
        case Func_GFxMovieRoot_InvokeAlias:
        case Func_GFxMovieRoot_InvokeArgs:
        case Func_GFxMovieRoot_InvokeAliasArgs:
        case Func_GFxValue_ObjectInterface_Invoke:
            frameProfile->InvokeTime += functionTime;
            break;
        case Func_GFxMovieRoot_Display:
            frameProfile->DisplayTime += functionTime;
            break;
        case Func_GFxShapeBase_TessellateImpl:
            frameProfile->TesselationTime += functionTime;
            break;
        case Func_GFxFillStyle_GetGradientFillTexture:
            frameProfile->GradientGenTime += functionTime;
            break;
        case Func_Begin_GFxValue_ObjectInterface:
            frameProfile->UserTime += functionTime;
            break;
        }
    }
}


ScopeFunctionTimer::ScopeFunctionTimer(GFxAmpViewStats* viewStats, UInt32 swdHandle, UInt pc, AmpProfileLevel profileLevel) : 
    Stats(viewStats), 
    SwdHandle(swdHandle), 
    PC(pc)
{ 
#ifdef GFX_AMP_SERVER
    if (GFxAmpServer::GetInstance().GetProfileLevel() < profileLevel)
    {
        Stats = NULL;
    }
#else
    GUNUSED(profileLevel);
#endif
    if (Stats != NULL)
    {
        StartTicks = GTimer::GetProfileTicks();
        Stats->PushCallstack(swdHandle, pc, StartTicks);
    }
    else
    {
        StartTicks = 0;
    }
}

ScopeFunctionTimer::~ScopeFunctionTimer()
{
    if (Stats != NULL)
    {
        Stats->PopCallstack(SwdHandle, PC, GTimer::GetProfileTicks() - StartTicks);
    }
}


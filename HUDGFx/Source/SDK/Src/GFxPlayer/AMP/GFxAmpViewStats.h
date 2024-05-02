/**********************************************************************

Filename    :   GFxAmpViewStats.h
Content     :   Performance statistics for a MovieView
Created     :   February, 2010
Authors     :   Alex Mantzaris

Copyright   :   (c) 2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFX_AMP_SCRIPT_PROFILE_STATS_H
#define GFX_AMP_SCRIPT_PROFILE_STATS_H

#include "GRefCount.h"
#include "GString.h"
#include "GStringHash.h"
#include "GFxRandom.h"
#include "GFxAmpProfileFrame.h"

class GFxMovieRoot;
struct GFxAmpMovieInstructionStats;
struct GFxAmpMovieFunctionStats;
class GFxAmpProfileFrame;
struct GFxMovieStats;
class GFxMovieDef;

// For mixing ActionScript and C++ call stack 
enum AmpHandleType
{ 
    NativeCodeSwdHandle = 1,
};

enum AmpNativeFunctionId
{
    Func_GFxMovieRoot_Display = 0,
    Func_GFxVideoProviderDisplay,
    Func_GFxMeshSet_Display,
    Func_GFxTexture9Grid_Display,
    Func_GFxMesh_Display,
    Func_GFxFontCacheManagerImpl_CreateBatchPackage,
    Func_GFxMovieRoot_Advance,
    Func_GFxMovieRoot_DoActions,
    Func_GFxMovieRoot_ProcessInput,
    Func_GFxMovieRoot_ProcessKeyboard,
    Func_GFxMovieRoot_ProcessMouse,
    Func_GFxMovieRoot_GetTopMostEntity,
    Func_GFxMovieRoot_AdvanceFrame,
    Func_GFxShapeBase_TessellateImpl,
    Func_GFxMovieRoot_HitTest,
    Func_GFxMovieRoot_DisplayPrePass,
    Func_GFxMovieView_Invoke,
    Func_GFxMovieView_InvokeArgs,
    Func_GFxFillStyle_GetGradientFillTexture,
    Func_GFxFillStyle_GetImageFillTexture,
    Func_GFxFSCommandHandler_Callback,
    Func_GFxExternalInterface_Callback,
    Func_Begin_GFxValue_ObjectInterface,
    Func_GFxMovieRoot_Invoke,
    Func_GFxMovieRoot_InvokeArgs,
    Func_GFxMovieRoot_InvokeAlias,
    Func_GFxMovieRoot_InvokeAliasArgs,
    Func_GFxMovieRoot_SetVariable,
    Func_GFxMovieRoot_SetVariableArray,
    Func_GFxMovieRoot_SetVariableArraySize,
    Func_GFxMovieRoot_GetVariable,
    Func_GFxMovieRoot_GetVariableArray,
    Func_GFxMovieRoot_GetVariableArraySize,
    Func_GFxValue_ObjectInterface_HasMember,
    Func_GFxValue_ObjectInterface_GetMember,
    Func_GFxValue_ObjectInterface_SetMember,
    Func_GFxValue_ObjectInterface_Invoke,
    Func_GFxValue_ObjectInterface_DeleteMember,
    Func_GFxValue_ObjectInterface_VisitMembers,
    Func_GFxValue_ObjectInterface_GetArraySize,
    Func_GFxValue_ObjectInterface_SetArraySize,
    Func_GFxValue_ObjectInterface_GetElement,
    Func_GFxValue_ObjectInterface_SetElement,
    Func_GFxValue_ObjectInterface_VisitElements,
    Func_GFxValue_ObjectInterface_PushBack,
    Func_GFxValue_ObjectInterface_PopBack,
    Func_GFxValue_ObjectInterface_RemoveElements,
    Func_GFxValue_ObjectInterface_GetDisplayMatrix,
    Func_GFxValue_ObjectInterface_SetDisplayMatrix,
    Func_GFxValue_ObjectInterface_GetMatrix3D,
    Func_GFxValue_ObjectInterface_SetMatrix3D,
    Func_GFxValue_ObjectInterface_IsDisplayObjectActive,
    Func_GFxValue_ObjectInterface_GetDisplayInfo,
    Func_GFxValue_ObjectInterface_SetDisplayInfo,
    Func_GFxValue_ObjectInterface_SetText,
    Func_GFxValue_ObjectInterface_GetText,
    Func_GFxValue_ObjectInterface_GotoAndPlay,
    Func_GFxValue_ObjectInterface_GetCxform,
    Func_GFxValue_ObjectInterface_SetCxform,
    Func_GFxValue_ObjectInterface_CreateEmptyMovieClip,
    Func_GFxValue_ObjectInterface_AttachMovie,
    Func_End_GFxValue_ObjectInterface,
    Func_GFxAmpServer_WaitForAmpThread,

    Func_NumNativeFunctionIds,
};

// AMP function profile levels
enum AmpProfileLevel
{
    Amp_Profile_Level_Null = -1,
    Amp_Profile_Level_Low,
    Amp_Profile_Level_Medium,
    Amp_Profile_Level_High,
};


class GFxAmpViewStats : public GRefCountBase<GFxAmpViewStats, GStat_Default_Mem>
{
public:

    // Struct for holding function execution metrics
    struct AmpFunctionStats
    {
		UInt32 TimesCalled;
		UInt64 TotalTime;    // Microseconds
    };

    // This struct holds timing information for one GASActionBuffer
    // The times are stored in an array of size equal to the size of the buffer for fast access
    // Many entries will be zero, as instructions are typically several bytes long
    struct GFxAmpBufferInstructionTimes : public GRefCountBase<GFxAmpBufferInstructionTimes, GStat_Default_Mem>
    {
        GFxAmpBufferInstructionTimes(UInt32 size) : Times(size)
        {
            memset(&Times[0], 0, size * sizeof(Times[0])); // 
        }
        GArrayLH<UInt64> Times;
    };

    // Function executions are tracked per function called for each caller
    // This means a timings map will have a caller-callee pair as a key
    // ParentChildFunctionPair is this key
    struct ParentChildFunctionPair
    {
        UInt64 CallerId;    // SWF handle and byte offset
        UInt64 FunctionId;  // SWF handle and byte offset
        bool operator==(const ParentChildFunctionPair& kRhs) const 
        { 
            return (CallerId == kRhs.CallerId && FunctionId == kRhs.FunctionId); 
        }
    };
    typedef GHashLH<ParentChildFunctionPair, AmpFunctionStats> FunctionStatMap;

    // Struct for holding per-line timings
    struct FileLinePair
    {
        UInt64 FileId;
        UInt32 LineNumber;
        bool operator==(const FileLinePair& rhs) const
        {
            return (FileId == rhs.FileId && LineNumber == rhs.LineNumber);
        }
    };
    typedef GHashLH<FileLinePair, UInt64> SourceLineStatMap;

    // Constructor
    GFxAmpViewStats();
    ~GFxAmpViewStats();

    // Methods for ActionScript profiling
    void                RegisterScriptFunction(UInt32 swdHandle, UInt32 swfOffset, const char* name, UInt length);
    void                RegisterSourceFile(UInt32 swdHandle, UInt32 index, const char* name);
    void                PushCallstack(UInt32 swdHandle, UInt32 swfOffset, UInt64 funcTime);
    void                PopCallstack(UInt32 swdHandle, UInt32 swfOffset, UInt64 funcTime);
    GArrayLH<UInt64>&   LockBufferInstructionTimes(UInt32 swdHandle, UInt32 swfBufferOffset, UInt length);
    void                ReleaseBufferInstructionTimes();
    void                RecordSourceLineTime(UInt64 fileId, UInt32 lineNumber, UInt64 lineTime);
    static const char*  GetNativeFunctionName(AmpNativeFunctionId functionId);
    static bool         IsFunctionAggregation();
    
    // Methods for AMP stats collecting
    void                CollectAmpInstructionStats(GFxMovieStats* movieProfile);
    void                ClearAmpInstructionStats();
    void                CollectAmpFunctionStats(GFxMovieStats* movieProfile);
    void                ClearAmpFunctionStats();
    void                CollectAmpSourceLineStats(GFxMovieStats* movieProfile);
    void                ClearAmpSourceLineStats();
    void                CollectTimingStats(GFxAmpProfileFrame* frameProfile);
    void                CollectMarkers(GFxMovieStats* movieProfile);
    void                ClearMarkers();
    void                CollectMemoryStats(GFxAmpProfileFrame* frameProfile);
    void                GetStats(GStatBag* bag, bool reset);

    // View information
    void            SetName(const char* pcName);
    const GString&  GetName() const;
    UInt32          GetViewHandle() const;
    void            SetMovieDef(GFxMovieDef* movieDef);
    UInt32          GetVersion() const;
    float           GetWidth() const;
    float           GetHeight() const;
    float           GetFrameRate() const;
    UInt32          GetFrameCount() const;

    // Flash frame for this view
    UInt32      GetCurrentFrame() const;
    void        SetCurrentFrame(UInt32 frame);

    // For instruction profiling with random sampling
    UInt64      GetInstructionTime(UInt32 samplePeriod);

    // Frame markers
    void        AddMarker(const char* markerType);

    void        UpdateStats(UInt64 functionId, UInt32 functionTime, GFxAmpProfileFrame* frameProfile);

private:

    // ActionScript function profiling
    FunctionStatMap                                             FunctionTimingMap;
    GFxAmpFunctionDescMap                                       FunctionInfoMap;
    typedef GArrayConstPolicy<0, 4, true> NeverShrinkPolicy;
    GArrayLH<GPtr<FuncTreeItem>, GStat_Default_Mem, NeverShrinkPolicy> Callstack;
    GArrayLH< GPtr<FuncTreeItem> >                              FunctionRoots;
    UInt32                                                      NextTreeItemId;

    // ActionScript instruction timing
    typedef GHashLH<UInt64, GPtr<GFxAmpBufferInstructionTimes> >    InstructionTimingMap;
    InstructionTimingMap                                            InstructionTimingsMap;

    // ActionScript source line timing
    SourceLineStatMap                                           SourceLineTimingsMap;
    SourceFileDescMap                                           SourceLineInfoMap;

    // View information
    mutable GLock   ViewLock;
    UInt32          ViewHandle;   // Unique GFxMovieRoot ID so that AMP can keep track of each view
    GStringLH       ViewName;  
    UInt            CurrentFrame;
    UInt32          Version;
    float           Width;
    float           Height;
    float           FrameRate;
    UInt32          FrameCount;

    // Instruction sampling
    GFxRandom::Generator    RandomGen;
    UInt32                  SkipSamples;
    UInt64                  LastTimer;

    // Markers
    GStringHashLH<UInt32>   Markers;

    // Helper method for key generation
    static UInt64 MakeSwdOffsetKey(UInt32 swdHandle, UInt32 iOffset)
    {
        UInt64 iKey = swdHandle;
        iKey <<= 32;
        iKey += iOffset;
        return iKey;
    }
};

// This class keeps track of function execution time and call stack
// Time starts counting in constructor and stops in destructor
// Updates the view stats object with the results
class ScopeFunctionTimer
{
public:
    ScopeFunctionTimer(GFxAmpViewStats* viewStats, UInt32 swdHandle, UInt pc, AmpProfileLevel profileLevel);
    ~ScopeFunctionTimer();
    UInt64                GetStartTicks() const   { return StartTicks; }
private:
    UInt64              StartTicks;
    GFxAmpViewStats*    Stats;
    UInt32              SwdHandle;
    UInt32              PC;
};

#endif

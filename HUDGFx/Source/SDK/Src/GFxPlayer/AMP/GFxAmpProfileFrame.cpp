#include "GFxAmpProfileFrame.h"
#include "GFxAmpMessage.h"
#include "GFile.h"
#include "GHeapNew.h"
#include "GMsgFormat.h"

// utility function
void readString(GFile& file, GString* str)
{
    UInt32 stringLength = file.ReadUInt32();
    for (UInt32 i = 0; i < stringLength; ++i)
    {
        str->AppendChar(file.ReadSByte());
    }
}

// utility function
void writeString(GFile& file, const GString& str)
{
    file.WriteUInt32(static_cast<UInt32>(str.GetLength()));
    for (UPInt i = 0; i < str.GetLength(); ++i)
    {
        file.WriteSByte(str[i]);
    }
}


// Constructor
GFxAmpProfileFrame::GFxAmpProfileFrame() :
    TimeStamp(0),
    FramesPerSecond(0),
    AdvanceTime(0),
    ActionTime(0),
    TimelineTime(0),
    InputTime(0),
    MouseTime(0),
    GetVariableTime(0),
    SetVariableTime(0),
    InvokeTime(0),
    DisplayTime(0),
    TesselationTime(0),
    GradientGenTime(0),
    UserTime(0),
    LineCount(0),
    MaskCount(0),
    FilterCount(0),
    MeshCount(0),
    TriangleCount(0),
    DrawPrimitiveCount(0),
    StrokeCount(0),
    GradientFillCount(0),
    MeshThrashing(0),
    RasterizedGlyphCount(0),
    FontTextureCount(0),
    NumFontCacheTextureUpdates(0),
    FontThrashing(0),
    FontFill(0),
    FontFail(0),
    FontMisses(0),
    TotalMemory(0),
    ImageMemory(0),
    MovieDataMemory(0),
    MovieViewMemory(0),
    MeshCacheMemory(0),
    FontCacheMemory(0),
    VideoMemory(0),
    SoundMemory(0),
    OtherMemory(0)
{
    MemoryByStatId = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(0);
    Images = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(0);
    Fonts = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(0);
    MemFragReport = *GHEAP_AUTO_NEW(this) GFxAmpMemFragReport();
    DisplayStats = *GHEAP_AUTO_NEW(this) GFxAmpMovieFunctionStats();
    DisplayFunctionStats = *GHEAP_AUTO_NEW(this) GFxAmpFunctionTreeStats();
}


GFxAmpProfileFrame& GFxAmpProfileFrame::operator+=(const GFxAmpProfileFrame& rhs)
{
    FramesPerSecond += rhs.FramesPerSecond;
    AdvanceTime += rhs.AdvanceTime;
    TimelineTime += rhs.TimelineTime;
    ActionTime += rhs.ActionTime;
    InputTime += rhs.InputTime;
    MouseTime += rhs.MouseTime;
    GetVariableTime += rhs.GetVariableTime;
    SetVariableTime += rhs.SetVariableTime;
    InvokeTime += rhs.InvokeTime;
    DisplayTime += rhs.DisplayTime;
    TesselationTime += rhs.TesselationTime;
    GradientGenTime += rhs.GradientGenTime;
    UserTime += rhs.UserTime;
    LineCount += rhs.LineCount;
    MaskCount += rhs.MaskCount;
    FilterCount += rhs.FilterCount;
    MeshCount += rhs.MeshCount;
    TriangleCount += rhs.TriangleCount;
    DrawPrimitiveCount += rhs.DrawPrimitiveCount;
    StrokeCount += rhs.StrokeCount;
    GradientFillCount += rhs.GradientFillCount;
    MeshThrashing += rhs.MeshThrashing;
    RasterizedGlyphCount += rhs.RasterizedGlyphCount;
    FontTextureCount += rhs.FontTextureCount;
    NumFontCacheTextureUpdates += rhs.NumFontCacheTextureUpdates;
    FontThrashing += rhs.FontThrashing;
    FontFill += rhs.FontFill;
    FontFail += rhs.FontFail;
    FontMisses += rhs.FontMisses;
    TotalMemory += rhs.TotalMemory;
    ImageMemory += rhs.ImageMemory;
    MovieDataMemory += rhs.MovieDataMemory;
    MovieViewMemory += rhs.MovieViewMemory;
    MeshCacheMemory += rhs.MeshCacheMemory;
    FontCacheMemory += rhs.FontCacheMemory;
    VideoMemory += rhs.VideoMemory;
    SoundMemory += rhs.SoundMemory;
    OtherMemory += rhs.OtherMemory;

    for (UPInt i = 0; i < rhs.MovieStats.GetSize(); ++i)
    {
        bool found = false;
        for (UPInt j = 0; j < MovieStats.GetSize(); ++j)
        {
            if (MovieStats[j]->ViewHandle == rhs.MovieStats[i]->ViewHandle)
            {
                MovieStats[j]->Merge(*rhs.MovieStats[i]);
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            GFxMovieStats* newStats = GHEAP_AUTO_NEW(this) GFxMovieStats();
            *newStats = *rhs.MovieStats[i];
            MovieStats.PushBack(*newStats);
        }
    }
    DisplayStats->Merge(*rhs.DisplayStats);
    DisplayFunctionStats->Merge(*rhs.DisplayFunctionStats);

    for (UPInt i = 0; i < rhs.SwdHandles.GetSize(); ++i)
    {
        bool found = false;
        for (UPInt j = 0; j < SwdHandles.GetSize(); ++j)
        {
            if (SwdHandles[j] == rhs.SwdHandles[i])
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            SwdHandles.PushBack(rhs.SwdHandles[i]);
        }
    }

    for (UPInt i = 0; i < rhs.FileHandles.GetSize(); ++i)
    {
        bool found = false;
        for (UPInt j = 0; j < FileHandles.GetSize(); ++j)
        {
            if (FileHandles[j] == rhs.FileHandles[i])
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            FileHandles.PushBack(rhs.FileHandles[i]);
        }
    }

    MemoryByStatId->Merge(*rhs.MemoryByStatId);
    Images->Merge(*rhs.Images);
    Fonts->Merge(*rhs.Fonts);

    for (UPInt i = 0; i < rhs.ImageList.GetSize(); ++i)
    {
        bool found = false;
        for (UPInt j = 0; j < ImageList.GetSize(); ++j)
        {
            if (ImageList[j]->Id == rhs.ImageList[i]->Id)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            ImageList.PushBack(rhs.ImageList[i]);
        }
    }

    // Fragmentation over multiple frames not supported
    MemFragReport->MemorySegments.Clear();
    MemFragReport->HeapInformation.Clear();

    return *this;
}

GFxAmpProfileFrame& GFxAmpProfileFrame::operator/=(UInt numFrames)
{
    FramesPerSecond /= numFrames;
    AdvanceTime /= numFrames;
    TimelineTime /= numFrames;
    ActionTime /= numFrames;
    InputTime /= numFrames;
    MouseTime /= numFrames;
    GetVariableTime /= numFrames;
    SetVariableTime /= numFrames;
    InvokeTime /= numFrames;
    DisplayTime /= numFrames;
    TesselationTime /= numFrames;
    GradientGenTime /= numFrames;
    UserTime /= numFrames;
    LineCount /= numFrames;
    MaskCount /= numFrames;
    FilterCount /= numFrames;
    MeshCount /= numFrames;
    TriangleCount /= numFrames;
    DrawPrimitiveCount /= numFrames;
    StrokeCount /= numFrames;
    GradientFillCount /= numFrames;
    MeshThrashing /= numFrames;
    RasterizedGlyphCount /= numFrames;
    FontTextureCount /= numFrames;
    NumFontCacheTextureUpdates /= numFrames;
    FontThrashing /= numFrames;
    FontFill /= numFrames;
    FontFail /= numFrames;
    FontMisses /= numFrames;
    TotalMemory /= numFrames;
    ImageMemory /= numFrames;
    MovieDataMemory /= numFrames;
    MovieViewMemory /= numFrames;
    MeshCacheMemory /= numFrames;
    FontCacheMemory /= numFrames;
    VideoMemory /= numFrames;
    SoundMemory /= numFrames;
    OtherMemory /= numFrames;

    for (UPInt i = 0; i < MovieStats.GetSize(); ++i)
    {
        *MovieStats[i] /= numFrames;
    }
    *DisplayStats /= numFrames;

    *MemoryByStatId /= numFrames;
    *Images /= numFrames;
    *Fonts /= numFrames;

    // MemFragReport not affected

    return *this;
}


GFxAmpProfileFrame& GFxAmpProfileFrame::operator*=(UInt num)
{
    FramesPerSecond *= num;
    AdvanceTime *= num;
    TimelineTime *= num;
    ActionTime *= num;
    InputTime *= num;
    MouseTime *= num;
    GetVariableTime *= num;
    SetVariableTime *= num;
    InvokeTime *= num;
    DisplayTime *= num;
    TesselationTime *= num;
    GradientGenTime *= num;
    UserTime *= num;
    LineCount *= num;
    MaskCount *= num;
    FilterCount *= num;
    MeshCount *= num;
    TriangleCount *= num;
    DrawPrimitiveCount *= num;
    StrokeCount *= num;
    GradientFillCount *= num;
    MeshThrashing *= num;
    RasterizedGlyphCount *= num;
    FontTextureCount *= num;
    NumFontCacheTextureUpdates *= num;
    FontThrashing *= num;
    FontFill *= num;
    FontFail *= num;
    FontMisses *= num;
    TotalMemory *= num;
    ImageMemory *= num;
    MovieDataMemory *= num;
    MovieViewMemory *= num;
    MeshCacheMemory *= num;
    FontCacheMemory *= num;
    VideoMemory *= num;
    SoundMemory *= num;
    OtherMemory *= num;

    for (UPInt i = 0; i < MovieStats.GetSize(); ++i)
    {
        *MovieStats[i] *= num;
    }
    *DisplayStats *= num;

    *MemoryByStatId *= num;
    *Images *= num;
    *Fonts *= num;

    // MemFragReport not affected

    return *this;
}


// Serialization
void GFxAmpProfileFrame::Read(GFile& str, UInt32 version)
{
    TimeStamp = str.ReadUInt64();
    FramesPerSecond = str.ReadUInt32();
    AdvanceTime = str.ReadUInt32();
    TimelineTime = str.ReadUInt32();
    ActionTime = str.ReadUInt32();
    if (version < 21)
    {
        str.ReadUInt32();
    }
    InputTime = str.ReadUInt32();
    MouseTime = str.ReadUInt32();
    GetVariableTime = str.ReadUInt32();
    SetVariableTime = str.ReadUInt32();
    InvokeTime = str.ReadUInt32();
    DisplayTime = str.ReadUInt32();
    TesselationTime = str.ReadUInt32();
    GradientGenTime = str.ReadUInt32();
    UserTime = str.ReadUInt32();
    LineCount = str.ReadUInt32();
    MaskCount = str.ReadUInt32();
    FilterCount = str.ReadUInt32();
    if (version >= 16)
    {
        MeshCount = str.ReadUInt32();
    }
    TriangleCount = str.ReadUInt32();
    DrawPrimitiveCount = str.ReadUInt32();
    StrokeCount = str.ReadUInt32();
    GradientFillCount = str.ReadUInt32();
    MeshThrashing = str.ReadUInt32();
    RasterizedGlyphCount = str.ReadUInt32();
    FontTextureCount = str.ReadUInt32();
    NumFontCacheTextureUpdates = str.ReadUInt32();
    if (version >= 14)
    {
        FontThrashing = str.ReadUInt32();
        FontFill = str.ReadUInt32();
        FontFail = str.ReadUInt32();
        if (version >= 24)
        {
            FontMisses = str.ReadUInt32();
        }
    }
    TotalMemory = str.ReadUInt32();
    ImageMemory = str.ReadUInt32();
    MovieDataMemory = str.ReadUInt32();
    MovieViewMemory = str.ReadUInt32();
    MeshCacheMemory = str.ReadUInt32();
    FontCacheMemory = str.ReadUInt32();
    VideoMemory = str.ReadUInt32();
    SoundMemory = str.ReadUInt32();
    OtherMemory = str.ReadUInt32();
    MovieStats.Resize(str.ReadUInt32());
    for (UPInt i = 0; i < MovieStats.GetSize(); ++i)
    {
        MovieStats[i] = *GHEAP_AUTO_NEW(this) GFxMovieStats();
        MovieStats[i]->Read(str, version);
    }
    if (version >= 15)
    {
        DisplayStats->Read(str, version);
    }
    if (version >= 25)
    {
        DisplayFunctionStats->Read(str, version);
    }

    SwdHandles.Resize(str.ReadUInt32());
    for (UPInt i = 0; i < SwdHandles.GetSize(); ++i)
    {
        SwdHandles[i] = str.ReadUInt32();
    }
    if (version >= 9)
    {
        FileHandles.Resize(str.ReadUInt32());
        for (UPInt i = 0; i < FileHandles.GetSize(); ++i)
        {
            FileHandles[i] = str.ReadUInt64();
        }
    }

    MemoryByStatId->Read(str, version);
    if (version <= 18)
    {
        GPtr<GFxAmpMemItem> tmp = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(0);
        tmp->Read(str, version);
    }
    if (version >= 3)
    {
        Images->Read(str, version);
    }
    if (version >= 7)
    {
        Fonts->Read(str, version);
    }

    if (version >= 17)
    {
        ImageList.Resize(str.ReadUInt32());
        for (UInt32 i = 0; i < ImageList.GetSize(); ++i)
        {
            ImageList[i] = *GHEAP_AUTO_NEW(this) ImageInfo();
            ImageList[i]->Read(str, version);
        }
    }

    if (version < 8)
    {
        MemFragReport->Read(str, version);
    }
}

// Serialization
void GFxAmpProfileFrame::Write(GFile& str, UInt32 version) const
{
    str.WriteUInt64(TimeStamp);
    str.WriteUInt32(FramesPerSecond);
    str.WriteUInt32(AdvanceTime);
    str.WriteUInt32(TimelineTime);
    str.WriteUInt32(ActionTime);
    if (version < 21)
    {
        str.WriteUInt32(0);
    }
    str.WriteUInt32(InputTime);
    str.WriteUInt32(MouseTime);
    str.WriteUInt32(GetVariableTime);
    str.WriteUInt32(SetVariableTime);
    str.WriteUInt32(InvokeTime);
    str.WriteUInt32(DisplayTime);
    str.WriteUInt32(TesselationTime);
    str.WriteUInt32(GradientGenTime);
    str.WriteUInt32(UserTime);
    str.WriteUInt32(LineCount);
    str.WriteUInt32(MaskCount);
    str.WriteUInt32(FilterCount);
    if (version >= 16)
    {
        str.WriteUInt32(MeshCount);
    }
    str.WriteUInt32(TriangleCount);
    str.WriteUInt32(DrawPrimitiveCount);
    str.WriteUInt32(StrokeCount);
    str.WriteUInt32(GradientFillCount);
    str.WriteUInt32(MeshThrashing);
    str.WriteUInt32(RasterizedGlyphCount);
    str.WriteUInt32(FontTextureCount);
    str.WriteUInt32(NumFontCacheTextureUpdates);
    if (version >= 14)
    {
        str.WriteUInt32(FontThrashing);
        str.WriteUInt32(FontFill);
        str.WriteUInt32(FontFail);
        if (version >= 24)
        {
            str.WriteUInt32(FontMisses);
        }
    }
    str.WriteUInt32(TotalMemory);
    str.WriteUInt32(ImageMemory);
    str.WriteUInt32(MovieDataMemory);
    str.WriteUInt32(MovieViewMemory);
    str.WriteUInt32(MeshCacheMemory);
    str.WriteUInt32(FontCacheMemory);
    str.WriteUInt32(VideoMemory);
    str.WriteUInt32(SoundMemory);
    str.WriteUInt32(OtherMemory);

    str.WriteUInt32(static_cast<UInt32>(MovieStats.GetSize()));
    for (UPInt i = 0; i < MovieStats.GetSize(); ++i)
    {
        MovieStats[i]->Write(str, version);
    }
    if (version >= 15)
    {
        DisplayStats->Write(str, version);
    }
    if (version >= 25)
    {
        DisplayFunctionStats->Write(str, version);
    }
    str.WriteUInt32(static_cast<UInt32>(SwdHandles.GetSize()));
    for (UPInt i = 0; i < SwdHandles.GetSize(); ++i)
    {
        str.WriteUInt32(SwdHandles[i]);
    }
    if (version >= 9)
    {
        str.WriteUInt32(static_cast<UInt32>(FileHandles.GetSize()));
        for (UPInt i = 0; i < FileHandles.GetSize(); ++i)
        {
            str.WriteUInt64(FileHandles[i]);
        }
    }

    MemoryByStatId->Write(str, version);
    if (version <= 18)
    {
        GPtr<GFxAmpMemItem> tmp = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(0);
        tmp->Write(str, version);
    }
    if (version >= 3)
    {
        Images->Write(str, version);
    }
    if (version >= 7)
    {
        Fonts->Write(str, version);
    }
    if (version >= 17)
    {
        str.WriteUInt32(static_cast<UInt32>(ImageList.GetSize()));
        for (UInt32 i = 0; i < ImageList.GetSize(); ++i)
        {
            ImageList[i]->Write(str, version);
        }
    }

    if (version < 8)
    {
        MemFragReport->Write(str, version);
    }
}

//////////////////////////////////////////////////////////

// Constructor
GFxMovieStats::GFxMovieStats() : 
    ViewHandle(0),
    MinFrame(0),
    MaxFrame(0)
{   
    InstructionStats = *GHEAP_AUTO_NEW(this) GFxAmpMovieInstructionStats();
    FunctionStats = *GHEAP_AUTO_NEW(this) GFxAmpMovieFunctionStats();
    SourceLineStats = *GHEAP_AUTO_NEW(this) MovieSourceLineStats();
    FunctionTreeStats = *GHEAP_AUTO_NEW(this) GFxAmpFunctionTreeStats();
}


void GFxMovieStats::Merge(const GFxMovieStats& rhs)
{
    GASSERT(ViewHandle == rhs.ViewHandle);
    GASSERT(ViewName == rhs.ViewName);
    MinFrame = G_Min(MinFrame, rhs.MinFrame);
    MaxFrame = G_Max(MaxFrame, rhs.MaxFrame);

    for (UPInt i = 0; i < rhs.Markers.GetSize(); ++i)
    {
        bool found = false;
        for (UPInt j = 0; j < Markers.GetSize(); ++j)
        {
            if (rhs.Markers[i]->Name == Markers[j]->Name)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            Markers.PushBack(Markers[i]);
        }
    }

    InstructionStats->Merge(*rhs.InstructionStats);
    FunctionStats->Merge(*rhs.FunctionStats);
    SourceLineStats->Merge(*rhs.SourceLineStats);
    FunctionTreeStats->Merge(*rhs.FunctionTreeStats);
}

GFxMovieStats& GFxMovieStats::operator=(const GFxMovieStats& rhs)
{
    ViewHandle = rhs.ViewHandle;
    MinFrame = rhs.MinFrame;
    MaxFrame = rhs.MaxFrame;
    ViewName = rhs.ViewName;
    Version = rhs.Version;
    Width = rhs.Width;
    Height = rhs.Height;
    FrameRate = rhs.FrameRate;
    FrameCount = rhs.FrameCount;
    Markers = rhs.Markers;
    *InstructionStats = *rhs.InstructionStats;
    *FunctionStats = *rhs.FunctionStats;
    *SourceLineStats = *rhs.SourceLineStats;
    *FunctionTreeStats = *rhs.FunctionTreeStats;
    return *this;
}

GFxMovieStats& GFxMovieStats::operator/=(UInt numFrames)
{
    *InstructionStats /= numFrames;
    *FunctionStats /= numFrames;
    *SourceLineStats /= numFrames;
    return *this;
}

GFxMovieStats& GFxMovieStats::operator*=(UInt num)
{
    *InstructionStats *= num;
    *FunctionStats *= num;
    *SourceLineStats *= num;
    return *this;
}

// Serialization
void GFxMovieStats::Read(GFile& str, UInt32 version)
{
    ViewHandle = str.ReadUInt32();
    MinFrame = str.ReadUInt32();
    MaxFrame = str.ReadUInt32();
    if (version >= 4)
    {
        readString(str, &ViewName);
        Version = str.ReadUInt32();
        Width = str.ReadFloat();
        Height = str.ReadFloat();
        FrameRate = str.ReadFloat();
        FrameCount = str.ReadUInt32();
    }
    if (version >= 6)
    {
        UInt32 numMarkers = str.ReadUInt32();
        Markers.Resize(numMarkers);
        for (UInt32 i = 0; i < numMarkers; ++i)
        {
            Markers[i] = *GHEAP_AUTO_NEW(this) MarkerInfo();
            if (version >= 11)
            {
                readString(str, &Markers[i]->Name);
            }
            else
            {
                Markers[i]->Name = "Marker";
            }
            Markers[i]->Number = str.ReadUInt32();
        }
    }
    GASSERT(InstructionStats);
    InstructionStats->Read(str, version);
    GASSERT(FunctionStats);
    FunctionStats->Read(str, version);
    GASSERT(SourceLineStats);
    SourceLineStats->Read(str, version);
    if (version >= 25)
    {
        GASSERT(FunctionTreeStats);
        FunctionTreeStats->Read(str, version);
    }
}

// Serialization
void GFxMovieStats::Write(GFile& str, UInt32 version) const
{
    str.WriteUInt32(ViewHandle);
    str.WriteUInt32(MinFrame);
    str.WriteUInt32(MaxFrame);
    if (version >= 4)
    {
        writeString(str, ViewName);
        str.WriteUInt32(Version);
        str.WriteFloat(Width);
        str.WriteFloat(Height);
        str.WriteFloat(FrameRate);
        str.WriteUInt32(FrameCount);
    }
    if (version >= 6)
    {
        str.WriteUInt32(static_cast<UInt32>(Markers.GetSize()));
        for (UPInt i = 0; i < Markers.GetSize(); ++i)
        {
            if (version >= 11)
            {
                writeString(str, Markers[i]->Name);
            }
            str.WriteUInt32(Markers[i]->Number);
        }
    }
    GASSERT(InstructionStats);
    InstructionStats->Write(str, version);
    GASSERT(FunctionStats);
    FunctionStats->Write(str, version);
    GASSERT(SourceLineStats);
    SourceLineStats->Write(str, version);
    if (version >= 25)
    {
        GASSERT(FunctionTreeStats);
        FunctionTreeStats->Write(str, version);
    }
}

/////////////////////////////////////////////////////

void ImageInfo::Read(GFile& str, UInt32 version)
{
    GUNUSED(version);
    Id = str.ReadUInt32();
    readString(str, &Name);
    readString(str, &HeapName);
    Bytes = str.ReadUInt32();
    External = (str.ReadUByte() != 0);
    AtlasId = str.ReadUInt32();
    AtlasTop = str.ReadUInt32();
    AtlasBottom = str.ReadUInt32();
    AtlasLeft = str.ReadUInt32();
    AtlasRight = str.ReadUInt32();
}

void ImageInfo::Write(GFile& str, UInt32 version) const
{
    GUNUSED(version);
    str.WriteUInt32(Id);
    writeString(str, Name);
    writeString(str, HeapName);
    str.WriteUInt32(Bytes);
    str.WriteUByte(External ? 1 : 0);
    str.WriteUInt32(AtlasId);
    str.WriteUInt32(AtlasTop);
    str.WriteUInt32(AtlasBottom);
    str.WriteUInt32(AtlasLeft);
    str.WriteUInt32(AtlasRight);
}

/////////////////////////////////////////////////////

GFxAmpMovieInstructionStats& GFxAmpMovieInstructionStats::operator/=(UInt numFrames)
{
    for (UPInt i = 0; i < BufferStatsArray.GetSize(); ++i)
    {
        GArrayLH<InstructionTimePair>& times = BufferStatsArray[i]->InstructionTimesArray;
        for (UPInt j = 0; j < times.GetSize(); ++j)
        {
            times[j].Time /= numFrames;
        }
    }
    return *this;
}

GFxAmpMovieInstructionStats& GFxAmpMovieInstructionStats::operator*=(UInt num)
{
    for (UPInt i = 0; i < BufferStatsArray.GetSize(); ++i)
    {
        GArrayLH<InstructionTimePair>& times = BufferStatsArray[i]->InstructionTimesArray;
        for (UPInt j = 0; j < times.GetSize(); ++j)
        {
            times[j].Time *= num;
        }
    }
    return *this;
}


void GFxAmpMovieInstructionStats::Merge(const GFxAmpMovieInstructionStats& other)
{
    for (UPInt i = 0; i < other.BufferStatsArray.GetSize(); ++i)
    {
        bool buffersMerged = false;
        for (UPInt j = 0; j < BufferStatsArray.GetSize(); ++j)
        {
            if (BufferStatsArray[j]->SwdHandle == other.BufferStatsArray[i]->SwdHandle
                && BufferStatsArray[j]->BufferOffset == other.BufferStatsArray[i]->BufferOffset)
            {
                GASSERT(BufferStatsArray[j]->BufferLength == other.BufferStatsArray[i]->BufferLength);
                GArrayLH<InstructionTimePair>& thisArray = BufferStatsArray[j]->InstructionTimesArray;
                const GArrayLH<InstructionTimePair>& otherArray = BufferStatsArray[i]->InstructionTimesArray;
                for (UPInt k = 0; k < otherArray.GetSize(); ++k)
                {
                    bool instructionsMerged = false;
                    for (UPInt l = 0; l < thisArray.GetSize(); ++l)
                    {
                        if (thisArray[l].Offset == otherArray[k].Offset)
                        {
                            thisArray[l].Time += otherArray[k].Time;
                            instructionsMerged = true;
                            break;
                        }
                    }

                    if (!instructionsMerged)
                    {
                        thisArray.PushBack(otherArray[k]);
                    }
                }
                buffersMerged = true;
                break;
            }
        }

        if (!buffersMerged)
        {
            ScriptBufferStats* newBufferStats = GHEAP_AUTO_NEW(this) ScriptBufferStats();
            *newBufferStats = *other.BufferStatsArray[i];
            BufferStatsArray.PushBack(*newBufferStats);
        }
    }
}


// Serialization
void GFxAmpMovieInstructionStats::Read(GFile& str, UInt32 version)
{
    UInt32 iSize = str.ReadUInt32();
    BufferStatsArray.Resize(iSize);
    for (UPInt i = 0; i < BufferStatsArray.GetSize(); ++i)
    {
        BufferStatsArray[i] = *GHEAP_AUTO_NEW(this) ScriptBufferStats();
        BufferStatsArray[i]->Read(str, version);
    }
}

// Serialization
void GFxAmpMovieInstructionStats::Write(GFile& str, UInt32 version) const
{
    str.WriteUInt32(static_cast<UInt32>(BufferStatsArray.GetSize()));
    for (UPInt i = 0; i < BufferStatsArray.GetSize(); ++i)
    {
        BufferStatsArray[i]->Write(str, version);
    }
}

///////////////////////////////////////////////////////////

// Serialization
void GFxAmpMovieInstructionStats::ScriptBufferStats::Read(GFile& str, UInt32 version)
{
    GUNUSED(version);
    SwdHandle = str.ReadUInt32();
    BufferOffset = str.ReadUInt32();
    BufferLength = str.ReadUInt32();
    UInt32 iSize = str.ReadUInt32();
    InstructionTimesArray.Resize(iSize);
    for (UPInt i = 0; i < InstructionTimesArray.GetSize(); ++i)
    {
        InstructionTimesArray[i].Offset = str.ReadUInt32();
        InstructionTimesArray[i].Time = str.ReadUInt64();
    }
}

// Serialization
void GFxAmpMovieInstructionStats::ScriptBufferStats::Write(GFile& str, UInt32 version) const
{
    GUNUSED(version);
    str.WriteUInt32(SwdHandle);
    str.WriteUInt32(BufferOffset);
    str.WriteUInt32(BufferLength);
    str.WriteUInt32(static_cast<UInt32>(InstructionTimesArray.GetSize()));
    for (UPInt i = 0; i < InstructionTimesArray.GetSize(); ++i)
    {
        str.WriteUInt32(InstructionTimesArray[i].Offset);
        str.WriteUInt64(InstructionTimesArray[i].Time);
    }
}

///////////////////////////////////////////////////////////////

void FuncTreeItem::GetAllFunctions(GHashSet<UInt64>* functionIds) const
{
    functionIds->Set(FunctionId);
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        Children[i]->GetAllFunctions(functionIds);
    }
}

const FuncTreeItem* FuncTreeItem::GetTreeItem(UInt32 treeItemId) const
{
    if (treeItemId == TreeItemId)
    {
        return this;
    }
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        const FuncTreeItem* childItem = Children[i]->GetTreeItem(treeItemId);
        if (childItem != NULL)
        {
            return childItem;
        }
    }
    return NULL;
}

void FuncTreeItem::Read(GFile& str, UInt32 version)
{
    FunctionId = str.ReadUInt64();
    BeginTime = str.ReadUInt64();
    EndTime = str.ReadUInt64();
    TreeItemId = str.ReadUInt32();
    Children.Resize(str.ReadUInt32());
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        Children[i] = *GHEAP_AUTO_NEW(this) FuncTreeItem();
        Children[i]->Read(str, version);
    }
}

void FuncTreeItem::Write(GFile& str, UInt32 version) const
{
    str.WriteUInt64(FunctionId);
    str.WriteUInt64(BeginTime);
    str.WriteUInt64(EndTime);
    str.WriteUInt32(TreeItemId);
    str.WriteUInt32(static_cast<UInt32>(Children.GetSize()));
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        Children[i]->Write(str, version);
    }
}


///////////////////////////////////////////////////////////////

GFxAmpMovieFunctionStats& GFxAmpMovieFunctionStats::operator/=(UInt numFrames)
{
    for (UPInt i = 0; i < FunctionTimings.GetSize(); ++i)
    {
        FunctionTimings[i].TimesCalled /= numFrames;
        FunctionTimings[i].TotalTime /= numFrames;        
    }
    return *this;
}

GFxAmpMovieFunctionStats& GFxAmpMovieFunctionStats::operator*=(UInt num)
{
    for (UPInt i = 0; i < FunctionTimings.GetSize(); ++i)
    {
        FunctionTimings[i].TimesCalled *= num;
        FunctionTimings[i].TotalTime *= num;        
    }
    return *this;
}

void GFxAmpMovieFunctionStats::Merge(const GFxAmpMovieFunctionStats& other)
{
    for (UPInt i = 0; i < other.FunctionTimings.GetSize(); ++i)
    {
        bool merged = false;
        for (UPInt j = 0; j < FunctionTimings.GetSize(); ++j)
        {
            if (FunctionTimings[j].FunctionId == other.FunctionTimings[i].FunctionId
                && FunctionTimings[j].CallerId == other.FunctionTimings[i].CallerId)
            {
                FunctionTimings[j].TimesCalled += other.FunctionTimings[i].TimesCalled;
                FunctionTimings[j].TotalTime += other.FunctionTimings[i].TotalTime;
                merged = true;
                break;
            }
        }
        if (!merged)
        {
            FunctionTimings.PushBack(other.FunctionTimings[i]);
        }
    }

    GFxAmpFunctionDescMap::ConstIterator it;
    for (it = other.FunctionInfo.Begin(); it != other.FunctionInfo.End(); ++it)
    {
        FunctionInfo.Set(it->First, it->Second);
    }
}


// Serialization
void GFxAmpMovieFunctionStats::Read(GFile& str, UInt32 version)
{
    GUNUSED(version);
    UInt32 size = str.ReadUInt32();
    FunctionTimings.Resize(size);
    for (UInt32 i = 0; i < size; i++)
    {
        FunctionTimings[i].FunctionId = str.ReadUInt64();
        FunctionTimings[i].CallerId = str.ReadUInt64();
        FunctionTimings[i].TimesCalled = str.ReadUInt32();
        FunctionTimings[i].TotalTime = str.ReadUInt64();
    }
    
    size = str.ReadUInt32();
    for (UInt32 i = 0; i < size; i++)
    {
        UInt64 iKey = str.ReadUInt64();
        GFxAmpFunctionDesc* pDesc = GHEAP_AUTO_NEW(this) GFxAmpFunctionDesc();
        readString(str, &pDesc->Name);
        pDesc->Length = str.ReadUInt32();
        if (version >= 9)
        {
            pDesc->FileId = str.ReadUInt64();
            pDesc->FileLine = str.ReadUInt32();
            if (version >= 13)
            {
                pDesc->ASVersion = str.ReadUInt32();
            }
        }
        FunctionInfo.Set(iKey, *pDesc);
    }
}

// Serialization
void GFxAmpMovieFunctionStats::Write(GFile& str, UInt32 version) const
{
    GUNUSED(version);
    str.WriteUInt32(static_cast<UInt32>(FunctionTimings.GetSize()));
    for (UPInt i = 0; i < FunctionTimings.GetSize(); ++i)
    {
        str.WriteUInt64(FunctionTimings[i].FunctionId);
        str.WriteUInt64(FunctionTimings[i].CallerId);
        str.WriteUInt32(FunctionTimings[i].TimesCalled);
        str.WriteUInt64(FunctionTimings[i].TotalTime);
    }

    str.WriteUInt32(static_cast<UInt32>(FunctionInfo.GetSize()));
    GFxAmpFunctionDescMap::ConstIterator descIt;
    for (descIt = FunctionInfo.Begin(); descIt != FunctionInfo.End(); ++descIt)
    {
        str.WriteUInt64(descIt->First);
        writeString(str, descIt->Second->Name);
        str.WriteUInt32(descIt->Second->Length);
        if (version >= 9)
        {
            str.WriteUInt64(descIt->Second->FileId);
            str.WriteUInt32(descIt->Second->FileLine);
            if (version >= 13)
            {
                str.WriteUInt32(descIt->Second->ASVersion);
            }
        }
    }
}

// Debug message for visualizing the call graph
void GFxAmpMovieFunctionStats::DebugPrint() const
{
    for (UPInt i = 0; i < FunctionTimings.GetSize(); ++i)
    {
        GString entry;
        GString name;
        GFxAmpFunctionDescMap::ConstIterator descIt;
        descIt = FunctionInfo.Find(FunctionTimings[i].FunctionId);
        if (descIt != FunctionInfo.End())
        {
            name = descIt->Second->Name;
        }
        GString parentName;
        descIt = FunctionInfo.Find(FunctionTimings[i].CallerId);
        if (descIt != FunctionInfo.End())
        {
            parentName = descIt->Second->Name;
        }
        G_Format(entry, "{0} ({1}) from {2} ({3}): {4} times\n", name.ToCStr(), FunctionTimings[i].FunctionId, parentName.ToCStr(), FunctionTimings[i].CallerId, FunctionTimings[i].TimesCalled);
        GFC_DEBUG_MESSAGE(1, entry);
    }
    GFC_DEBUG_MESSAGE(1, "------------------------------");
}

///////////////////////////////////////////////////////////////

bool GFxAmpFunctionTreeStats::IsEmpty() const
{
    return FunctionRoots.GetSize() == 0;
}

///////////////////////////////////////////////////////////////

void GFxAmpFunctionTreeStats::Merge(const GFxAmpFunctionTreeStats& other)
{
    FunctionRoots.Append(other.FunctionRoots);

    GFxAmpFunctionDescMap::ConstIterator it;
    for (it = other.FunctionInfo.Begin(); it != other.FunctionInfo.End(); ++it)
    {
        FunctionInfo.Set(it->First, it->Second);
    }
}


// Serialization
void GFxAmpFunctionTreeStats::Read(GFile& str, UInt32 version)
{
    GUNUSED(version);

    readString(str, &ViewName);

    FunctionRoots.Resize(str.ReadUInt32());
    for (UInt32 i = 0; i < FunctionRoots.GetSize(); ++i)
    {
        FunctionRoots[i] = *GHEAP_AUTO_NEW(this) FuncTreeItem();
        FunctionRoots[i]->Read(str, version);
    }

    UInt32 size = str.ReadUInt32();
    for (UInt32 i = 0; i < size; i++)
    {
        UInt64 iKey = str.ReadUInt64();
        GFxAmpFunctionDesc* pDesc = GHEAP_AUTO_NEW(this) GFxAmpFunctionDesc();
        readString(str, &pDesc->Name);
        pDesc->Length = str.ReadUInt32();
        pDesc->FileId = str.ReadUInt64();
        pDesc->FileLine = str.ReadUInt32();
        pDesc->ASVersion = str.ReadUInt32();
        FunctionInfo.Set(iKey, *pDesc);
    }
}

// Serialization
void GFxAmpFunctionTreeStats::Write(GFile& str, UInt32 version) const
{
    GUNUSED(version);

    writeString(str, ViewName);

    str.WriteUInt32(static_cast<UInt32>(FunctionRoots.GetSize()));
    for (UPInt i = 0; i < FunctionRoots.GetSize(); ++i)
    {
        FunctionRoots[i]->Write(str, version);
    }

    str.WriteUInt32(static_cast<UInt32>(FunctionInfo.GetSize()));
    GFxAmpFunctionDescMap::ConstIterator descIt;
    for (descIt = FunctionInfo.Begin(); descIt != FunctionInfo.End(); ++descIt)
    {
        str.WriteUInt64(descIt->First);
        writeString(str, descIt->Second->Name);
        str.WriteUInt32(descIt->Second->Length);
        str.WriteUInt64(descIt->Second->FileId);
        str.WriteUInt32(descIt->Second->FileLine);
        str.WriteUInt32(descIt->Second->ASVersion);
    }
}



///////////////////////////////////////////////////////////////

MovieSourceLineStats& MovieSourceLineStats::operator/=(unsigned numFrames)
{
    for (UPInt i = 0; i < SourceLineTimings.GetSize(); ++i)
    {
        SourceLineTimings[i].TotalTime /= numFrames;        
    }
    return *this;
}

MovieSourceLineStats& MovieSourceLineStats::operator*=(unsigned num)
{
    for (UPInt i = 0; i < SourceLineTimings.GetSize(); ++i)
    {
        SourceLineTimings[i].TotalTime *= num;        
    }
    return *this;
}

void MovieSourceLineStats::Merge(const MovieSourceLineStats& other)
{
    for (UPInt i = 0; i < other.SourceLineTimings.GetSize(); ++i)
    {
        bool merged = false;
        for (UPInt j = 0; j < SourceLineTimings.GetSize(); ++j)
        {
            if (SourceLineTimings[j].FileId == other.SourceLineTimings[i].FileId
                && SourceLineTimings[j].LineNumber == other.SourceLineTimings[i].LineNumber)
            {
                SourceLineTimings[j].TotalTime += other.SourceLineTimings[i].TotalTime;
                merged = true;
                break;
            }
        }
        if (!merged)
        {
            SourceLineTimings.PushBack(other.SourceLineTimings[i]);
        }
    }

    SourceFileDescMap::ConstIterator it;
    for (it = other.SourceFileInfo.Begin(); it != other.SourceFileInfo.End(); ++it)
    {
        SourceFileInfo.Set(it->First, it->Second);
    }
}


// Serialization
void MovieSourceLineStats::Read(GFile& str, UInt32 version)
{
    if (version >= 9)
    {
        UInt32 size = str.ReadUInt32();
        SourceLineTimings.Resize(size);
        for (UInt32 i = 0; i < size; i++)
        {
            SourceLineTimings[i].FileId = str.ReadUInt64();
            SourceLineTimings[i].LineNumber = str.ReadUInt32();
            SourceLineTimings[i].TotalTime = str.ReadUInt64();
        }

        size = str.ReadUInt32();
        for (UInt32 i = 0; i < size; i++)
        {
            UInt64 iKey = str.ReadUInt64();
            GString desc;
            readString(str, &desc);
            SourceFileInfo.Set(iKey, desc);
        }
    }
}

// Serialization
void MovieSourceLineStats::Write(GFile& str, UInt32 version) const
{
    if (version >= 9)
    {
        str.WriteUInt32(static_cast<UInt32>(SourceLineTimings.GetSize()));
        for (UPInt i = 0; i < SourceLineTimings.GetSize(); ++i)
        {
            str.WriteUInt64(SourceLineTimings[i].FileId);
            str.WriteUInt32(SourceLineTimings[i].LineNumber);
            str.WriteUInt64(SourceLineTimings[i].TotalTime);
        }

        str.WriteUInt32(static_cast<UInt32>(SourceFileInfo.GetSize()));
        SourceFileDescMap::ConstIterator descIt;
        for (descIt = SourceFileInfo.Begin(); descIt != SourceFileInfo.End(); ++descIt)
        {
            str.WriteUInt64(descIt->First);
            writeString(str, descIt->Second);
        }
    }
}

///////////////////////////////////////

void GFxAmpMemItemExtra::Read(GFile& str, UInt32 version)
{
    ImageId = str.ReadUInt32();
    if (version > 11)
    {
        AtlasId = str.ReadUInt32();
        AtlasRect.Left = str.ReadSInt32();
        AtlasRect.Top = str.ReadSInt32();
        AtlasRect.Right = str.ReadSInt32();
        AtlasRect.Bottom = str.ReadSInt32();
    }
}

void GFxAmpMemItemExtra::Write(GFile& str, UInt32 version) const
{
    str.WriteUInt32(ImageId);
    if (version > 11)
    {
        str.WriteUInt32(AtlasId);
        str.WriteSInt32(AtlasRect.Left);
        str.WriteSInt32(AtlasRect.Top);
        str.WriteSInt32(AtlasRect.Right);
        str.WriteSInt32(AtlasRect.Bottom);
    }
}


///////////////////////////////////////

void GFxAmpMemItem::SetValue(UInt32 memValue)
{
    Value = memValue;
    HasValue = true;
}

GFxAmpMemItem* GFxAmpMemItem::AddChild(UInt32 id, const char* name)
{
    GFxAmpMemItem* child = GHEAP_AUTO_NEW(this) GFxAmpMemItem(id);
    child->Name = name;
    Children.PushBack(*child);
    return child;
}

GFxAmpMemItem* GFxAmpMemItem::AddChild(UInt32 id, const char* name, UInt32 memValue)
{
    GFxAmpMemItem* child = GHEAP_AUTO_NEW(this) GFxAmpMemItem(id);
    child->Name = name;
    child->SetValue(memValue);
    Children.PushBack(*child);
    return child;
}

// Depth-first search for matching name
GFxAmpMemItem* GFxAmpMemItem::SearchForName(const char* name)
{
    if (Name == name)
    {
        return this;
    }
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        GFxAmpMemItem* childFound = Children[i]->SearchForName(name);
        if (childFound != NULL)
        {
            return childFound;
        }
    }
    return NULL;
}

UInt32 GFxAmpMemItem::SumValues(const char* name) const
{
    if (Name == name)
    {
        return Value;
    }

    UInt32 sum = 0;
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        sum += Children[i]->SumValues(name);
    }
    return sum;
}

UInt32 GFxAmpMemItem::GetValue(const char* name) const
{
    if (Name == name)
    {
        return Value;
    }

    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        UInt32 childValue = Children[i]->GetValue(name);
        if (childValue != 0)
        {
            return childValue;
        }
    }
    return 0;
}


// Returns the greatest ID for this item and its children, 
// so we can add more items with unique IDs
UInt32 GFxAmpMemItem::GetMaxId() const
{
    UInt32 maxId = ID;
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        maxId = G_Max(maxId, Children[i]->GetMaxId());
    }
    return maxId;
}

GFxAmpMemItem& GFxAmpMemItem::operator=(const GFxAmpMemItem& rhs)
{
    Name = rhs.Name;
    Value = rhs.Value;
    HasValue = rhs.HasValue;
    StartExpanded = rhs.StartExpanded;
    ID = rhs.ID;
    if (rhs.ImageData)
    {
        ImageData = *GHEAP_AUTO_NEW(this) GFxAmpMemItemExtra(0);
        *ImageData = *rhs.ImageData;
    }
    else
    {
        ImageData = NULL;
    }
    Children.Clear();
    for (UPInt i = 0; i < rhs.Children.GetSize(); ++i)
    {
        GFxAmpMemItem* newChild = GHEAP_AUTO_NEW(this) GFxAmpMemItem(rhs.Children[i]->ID);
        *newChild = *rhs.Children[i];
        Children.PushBack(*newChild);
    }
    return *this;
}


GFxAmpMemItem& GFxAmpMemItem::operator/=(UInt numFrames)
{
    Value /= numFrames;
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        *Children[i] /= numFrames;
    }
    return *this;
}

GFxAmpMemItem& GFxAmpMemItem::operator*=(UInt num)
{
    Value *= num;
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        *Children[i] *= num;
    }
    return *this;
}

bool GFxAmpMemItem::Merge(const GFxAmpMemItem& other)
{
    if (!HasValue && Name.IsEmpty() && Children.GetSize() == 0)
    {
        *this = other;
        return true;
    }

    if (other.Name != Name)
    {
        return false;
    }

    Value += other.Value;
    HasValue = (HasValue || other.HasValue);
    for (UPInt i = 0; i < other.Children.GetSize(); ++i)
    {
        bool merged = false;
        for (UPInt j = 0; j < Children.GetSize(); ++j)
        {
            if (Children[j]->Merge(*other.Children[i]))
            {
                merged = true;
                break;
            }
        }
        if (!merged)
        {
            GFxAmpMemItem* newItem = GHEAP_AUTO_NEW(this) GFxAmpMemItem(other.Children[i]->ID);
            *newItem = *other.Children[i];
            Children.PushBack(*newItem);
        }
    }
    return true;
}


// Serialization
void GFxAmpMemItem::Read(GFile& str, UInt32 version)
{
    readString(str, &Name);
    HasValue = (str.ReadUByte() != 0);
    StartExpanded = (str.ReadUByte() != 0);
    Value = str.ReadUInt32();
    ID = str.ReadUInt32();
    if (version > 11)
    {
        UByte hasData = str.ReadUByte();
        if (hasData != 0)
        {
            ImageData = *GHEAP_AUTO_NEW(this) GFxAmpMemItemExtra(0);
            ImageData->Read(str, version);
        }
    }
    else
    {
        UInt32 imageId = str.ReadUInt32();
        if (imageId != 0)
        {
            ImageData = *GHEAP_AUTO_NEW(this) GFxAmpMemItemExtra(imageId);
        }
    }

    UInt32 numChildren = str.ReadUInt32();
    Children.Resize(numChildren);
    for (UInt32 i = 0; i < numChildren; ++i)
    {
        Children[i] = *GHEAP_AUTO_NEW(this) GFxAmpMemItem(0);
        Children[i]->Read(str, version);
    }
}

void GFxAmpMemItem::Write(GFile& str, UInt32 version) const
{
    writeString(str, Name);
    str.WriteUByte(HasValue ? 1 : 0);
    str.WriteUByte(StartExpanded ? 1 : 0);
    str.WriteUInt32(Value);
    str.WriteUInt32(ID);
    if (version > 11)
    {
        if (ImageData)
        {
            str.WriteUByte(1);
            ImageData->Write(str, version);
        }
        else
        {
            str.WriteUByte(0);
        }
    }
    else
    {
        if (ImageData)
        {
            str.WriteUInt32(ImageData->ImageId);
        }
        else
        {
            str.WriteUInt32(0);
        }
    }
    str.WriteUInt32(static_cast<UInt32>(Children.GetSize()));
    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        Children[i]->Write(str, version);
    }
}

void GFxAmpMemItem::ToString(GStringBuffer* report, UByte indent) const
{
    GArray<char> sft(indent + 1);
    memset(&sft[0], ' ', indent);
    sft[indent] = '\0';
    report->AppendString(&sft[0]);

    report->AppendString(Name.ToCStr());
    if (HasValue)
    {
        const unsigned tabSize = 50;
        unsigned numSpaces = tabSize - G_Min(static_cast<unsigned>(Name.GetLength()) + indent, tabSize);
        sft.Resize(numSpaces + 1);
        memset(&sft[0], ' ', numSpaces);
        sft[numSpaces] = '\0';
        report->AppendString(&sft[0]);
        G_Format(*report, " {0:sep:,}", Value);
    }
    report->AppendChar('\n');

    for (UPInt i = 0; i < Children.GetSize(); ++i)
    {
        Children[i]->ToString(report, indent + 4);
    }
}


///////////////////////////////////////////////////////////////////////////

void GFxAmpMemSegment::Read(GFile& str, UInt32 version)
{
    GUNUSED(version);
    Addr = str.ReadUInt64();
    Size = str.ReadUInt64();
    Type = str.ReadUByte();
    HeapId = str.ReadUInt64();
}

void GFxAmpMemSegment::Write(GFile& str, UInt32 version) const
{
    GUNUSED(version);
    str.WriteUInt64(Addr);
    str.WriteUInt64(Size);
    str.WriteUByte(Type);
    str.WriteUInt64(HeapId);
}


////////////////////////////////////////////////////////////////////////////////

GFxAmpHeapInfo::GFxAmpHeapInfo(const GMemoryHeap* heap) : Arena(0), Flags(0)
{
    if (heap != NULL)
    {
        Name = heap->GetName();
        Flags = heap->GetFlags();
        GMemoryHeap::HeapInfo info;
        heap->GetHeapInfo(&info);
        Arena = info.Desc.Arena;
    }
}


void GFxAmpHeapInfo::Read(GFile& str, UInt32 version)
{
    GUNUSED(version);
    Arena = str.ReadUInt64();
    Flags = str.ReadUInt32();
    readString(str, &Name);
}

void GFxAmpHeapInfo::Write(GFile& str, UInt32 version) const
{
    GUNUSED(version);
    str.WriteUInt64(Arena);
    str.WriteUInt32(Flags);
    writeString(str, Name);
}

///////////////////////////////////////////////////////////////////////////////////

void GFxAmpMemFragReport::Read(GFile& str, UInt32 version)
{
    UInt32 numSegments = str.ReadUInt32();
    MemorySegments.Resize(numSegments);
    for (UInt32 i = 0; i < numSegments; ++i)
    {
        MemorySegments[i].Read(str, version);
    }
    UInt32 numHeaps = str.ReadUInt32();
    for (UInt32 i = 0; i < numHeaps; ++i)
    {
        UInt64 key = str.ReadUInt64();
        GFxAmpHeapInfo* heapInfo = GHEAP_AUTO_NEW(this) GFxAmpHeapInfo();
        heapInfo->Read(str, version);
        HeapInformation.Set(key, *heapInfo);
    }
}

void GFxAmpMemFragReport::Write(GFile& str, UInt32 version) const
{
    str.WriteUInt32(static_cast<UInt32>(MemorySegments.GetSize()));
    for (UPInt i = 0; i < MemorySegments.GetSize(); ++i)
    {
        MemorySegments[i].Write(str, version);
    }
    str.WriteUInt32(static_cast<UInt32>(HeapInformation.GetSize()));
    GHashLH<UInt64, GPtr<GFxAmpHeapInfo> >::ConstIterator it;
    for (it = HeapInformation.Begin(); it != HeapInformation.End(); ++it)
    {
        str.WriteSInt64(it->First);
        it->Second->Write(str, version);
    }
}

//////////////////////////////////////////////////////////

GFxAmpCurrentState::GFxAmpCurrentState() :
    StateFlags(0),
    CurveTolerance(0.0f), 
    CurveToleranceMin(0.0f),
    CurveToleranceMax(0.0f),
    CurveToleranceStep(0.0f),
    CurrentFileId(0),
    CurrentLineNumber(0),
    ProfileLevel(0)
{
}

GFxAmpCurrentState& GFxAmpCurrentState::operator=(const GFxAmpCurrentState& rhs)
{
    StateFlags = rhs.StateFlags;
    ConnectedApp = rhs.ConnectedApp;
    ConnectedFile = rhs.ConnectedFile;
    AaMode = rhs.AaMode;
    StrokeType = rhs.StrokeType;
    CurrentLocale = rhs.CurrentLocale;
    Locales.Resize(rhs.Locales.GetSize());
    for (UPInt i = 0; i < Locales.GetSize(); ++i)
    {
        Locales[i] = rhs.Locales[i];
    }
    CurveTolerance = rhs.CurveTolerance;
    CurveToleranceMin = rhs.CurveToleranceMin;
    CurveToleranceMax = rhs.CurveToleranceMax;
    CurveToleranceStep = rhs.CurveToleranceStep;
    CurrentFileId = rhs.CurrentFileId;
    CurrentLineNumber = rhs.CurrentLineNumber;
    ProfileLevel = rhs.ProfileLevel;
    return *this;
}

bool GFxAmpCurrentState::operator!=(const GFxAmpCurrentState& rhs) const
{
    if (StateFlags != rhs.StateFlags) return true;
    if (ConnectedApp != rhs.ConnectedApp) return true;
    if (ConnectedFile != rhs.ConnectedFile) return true;
    if (AaMode != rhs.AaMode) return true;
    if (StrokeType != rhs.StrokeType) return true;
    if (CurrentLocale != rhs.CurrentLocale) return true;
    if (Locales.GetSize() != rhs.Locales.GetSize()) return true;
    for (UPInt i = 0; i < Locales.GetSize(); ++i)
    {
        if (Locales[i] != rhs.Locales[i]) return true;
    }
    if (G_Abs(CurveTolerance - rhs.CurveTolerance) > 0.0001) return true;
    if (G_Abs(CurveToleranceMin - rhs.CurveToleranceMin) > 0.0001) return true;
    if (G_Abs(CurveToleranceMax - rhs.CurveToleranceMax) > 0.0001) return true;
    if (G_Abs(CurveToleranceStep - rhs.CurveToleranceStep) > 0.0001) return true;
    if (CurrentFileId != rhs.CurrentFileId) return true;
    if (CurrentLineNumber != rhs.CurrentLineNumber) return true;
    if (ProfileLevel != rhs.ProfileLevel) return true;

    return false;
}


// Serialization
void GFxAmpCurrentState::Read(GFile& str, UInt32 version)
{
    StateFlags = str.ReadUInt32();
    if (version >= 20)
    {
        ProfileLevel = str.ReadSInt32();
    }
    readString(str, &ConnectedApp);
    if (version >= 5)
    {
        readString(str, &ConnectedFile);
    }
    readString(str, &AaMode);
    readString(str, &StrokeType);
    readString(str, &CurrentLocale);
    UInt32 size = str.ReadUInt32();
    Locales.Resize(size);
    for (UInt32 i = 0; i < size; ++i)
    {
        readString(str, &Locales[i]);
    }
    CurveTolerance = str.ReadFloat();
    CurveToleranceMin = str.ReadFloat();
    CurveToleranceMax = str.ReadFloat();
    CurveToleranceStep = str.ReadFloat();
    if (version >= 10)
    {
        CurrentFileId = str.ReadUInt64();
        CurrentLineNumber = str.ReadUInt32();
    }
}

// Serialization
void GFxAmpCurrentState::Write(GFile& str, UInt32 version) const
{
    str.WriteUInt32(StateFlags);
    if (version >= 20)
    {
        str.WriteSInt32(ProfileLevel);
    }
    writeString(str, ConnectedApp);
    if (version >= 5)
    {
        writeString(str, ConnectedFile);
    }
    writeString(str, AaMode);
    writeString(str, StrokeType);
    writeString(str, CurrentLocale);
    str.WriteUInt32(static_cast<UInt32>(Locales.GetSize()));
    for (UPInt i = 0; i < Locales.GetSize(); ++i)
    {
        writeString(str, Locales[i]);
    }
    str.WriteFloat(CurveTolerance);
    str.WriteFloat(CurveToleranceMin);
    str.WriteFloat(CurveToleranceMax);
    str.WriteFloat(CurveToleranceStep);
    if (version >= 10)
    {
        str.WriteUInt64(CurrentFileId);
        str.WriteUInt32(CurrentLineNumber);
    }
}


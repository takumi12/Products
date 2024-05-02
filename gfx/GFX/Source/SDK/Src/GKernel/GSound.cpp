/**********************************************************************

Filename    :   GSound.h
Content     :   Sound samples
Created     :   January 23, 2007
Authors     :   Andrew Reisse

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GSound.h"
#ifndef GFC_NO_SOUND

#include "GMemory.h"
#include "GHeapNew.h"

#include "GFunctions.h"
#include "GStd.h"
#if defined(GFX_AMP_SERVER) && !defined(GHEAP_DEBUG_INFO)
#include "GFxAmpServer.h"
#endif

GSoundData::GSoundData(UInt format, UInt rate, UInt length, UInt dsize) 
    : GSoundDataBase(format, rate, length), pData(NULL), DataSize(dsize)
{
    pData = (UByte*)GMEMALIGN(DataSize, 32, GStat_Sound_Mem);
    GMemUtil::Set(pData,0,DataSize);
#if defined(GFX_AMP_SERVER) && !defined(GHEAP_DEBUG_INFO)
    GFxAmpServer::GetInstance().AddSound(DataSize);
#endif
}

GSoundData::~GSoundData()
{
#if defined(GFX_AMP_SERVER) && !defined(GHEAP_DEBUG_INFO)
    GFxAmpServer::GetInstance().RemoveSound(DataSize);
#endif
    GFREE_ALIGN(pData); 
}

#define STREAM_SOUND_DATA_CHUNK_SIZE 16384
//////////////////////////////////////////////////////////////////////////
//
GAppendableSoundData::DataChunk::DataChunk(UInt capacity)
: pNext(NULL), DataSize(0), StartSample(0), SampleCount(0)
{
    pData = (UByte*)GMEMALIGN(capacity, 32, GStat_Sound_Mem);
    GMemUtil::Set(pData,0,capacity);
}
GAppendableSoundData::DataChunk::~DataChunk()
{
    GFREE_ALIGN(pData);
}

GAppendableSoundData::GAppendableSoundData(UInt format, UInt rate)
: GSoundDataBase(format, rate, 0), pFirstChunk(NULL), pFillChunk(NULL), pReadChunk(NULL), 
  DataSize(0), ReadPos(0)
{
    Format |= GSoundDataBase::Sample_Stream;
}

GAppendableSoundData::~GAppendableSoundData()
{
    while(pFirstChunk)
    {
        DataChunk* ptmp = pFirstChunk->pNext;
        delete pFirstChunk;
        pFirstChunk = ptmp;
    }
}

UByte*  GAppendableSoundData::LockDataForAppend(UInt length, UInt dsize)
{
    ChunkLock.Lock();
    GASSERT(dsize < STREAM_SOUND_DATA_CHUNK_SIZE);
    if (!pFirstChunk)
    {
        pFirstChunk = GNEW DataChunk(STREAM_SOUND_DATA_CHUNK_SIZE);
        pFillChunk = pFirstChunk;
        pReadChunk = pFillChunk;
    }
    UByte* pbuffer = 0;
    if (dsize > STREAM_SOUND_DATA_CHUNK_SIZE - pFillChunk->DataSize)
    {
        pFillChunk->pNext = GNEW DataChunk(STREAM_SOUND_DATA_CHUNK_SIZE);
        pFillChunk->pNext->StartSample = pFillChunk->StartSample + pFillChunk->SampleCount;
        pFillChunk = pFillChunk->pNext;
    }
    pbuffer = pFillChunk->pData + pFillChunk->DataSize;
    pFillChunk->DataSize += dsize;
    pFillChunk->SampleCount += length;
    DataSize += dsize;
    Length += length;
    return pbuffer;
}

void GAppendableSoundData::UnlockData()
{
    ChunkLock.Unlock();
}

UInt GAppendableSoundData::GetData(UByte* buff, UInt dsize)
{
    GLock::Locker lock(&ChunkLock);
    if (!pReadChunk)
        return 0;
    UInt got_bytes = 0;
    while(dsize > 0)
    {
        UInt ds = dsize;
        if (ReadPos + dsize > pReadChunk->DataSize)
        {
            ds = pReadChunk->DataSize - ReadPos;
            if (ds == 0)
            {
                if (!pReadChunk->pNext)
                    return got_bytes;
                pReadChunk = pReadChunk->pNext;
                ReadPos = 0;
                continue;
            }
        }
        GMemUtil::Copy(buff+got_bytes, pReadChunk->pData + ReadPos, ds);
        dsize -= ds;
        ReadPos += ds;
        got_bytes += ds;            
    }
    return got_bytes;
}

bool GAppendableSoundData::SeekPos(UInt pos)
{
    GLock::Locker lock(&ChunkLock);
    if (!pReadChunk)
        return false;
    UInt lpos = 0;
    pReadChunk = pFirstChunk;
    while(1)
    {
        lpos += pReadChunk->DataSize;
        if (pos < lpos)
        {
            ReadPos = pos - (lpos - pReadChunk->DataSize);
            return true;
        }
        if (!pReadChunk->pNext)
            break;
        pReadChunk = pReadChunk->pNext;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
//
GSoundFile::GSoundFile(const char* fname, UInt rate, UInt length, bool streaming)
: GSoundDataBase(GSoundDataBase::Sample_File, rate, length)
{
    if (streaming)
        Format |= GSoundDataBase::Sample_Stream;
    UPInt len = G_strlen(fname) + 1;
    pFileName = (char*)GALLOC(len, GStat_Sound_Mem);
    GMemUtil::Set(pFileName,0,len);

    G_strcpy(pFileName, len, fname);
}
GSoundFile::~GSoundFile()
{
    GFREE(pFileName);
}

#endif // GFC_NO_SOUND

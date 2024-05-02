/**********************************************************************

Filename    :   GFxZlibSupport.cpp
Content     :   
Created     :   April 15, 2008
Authors     :   

Notes       :   Redesigned to use inherited state objects for GFx 2.2.

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

///////////// WARNING ///////////////////////////////////////////////
// The implementation of GFxZlibSupport class is moved into a separate 
// file to avoid linking ZLIB functionality into the application if 
// it doesn't need ZLIB support and GFxZlibSupport state is not set to 
// the loader.

#include "GFxStream.h"
#include "GFxLoader.h"

#ifdef GFC_USE_ZLIB
#include <zlib.h>
#include "GZlibFile.h"
#include "GHeapNew.h"


GFile* GFxZlibSupport::CreateZlibFile(GFile* in)
{
    return GHEAP_AUTO_NEW(this) GZLibFile(in);
}

// Wrapper function -- uses Zlib to uncompress bufferBytes worth
// of data from the input file into BufferBytes worth of data
// into *buffer.
void  GFxZlibSupport::InflateWrapper(GFxStream* pinStream, void* buffer, int bufferBytes)
{
    GASSERT(buffer);
    GASSERT(bufferBytes > 0);
    
    // Initialize decompression stream.
    z_stream D_stream; 
    int      err = GZLibFile::ZLib_InitStream(&D_stream, this, buffer, bufferBytes);

    if (err != Z_OK)
    {
        pinStream->LogError("Error: GFx_InflateWrapper() inflateInit() returned %d\n", err);
        return;
    }

    enum { DStreamSourceBuffSize = 32};
    UByte    buff[DStreamSourceBuffSize];

    while(1)
    {
        // Fill a buffer.
        D_stream.next_in  = &buff[0];
        D_stream.avail_in = pinStream->ReadToBuffer(buff, DStreamSourceBuffSize);

        // D_stream.avail_in unsigned ...
        // Maybe we should check for overflow instead ...
        //if (D_stream.avail_in < 0)
        //{            
        //    pinStream->LogError("Error: GFx_InflateWrapper() read data error");
        //    break;
        //}

        err = inflate(&D_stream, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END) break;        
        if (err != Z_OK)
        {            
            pinStream->LogError("Error: GFx_InflateWrapper() Inflate() returned %d\n", err);
            if (D_stream.avail_in > 0)
                pinStream->SetPosition(pinStream->Tell() - D_stream.avail_in);
            GASSERT(0);
            break;
        }
        //buf[0] = pinStream->ReadU8();
    }

    if (D_stream.avail_in > 0)
        pinStream->SetPosition(pinStream->Tell() - D_stream.avail_in);

    err = inflateEnd(&D_stream);
    if (err != Z_OK)
    {
        pinStream->LogError("Error: GFx_InflateWrapper() InflateEnd() return %d\n", err);
    }
}
#else

GFile* GFxZlibSupport::CreateZlibFile(GFile* in)
{
    GUNUSED(in);
    GFC_DEBUG_WARNING(1, "GFxZlibSupport::CreateZlibFile failed - GFC_USE_ZLIB not defined");
    return NULL;
}
void  GFxZlibSupport::InflateWrapper(GFxStream* pinStream, void* buffer, int BufferBytes)
{
    GUNUSED3(pinStream,buffer,BufferBytes);
    GFC_DEBUG_WARNING(1, "GFxZlibSupport::InflateWrapper failed - GFC_USE_ZLIB not defined");
}

#endif


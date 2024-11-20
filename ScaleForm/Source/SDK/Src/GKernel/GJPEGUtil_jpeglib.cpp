/**********************************************************************

Filename    :   GJPEGUtil.h
Content     :   JPEG image I/O utility classes
Created     :   June 24, 2005
Authors     :   Michael Antonov

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GJPEGUtil.h"
#include "GFile.h"

#include <stdlib.h>
#include "GStd.h"
#include "GHeapNew.h"

#ifdef GFC_USE_LIBJPEG

#ifndef GFC_NO_LONGJMP
#include <setjmp.h>
#endif

#ifdef GFC_CC_MSVC
// disable warning "warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable"
#pragma warning(disable:4611)
#endif

#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360) || defined(GFC_OS_XBOX)
    #if !defined(GFC_CPP_LIBJPEG)
        #define GFC_CPP_LIBJPEG
    #endif  
#endif

#if !defined(GFC_CPP_LIBJPEG)
extern "C" {
#endif //GFC_CPP_LIBJPEG

#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360) || defined(GFC_OS_XBOX) || defined(GFC_OS_WINCE)
// Windows typedefs INT32 correctly but not INT16
    typedef short INT16;
// Remove JPEG's INT32 typedef
    #define  XMD_H
// Undef FAR, jpeglib redefines it identically
    #undef FAR
#endif

#ifdef GFC_PS3_MSVC_BUILD
#include <jconfigPS3.h>
#define JCONFIG_INCLUDED
#endif

#include <jpeglib.h>
#if !defined(GFC_CPP_LIBJPEG)
}
#endif //GFC_CPP_LIBJPEG


enum GJPEGUtilConstants
{
    // This constant should match/be smaller then ZLib_BacktrackSize in GZLibFile.cpp to
    // avoid zlib restarts during embedded JPEG loading in SWFs.
    JPEG_BufferSize = 2048
};

// jpeglib data source constructors, for using GFile* instead
// of stdio for jpeg IO.
void    GJPEGUtil_SetupRwSource(jpeg_decompress_struct* pcinfo, GFile* pinstream);
void    GJPEGUtil_SetupRwDest(jpeg_compress_struct* pcinfo, GFile* poutstream);



// ***** GPEG Input helper class

// A jpeglib source manager that reads from a GFile.  
// Paraphrased from IJG jpeglib jdatasrc.c
class GJPEGRwSource : public GNewOverrideBase<GStat_Default_Mem>
{
public:
    struct jpeg_source_mgr  SMgr;           // public fields

    GPtr<GFile> pInStream;                  // source stream
    bool        StartOfFile;                // have we gotten any data yet?
    JOCTET      Buffer[JPEG_BufferSize];    // start of Buffer


    // Constructor.  The caller is responsible for closing the input stream
    // after it's done using us.
    GJPEGRwSource(GFile* pin)           
    {
        pInStream   = pin;
        StartOfFile = 1;
        
        // fill in function pointers...
        SMgr.init_source            = InitSource;
        SMgr.fill_input_buffer      = FillInputBuffer;
        SMgr.skip_input_data        = SkipInputData;
        SMgr.resync_to_restart      = jpeg_resync_to_restart;   // use default method
        SMgr.term_source            = TermSource;
        SMgr.bytes_in_buffer        = 0;
        SMgr.next_input_byte        = NULL;
    }

    GJPEGRwSource(const struct jpeg_source_mgr&  smgr)
    {
        pInStream   = 0;
        StartOfFile = 1;
        SMgr = smgr;
    }

    static void InitSource(j_decompress_ptr cinfo)
    {
        GJPEGRwSource*  psrc = (GJPEGRwSource*) cinfo->src;
        psrc->StartOfFile = true;
    }

    // Read data into our input Buffer.  Client calls this
    // when it needs more data from the file.
    static boolean  FillInputBuffer(j_decompress_ptr cinfo)     
    {
        GJPEGRwSource*  psrc = (GJPEGRwSource*) cinfo->src;

        UPInt  bytesRead = psrc->pInStream->Read(psrc->Buffer, JPEG_BufferSize);

        if (bytesRead <= 0) {
            // Is the file completely empty?
            if (psrc->StartOfFile) {
                // Treat this as a fatal error.
            //throw "empty jpeg source stream.";
            GFC_DEBUG_WARNING(1, "empty jpeg source stream.");
        return 0;
            }
            // Warn("jpeg end-of-stream");

            // Insert a fake EOI marker.
            psrc->Buffer[0] = (JOCTET) 0xFF;
            psrc->Buffer[1] = (JOCTET) JPEG_EOI;
            bytesRead = 2;
        }

        // Hack to work around SWF bug: sometimes data
        // starts with FFD9FFD8, when it should be
        // FFD8FFD9!
        if (psrc->StartOfFile && bytesRead >= 4)
        {
            if (psrc->Buffer[0] == 0xFF
                && psrc->Buffer[1] == 0xD9 
                && psrc->Buffer[2] == 0xFF
                && psrc->Buffer[3] == 0xD8)
            {
                psrc->Buffer[1] = 0xD8;
                psrc->Buffer[3] = 0xD9;
            }
        }

        // Expose Buffer state to clients.
        psrc->SMgr.next_input_byte = psrc->Buffer;
        psrc->SMgr.bytes_in_buffer = bytesRead;
        psrc->StartOfFile = 0;

        return TRUE;
    }

    // Called by client when it wants to advance past some
    // uninteresting data.
    static void SkipInputData(j_decompress_ptr cinfo, long numBytes)
    {
        GJPEGRwSource*  psrc = (GJPEGRwSource*) cinfo->src;

        // According to jpeg docs, large skips are
        // infrequent.  So let's just do it the simple
        // way.
        if (numBytes > 0)
        {
            while (numBytes > (long) psrc->SMgr.bytes_in_buffer)
            {
                numBytes -= (long) psrc->SMgr.bytes_in_buffer;
                FillInputBuffer(cinfo);
            }
            // Handle remainder.
            psrc->SMgr.next_input_byte += (UPInt) numBytes;
            psrc->SMgr.bytes_in_buffer -= (UPInt) numBytes;
        }
    }

    // Terminate the source.  Make sure we get deleted.
    static void TermSource(j_decompress_ptr cinfo)
    {
        GUNUSED(cinfo);

        /*GJPEGRwSource*    psrc = (GJPEGRwSource*) cinfo->src;
        GASSERT(psrc);

        // @@ it's kind of bogus to be deleting here
        // -- TermSource happens at the end of
        // reading an image, but we're probably going
        // to want to init a source and use it to read
        // many images, without reallocating our Buffer.
        delete psrc;
        cinfo->src = NULL;*/
    }


    void    DiscardPartialBuffer()
    {
        // Discard existing bytes in our Buffer.
        SMgr.bytes_in_buffer = 0;
        SMgr.next_input_byte = NULL;
    }
};


// Set up the given decompress object to read from the given stream.
void    GJPEGUtil_SetupRwSource(jpeg_decompress_struct* cinfo, GFile* pinstream)
{
    // GASSERT(cinfo->src == NULL);
    cinfo->src = (jpeg_source_mgr*) GNEW GJPEGRwSource(pinstream);
}



// ***** GPEG Output helper class

// A jpeglib destination manager that writes to a GFile.
// Paraphrased from IJG jpeglib jdatadst.c.
class GJPEGRwDest : public GNewOverrideBase<GStat_Default_Mem>
{
public:
    struct jpeg_destination_mgr     DMgr;       // public fields

    GPtr<GFile>     pOutStream;                 // source stream
    JOCTET          Buffer[JPEG_BufferSize];    // start of Buffer


    // Constructor.  The caller is responsible for closing
    // the output stream after it's done using us.
    GJPEGRwDest(GFile* pout)            
    {
        pOutStream = pout;
        // fill in function pointers...
        DMgr.init_destination       = InitDestination;
        DMgr.empty_output_buffer    = EmptyOutputBuffer;
        DMgr.term_destination       = TermDestination;

        DMgr.next_output_byte       = Buffer;
        DMgr.free_in_buffer         = JPEG_BufferSize;
    }

    static void InitDestination(j_compress_ptr cinfo)
    {
        GJPEGRwDest*    pdest = (GJPEGRwDest*) cinfo->dest;
    //  GASSERT(dest);

        pdest->DMgr.next_output_byte    = pdest->Buffer;
        pdest->DMgr.free_in_buffer  = JPEG_BufferSize;
    }

    // Write the output Buffer into the stream.
    static boolean  EmptyOutputBuffer(j_compress_ptr cinfo)
    {
        GJPEGRwDest*    pdest = (GJPEGRwDest*) cinfo->dest;
    //  GASSERT(dest);

        if (pdest->pOutStream->Write(pdest->Buffer, JPEG_BufferSize) != JPEG_BufferSize)
        {
            // Error.
            //throw "GJPEGRwDest couldn't write data.";
        GFC_DEBUG_WARNING(1, "GJPEGRwDest couldn't write data.");
        return 0;
        }

        pdest->DMgr.next_output_byte    = pdest->Buffer;
        pdest->DMgr.free_in_buffer  = JPEG_BufferSize;

        return TRUE;
    }

    // Terminate the destination.  Flush any leftover
    // data, and make sure we get deleted.
    static void TermDestination(j_compress_ptr cinfo)
    {
        GJPEGRwDest* pdest = (GJPEGRwDest*) cinfo->dest;
    //  GASSERT(dest);

        // Write any remaining data.
        SPInt   datacount = JPEG_BufferSize - pdest->DMgr.free_in_buffer;
        if (datacount > 0) {
            if (pdest->pOutStream->Write(pdest->Buffer, SInt(datacount)) != datacount)
            {
                // Error.
            //throw "GJPEGRwDest::TermDestination couldn't write data.";
            GFC_DEBUG_WARNING(1, "GJPEGRwDest::TermDestination couldn't write data.");
        return;
            }
        }

        // Clean ourselves up.
        delete pdest;
        cinfo->dest = NULL;
    }
};


// Set up the given compress object to write to the given output stream.
void    GJPEGUtil_SetupRwDest(j_compress_ptr cinfo, GFile* poutstream)
{
    cinfo->dest = (jpeg_destination_mgr*) GNEW GJPEGRwDest(poutstream);
}


    
// ***** Error handler
//
struct GFxJpegErrorHandler 
{
    struct jpeg_error_mgr pub;  // "public" fields 
    char    errorMessage[JMSG_LENGTH_MAX];
#ifndef GFC_NO_LONGJMP
    //!AB: The jmp_buf should be aligned, and the alignment depends on platform.
    // For example, for Win64 this buffer should be aligned on 16-bytes boundary,
    // because it stores 128-bit XMMx registers in it.
    jmp_buf* psetjmp_buffer;    // for return to caller
#endif

    GFxJpegErrorHandler()
    {
        // align jmp_buf on 32-bytes boundary (see comment above)
        psetjmp_buffer = (jmp_buf*)GMEMALIGN(sizeof(jmp_buf), 32, GStat_Default_Mem);
    }
    ~GFxJpegErrorHandler() { GFREE_ALIGN(psetjmp_buffer); }
};


// Called when jpeglib has a fatal error.
static
void    JpegErrorExit(j_common_ptr cinfo)
{
    GFxJpegErrorHandler* myerr = (GFxJpegErrorHandler*) cinfo->err;

    char buffer[JMSG_LENGTH_MAX];

    /* Create the message */
    (*cinfo->err->format_message) (cinfo, buffer);
    G_strcpy(myerr->errorMessage, sizeof(myerr->errorMessage), buffer);

    GFC_DEBUG_ERROR1(1, "GJPEGUtil - internal error in jpeglib: %s\n", myerr->errorMessage);
#ifndef GFC_NO_LONGJMP
    // Return control to the setjmp point
    longjmp(*myerr->psetjmp_buffer, 1);
#else
    exit(1);
#endif
}


// Set up some error handlers for the jpeg lib.
static jpeg_error_mgr* SetupJpegErr(GFxJpegErrorHandler* jerr)
{
    // Set up defaults.
    jpeg_error_mgr* pe = jpeg_std_error(&jerr->pub);
    jerr->pub.error_exit = JpegErrorExit;
    return pe;
}


// ***** GJPEGInput implementation classes
#ifndef GFC_NO_LONGJMP
#define GFC_JPEG_SET_JMP(pcinfo,pjerr,rv) do { \
if (setjmp(*(pjerr)->psetjmp_buffer)) { \
jpeg_destroy_decompress(pcinfo); \
return rv; \
}} while(0)

#define GFC_JPEG_SET_JMP1(_this,rv) do { \
if (setjmp(*(_this)->JErr.psetjmp_buffer)) { \
jpeg_destroy_decompress(&(_this)->CInfo); \
(_this)->CompressorOpened = false; \
(_this)->ErrorOccurred = true; \
return rv; \
}} while(0)
#else
#define GFC_JPEG_SET_JMP(pcinfo,pjerr,rv) (void*)0
#define GFC_JPEG_SET_JMP1(_this,rv)       (void*)0
#endif

// Basically this is a thin wrapper around JpegDecompress object.   
class GJPEGInputImpl_jpeglib : public GJPEGInput
{
public:
    // State needed for GJPEGInput.
    struct jpeg_decompress_struct   CInfo;
    struct GFxJpegErrorHandler      JErr;
private:
    static int JpegCreateDecompress(jpeg_decompress_struct* pcinfo, GFxJpegErrorHandler* pjerr);
    static int JpegReadHeader(jpeg_decompress_struct* pcinfo, GFxJpegErrorHandler* pjerr, bool require_image);

public:
    bool    CompressorOpened;
    bool    ErrorOccurred;

    enum SWF_DEFINE_BITS_JPEG2 { SWF_JPEG2 };
    enum SWF_DEFINE_BITS_JPEG2_HEADER_ONLY { SWF_JPEG2_HEADER_ONLY };


    // Constructor.  Read the header data from in, and
    // prepare to read data.
    GJPEGInputImpl_jpeglib(GFile* pin)      
    {
        CompressorOpened = ErrorOccurred = false;

        CInfo.err = SetupJpegErr(&JErr);

        // Initialize decompression object.
        if (!JpegCreateDecompress(&CInfo, &JErr))
            return;

        GJPEGUtil_SetupRwSource(&CInfo, pin);

        if (!StartImage())
            return;
    }


    // The SWF file format stores JPEG images with the
    // encoding tables separate from the image data.  This
    // constructor reads the encoding table only and keeps
    // them in this object.  You need to call
    // StartImage() and FinishImage() around any calls
    // to GetWidth/height/components and ReadScanline.
    GJPEGInputImpl_jpeglib(SWF_DEFINE_BITS_JPEG2_HEADER_ONLY e, GFile* pin)
    {
        GUNUSED(e);

        CompressorOpened = ErrorOccurred = false;

        CInfo.err = SetupJpegErr(&JErr);

        // Initialize decompression object.
        if (!JpegCreateDecompress(&CInfo, &JErr))
            return;

        GJPEGUtil_SetupRwSource(&CInfo, pin);

        // Read the encoding tables.
        if (!JpegReadHeader(&CInfo, &JErr, FALSE))
            return;

        // Don't start reading any image data!
        // App does that manually using StartImage.
    }

    GJPEGInputImpl_jpeglib(SWF_DEFINE_BITS_JPEG2_HEADER_ONLY e, const UByte* pbuf, UPInt bufSize)
    {
        GUNUSED(e);
        struct jpeg_source_mgr smgr;
        memset(&smgr, 0, sizeof(smgr));
        smgr.bytes_in_buffer        = bufSize;
        smgr.next_input_byte        = pbuf;

        CompressorOpened = ErrorOccurred = false;

        CInfo.err = SetupJpegErr(&JErr);

        // Initialize decompression object.
        if (!JpegCreateDecompress(&CInfo, &JErr))
            return;

        CInfo.src = (jpeg_source_mgr*) GNEW GJPEGRwSource(smgr);

        // Read the encoding tables.
        if (!JpegReadHeader(&CInfo, &JErr, FALSE))
            return;

        // Don't start reading any image data!
        // App does that manually using StartImage.
    }

    // Destructor.  Clean up our jpeg reader state.
    ~GJPEGInputImpl_jpeglib()
    {
        FinishImage();

        GJPEGRwSource* psrc = (GJPEGRwSource*) CInfo.src;
        delete psrc;
        CInfo.src = NULL;


        jpeg_destroy_decompress(&CInfo);
    }

    bool IsErrorOccurred() const { return ErrorOccurred; }

    // Discard any data sitting in our GJPEGInput Buffer.  Use
    // this before/after reading headers or partial image
    // data, to avoid screwing up future reads.
    void    DiscardPartialBuffer()
    {
        GJPEGRwSource* psrc = (GJPEGRwSource*) CInfo.src;

        // We only have to discard the GJPEGInput Buffer after reading the tables.
        if (psrc)           
            psrc->DiscardPartialBuffer();           
    }


    // This is something you can do with "abbreviated"
    // streams; i.e. if you constructed this inputter
    // Using (SWFJPEG2HEADERONLY) to just load the
    // tables, or if you called FinishImage() and want to
    // load another image using the existing tables.
    int    StartImage()
    {
        if (ErrorOccurred) return 0;
        GFC_DEBUG_WARNING(CompressorOpened != false, "GJPEGInput::StartImage - image already started");

        GFC_JPEG_SET_JMP1(this, 0);

        // Now, read the image header.
        // MA: If already in READY state (202), don't call read_header. This makes
        // loading work for SW8. Need to reseatcj JPeg integration further.
        if (CInfo.global_state != 202) 
            jpeg_read_header(&CInfo, TRUE);
        jpeg_start_decompress(&CInfo);
        CompressorOpened = true;
        return 1;
    }

    int    StartRawImage()
    {
        if (ErrorOccurred) return 0;
        GFC_DEBUG_WARNING(CompressorOpened != false, "GJPEGInput::StartRawImage - image already started");

        GFC_JPEG_SET_JMP1(this, 0);

        // Now, read the image header.
        // MA: If already in READY state (202), don't call read_header. This makes
        // loading work for SW8. Need to reseatcj JPeg integration further.
        if (CInfo.global_state != 202) 
            jpeg_read_header(&CInfo, TRUE);
        CompressorOpened = true;
        return 1;
    }

    int    FinishImage()
    {
        if (ErrorOccurred) return 0;
        if (CompressorOpened)
        {
            GFC_JPEG_SET_JMP1(this, 0);
            jpeg_finish_decompress(&CInfo);
            CompressorOpened = false;
        }
        return 1;
    }

    // Return the height of the image.  Take the data from our cinfo struct.
    UInt    GetHeight() const
    {
    //  GASSERT(CompressorOpened);
        return CInfo.output_height;
    }

    // Return the width of the image.  Take the data from our cinfo struct.
    UInt    GetWidth() const
    {
    //  GASSERT(CompressorOpened);
        return CInfo.output_width;
    }

    // Return number of Components (i.e. == 3 for RGB
    // data).  The size of the data for a scanline is
    // GetWidth() * GetComponents().
    int GetComponents() const
    {
    //  GASSERT(CompressorOpened);
        return CInfo.output_components;
    }


    // Read a scanline's worth of image data into the
    // given Buffer.  The amount of data read is
    // GetWidth() * GetComponents().
    int    ReadScanline(unsigned char* prgbData)
    {
        if (ErrorOccurred) return 0;
        GFC_DEBUG_ERROR(CompressorOpened != true, "GJPEGInput::ReadScanline - image not yet started");
        GFC_JPEG_SET_JMP1(this, 0);
            
    //  GASSERT(cinfo.OutputScanline < cinfo.OutputHeight);
        int LinesRead = jpeg_read_scanlines(&CInfo, &prgbData, 1);
    //  GASSERT(LinesRead == 1);
        LinesRead = LinesRead;  // avoid warning in NDEBUG
        return 1;
    }

    int ReadRawData(void** pprawData)
    {
        if (ErrorOccurred) return 0;
        GFC_JPEG_SET_JMP1(this, 0);
        jvirt_barray_ptr * src_coef_arrays = jpeg_read_coefficients(&CInfo);
        *pprawData = src_coef_arrays;
        return 1;
    }

    void*   GetCInfo() { return &CInfo; }

};

int GJPEGInputImpl_jpeglib::JpegCreateDecompress(jpeg_decompress_struct* pcinfo, GFxJpegErrorHandler* pjerr)
{
    GUNUSED(pjerr); // in case if GFC_NO_LONGJMP is defined.
    GFC_JPEG_SET_JMP(pcinfo, pjerr, 0);

    // Initialize decompression object.
    jpeg_create_decompress(pcinfo);

    return 1;
}

int GJPEGInputImpl_jpeglib::JpegReadHeader(jpeg_decompress_struct* pcinfo,
                                           GFxJpegErrorHandler* pjerr, bool require_image)
{
    GUNUSED(pjerr); // in case if GFC_NO_LONGJMP is defined.
    GFC_JPEG_SET_JMP(pcinfo, pjerr, 0);

    jpeg_read_header(pcinfo, require_image);
    return 1;
}


// Basically this is a thin wrapper around JpegCompress object. 
class GJPEGOutputImpl_jpeglib : public GJPEGOutput
{
public:
    // State needed for GJPEGOutput.
    struct jpeg_compress_struct CInfo;
    struct jpeg_error_mgr       JErr;

    // Constructor.  Read the header data from in, and
    // prepare to read data.
    GJPEGOutputImpl_jpeglib(GFile* out, int width, int height, int quality)
    {
        CInfo.err = jpeg_std_error(&JErr);

        // Initialize decompression object.
        jpeg_create_compress(&CInfo);

        GJPEGUtil_SetupRwDest(&CInfo, out);
        CInfo.image_width       = width;
        CInfo.image_height      = height;
        CInfo.input_components  = 3;
        CInfo.in_color_space    = JCS_RGB;
        jpeg_set_defaults(&CInfo);
        jpeg_set_quality(&CInfo, quality, TRUE);

        jpeg_start_compress(&CInfo, TRUE);
    }

    // AB: to use with CopyCriticalParams only!
    GJPEGOutputImpl_jpeglib(GFile* out)
    {
        CInfo.err = jpeg_std_error(&JErr);

        // Initialize decompression object.
        jpeg_create_compress(&CInfo);

        GJPEGUtil_SetupRwDest(&CInfo, out);
    }


    // Destructor.  Clean up our jpeg reader state.
    ~GJPEGOutputImpl_jpeglib()
    {
        jpeg_finish_compress(&CInfo);
    /*
        RwDest* psrc = (RwSource*) CInfo.dest;
        delete dest;
        CInfo.dest = NULL;
    */
        jpeg_destroy_compress(&CInfo);
    }


    // Write out a single scanline.
    void    WriteScanline(unsigned char* RgbData)
    {
        jpeg_write_scanlines(&CInfo, &RgbData, 1);
    }

    void    WriteRawData(const void* prawData)
    {
        jpeg_write_coefficients(&CInfo, (jvirt_barray_ptr*)prawData);
    }

    void CopyCriticalParams(void* pSrcDecompressInfo)
    {
        jpeg_copy_critical_parameters((struct jpeg_decompress_struct*)pSrcDecompressInfo, &CInfo);
    }

    void*   GetCInfo() { return &CInfo; }
};

class GJPEGSystem_jpeglib : public GJPEGSystem
{
public:
    virtual ~GJPEGSystem_jpeglib() {}
    GJPEGInput* CreateInput(GFile* pin)
    {
        if (!pin || !pin->IsValid()) return 0;
        return GHEAP_AUTO_NEW(this) GJPEGInputImpl_jpeglib(pin);
    }
    GJPEGInput* CreateSwfJpeg2HeaderOnly(GFile* pin)
    {
        if (!pin || !pin->IsValid()) return 0;
        return GHEAP_AUTO_NEW(this)
            GJPEGInputImpl_jpeglib(GJPEGInputImpl_jpeglib::SWF_JPEG2_HEADER_ONLY, pin);
    }
    GJPEGInput* CreateSwfJpeg2HeaderOnly(const UByte* pbuffer, UPInt bufSize)
    {
        return GHEAP_AUTO_NEW(this)
            GJPEGInputImpl_jpeglib(GJPEGInputImpl_jpeglib::SWF_JPEG2_HEADER_ONLY, pbuffer, bufSize);
    }

    GJPEGOutput* CreateOutput(GFile* pout, int width, int height, int quality)
    {
        if (!pout || !pout->IsWritable()) return 0;
        return GHEAP_AUTO_NEW(this) GJPEGOutputImpl_jpeglib(pout, width, height, quality);
    }
    GJPEGOutput* CreateOutput(GFile* pout)
    {
        if (!pout || !pout->IsWritable()) return 0;
        return GHEAP_AUTO_NEW(this) GJPEGOutputImpl_jpeglib(pout);
    }
};

// static
GJPEGSystem* GJPEGSystem::CreateDefaultSystem()
{
    return GNEW GJPEGSystem_jpeglib;
}

#else

// static
GJPEGSystem* GJPEGSystem::CreateDefaultSystem()
{
    return NULL;
}

#endif // GFC_USE_LIBJPEG


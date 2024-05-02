/**********************************************************************

Filename    :   GFxImageResource.cpp
Content     :   Image resource representation for GFxPlayer
Created     :   January 30, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxImageResource.h"
#include "GHeapNew.h"


// ***** GFxImageFileKeyData implementation

class GFxImageFileInfoKeyData : public GRefCountBase<GFxImageFileInfoKeyData, GStat_Default_Mem>
{
    // Image States.  
    GPtr<GFxFileOpener>     pFileOpener;
    GPtr<GFxImageCreator>   pImageCreator;
    // We store image heap as a part of the key to make sure that images
    // in different heaps don't match accidentally, causing incorrect lifetime
    // crashes. Note that right now, this probably won't happen since GFxResourceLib
    // is only accessed internally..
    GMemoryHeap*            pImageHeap;

public:
    // Key File Info - provides file name and image dimensions.
    // Note that this key data is potentially different from the ResourceFileInfo
    // specified in the export because its filename could have been translated
    // and/or adjusted to have a different relative/absolute path.
    GPtr<GFxImageFileInfo>  pFileInfo;

    GFxImageFileInfoKeyData(GFxImageFileInfo* pfileInfo,
                            GFxFileOpener* pfileOpener,
                            GFxImageCreator* pimageCreator,
                            GMemoryHeap *pimageHeap)
    {        
        pFileInfo   = pfileInfo;
        pFileOpener = pfileOpener;
        pImageCreator = pimageCreator;
        pImageHeap  = pimageHeap;
    }

    bool operator == (GFxImageFileInfoKeyData& other) const
    {
        return (pFileOpener == other.pFileOpener &&
                pImageCreator == other.pImageCreator &&
                pImageHeap == other.pImageHeap &&
                pFileInfo->FileName == other.pFileInfo->FileName);
    }
    bool operator != (GFxImageFileInfoKeyData& other) const
    {
        return !operator == (other);
    }

    UPInt  GetHashCode() const
    {
        return pFileInfo->GetHashCode() ^
            ((UPInt)pFileOpener.GetPtr()) ^ (((UPInt)pFileOpener.GetPtr()) >> 7) ^
            ((UPInt)pImageCreator.GetPtr()) ^ (((UPInt)pImageCreator.GetPtr()) >> 7) ^
            ((UPInt)pImageHeap) ^ (((UPInt)pImageHeap) >> 7);
    }
};

class GFxImageFileKeyInterface : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;    

    // GFxResourceKey::KeyInterface implementation.    
    virtual void                AddRef(KeyHandle hdata);  
    virtual void                Release(KeyHandle hdata);
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const;
    virtual UPInt               GetHashCode(KeyHandle hdata) const;
    virtual bool                KeyEquals(KeyHandle hdata, const GFxResourceKey& other);
   // const  GFxResourceFileInfo* GetFileInfo(KeyHandle hdata) const;

     const  char* GetFileURL(KeyHandle hdata) const;
};

static GFxImageFileKeyInterface GFxImageFileKeyInterface_Instance;


// Reference counting on the data object, if necessary.
void    GFxImageFileKeyInterface::AddRef(KeyHandle hdata)
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    pdata->AddRef();
}

void    GFxImageFileKeyInterface::Release(KeyHandle hdata)
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    pdata->Release();
}

// Key/Hash code implementation.
GFxResourceKey::KeyType GFxImageFileKeyInterface::GetKeyType(KeyHandle hdata) const
{
    GUNUSED(hdata);
    return GFxResourceKey::Key_File;
}

UPInt  GFxImageFileKeyInterface::GetHashCode(KeyHandle hdata) const
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    return pdata->GetHashCode();    
}

bool    GFxImageFileKeyInterface::KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
{
    if (this != other.GetKeyInterface())
        return 0;

    GFxImageFileInfoKeyData* pthisData = (GFxImageFileInfoKeyData*) hdata;
    GFxImageFileInfoKeyData* potherData = (GFxImageFileInfoKeyData*) other.GetKeyData();
    GASSERT(pthisData);
    GASSERT(potherData);
    return (*pthisData == *potherData);
}

// Get file info about the resource, potentially available with Key_File.
/*
const GFxResourceFileInfo* GFxImageFileKeyInterface::GetFileInfo(KeyHandle hdata) const
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    return pdata->pFileInfo.GetPtr();
}
*/

const char* GFxImageFileKeyInterface::GetFileURL(KeyHandle hdata) const
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    return pdata->pFileInfo->FileName.ToCStr();
}


// ***** GFxImageResource

GFxResourceKey  GFxImageResource::CreateImageFileKey(GFxImageFileInfo* pfileInfo,
                                                     GFxFileOpener* pfileOpener,
                                                     GFxImageCreator* pimageCreator,
                                                     GMemoryHeap* pimageHeap)
{
    GMemoryHeap* pheap = pimageHeap ? pimageHeap : GMemory::GetGlobalHeap();
    GPtr<GFxImageFileInfoKeyData> pdata =
        *GHEAP_NEW(pheap) GFxImageFileInfoKeyData(pfileInfo, pfileOpener,
                                                  pimageCreator, pimageHeap);

    return GFxResourceKey(&GFxImageFileKeyInterface_Instance,
                          (GFxResourceKey::KeyHandle)pdata.GetPtr() );
}



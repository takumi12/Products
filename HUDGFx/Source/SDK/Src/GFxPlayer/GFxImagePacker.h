/**********************************************************************

Filename    :   GFxImagePacker.h
Content     :   
Created     :   
Authors     :   Andrew Reisse

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXIMAGEPACKER_H
#define INC_GFXIMAGEPACKER_H

#include "GFxMovieDef.h"
#include "GRectPacker.h"

// Track state of packer while binding a movie.

class GFxImagePacker : public GRefCountBaseNTS<GFxImagePacker,GStat_Default_Mem>
{
public:
    virtual void SetBindData(GFxMovieDefImpl::BindTaskData* pbinddata) = 0;
    virtual void AddImageFromResource(GFxImageResource* presource, const char* pexportname) = 0;
    virtual void AddResource(GFxResourceDataNode* presNode, GFxImageResource* presource) = 0;
    virtual void Finish() = 0;
};


class GFxImagePackerImpl : public GFxImagePacker
{
    struct InputImage
    {
        GFxResourceDataNode *pResNode;                  // needed to update binding with subreference to packed texture
        GPtr<GImage>         pImage;
    };
    GHashLH<GFxResource*, GFxResourceDataNode*>         InputResources;
    GArrayLH<InputImage>            InputImages;
    const GFxImagePackParams*     pImpl;
    GFxResourceId*                  pIdGen;             // shared with FontTextures
    GRectPacker                     Packer;
    GFxImageCreator*                pImageCreator;
    GFxImageCreateInfo              ImageCreateInfo;
    GFxMovieDefImpl::BindTaskData*  pBindData;
    //GFxImageSubstProvider* pImageSubsProvider;

    void CopyImage(GImage* pdest, GImageBase* psrc, GRectPacker::RectType rect);

public:
    GFxImagePackerImpl(const GFxImagePackParams* pimpl, GFxResourceId* pidgen,
        GFxImageCreator* pic, GFxImageCreateInfo* pici)
        : pImpl(pimpl), pIdGen(pidgen), pImageCreator(pic), ImageCreateInfo(*pici) { }

    virtual void SetBindData(GFxMovieDefImpl::BindTaskData* pbinddata) { pBindData = pbinddata; }
    virtual void AddImageFromResource(GFxImageResource* presource, const char *pexportname);
    virtual void AddResource(GFxResourceDataNode* presNode, GFxImageResource* presource);
    virtual void Finish();
};

#endif

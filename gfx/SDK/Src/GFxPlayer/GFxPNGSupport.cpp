/**********************************************************************

Filename    :   GFxPNGSupport.cpp
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

///////////// WARNINNG ///////////////////////////////////////////////
// The implementation of GFxPNGSupport class is moved into a separate 
// file to avoid linking PNG functionality into the application if 
// it does't need PNG support and GFxPNGSupport state is not set to 
// the loader

#include "GFxLoader.h"
#include "GImage.h"

GImage* GFxPNGSupport::CreateImage(GFile* pin, GMemoryHeap* pimageHeap)
{
    return GImage::ReadPng(pin, pimageHeap);
}

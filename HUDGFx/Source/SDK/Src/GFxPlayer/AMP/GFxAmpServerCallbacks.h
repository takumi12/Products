/**********************************************************************

Filename    :   GFxAmpSendThreadCallback.h
Content     :   AMP server interface implementations
Created     :   January 2010
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2010 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXAMP_SEND_THREAD_CALLBACK_H
#define INC_GFXAMP_SEND_THREAD_CALLBACK_H

#include "GRefCount.h"
#include "GFxAmpInterfaces.h"
#include "GFxGlyphParam.h"

class GEvent;

// GFxAmpSendThreadCallback::OnSendLoop is called once per "frame" 
// from the GFxAmpThreadManager send thread
//
class GFxAmpSendThreadCallback : 
    public GRefCountBase<GFxAmpSendThreadCallback, GStat_Default_Mem>, 
    public GFxAmpSendInterface
{
public:
    virtual ~GFxAmpSendThreadCallback() { }
    virtual bool OnSendLoop();
};

// GFxAmpStatusChangedCallback::OnStatusChanged is called by GFxAmpThreadManager 
// whenever a change in the connection status has been detected
//
class GFxAmpStatusChangedCallback : public GRefCountBase<GFxAmpStatusChangedCallback, GStat_Default_Mem>, public GFxAmpConnStatusInterface
{
public:
    GFxAmpStatusChangedCallback(GEvent* connectedEvent = NULL);
    virtual ~GFxAmpStatusChangedCallback() { }
    virtual void OnStatusChanged(StatusType newStatus, StatusType oldStatus, const char* message);
    virtual void OnMsgVersionMismatch(int otherVersion);
    virtual void OnMsgGFxVersionChanged(GFxAmpMessage::GFxVersionType newVersion);

protected:
    GEvent* ConnectedEvent;   // notifies GFx that connection has been established
};


class GFxAmpGlyphCacheEventHandler : public GRefCountBase<GFxAmpSendThreadCallback, GStat_Default_Mem>, public GFxGlyphCacheEventHandler
{
public:
    GFxAmpGlyphCacheEventHandler() : Thrashing(0), Failures(0) {}

    virtual void OnEvictGlyph(const GFxGlyphParam& param, const GRectF& uvRect, UInt textureIdx)
    {
        GUNUSED3(param, uvRect, textureIdx);
        ++Thrashing;
    }

    virtual void OnUpdateGlyph(const GFxGlyphParam& param, const GRectF& uvRect, UInt textureIdx,
        const UByte* img, UInt imgW, UInt imgH, UInt imgPitch)
    {
        GUNUSED3(param, uvRect, textureIdx);
        GUNUSED4(img, imgW, imgH, imgPitch);
    }

    virtual void OnFailure(const GFxGlyphParam& param, UInt imgW, UInt imgH)
    {
        GUNUSED3(param, imgW, imgH);
        ++Failures;
    }

    UInt32 GetThrashing() const
    {
        return Thrashing;
    }

    UInt32 GetFailures() const
    {
        return Failures;
    }

    void ResetStats()
    {
        Thrashing = 0;
        Failures = 0;
    }

private:
    GAtomicInt<UInt32>  Thrashing;
    GAtomicInt<UInt32>  Failures;
};


#endif

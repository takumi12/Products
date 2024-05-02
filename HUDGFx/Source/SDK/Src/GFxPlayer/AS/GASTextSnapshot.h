/**********************************************************************

Filename    :   GFxASTextSnapshot.h
Content     :   TextSnapshot reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GASTextSnapshot_H
#define INC_GASTextSnapshot_H

#include "GConfig.h"
#include "GFxStaticText.h"

#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

#include "AS/GASObject.h"
#include "AS/GASObjectProto.h"

// ***** Declared Classes
class GASTextSnapshotObject;
class GASTextSnapshotProto;
class GASTextSnapshotCtorFunction;

class GFxSprite;

// 
// Actionscript TextSnapshot object declaration
//

class GASTextSnapshotObject : public GASObject
{
    friend class GASTextSnapshotProto;
    friend class GASTextSnapshotCtorFunction;

#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        SnapshotData.~GFxStaticTextSnapshotData();
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
protected:

    GFxStaticTextSnapshotData       SnapshotData;

    GASTextSnapshotObject(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }

    virtual         ~GASTextSnapshotObject();

public:

    GASTextSnapshotObject(GASEnvironment* penv);

    virtual ObjectType      GetObjectType() const { return Object_TextSnapshot; }

    void                    Process(GFxSprite* pmovieClip);
    
    GINLINE const GFxStaticTextSnapshotData&    GetData()   { return SnapshotData; }
    GINLINE void            SetSelectColor(const GColor& color) { SnapshotData.SetSelectColor(color); }
};


class GASTextSnapshotProto : public GASPrototype<GASTextSnapshotObject>
{
public:
    GASTextSnapshotProto(GASStringContext *psc, GASObject* prototype, 
        const GASFunctionRef& constructor);

    static const GASNameFunction FunctionTable[];

    static void             FindText(const GASFnCall& fn);
    static void             GetCount(const GASFnCall& fn);
    static void             GetSelected(const GASFnCall& fn);
    static void             GetSelectedText(const GASFnCall& fn);
    static void             GetText(const GASFnCall& fn);
    static void             GetTextRunInfo(const GASFnCall& fn);
    static void             HitTestTextNearPos(const GASFnCall& fn);
    static void             SetSelectColor(const GASFnCall& fn);
    static void             SetSelected(const GASFnCall& fn);
};


class GASTextSnapshotCtorFunction : public GASCFunctionObject
{
public:
    GASTextSnapshotCtorFunction(GASStringContext *psc);

    static GASFunctionRef Register(GASGlobalContext* pgc);
};


#endif  // GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

#endif  // INC_GASTextSnapshot_H

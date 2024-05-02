/**********************************************************************

Filename    :   AS/GASMovieClipLoader.h
Content     :   Implementation of MovieClipLoader class
Created     :   March, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXMOVIECLIPLOADER_H
#define INC_GFXMOVIECLIPLOADER_H

#include "GConfig.h"
#include "AS/GASObject.h"
#ifndef GFC_NO_FXPLAYER_AS_MOVIECLIPLOADER
#include "GFxAction.h"
#include "GFxString.h"
#include "GFxStringHash.h"
#include "GFxCharacter.h"
#include "AS/GASObjectProto.h"


// ***** Declared Classes
class GASMovieClipLoader;
class GASMovieClipLoaderProto;
class GASMovieClipLoaderCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASMovieClipLoader : public GASObject
{
    friend class GASMovieClipLoaderProto;

    struct ProgressDesc
    {
        int LoadedBytes;
        int TotalBytes;

        ProgressDesc() {}
        ProgressDesc(int loadedBytes, int totalBytes): LoadedBytes(loadedBytes), TotalBytes(totalBytes) {}
    };
    GFxStringHashLH<ProgressDesc> ProgressInfo;
    void commonInit (GASEnvironment* penv);
#ifndef GFC_NO_GC
protected:
    virtual void Finalize_GC()
    {
        ProgressInfo.~GFxStringHashLH();
        GASObject::Finalize_GC();
    }
#endif //GFC_NO_GC
protected:
    GASMovieClipLoader(GASStringContext *psc, GASObject* pprototype)
        : GASObject(psc)
    { 
        Set__proto__(psc, pprototype); // this ctor is used for prototype obj only
    }
public:
    GASMovieClipLoader(GASEnvironment* penv);

    ObjectType      GetObjectType() const   { return Object_MovieClipLoader; }

    virtual void NotifyOnLoadStart(GASEnvironment* penv, GFxASCharacter* ptarget);
    virtual void NotifyOnLoadComplete(GASEnvironment* penv, GFxASCharacter* ptarget, int status);
    virtual void NotifyOnLoadInit(GASEnvironment* penv, GFxASCharacter* ptarget);
    virtual void NotifyOnLoadError(GASEnvironment* penv, GFxASCharacter* ptarget, const char* errorCode, int status);
    virtual void NotifyOnLoadProgress(GASEnvironment* penv, GFxASCharacter* ptarget, int loadedBytes, int totalBytes);

    int GetLoadedBytes(GFxASCharacter* pch) const;
    int GetTotalBytes(GFxASCharacter* pch)  const;
};

class GASMovieClipLoaderProto : public GASPrototype<GASMovieClipLoader>
{
public:
    GASMovieClipLoaderProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GetProgress(const GASFnCall& fn);
    static void LoadClip(const GASFnCall& fn);
    static void UnloadClip(const GASFnCall& fn);
};

class GASMovieClipLoaderCtorFunction : public GASCFunctionObject
{
public:
    GASMovieClipLoaderCtorFunction (GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASEnvironment* penv) const;    

    static void GlobalCtor(const GASFnCall& fn);

    static GASFunctionRef Register(GASGlobalContext* pgc);
};

#else
class GASMovieClipLoader : public GASObject
{
public:
    GASMovieClipLoader(GASStringContext * psc) : GASObject(psc) {}
    void NotifyOnLoadStart(GASEnvironment*, GFxASCharacter*) {}
    void NotifyOnLoadComplete(GASEnvironment*, GFxASCharacter*, int) {}
    void NotifyOnLoadInit(GASEnvironment*, GFxASCharacter*) {}
    void NotifyOnLoadError(GASEnvironment*, GFxASCharacter*, const char*, int) {}
    void NotifyOnLoadProgress(GASEnvironment*, GFxASCharacter*, int, int) {}
};
#endif //#ifndef GFC_NO_FXPLAYER_AS_MOVIECLIPLOADER

#endif // INC_GFXMOVIECLIPLOADER_H

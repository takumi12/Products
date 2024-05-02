/**********************************************************************

Filename    :   GFxPlayerImpl.cpp
Content     :   MovieRoot and Definition classes
Created     :   
Authors     :   Michael Antonov, Artem Bolgar

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GFile.h"
#include "GImage.h"
#include "GJPEGUtil.h"
#include "GFunctions.h"

#include "GRenderer.h"
#include "GUTF8Util.h"

#include "AS/GASObject.h"
#include "GFxAction.h"
#include "AS/GASArrayObject.h"
#include "GFxButton.h"
#include "AS/GASMouse.h"
#include "GFxPlayerImpl.h"
#include "GFxLoaderImpl.h"
#include "GFxLoadProcess.h"
#include "GFxRenderGen.h"

#include "GFxFontResource.h"
#include "GFxLog.h"
#include "GFxMorphCharacter.h"
#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxStyles.h"
#include "GFxDlist.h"
#include "AS/GASTimers.h"
#include "GFxSprite.h"
#include "AS/GASStage.h"
#include "AS/GASSelection.h"

#include "GFxDisplayContext.h"

#include <GFxIMEManager.h>

#include "GASAsFunctionObject.h"
#include "GFxAudio.h"
#include "GFxVideoBase.h"
#include "GASSoundObject.h"
#include "GMsgFormat.h"

#include "GFxPlayerStats.h"
#include "GFxAmpServer.h"
#include "AMP/GFxAmpViewStats.h"

#include <string.h> // for memset
#include <float.h>
#include <stdlib.h>
#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif

#ifdef GFC_BUILD_LOGO
#include "GFxLogo.cpp"
#endif

// Increment this when the cache data format changes.
#define CACHE_FILE_VERSION  4


// ***** Statistics declarations

// GFxMovieDef Memory.
GDECLARE_MEMORY_STAT_AUTOSUM_GROUP(GFxStatMD_Mem, "MovieDef", GStat_Mem)
GDECLARE_MEMORY_STAT(GFxStatMD_CharDefs_Mem,  "CharDefs", GFxStatMD_Mem)
GDECLARE_MEMORY_STAT(GFxStatMD_ShapeData_Mem, "ShapeData",GFxStatMD_Mem)
GDECLARE_MEMORY_STAT(GFxStatMD_Tags_Mem,      "Tags",     GFxStatMD_Mem)
GDECLARE_MEMORY_STAT(GFxStatMD_Fonts_Mem,     "Fonts",    GFxStatMD_Mem)
GDECLARE_MEMORY_STAT(GFxStatMD_Images_Mem,    "Images",   GFxStatMD_Mem)
#ifndef GFC_NO_SOUND
GDECLARE_MEMORY_STAT(GFxStatMD_Sounds_Mem,    "Sounds",   GFxStatMD_Mem)
#endif
GDECLARE_MEMORY_STAT(GFxStatMD_ActionOps_Mem, "ActionOps",   GFxStatMD_Mem)
GDECLARE_MEMORY_STAT(GFxStatMD_Other_Mem,     "MD_Other",   GFxStatMD_Mem)
// Load timings.
GDECLARE_TIMER_STAT_SUM_GROUP(GFxStatMD_Time, "Time",     GStatGroup_Default)
GDECLARE_TIMER_STAT(GFxStatMD_Load_Tks,       "Load",     GFxStatMD_Time)
GDECLARE_TIMER_STAT(GFxStatMD_Bind_Tks,       "Bind",     GFxStatMD_Time)
      
// MovieView Memory.
GDECLARE_MEMORY_STAT_AUTOSUM_GROUP(GFxStatMV_Mem,   "MovieView",    GStat_Mem)
GDECLARE_MEMORY_STAT(GFxStatMV_MovieClip_Mem,       "MovieClip",    GFxStatMV_Mem)
GDECLARE_MEMORY_STAT(GFxStatMV_ActionScript_Mem,    "ActionScript", GFxStatMV_Mem)
GDECLARE_MEMORY_STAT(GFxStatMV_Text_Mem,            "Text",         GFxStatMV_Mem)
GDECLARE_MEMORY_STAT(GFxStatMV_XML_Mem,             "XML",          GFxStatMV_Mem)
GDECLARE_MEMORY_STAT(GFxStatMV_Other_Mem,           "MV_Other",     GFxStatMV_Mem)

GDECLARE_MEMORY_STAT(GFxStatIME_Mem,                "IME",          GStat_Mem)

GDECLARE_MEMORY_STAT_AUTOSUM_GROUP(GFxStatFC_Mem,   "FontCache",    GStat_Mem)
GDECLARE_MEMORY_STAT(GFxStatFC_Batch_Mem,           "Batches",      GFxStatFC_Mem)
GDECLARE_MEMORY_STAT(GFxStatFC_GlyphCache_Mem,      "GlyphCache",   GFxStatFC_Mem)
GDECLARE_MEMORY_STAT(GFxStatFC_Other_Mem,           "FC_Other",     GFxStatFC_Mem)


// MovieView Timings.
GDECLARE_TIMER_STAT_AUTOSUM_GROUP(GFxStatMV_Tks,            "Ticks",          GStatGroup_Default)
GDECLARE_TIMER_STAT_SUM_GROUP(    GFxStatMV_Advance_Tks,    "Advance",        GFxStatMV_Tks)
GDECLARE_TIMER_STAT_SUM_GROUP(    GFxStatMV_Action_Tks,     "Action",         GFxStatMV_Advance_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_Timeline_Tks,   "Timeline",       GFxStatMV_Advance_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_Input_Tks,      "Input",          GFxStatMV_Advance_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_Mouse_Tks,      "Mouse",          GFxStatMV_Input_Tks)
GDECLARE_TIMER_STAT_AUTOSUM_GROUP(GFxStatMV_ScriptCommunication_Tks, "Script Communication",  GFxStatMV_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_GetVariable_Tks,"GetVariable",    GFxStatMV_ScriptCommunication_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_SetVariable_Tks,"SetVariable",    GFxStatMV_ScriptCommunication_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_Invoke_Tks,     "Invoke",         GFxStatMV_ScriptCommunication_Tks)

GDECLARE_TIMER_STAT_SUM_GROUP(    GFxStatMV_Display_Tks,    "Display",        GFxStatMV_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_Tessellate_Tks,    "Tessellate",  GFxStatMV_Display_Tks)
GDECLARE_TIMER_STAT(              GFxStatMV_GradientGen_Tks,    "GradientGen",GFxStatMV_Display_Tks)



static inline GString GetUrlStrGfx(const GString& url)
{
    GString urlStrGfx;
    if ((url.GetSize() > 4) &&
        (GString::CompareNoCase(url.ToCStr() + (url.GetSize() - 4), ".swf") == 0) )
    {
        urlStrGfx.Clear();
        urlStrGfx.AppendString(url.ToCStr(), url.GetSize() - 4);
        urlStrGfx += ".gfx";
    }
    return urlStrGfx;
}

// 
GFxMoviePreloadTask::GFxMoviePreloadTask(GFxMovieRoot* pmovieRoot, const GString& url, bool stripped, bool quietOpen)
    : GFxTask(GFxTask::Id_MovieDataLoad), Url(url), Done(0)
{
    pLoadStates = *GNEW GFxLoadStates(pmovieRoot->pLevel0Def->pLoaderImpl, pmovieRoot->GetStateBagImpl());
    LoadFlags = pmovieRoot->pLevel0Def->GetLoadFlags() | GFxLoader::LoadImageFiles;
    // we don't want to wait for anything
    LoadFlags &= ~(GFxLoader::LoadWaitFrame1 | GFxLoader::LoadWaitCompletion);
    if (quietOpen)
        LoadFlags |= GFxLoader::LoadQuietOpen;
    pmovieRoot->GetLevel0Path(&Level0Path);
    if (stripped)
        UrlStrGfx = GetUrlStrGfx(Url);
}

void GFxMoviePreloadTask::Execute()
{
    if (UrlStrGfx.GetLength() > 0)
    {
        GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadMovie, UrlStrGfx, Level0Path);
        pDefImpl = *GFxLoaderImpl::CreateMovie_LoadState(pLoadStates, loc, LoadFlags);
    }
    if (!pDefImpl) 
    {
        GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadMovie, Url, Level0Path);
        pDefImpl = *GFxLoaderImpl::CreateMovie_LoadState(pLoadStates, loc, LoadFlags);
    }
    GAtomicOps<UInt>::Store_Release(&Done, 1);
}

bool GFxMoviePreloadTask::IsDone() const 
{
    UInt done = GAtomicOps<UInt>::Load_Acquire(&Done);
    return done == 1;
}

GFxMovieDefImpl* GFxMoviePreloadTask::GetMoiveDefImpl()
{
    return pDefImpl;
}

// ****** GFxLoadingMovieEntry

GFxLoadQueueEntryMT::GFxLoadQueueEntryMT(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot)
    : pNext(NULL), pPrev(NULL), pMovieRoot(pmovieRoot), pQueueEntry(pqueueEntry)
{
}

GFxLoadQueueEntryMT::~GFxLoadQueueEntryMT()
{
    delete pQueueEntry;
}

void GFxLoadQueueEntryMT::Cancel()
{
    pQueueEntry->Canceled = true;
}

GFxLoadQueueEntryMT_LoadMovie::GFxLoadQueueEntryMT_LoadMovie(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot)
    : GFxLoadQueueEntryMT(pqueueEntry, pmovieRoot),
      NewCharId(GFxCharacterDef::CharId_EmptyMovieClip), CharSwitched(false), 
      BytesLoaded(0), FirstFrameLoaded(false)
{
    bool stripped = false;

    if (pQueueEntry->pCharacter)
    {
        // This is loadMovie, not level.
        GASSERT(pQueueEntry->Level == -1);
        GASSERT(!(pQueueEntry->Type & GFxLoadQueueEntry::LTF_LevelFlag));

        // Make sure we have character targets and parents.
        GPtr<GFxASCharacter> poldChar = pQueueEntry->pCharacter->ResolveCharacter(pMovieRoot);
        if (poldChar)
            stripped = (poldChar->GetResourceMovieDef()->GetSWFFlags() 
                                                       & GFxMovieInfo::SWF_Stripped) != 0;
    }
    else if (pQueueEntry->Level != -1)
    {
        GASSERT(pQueueEntry->Type & GFxLoadQueueEntry::LTF_LevelFlag);
        if (pMovieRoot->GetLevelMovie(pQueueEntry->Level))
            stripped = ((pMovieRoot->GetLevelMovie(pQueueEntry->Level)->GetResourceMovieDef()->GetSWFFlags()
            & GFxMovieInfo::SWF_Stripped) != 0);
        else if (pMovieRoot->GetLevelMovie(0))
            stripped = ((pMovieRoot->GetLevelMovie(0)->GetResourceMovieDef()->GetSWFFlags()
            & GFxMovieInfo::SWF_Stripped) != 0);
    }
    pPreloadTask = *GNEW GFxMoviePreloadTask(pMovieRoot, pQueueEntry->URL, stripped, pqueueEntry->QueitOpen);
    pMovieRoot->GetTaskManager()->AddTask(pPreloadTask);
}

GFxLoadQueueEntryMT_LoadMovie::~GFxLoadQueueEntryMT_LoadMovie()
{
}

// Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
// Returns true if the movie is loaded completely or in the case of errors.
bool GFxLoadQueueEntryMT_LoadMovie::LoadFinished()
{
    bool btaskDone = pPreloadTask->IsDone();
    // If canceled, wait until task is done
    // [PPS] Change this logic to call AbandonTask and 
    // handle it appropriately inside the task to die
    // instantly
    if (pQueueEntry->Canceled && btaskDone)
        return true;

    if (btaskDone)
    {
        // make sure that the character that we want to replace is alive.
        if (!pOldChar && pQueueEntry->pCharacter)
        {
            //!AB: We need to get the character by name first time. It is kind of weird but
            // it seems that is how Flash works. For example, we have a movieclip 'mc'
            // and it already contains child 'holder'. Then we create an empty movie clip
            // within 'mc' and call it 'holder' as well. If we resolve string 'mc.holder' to
            // a character when the first (native) 'holder' will be returned. However, if we 
            // use returning value from createEmptyMovieClip we will have a pointer to the new
            // 'holder'. Now, we call loadMovie using the pointer returned from createEmptyMovieClip.
            // We expect, that movie will be loaded into newly created 'holder'. However, it loads
            // it into the native 'holder'. So, looks like the implementation of 'loadMovie' always
            // resolve character by name. Test file: Test\LoadMovie\test_loadmovie_with_two_same_named_clips.swf 
            pOldChar = pQueueEntry->pCharacter->ForceResolveCharacter(pMovieRoot);
            if (!pOldChar)
            {
                GFxSprite* pmovie0 = pMovieRoot->GetLevelMovie(0);
                if (pmovie0)
                {
                    GASEnvironment* penv = pmovie0->GetASEnvironment();
                    GASSERT(penv);
                    GASMovieClipLoader* pmovieClipLoader = 
                        static_cast<GASMovieClipLoader*>(pQueueEntry->MovieClipLoaderHolder.ToObject(penv));
                    if (pmovieClipLoader)
                        pmovieClipLoader->NotifyOnLoadError(penv, NULL, "Error", 0);
                }
                return true;
            }
            //AB: if loading in the character then we need to assign the
            // same Id as the old character had; this is important to avoid
            // re-creation of the original character when timeline rollover 
            // of the parent character occurs.
            NewCharId = pOldChar->GetId();
        }
        // prepare env and listener object.
        GFxSprite* pmovie0 = pMovieRoot->GetLevelMovie(0);
        // Just finish loading if _level0 has already been unloaded
        if (!pmovie0)
            return true;

        GASEnvironment* penv0 = pmovie0->GetASEnvironment();
        GASSERT(penv0);
        GASMovieClipLoader* pmovieClipLoader = 
            static_cast<GASMovieClipLoader*>(pQueueEntry->MovieClipLoaderHolder.ToObject(penv0));
        bool extensions = penv0->CheckExtensions();
        penv0   = NULL; // can't use later, might be invalid!
        pmovie0 = NULL; // can't use later, might be invalid (if loading into _level0)

        // Preload task is finished by now. Check if a movedef is created
        GFxMovieDefImpl* pdefImpl = pPreloadTask->GetMoiveDefImpl();
        if (!pdefImpl)
        {
            // A moviedef was not created. This means that loader couldn't load a file. 
            // Creating an empty movie and set it instead of the old one.
            if (pQueueEntry->pCharacter)
            {
                GFxASCharacter* pparent = pOldChar->GetParent();
                // make sure that the parent of the character that we want to
                // replace is still alive.
                if (!pparent)
                    return true;
                GFxCharacterCreateInfo ccinfo =
                    pparent->GetResourceMovieDef()->GetCharacterCreateInfo
                    (GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip));
                GPtr<GFxSprite> emptyChar = *(GFxSprite*)ccinfo.pCharDef->CreateCharacterInstance(pparent, NewCharId,
                                                                                                  ccinfo.pBindDefImpl);
                //AB: we need to replicate the same CreateFrame, Depth and the Name.
                // Along with replicated Id this is important to maintain the loaded
                // movie on the stage. Otherwise, once the parent clip reaches the end
                // of its timeline the original movieclip will be recreated. Flash
                // doesn't recreate the original movie clip in this case, and it also
                // doesn't stop timeline animation for the target movieclip. The only way
                // to achieve both of these features is to copy Id, CreateFrame, Depth and
                // Name from the old character to the new one.
                // See last_to_first_frame_mc_no_reloadX.fla and 
                // test_timeline_anim_after_loadmovie.swf
                emptyChar->SetCreateFrame(pOldChar->GetCreateFrame());
                emptyChar->SetDepth(pOldChar->GetDepth());
                if (!pOldChar->HasInstanceBasedName())
                    emptyChar->SetName(pOldChar->GetName());
                emptyChar->AddToPlayList(pMovieRoot);
                pparent->ReplaceChildCharacterOnLoad(pOldChar, emptyChar);
                pOldChar = emptyChar;
            } else if (pQueueEntry->Level != -1)
                pOldChar = pMovieRoot->GetLevelMovie(pQueueEntry->Level);
            if (pOldChar && pmovieClipLoader)
            {
                GASEnvironment* penv = pOldChar->GetASEnvironment();
                GASSERT(penv);
                pmovieClipLoader->NotifyOnLoadError(penv, pOldChar, "URLNotFound", 0);
            }
            if (pQueueEntry->Level != -1)
                pMovieRoot->ReleaseLevelMovie(pQueueEntry->Level);
            return true;
        }
        else 
        {
            // check if the movie is already unloaded or being unloaded. This is possible when
            // loadMovie is initiated and then the container movie is removed. In this case 
            // one of the _parent of pOldChar will be a dangling pointer. Thus, if the movie
            // is unloaded already - do nothing.
            if (pOldChar && (pOldChar->IsUnloaded() || pOldChar->IsUnloading()))
                return true;

            // A moviedef was created successfully.
            if (!CharSwitched)
            {
                //Replace the old character with the new sprite. This operation should be done only once
                //per loading movie.
                GFxASCharacter* pparent = NULL;
                if (pQueueEntry->Level != -1)
                {
                    pMovieRoot->ReleaseLevelMovie(pQueueEntry->Level);
                    NewCharId = GFxResourceId(); // Assign invalid id.
                } 
                else if (pQueueEntry->pCharacter)
                {
                    pparent = pOldChar->GetParent();
                    // make sure that the parent of the character that we want to
                    // replace is still alive.
                    if (!pparent)
                        return true;
                }
                GPtr<GFxSprite> pnewChar = *GHEAP_NEW(pMovieRoot->GetMovieHeap())
                                                      GFxSprite(pdefImpl->GetDataDef(), pdefImpl,
                                                      pMovieRoot, pparent, NewCharId, true);

                if (!extensions)
                    pnewChar->SetNoAdvanceLocalFlag();

                if (pQueueEntry->pCharacter)
                {
                    pnewChar->AddToPlayList(pMovieRoot);
                    //AB: we need to replicate the same CreateFrame, Depth and the Name.
                    // Along with replicated Id this is important to maintain the loaded
                    // movie on the stage. Otherwise, once the parent clip reaches the end
                    // of its timeline the original movieclip will be recreated. Flash
                    // doesn't recreate the original movie clip in this case, and it also
                    // doesn't stop timeline animation for the target movieclip. The only way
                    // to achieve both of these features is to copy Id, CreateFrame, Depth and
                    // Name from the old character to the new one.
                    // See last_to_first_frame_mc_no_reloadX.fla and 
                    // test_timeline_anim_after_loadmovie.swf
                    pnewChar->SetCreateFrame(pOldChar->GetCreateFrame());
                    pnewChar->SetDepth(pOldChar->GetDepth());
                    if (!pOldChar->HasInstanceBasedName())
                        pnewChar->SetName(pOldChar->GetName());
                    pparent->ReplaceChildCharacter(pOldChar, pnewChar);
                }
                else 
                {
                    pnewChar->SetLevel(pQueueEntry->Level);
                    pMovieRoot->SetLevelMovie(pQueueEntry->Level, pnewChar);
                    // we will execute action script for frame 0 ourself here
                    // this is needed because this script has to be executed 
                    // before calling OnLoadInit
                    G_SetFlag<GFxMovieRoot::Flag_LevelClipsChanged>(pMovieRoot->Flags, false);
                }
                // we need to disable calling advance on this movie until its first frame is loaded
                pnewChar->SetPlayState(GFxMovie::Stopped);

                pOldChar = pnewChar;
                // If we have a movie loader listener send notifications.
                if (pmovieClipLoader)
                {
                    GASEnvironment* penv = pOldChar->GetASEnvironment();
                    GASSERT(penv);
                    pmovieClipLoader->NotifyOnLoadStart(penv, pOldChar);
                    BytesLoaded = pdefImpl->GetBytesLoaded();
                    pmovieClipLoader->NotifyOnLoadProgress(penv, pOldChar, BytesLoaded, pdefImpl->GetFileBytes());
                }
                CharSwitched = true;
            }

            // The number of loaded bytes has changed since the last iteration and we have a movie loader listener
            // sent a progress notification.
            if (BytesLoaded != pdefImpl->GetBytesLoaded() && pmovieClipLoader)
            {
                GASEnvironment* penv = pOldChar->GetASEnvironment();
                GASSERT(penv);
                BytesLoaded = pdefImpl->GetBytesLoaded();
                pmovieClipLoader->NotifyOnLoadProgress(penv, pOldChar, BytesLoaded, pdefImpl->GetFileBytes());
            }
            // when we loaded the first frame process its actions.
            if (extensions && !FirstFrameLoaded && ((pdefImpl->pBindData->GetBindState() & GFxMovieDefImpl::BSF_Frame1Loaded) != 0))
            {
                // This branch will be executed when gfxExtensions == true, to 
                // produce different (better) behavior during the loading. If extensions
                // are on then we can start the loading movie after the first frame is loaded.
                // Flash, for some reasons, waits until whole movie is loaded.
                GPtr<GFxSprite> psprite;
                if (pQueueEntry->Level == -1)
                    psprite = pOldChar->ToSprite();
                else
                    psprite = pMovieRoot->GetLevelMovie(pQueueEntry->Level);
                GASSERT(psprite.GetPtr());
                if (psprite)
                {
                    psprite->SetPlayState(GFxMovie::Playing);
                    psprite->SetRootNodeLoadingStat(pdefImpl->GetBytesLoaded(), pdefImpl->GetLoadingFrame());
                    //psprite->OnEventLoad();
                    //!AB: note, we can't use OnEventLoad here, need to use ExecuteFrame0Events. 
                    // ExecuteFrame0Events executes onLoad event AFTER the first frame and Flash
                    // does exactly the same.
                    psprite->ExecuteFrame0Events();
                    pMovieRoot->DoActions();
                    if (pmovieClipLoader)
                    {
                        GASEnvironment* penv = psprite->GetASEnvironment();
                        GASSERT(penv);
                        pmovieClipLoader->NotifyOnLoadInit(penv, psprite);
                    }
                }
                FirstFrameLoaded = true;
            }
            // If the movie is not completely loaded yet try it on the next iteration.
            if ((pdefImpl->pBindData->GetBindState() 
                 & (GFxMovieDefImpl::BS_InProgress | GFxMovieDefImpl::BS_Finished)) < GFxMovieDefImpl::BS_Finished)
                return false;
            if (pdefImpl->pBindData->GetBindState() & GFxMovieDefImpl::BS_Finished)
            {
                GPtr<GFxSprite> psprite;
                if (pQueueEntry->Level == -1)
                {
                    GASSERT(pOldChar.GetPtr());
                    psprite = pOldChar->ToSprite();
                    pMovieRoot->ResolveStickyVariables(pOldChar);
                }
                else
                    psprite = pMovieRoot->GetLevelMovie(pQueueEntry->Level);
                GASSERT(psprite.GetPtr());

                if (psprite)
                {
                    if (!extensions)
                        psprite->ClearNoAdvanceLocalFlag();
                    // movie is loaded completely here. Send notifications for a listener.
                    if (pmovieClipLoader)
                    {
                        GASEnvironment* penv = psprite->GetASEnvironment();
                        GASSERT(penv);
                        pmovieClipLoader->NotifyOnLoadComplete(penv, psprite, 0);
                    }
                    // If we did not process the actions in the first frame do it now
                    if (!FirstFrameLoaded)
                    {
                        psprite->SetPlayState(GFxMovie::Playing);
                        psprite->SetRootNodeLoadingStat(pdefImpl->GetBytesLoaded(), pdefImpl->GetLoadingFrame());
                        //psprite->OnEventLoad();
                        //!AB: note, we can't use OnEventLoad here, need to use ExecuteFrame0Events. 
                        // ExecuteFrame0Events executes onLoad event AFTER the first frame and Flash
                        // does exactly the same.
                        psprite->ExecuteFrame0Events();
                        pMovieRoot->DoActions();
                        if (pmovieClipLoader)
                        {
                            GASEnvironment* penv = psprite->GetASEnvironment();
                            GASSERT(penv);
                            pmovieClipLoader->NotifyOnLoadInit(penv, psprite);
                        }
                    }
                    FirstFrameLoaded = true;
                }
            } 
            else 
            {
                // A error happened during the movie loading 
                if (pmovieClipLoader)
                {
                    GASSERT(pOldChar.GetPtr());
                    GASEnvironment* penv = pOldChar->GetASEnvironment();
                    GASSERT(penv);
                    if (pdefImpl->GetLoadState() == GFxMovieDataDef::LS_LoadError)
                        pmovieClipLoader->NotifyOnLoadError(penv, pOldChar, "Error", 0);
                    else 
                        pmovieClipLoader->NotifyOnLoadError(penv, pOldChar, "Canceled", 0);
                }
            }
            // Loading finished. Remove the item from the queue.
            return true;
        }
    }
    // Preload task is not finished yet. Check again on the next iteration.
    return false;
}

bool GFxLoadQueueEntryMT_LoadMovie::IsPreloadingFinished()
{
    return pPreloadTask->IsDone();
}

#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
/////////////////// load vars /////////////////
GFxLoadVarsTask::GFxLoadVarsTask(GFxLoadStates* pls, const GString& level0Path, const GString& url)
    : GFxTask(GFxTask::Id_MovieDataLoad),
      pLoadStates(pls), Level0Path(level0Path), Url(url), FileLen(0), Done(0), Succeeded(false)
{
}

bool GFx_ReadLoadVariables(GFile* pfile, GString* pdata, SInt* pfileLen);

void GFxLoadVarsTask::Execute()
{
    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadVars, Url, Level0Path);
    GString                   fileName;
    pLoadStates->BuildURL(&fileName, loc);

   // File loading protocol
    GPtr<GFile> pfile;
    pfile = *pLoadStates->OpenFile(fileName.ToCStr());
    if (pfile)
        Succeeded = GFx_ReadLoadVariables(pfile, &Data, &FileLen);
    else
        Succeeded = true;

    GAtomicOps<UInt>::Store_Release(&Done, 1);
}
bool GFxLoadVarsTask::GetData(GString* data, SInt* fileLen, bool* succeeded) const
{
    GASSERT(data);
    GASSERT(fileLen);
    UInt done = GAtomicOps<UInt>::Load_Acquire(&Done);
    if (done != 1)
        return false;

    *data = Data;
    *fileLen = FileLen;
    *succeeded = Succeeded;

    return true;
}

GFxLoadQueueEntryMT_LoadVars::GFxLoadQueueEntryMT_LoadVars(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot)
    : GFxLoadQueueEntryMT(pqueueEntry,pmovieRoot)
{
    pLoadStates = *new GFxLoadStates(pMovieRoot->pLevel0Def->pLoaderImpl, pMovieRoot->GetStateBagImpl());
    GString level0Path;
    pMovieRoot->GetLevel0Path(&level0Path);
    pTask = *GNEW GFxLoadVarsTask(pLoadStates, level0Path,pqueueEntry->URL);
    pMovieRoot->GetTaskManager()->AddTask(pTask);
}

GFxLoadQueueEntryMT_LoadVars::~GFxLoadQueueEntryMT_LoadVars()
{
}

// Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
// Returns true if the movie is loaded completely or in the case of errors.
bool GFxLoadQueueEntryMT_LoadVars::LoadFinished()
{
    GString data;
    SInt      fileLen;
    bool      succeeded;
    
    bool btaskDone = pTask->GetData(&data,&fileLen,&succeeded);
    // If canceled, wait until task is done
    // [PPS] Change this logic to call AbandonTask and 
    // handle it appropriately inside the task to die
    // instantly
    if (pQueueEntry->Canceled && btaskDone)
        return true;
    if (!btaskDone)
        return false;    

    if (!succeeded && pQueueEntry->LoadVarsHolder.IsObject())
        pQueueEntry->LoadVarsHolder.DropRefs();
    pMovieRoot->LoadVars(pQueueEntry,pLoadStates,data,fileLen);
    return true;
}
#endif //#ifndef GFC_NO_FXPLAYER_AS_LOADVARS

#ifndef GFC_NO_XML_SUPPORT
/////////////////// load xml /////////////////
GFxLoadXMLTask::GFxLoadXMLTask(GFxLoadStates* pls, const GString& level0Path, const GString& url, 
                               GFxLoadQueueEntry::XMLHolderType xmlholder)
    : GFxTask(GFxTask::Id_MovieDataLoad),
      pLoadStates(pls), Level0Path(level0Path), Url(url), pXMLLoader(xmlholder.Loader), Done(0)
{
}

void GFxLoadXMLTask::Execute()
{
    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadXML, Url, Level0Path);
    GString                   fileName;
    pLoadStates->BuildURL(&fileName, loc);

    GASSERT(pXMLLoader);
    pXMLLoader->Load(fileName.ToCStr(), pLoadStates->GetFileOpener());

    GAtomicOps<UInt>::Store_Release(&Done, 1);
}

bool GFxLoadXMLTask::IsDone() const 
{
    UInt done = GAtomicOps<UInt>::Load_Acquire(&Done);
    return done == 1;
}

GFxLoadQueueEntryMT_LoadXML::GFxLoadQueueEntryMT_LoadXML(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot)
: GFxLoadQueueEntryMT(pqueueEntry,pmovieRoot)
{
    pLoadStates = *new GFxLoadStates(pMovieRoot->pLevel0Def->pLoaderImpl, pMovieRoot->GetStateBagImpl());
    GString level0Path;
    pMovieRoot->GetLevel0Path(&level0Path);
    pTask = *GNEW GFxLoadXMLTask(pLoadStates, level0Path,pqueueEntry->URL, pqueueEntry->XMLHolder);
    pMovieRoot->GetTaskManager()->AddTask(pTask);
}

GFxLoadQueueEntryMT_LoadXML::~GFxLoadQueueEntryMT_LoadXML()
{
}

// Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
// Returns true if the movie is loaded completely or in the case of errors.
bool GFxLoadQueueEntryMT_LoadXML::LoadFinished()
{
    bool btaskDone = pTask->IsDone();
    // If canceled, wait until task is done
    // [PPS] Change this logic to call AbandonTask and 
    // handle it appropriately inside the task to die
    // instantly
    if (pQueueEntry->Canceled && btaskDone)
        return true;
    if (!btaskDone)
        return false;
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    pQueueEntry->XMLHolder.Loader->InitASXml(penv, pQueueEntry->XMLHolder.ASObj.ToObject(penv));
    return true;
}
#endif


#ifndef GFC_NO_CSS_SUPPORT
/////////////////// load css /////////////////
GFxLoadCSSTask::GFxLoadCSSTask(GFxLoadStates* pls, const GString& level0Path, const GString& url, 
                               GFxLoadQueueEntry::CSSHolderType holder)
                               : GFxTask(GFxTask::Id_MovieDataLoad),
                               pLoadStates(pls), Level0Path(level0Path), Url(url), pLoader(holder.Loader), Done(0)
{
}

void GFxLoadCSSTask::Execute()
{
    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadCSS, Url, Level0Path);
    GString                   fileName;
    pLoadStates->BuildURL(&fileName, loc);

    GASSERT(pLoader);
    pLoader->Load(fileName.ToCStr(), pLoadStates->GetFileOpener());
    
    GAtomicOps<UInt>::Store_Release(&Done, 1);
}

bool GFxLoadCSSTask::IsDone() const 
{
    UInt done = GAtomicOps<UInt>::Load_Acquire(&Done);
    return done == 1;
}

GFxLoadQueueEntryMT_LoadCSS::GFxLoadQueueEntryMT_LoadCSS(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot)
: GFxLoadQueueEntryMT(pqueueEntry,pmovieRoot)
{
    pLoadStates = *new GFxLoadStates(pMovieRoot->pLevel0Def->pLoaderImpl, pMovieRoot->GetStateBagImpl());
    GString level0Path;
    pMovieRoot->GetLevel0Path(&level0Path);
    pTask = *GNEW GFxLoadCSSTask(pLoadStates, level0Path,pqueueEntry->URL, pqueueEntry->CSSHolder);
    pMovieRoot->GetTaskManager()->AddTask(pTask);
}

GFxLoadQueueEntryMT_LoadCSS::~GFxLoadQueueEntryMT_LoadCSS()
{
}

// Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
// Returns true if the movie is loaded completely or in the case of errors.
bool GFxLoadQueueEntryMT_LoadCSS::LoadFinished()
{
    bool btaskDone = pTask->IsDone();
    // If canceled, wait until task is done
    // [PPS] Change this logic to call AbandonTask and 
    // handle it appropriately inside the task to die
    // instantly
    if (pQueueEntry->Canceled && btaskDone)
        return true;
    if (!btaskDone)
        return false;
    GASEnvironment* penv = pMovieRoot->pLevel0Movie->GetASEnvironment();
    pQueueEntry->CSSHolder.Loader->Init(penv, pQueueEntry->CSSHolder.ASObj.ToObject(penv));
    return true;
}
#endif

//
// ***** GFxMovieRoot
//
// Global, shared root state for a GFxMovieSub and all its characters.


GFxMovieRoot::GFxMovieRoot(MemoryContextImpl* memContext) :           
    MemContext(memContext),
    NumAdvancesSinceCollection(0),
    LastCollectionFrame(0),
    pHeap(memContext->Heap),
    PixelScale(1.0f),
    VisibleFrameRect(0, 0, 0, 0),
    SafeRect(0, 0, 0, 0),
#ifndef GFC_NO_3D
	pPerspectiveMatrix3D(NULL),
    pViewMatrix3D(NULL),
#endif
    pXMLObjectManager(NULL),
    BackgroundColor(0, 0, 0, 255),  
#if defined(GFC_MOUSE_SUPPORT_ENABLED) && GFC_MOUSE_SUPPORT_ENABLED != 0
    MouseCursorCount(1),
#else
    MouseCursorCount(0),
#endif
    ControllerCount(1),
    UserData(NULL),
    ActionQueue(memContext->Heap),
    Flags(0)
{
    GASSERT(memContext->ASGC);
    GASSERT(memContext->StringManager);

    G_SetFlag<Flag_NeedMouseUpdate>(Flags, true);
    G_SetFlag<Flag_MovieIsFocused>(Flags, true);
    G_SetFlag<Flag_LevelClipsChanged>(Flags, true);

    pPlayListHead = pPlayListOptHead = pUnloadListHead = NULL;

    // Make sure all our allocations occur in the nested heap.
    TimeElapsed     = 0;
    TimeRemainder   = 0.0f;
    // Correct FrameTime is assigned to (1.0f / pLevel0Def->GetFrameRate())
    // when _level0 is set/changed.
    FrameTime       = 1.0f / 12.0f;
    ForceFrameCatchUp = 0;

    // No entries in load queue.
    pLoadQueueHead  = 0;

    pLoadQueueMTHead = 0;

    // Create a delegated shared state and ensure that it has a log.
    // Delegation is configured later to Level0Def's shared state.
    pStateBag       = *GHEAP_NEW(pHeap) GFxStateBagImpl(0);

    pASMouseListener    = 0;
#ifndef GFC_NO_KEYBOARD_SUPPORT
    for (UInt8 i = 0; i < GFC_MAX_KEYBOARD_SUPPORTED; ++i)
        KeyboardState[i].SetKeyboardIndex(i);
    //pKeyboardState      = *GHEAP_NEW(pHeap) GFxKeyboardState;
#endif
    // Create global variable context
    pGlobalContext      = *GHEAP_NEW(pHeap) GASGlobalContext(this, memContext->StringManager);
    pRetValHolder       =  GHEAP_NEW(pHeap) ReturnValueHolder(pGlobalContext);

    pFontManagerStates  = *GHEAP_NEW(pHeap) GFxFontManagerStates(pStateBag);

    InstanceNameCount  = 0;

    pLevel0Movie       = 0;    

    // Viewport: un-initialized by default.
    ViewScaleX = 1.0f;
    ViewScaleY = 1.0f;
    ViewOffsetX = ViewOffsetY = 0;
    ViewScaleMode = SM_ShowAll;
    ViewAlignment = Align_Center;

    // Focus
    FocusGroupsCnt          = 1;
    memset(FocusGroupIndexes, 0, sizeof(FocusGroupIndexes));

    LastIntervalTimerId = 0;

    pIMECandidateListStyle  = NULL;

    StartTickMs = GTimer::GetTicks()/1000;
    PauseTickMs = 0;

    SafeRect.Clear();

    pInvokeAliases = NULL;
#ifndef GFC_NO_SOUND
    pAudio = NULL;
    pSoundRenderer = NULL;
#endif

    pObjectInterface = GHEAP_NEW(pHeap) GFxValue::ObjectInterface(this);

    LastLoadQueueEntryCnt = 0;

#if defined(GFX_AMP_SERVER)
    AdvanceStats = *GHEAP_AUTO_NEW(&GFxAmpServer::GetInstance()) GFxAmpViewStats();
    DisplayStats = *GHEAP_AUTO_NEW(&GFxAmpServer::GetInstance()) GFxAmpViewStats();
    GFxAmpServer::GetInstance().AddMovie(this);
#elif !defined(GFC_NO_STAT)
    AdvanceStats = *GHEAP_NEW(pHeap) GFxAmpViewStats();
    DisplayStats = *GHEAP_NEW(pHeap) GFxAmpViewStats();
#endif

#ifndef GFC_NO_3D
    PerspFOV = DEFAULT_FLASH_FOV;
#endif
 }

GFxMovieRoot::~GFxMovieRoot()
{
    SpritesWithHitArea.Clear();
#ifdef GFX_AMP_SERVER
    GFxAmpServer::GetInstance().RemoveMovie(this);
#endif

#ifndef GFC_NO_IME_SUPPORT
    // clean up IME manager
    GPtr<GFxIMEManager> pIMEManager = GetIMEManager();
    if (pIMEManager)
    {
        if (pIMEManager->IsMovieActive(this))
            pIMEManager->ClearActiveMovie();
    }
    delete pIMECandidateListStyle;
#endif //#ifndef GFC_NO_IME_SUPPORT

    pFontManagerStates = NULL;
    // Set our log to String Manager so that it can report leaks if they happened.
    if (pLevel0Movie)
        pGlobalContext->GetStringManager()->SetLeakReportLog(
                                                pLevel0Movie->GetLog(), pLevel0Def->GetFileURL());


    ShutdownTimers();

#ifndef GFC_NO_VIDEO
    VideoProviders.Clear();
#endif

#ifndef GFC_NO_SOUND
    GFxSprite* psprite = GetLevelMovie(0);
    if (psprite)
        psprite->StopActiveSounds();
#endif

    // It is important to destroy the sprite before the global context,
    // so that is is not used from OnEvent(unload) in sprite destructor
    // NOTE: We clear the list here first because users can store pointers in _global,
    // which would cause pMovie assignment to not release it early (avoid "aeon.swf" crash).
    UPInt i;

    for (i = MovieLevels.GetSize(); i > 0; i--)
        MovieLevels[i-1].pSprite->ClearDisplayList();
    for (i = MovieLevels.GetSize(); i > 0; i--)
        MovieLevels[i-1].pSprite->ForceShutdown();
    // Release all refs.
    MovieLevels.Clear();

    ClearStickyVariables();

    delete pRetValHolder;

    // clean up threaded load queue before freeing
    GFxLoadQueueEntryMT* plq = pLoadQueueMTHead;
    UInt plqCount = 0;
    while (plq)
    {
        plqCount++;
        plq->Cancel();
        plq = plq->pNext;
    }
    // wait until all threaded loaders exit
    // this is required to avoid losing the movieroot
    // heap before the load tasks complete.
    UInt plqDoneCount = 0;
    while (plqCount > plqDoneCount)
    {
        plqDoneCount = 0;
        plq = pLoadQueueMTHead;
        while (plq)
        {
            if (plq->LoadFinished())
                plqDoneCount++;
            plq = plq->pNext;
        }
    }

    // free load queue
    while(pLoadQueueHead)
    {
        // Remove from queue.
        GFxLoadQueueEntry *pentry = pLoadQueueHead;
        pLoadQueueHead = pentry->pNext;
        delete pentry;
    }
    while(pLoadQueueMTHead)
    {
        GFxLoadQueueEntryMT *pentry = pLoadQueueMTHead;
        pLoadQueueMTHead = pentry->pNext;
        delete pentry;
    }
    pGlobalContext->DetachMovieRoot();
    delete pInvokeAliases;
    
    ReleaseUnloadList();

    ExternalIntfRetVal.DropRefs();

#ifndef GFC_NO_3D
    delete pPerspectiveMatrix3D;
    delete pViewMatrix3D;
#endif

#ifdef GFC_BUILD_DEBUG
    //
    // Check if there are GFxValues still holding refs to AS objects.
    // Warn the developer if there are any because this condition
    // will lead to crash where the GFxValue dtor, etc. will be
    // accessing freed memory.
    //
    if (pObjectInterface->HasTaggedValues())
    {
        pObjectInterface->DumpTaggedValues();
        GFC_DEBUG_MESSAGE(1, "");
        GFC_DEBUG_ERROR(1, "There are GFxValues still holding references to AS Objects. If not released\n"
                           "before the GFxMovieRoot that owns the AS objects dies, then a crash will occur\n"
                           "due to the GFxValue trying refer to freed memory. (When the GFxMovieRoot dies,\n"
                           "it drops the heap that contains all AS objects.) Make sure all GFxValues that\n"
                           "hold AS object references are released before GFxMovieRoot dies.\n");
        GASSERT(0);
    }
#endif
    delete pObjectInterface;

#ifndef GFC_NO_GC
    // need to release all "collectables" before freeing the RefCount Collector itself
    pGlobalContext->PreClean();
// #ifndef GFC_NO_KEYBOARD_SUPPORT
//     pKeyboardState = NULL;
// #endif //GFC_NO_KEYBOARD_SUPPORT
    ActionQueue.Clear();

    MemContext->ASGC->ForceCollect();
#endif // GFC_NO_GC
}

#ifdef GFC_BUILD_DEBUG
//#define GFC_TRACE_COLLECTIONS
#endif


/*
OnExceedLimit and OnFreeSegment are used to control MovieView's memory heap growth.
OnExceedLimit is called then the pre-set heap limit it exceeded.
OnFreeSegment is called then a chunk of memory is freed (not less than heap granularity).

User can set two parameters: UserLevelLimit (in bytes) and HeapLimitMultiplier (0..1].

The heap has initial pre-set limit 128K (so called "dynamic" limit). When this limit
is exceeded then OnExceedLimit is called. In this case it is necessary to determine:
either to collect to free up space, or to expand the heap.  The heuristic used to make the 
decision to collect or expand is taken from the Boehm-Demers-Weiser (BDW) garbage 
collector and memory allocator.
The BDW algorithm is (pseudo-code):

    if (allocs since collect >= heap footprint * HeapLimitMultiplier)
        collect
    else
        expand(heap footprint + overlimit + heap footprint * HeapLimitMultiplier)

The default value for HeapLimitMultiplier is 0.25.
Thus, it will collect only if allocated memory since the last collection is more
than 25% (default value of HeapLimitMultiplier) of the current heap size. Otherwise,
it will expand the limit up to requested size plus 25% of the heap size.

If collection occurred, it flushes all possible caches (text/paragraph formats, for example) and 
invoke garbage collector for ActionScript.

If user has specified UserLevelLimit then the algorithm above works the same way
up to that limit. If heap limit exceeds the UserLevelLimit then collection will be
invoked regardless to number of allocations since last collect. The dynamic heap limit
will be set to heap's footprint after collection + delta required for the requested allocation.

OnFreeSegment call reduces the limit by the freeing size.
*/
bool GFxMovieRoot::MemoryContextImpl::HeapLimit::OnExceedLimit(GMemoryHeap* heap, UPInt overLimit)
{
    UPInt footprint = heap->GetFootprint();
    UPInt heapLimit = heap->GetLimit();
#ifdef GFC_TRACE_COLLECTIONS        
    printf("\n! Limit is exceeded. Used: %u, footprint: %u, lim: %u, over limit: %u\n", 
        (UInt)heap->GetUsedSpace(), (UInt)footprint, (UInt)heapLimit, (UInt)overLimit);
#endif

    SPInt allocsSinceLastCollect = (SPInt)footprint - LastCollectionFootprint;
    UPInt newLimit = (heapLimit + overLimit) + (UPInt)(footprint * HeapLimitMultiplier);

    if (allocsSinceLastCollect >= (SPInt)(footprint * HeapLimitMultiplier) ||
        (UserLevelLimit != 0 && newLimit > UserLevelLimit))
    {
        Collect(heap);

        if (UserLevelLimit != 0 && newLimit > UserLevelLimit)
        {
            // check, if user limit is specified. If so, and if it is exceeded
            // then increase the limit just for absolutely required delta to minimize
            // the heap growth.
            GASSERT(LastCollectionFootprint <= footprint);
            if (overLimit > (footprint - LastCollectionFootprint))
            {
                CurrentLimit = heapLimit + (overLimit - (footprint - LastCollectionFootprint));
                heap->SetLimit(CurrentLimit);

#ifdef GFC_TRACE_COLLECTIONS        
                printf("-        UserLimit exceeded. increasing limit up to: %u (%u)\n", 
                    (UInt)CurrentLimit, (UInt)heap->GetLimit());
#endif

                CurrentLimit = heap->GetLimit(); // take an actual value of the limit
            }
            else
            {
                // even though limit is not changed - set it to heap again to make sure
                // the acutual heap's limit is set correctly.
                heap->SetLimit(CurrentLimit);

#ifdef GFC_TRACE_COLLECTIONS        
                printf("-        no limit increase is necessary. Current limit is %u (%u)\n", 
                    (UInt)CurrentLimit, (UInt)heap->GetLimit());
#endif

                CurrentLimit = heap->GetLimit(); // take an actual value of the limit
            }
        }
    }
    else
    {
        heap->SetLimit(newLimit);

#ifdef GFC_TRACE_COLLECTIONS        
        printf("-    increasing limit up to: %u (%u)\n", (UInt)newLimit, (UInt)heap->GetLimit());
#endif

        CurrentLimit = heap->GetLimit(); // take an actual value of the limit
    }

    return true;
}
void GFxMovieRoot::MemoryContextImpl::HeapLimit::OnFreeSegment(GMemoryHeap* heap, UPInt freeingSize)
{
    UPInt oldLimit = CurrentLimit;
    if (oldLimit > UserLevelLimit)
    {
        if (oldLimit > freeingSize)
        {
            CurrentLimit = oldLimit - freeingSize;
            heap->SetLimit(CurrentLimit);
#ifdef GFC_TRACE_COLLECTIONS        
            printf("!!   reducing limit from %u to %u (%u)\n", (UInt)oldLimit, (UInt)CurrentLimit, (UInt)heap->GetLimit());
#endif
        }
    }
}
void GFxMovieRoot::MemoryContextImpl::HeapLimit::Collect(GMemoryHeap* heap)
{
    // The collection part.
    // We may release cached text format objects from GFxTextAllocator...
    if (MemContext->TextAllocator)
    {
        MemContext->TextAllocator->FlushTextFormatCache(true);
        MemContext->TextAllocator->FlushParagraphFormatCache(true);
    }
    // If garbage collection is used - force collect. Use ForceEmergencyCollect - it
    // guarantees that no new allocations will be made during the collection.
#ifndef GFC_NO_GC
    MemContext->ASGC->ForceEmergencyCollect();
#endif
    LastCollectionFootprint = heap->GetFootprint(); 

#ifdef GFC_TRACE_COLLECTIONS        
    printf("+    footprint after collection: %u, used mem: %u\n", 
        (UInt)heap->GetFootprint(), (UInt)heap->GetUsedSpace());
#endif
}
void GFxMovieRoot::MemoryContextImpl::HeapLimit::Reset(GMemoryHeap* heap)
{
    Collect(heap);
    heap->SetLimit(INITIAL_DYNAMIC_LIMIT);   // reset to initial 128 
    CurrentLimit = heap->GetLimit(); // take an actual value of the limit
}

void GFxMovieRoot::RegisterAuxASClasses()
{
    GASStringContext sc(pGlobalContext, 8);

#ifndef GFC_NO_XML_SUPPORT
    // Register the XML and XMLNode classes
    GPtr<GFxXMLSupportBase> xmlstate = pStateBag->GetXMLSupport();
    if (xmlstate)
    {
        xmlstate->RegisterASClasses(*pGlobalContext, sc);
    }
#endif
#ifndef GFC_NO_SOUND
    GPtr<GFxAudioBase> audiostate = pStateBag->GetAudio();
    if (audiostate)
        audiostate->RegisterASClasses(*pGlobalContext, sc);
#endif
#ifndef GFC_NO_VIDEO
    GPtr<GFxVideoBase> videostate = pStateBag->GetVideo();
    if (videostate)
        videostate->RegisterASClasses(*pGlobalContext, sc);
#endif
}


GFxSprite*  GFxMovieRoot::GetLevelMovie(SInt level) const
{
    GASSERT(level >= 0);

    // Exhaustive for now; could do binary.
    for (UInt i = 0; i < MovieLevels.GetSize(); i++)
    {
        if (MovieLevels[i].Level == level)
            return MovieLevels[i].pSprite;
    }   
    return 0;
}

// Sets a movie at level; used for initialization.
bool        GFxMovieRoot::SetLevelMovie(SInt level, GFxSprite *psprite)
{
    UInt i = 0;
    GASSERT(level >= 0);
    GASSERT(psprite);

    for (; i< MovieLevels.GetSize(); i++)
    {
        if (MovieLevels[i].Level >= level)
        {           
            if (MovieLevels[i].Level == level)
            {
                GFC_DEBUG_WARNING1(1, "GFxMovieRoot::SetLevelMovie fails, level %d already occupied", level);
                return 0;
            }           
            // Found insert spot.
            break;              
        }
    }
    G_SetFlag<Flag_LevelClipsChanged>(Flags, true);

    // Insert the item.
    LevelInfo li;
    li.Level  = level;
    li.pSprite= psprite;
    MovieLevels.InsertAt(i, li);

    psprite->OnInsertionAsLevel(level);

    if (level == 0)
    {
        pLevel0Movie = psprite;
        pLevel0Def   = psprite->GetResourceMovieDef();
        if (pLevel0Def && AdvanceStats)
        {
            AdvanceStats->SetName(pLevel0Def->GetFileURL());
        }
        pStateBag->SetDelegate(pLevel0Def->pStateBag);
        // Frame timing
        FrameTime    = 1.0f / pLevel0Def->GetFrameRate();

        if (!G_IsFlagSet<Flag_ViewportSet>(Flags))
        {
            GFxMovieDefImpl* pdef = psprite->GetResourceMovieDef();
            GViewport desc((SInt)pdef->GetWidth(), (SInt)pdef->GetHeight(), 0,0, (SInt)pdef->GetWidth(), (SInt)pdef->GetHeight());
            SetViewport(desc);
        }
    }

    G_SetFlag<Flag_NeedMouseUpdate>(Flags, true);
    return 1;
}

// Destroys a level and its resources.
bool    GFxMovieRoot::ReleaseLevelMovie(SInt level)
{
    if (level == 0)
    {
        StopDrag();

        ShutdownTimers();

        // Not sure if this unload order is OK
        while (MovieLevels.GetSize())
        {
            GFxSprite* plevel = MovieLevels[MovieLevels.GetSize() - 1].pSprite;
            plevel->OnEventUnload();
            DoActions();
            plevel->ForceShutdown();
            MovieLevels.RemoveAt(MovieLevels.GetSize() - 1);
        }

        // Clear vars.
        pLevel0Movie = 0;   
        FrameTime    = 1.0f / 12.0f;        
        // Keep pLevel0Def till next load so that users don't get null from GetMovieDef().
        //  pLevel0Def   = 0;
        //  pStateBag->SetDelegate(0);

        G_SetFlag<Flag_LevelClipsChanged>(Flags, true);
        return 1;
    }

    // Exhaustive for now; could do binary.
    for (UInt i = 0; i < MovieLevels.GetSize(); i++)
    {
        if (MovieLevels[i].Level == level)
        {
            GPtr<GFxSprite> plevel = MovieLevels[i].pSprite;

            // Inform old character of unloading.
            plevel->OnEventUnload();
            DoActions();            

            // TBD: Is this right, or should local frames persist till level0 unload?
            plevel->ForceShutdown(); 

            // Remove us.
            MovieLevels.RemoveAt(i);

            G_SetFlag<Flag_LevelClipsChanged>(Flags, true);
            return 1;
        }
    }   
    return 0;
}

GFxASCharacter* GFxMovieRoot::FindTarget(const GASString& path) const
{
    if (!pLevel0Movie || path.IsEmpty())
        return 0;

    // This will work since environment parses _levelN prefixes correctly.
    // However, it would probably be good to move the general FindTarget logic here.
    return pLevel0Movie->GetASEnvironment()->FindTarget(path);
}


// Helper: parses _levelN tag and returns the end of it.
// Returns level index, of -1 if there is no match.
SInt        GFxMovieRoot::ParseLevelName(const char* pname, const char **ptail, bool caseSensitive)
{
    if ((pname[0] >= '0') && (pname[0] <= '9'))
    {
        char *ptail2 = 0;
        SInt level = (SInt)strtol(pname, &ptail2, 10);
        *ptail = ptail2;
        return level;
    }

    if (!pname || pname[0] != '_')
        return -1;

    if (caseSensitive)
    {       
        if ( (pname[1] != 'l') ||
            (pname[2] != 'e') ||
            (pname[3] != 'v') ||
            (pname[4] != 'e') ||
            (pname[5] != 'l') )
            return -1;
    }
    else
    {
        if ( ((pname[1] != 'l') && (pname[1] != 'L')) ||
            ((pname[2] != 'e') && (pname[2] != 'E')) ||
            ((pname[3] != 'v') && (pname[3] != 'V')) ||
            ((pname[4] != 'e') && (pname[4] != 'E')) ||
            ((pname[5] != 'l') && (pname[5] != 'L')) )
            return -1;
    }
    // Number must follow the level.
    if ((pname[6] < '0') || (pname[6] > '9'))
        return -1;

    char *ptail2 = 0;
    SInt level = (SInt)strtol(pname + 6, &ptail2, 10);
    *ptail = ptail2;

    return level;
}



// Create new instance names for unnamed objects.
GASString    GFxMovieRoot::CreateNewInstanceName()
{
    InstanceNameCount++; // Start at 0, so first value is 1.
    char pbuffer[48] = { 0 };

    G_Format(GStringDataPtr(pbuffer, sizeof(pbuffer)), "instance{0}", InstanceNameCount);

    GASSERT(pGlobalContext);
    return pGlobalContext->CreateString(pbuffer);
}



// *** Load/Unload movie support

void    GFxMovieRoot::AddLoadQueueEntryMT(GFxLoadQueueEntry* pentry)
{
    GFxLoadQueueEntryMT* pentryMT = NULL;
    if (pentry->Type & GFxLoadQueueEntry::LTF_VarsFlag)
    {
#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
        pentryMT = GHEAP_NEW(pHeap) GFxLoadQueueEntryMT_LoadVars(pentry,this);
#endif
    }
    else if (pentry->Type & GFxLoadQueueEntry::LTF_XMLFlag)
    {
#ifndef GFC_NO_XML_SUPPORT
        if (pentry->URL.GetLength())
        {
            pentryMT = GHEAP_NEW(pHeap) GFxLoadQueueEntryMT_LoadXML(pentry,this);
            if (pentryMT && pentryMT->pQueueEntry->XMLHolder.ASObj.ToObject(NULL))
            {
                GASObject* pxmlobj = pentryMT->pQueueEntry->XMLHolder.ASObj.ToObject(NULL);
                GFxLoadQueueEntryMT *ptail = pLoadQueueMTHead;
                while (ptail)    
                {
                    ptail->pQueueEntry->CancelByXMLASObjPtr(pxmlobj);
                    ptail = ptail->pNext;
                }
            }
        }
#endif
    } 
    else if (pentry->Type & GFxLoadQueueEntry::LTF_CSSFlag)
    {
#ifndef GFC_NO_CSS_SUPPORT
        if (pentry->URL.GetLength())
        {
            pentryMT = GHEAP_NEW(pHeap) GFxLoadQueueEntryMT_LoadCSS(pentry,this);
            if (pentryMT && pentryMT->pQueueEntry->CSSHolder.ASObj.ToObject(NULL))
            {
                GASObject* pxmlobj = pentryMT->pQueueEntry->CSSHolder.ASObj.ToObject(NULL);
                GFxLoadQueueEntryMT *ptail = pLoadQueueMTHead;
                while (ptail)    
                {
                    ptail->pQueueEntry->CancelByCSSASObjPtr(pxmlobj);
                    ptail = ptail->pNext;
                }
            }
        }
#endif
    }
    else 
    {
        pentryMT = GHEAP_NEW(pHeap) GFxLoadQueueEntryMT_LoadMovie(pentry,this);
        if (pentryMT)
        {
            GFxLoadQueueEntryMT *ptail = pLoadQueueMTHead;
            while (ptail)    
            {
                if (pentryMT->pQueueEntry->pCharacter)
                    ptail->pQueueEntry->CancelByNamePath(pentryMT->pQueueEntry->pCharacter->GetNamePath());
                else 
                    ptail->pQueueEntry->CancelByLevel(pentryMT->pQueueEntry->Level);
                ptail = ptail->pNext;
            }
        }
    }
    if (!pentryMT)
    {
        delete pentry;
        return;
    }
    pentry->EntryTime = ++LastLoadQueueEntryCnt;
    if (!pLoadQueueMTHead)
    {
        pLoadQueueMTHead = pentryMT;
    }
    else
    {
        // Find tail.
        GFxLoadQueueEntryMT *ptail = pLoadQueueMTHead;
        while (ptail->pNext)    
            ptail = ptail->pNext;   

        // Insert at tail.
        ptail->pNext = pentryMT;
        pentryMT->pPrev = ptail;
    }   
}

// ****************************************************************************
// Adds load queue entry and takes ownership of it.
//
void    GFxMovieRoot::AddLoadQueueEntry(GFxLoadQueueEntry *pentry)
{
    pentry->EntryTime = ++LastLoadQueueEntryCnt;
    if (!pLoadQueueHead)
    {
        pLoadQueueHead = pentry;
    }
    else
    {
        // Find tail.
        GFxLoadQueueEntry *ptail = pLoadQueueHead;
        while (ptail->pNext)    
            ptail = ptail->pNext;   

        // Insert at tail.
        ptail->pNext = pentry;
    }   
}

// ****************************************************************************
// Adds load queue entry based on parsed url and target path 
// (Load/unload Movie)
//
void    GFxMovieRoot::AddLoadQueueEntry(const char* ptarget, const char* purl, GASEnvironment* env,
                                        GFxLoadQueueEntry::LoadMethod method,
                                        GASMovieClipLoader* pmovieClipLoader)
{
    GFxLoadQueueEntry*  pentry        = 0;
    SInt                level         = -1;
    GFxASCharacter*     ptargetChar   = env ? env->FindTarget(pGlobalContext->CreateString(ptarget)) : 
                                             FindTarget(pGlobalContext->CreateString(ptarget));
    GFxSprite*          ptargetSprite = ptargetChar ? ptargetChar->ToSprite() : 0;  

    // If target leads to level, use level loading option.
    if (ptargetSprite)  
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }       
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method);
    }
    else
    {
        // It must be a level or bad target. 
        if (level == -1)
        {
            // Decode level name.
            const char* ptail = "";
            level = ParseLevelName(ptarget, &ptail, pLevel0Movie->IsCaseSensitive());
            if (*ptail != 0)
                level = -1; // path must end at _levelN.
        }

        if (level != -1)
        {
            pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(level, purl, method);
        }
    }

    if (pentry) 
    {
        pentry->MovieClipLoaderHolder.SetAsObject(pmovieClipLoader);
        AddMovieLoadQueueEntry(pentry);
    }
}

// ****************************************************************************
// Adds load queue entry based on parsed url and target path 
// (Load Variables)
//
void    GFxMovieRoot::AddVarLoadQueueEntry(const char* ptarget, const char* purl,
                                           GFxLoadQueueEntry::LoadMethod method)
                                          
{
    GFxLoadQueueEntry*  pentry        = 0;
    SInt                level         = -1;
    GFxASCharacter*     ptargetChar   = FindTarget(pGlobalContext->CreateString(ptarget));
    GFxSprite*          ptargetSprite = ptargetChar ? ptargetChar->ToSprite() : 0;  

    // If target leads to level, use level loading option.
    if (ptargetSprite)  
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }       
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method, true);
    }
    else
    {
        // It must be a level or bad target. 
        if (level == -1)
        {
            // Decode level name.
            const char* ptail = "";
            level = ParseLevelName(ptarget, &ptail, pLevel0Movie->IsCaseSensitive());
            if (*ptail != 0)
                level = -1; // path must end at _levelN.
        }

        if (level != -1)
        {
            pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(level, purl, method, true);
        }
    }

    if (pentry) 
    {
        //AddLoadQueueEntry(pentry);
        if (GetTaskManager())
            AddLoadQueueEntryMT(pentry);
        else
            AddLoadQueueEntry(pentry);
    }
}

// ****************************************************************************
// Adds load queue entry based on parsed url and target character 
// (Load/unload Movie)
//
void    GFxMovieRoot::AddLoadQueueEntry(GFxASCharacter* ptargetChar, const char* purl,
                                        GFxLoadQueueEntry::LoadMethod method,
                                        GASMovieClipLoader* pmovieClipLoader)
{   
    if (!ptargetChar)
        return;

    GFxLoadQueueEntry*  pentry  = 0;
    SInt                level   = -1;
    GFxSprite*          ptargetSprite = ptargetChar->ToSprite();

    // If target leads to level, use level loading option.
    if (ptargetSprite)
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }   
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method);
    }
    else if (level != -1)
    {
        pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(level, purl, method);
    }

    if (pentry)
    {
        pentry->MovieClipLoaderHolder.SetAsObject(pmovieClipLoader);
        AddMovieLoadQueueEntry(pentry);
    }
}

void GFxMovieRoot::AddMovieLoadQueueEntry(GFxLoadQueueEntry* pentry)
{
    if(!pentry)
        return;

    bool    userImageProtocol = 0;

    // Check to see if URL is a user image substitute.
    // pentry->URL.GetLength() == 0 means we got here from unloadMovie
    if (pentry->URL.GetLength() > 0 && (pentry->URL[0] == 'i' || pentry->URL[0] == 'I'))
    {
        GString urlLowerCase = pentry->URL.ToLower();

        if (urlLowerCase.Substring(0, 6) == "img://" || urlLowerCase.Substring(0, 8) == "imgps://")
            userImageProtocol = 1;
    }

    if ( pentry->URL.GetLength() > 0 && !userImageProtocol && GetTaskManager())
        AddLoadQueueEntryMT(pentry);
    else
        AddLoadQueueEntry(pentry);

}
// ****************************************************************************
// Adds load queue entry based on parsed url and target character 
// (Load Variables)
//
void    GFxMovieRoot::AddVarLoadQueueEntry(GFxASCharacter* ptargetChar, const char* purl,
                                           GFxLoadQueueEntry::LoadMethod method)
{
    if (!ptargetChar)
        return;

    GFxLoadQueueEntry*  pentry  = 0;
    SInt                level   = -1;
    GFxSprite*          ptargetSprite = ptargetChar->ToSprite();

    // If target leads to level, use level loading option.
    if (ptargetSprite)
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }   
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method, true);
    }
    else if (level != -1)
    {
        pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(level, purl, method, true);
    }

    if (pentry)
    {
        if (GetTaskManager())
            AddLoadQueueEntryMT(pentry);
        else
            AddLoadQueueEntry(pentry);
    }
}

// ****************************************************************************
// Adds load queue entry based on parsed url and LoadVars object
// (Load Variables)
//
#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
void GFxMovieRoot::AddVarLoadQueueEntry(GASLoadVarsObject* ploadVars, const char* purl,
                                        GFxLoadQueueEntry::LoadMethod method)
{
    GFxLoadQueueEntry* pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(purl, method, true);
    if (pentry)
    {
        pentry->LoadVarsHolder.SetAsObject(ploadVars);
        if (GetTaskManager())
            AddLoadQueueEntryMT(pentry);
        else
            AddLoadQueueEntry(pentry);
    }
}
#endif

// ****************************************************************************
// Adds load queue entry based on parsed url and CSS object
// (Load CSS)
//
#ifndef GFC_NO_CSS_SUPPORT
void GFxMovieRoot::AddCssLoadQueueEntry(GASObject* pobj, GFxASCSSFileLoader* pLoader, 
                                        const char* purl,GFxLoadQueueEntry::LoadMethod method)
{
    GFxLoadQueueEntry* pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(purl, method);
    if (pentry)
    {
        pentry->Type = GFxLoadQueueEntry::LT_LoadCSS;
        pentry->CSSHolder.ASObj.SetAsObject(pobj);
        pentry->CSSHolder.Loader = pLoader;
        if (GetTaskManager())
            AddLoadQueueEntryMT(pentry);
        else
            AddLoadQueueEntry(pentry);
    }
}
#endif


// ****************************************************************************
// Adds load queue entry based on parsed url and XML object
// (Load XML)
//
#ifndef GFC_NO_XML_SUPPORT
void GFxMovieRoot::AddXmlLoadQueueEntry(GASObject* pxmlobj, GFxASXMLFileLoader* pxmlLoader, 
                                        const char* purl,GFxLoadQueueEntry::LoadMethod method)
{
    GFxLoadQueueEntry* pentry = GHEAP_NEW(pHeap) GFxLoadQueueEntry(purl, method);
    if (pentry)
    {
        pentry->Type = GFxLoadQueueEntry::LT_LoadXML;
        pentry->XMLHolder.ASObj.SetAsObject(pxmlobj);
        pentry->XMLHolder.Loader = pxmlLoader;
        if (GetTaskManager())
            AddLoadQueueEntryMT(pentry);
        else
            AddLoadQueueEntry(pentry);
    }
}
#endif



GFxMovieDefImpl*  GFxMovieRoot::CreateImageMovieDef(
                        GFxImageResource *pimageResource, bool bilinear,
                        const char *purl, GFxLoadStates *pls)
{   
    GFxMovieDefImpl*    pdefImpl = 0; 
    GPtr<GFxLoadStates> plsRef;

    // If load states were not passed, create them. This is necessary if
    // we are being called from sprite's attachBitmap call.
    if (!pls)
    {
        plsRef = *GNEW GFxLoadStates(pLevel0Def->pLoaderImpl, pStateBag);
        pls = plsRef.GetPtr();
    }

    // Create a loaded image sprite based on imageInfo.
    if (pimageResource)
    {
        // No file opener here, since it was not used (ok since file openers are
        // used as a key only anyway). Technically this is not necessary since
        // image 'DataDefs' are not stored in the library right now, but the
        // argument must be passed for consistency.
        GFxResourceKey createKey = GFxMovieDataDef::CreateMovieFileKey(purl, 0, 0, 0, 0);
        GMemoryHeap* pheap       = GetMovieHeap();

        // Create a MovieDataDef containing our image (and an internal corresponding ShapeDef).
        GPtr<GFxMovieDataDef> pimageMovieDataDef =
            *GFxMovieDataDef::Create(createKey, GFxMovieDataDef::MT_Image, purl, pheap);

        if (!pimageMovieDataDef)
            return pdefImpl;        
        pimageMovieDataDef->InitImageFileMovieDef(0, pimageResource, bilinear);

        // We can alter load states since they are custom for out caller.
        // (ProcessLoadQueue creates a copy of LoadStates)
        pls->SetRelativePathForDataDef(pimageMovieDataDef);

        // TBD: Should we use pStateBag or its delegate??       
        pdefImpl = GHEAP_NEW(pheap) GFxMovieDefImpl(pimageMovieDataDef,
                                                    pls->GetBindStates(), pls->pLoaderImpl,
                                                    GFxLoader::LoadAll,
                                                    pStateBag->GetDelegate(),
                                                    pheap, true); // Images are fully loaded.
    }
    return pdefImpl;
}

void    GFxMovieRoot::ProcessLoadMovieClip(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& level0Path)
{
    GASSERT(pentry);
    GASSERT((pentry->Type & GFxLoadQueueEntry::LTF_VarsFlag) == 0);
    // *** Handle loading/unloading.

    GString            url(pentry->URL), urlStrGfx;
    GPtr<GFxASCharacter> poldChar;      
    GFxASCharacter*      pparent   = 0;
    GFxResourceId        newCharId(GFxCharacterDef::CharId_EmptyMovieClip);

    GFxLog*              plog = pls->GetLog();
    // We take out load flags from pLevel0Def so that its options
    // can be inherited into GFxMovieDef.
    UInt                loadFlags = pLevel0Def->GetLoadFlags();

    GPtr<GFxMovieDefImpl>   pmovieDef;
    GPtr<GFxSprite>         pnewChar;

    bool stripped = false;

    if (pentry->pCharacter)
    {
        // This is loadMovie, not level.
        GASSERT(pentry->Level == -1);
        GASSERT(!(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag));

        // Make sure we have character targets and parents.
        //!AB: We need to get the character by name first time. It is kind of weird but
        // it seems that is how Flash works. For example, we have a movieclip 'mc'
        // and it already contains child 'holder'. Then we create an empty movie clip
        // within 'mc' and call it 'holder' as well. If we resolve string 'mc.holder' to
        // a character when the first (native) 'holder' will be returned. However, if we 
        // use returning value from createEmptyMovieClip we will have a pointer to the new
        // 'holder'. Now, we call loadMovie using the pointer returned from createEmptyMovieClip.
        // We expect, that movie will be loaded into newly created 'holder'. However, it loads
        // it into the native 'holder'. So, looks like the implementation of 'loadMovie' always
        // resolve character by name. Test file: Test\LoadMovie\test_loadmovie_with_two_same_named_clips.swf 
        poldChar = pentry->pCharacter->ForceResolveCharacter(this);
        if (!poldChar)
            return;
        if ((pparent = poldChar->GetParent())==0)
            return;
        stripped = ((poldChar->GetResourceMovieDef()->GetSWFFlags()
                                        & GFxMovieInfo::SWF_Stripped) != 0);
        //AB: if loading in the character then we need to assign the
        // same Id as the old character had; this is important to avoid
        // re-creation of the original character when timeline rollover 
        // of the parent character occurs.
        newCharId = poldChar->GetId();
    }
    else if (pentry->Level != -1)
    {
        GASSERT(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag);
        if (GetLevelMovie(pentry->Level))
            stripped = ((GetLevelMovie(pentry->Level)->GetResourceMovieDef()->GetSWFFlags()
                                                              & GFxMovieInfo::SWF_Stripped) != 0);
        else if (GetLevelMovie(0))
            stripped = ((GetLevelMovie(0)->GetResourceMovieDef()->GetSWFFlags()
                                                              & GFxMovieInfo::SWF_Stripped) != 0);

        // Wipe out level, making it ready for load.
        // Note that this could wipe out all levels!
        // Do this only if loading movie, not variables
        ReleaseLevelMovie(pentry->Level);
        newCharId = GFxResourceId(); // Assign invalid id.
    }
    else
    {
        // If not level or target, then it should be a LoadVars object
        GASSERT(0);
        return;
    }

    SInt filelength = 0;
    // If the file is stripped, its loadMovie may be stripped too,
    // so try to load '.gfx' file first.
    if (stripped)
        urlStrGfx = GetUrlStrGfx(url);
   
    // Load movie: if there is no URL or it fails, do 'unloadMovie'.
    if (url.GetLength())
    {               
        bool    bilinearImage     = 0;
        bool    userImageProtocol = 0;

        // Check to see if URL is a user image substitute.
        if (url[0] == 'i' || url[0] == 'I')
        {
            GString urlLowerCase = url.ToLower();

            if (urlLowerCase.Substring(0, 6) == "img://")
                userImageProtocol = bilinearImage = 1;
            else if (urlLowerCase.Substring(0, 8) == "imgps://")
                userImageProtocol = 1;
        }

        if (userImageProtocol)
        {
            GPtr<GFxImageLoader> pimageLoader = pStateBag->GetImageLoader();
            // Create image through image callback
            GPtr<GFxImageResource> pimageRes =
                *GFxLoaderImpl::LoadMovieImage(url, pimageLoader, plog, GetMovieHeap());

            if (pimageRes)
            {
                pmovieDef = *CreateImageMovieDef(pimageRes, bilinearImage,
                                                 url.ToCStr(), pls);
            }
            else if (plog)
            {
                plog->LogScriptWarning(
                    "LoadMovieImageCallback failed to load image \"%s\"\n", url.ToCStr());
            }
        }
        else
        {
            // Load the movie, first trying the .gfx file if necessary.
            // Our file loading can automatically detect and load images as well,
            // so we take advantage of that by passing the GFxLoader::LoadImageFiles flag.

            // TBD: We will need to do something with delayed loading here in the future;
            // i.e. if the CreateMovie is executing in a different thread it will need to
            // post messages when it is done so that the handlers can be called.


            UInt lf = loadFlags | GFxLoader::LoadImageFiles | GFxLoader::LoadWaitCompletion;
            if (pentry->QueitOpen)
                lf |= GFxLoader::LoadQuietOpen;

            if (urlStrGfx.GetLength() > 0)
            {
                GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadMovie,
                    urlStrGfx, level0Path);

                pmovieDef = *GFxLoaderImpl::CreateMovie_LoadState(pls, loc, lf);
            }
            if (!pmovieDef)
            {
                GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadMovie,
                    url, level0Path);

                pmovieDef = *GFxLoaderImpl::CreateMovie_LoadState(pls, loc, lf);
            }

            if (pmovieDef)
            {
                // Record file length for progress report below. Once we do
                // threaded loading this will change.
                filelength = pmovieDef->GetFileBytes();
            }
            else if (plog && !pentry->QueitOpen)
            {
                plog->LogScriptWarning("Error loading URL \"%s\"\n", url.ToCStr());
            }
        }   
    } 
    else
    {
        // if we unloading a movieclip we need to go over the list of loading queue which 
        // and cancel all items which are going to load into the same level or target.
        // Note, we need to cancel ONLY entries, inserted BEFORE the "unloadMovie" entry!
        GFxLoadQueueEntryMT *plentry = pLoadQueueMTHead;
        while(plentry)
        {
            if (plentry->pQueueEntry->EntryTime < pentry->EntryTime)
            {
                if (pentry->Level != -1 && plentry->pQueueEntry->Level != -1) 
                {
                    if (pentry->Level == plentry->pQueueEntry->Level)
                        plentry->pQueueEntry->Canceled = true;
                }
                else if (pentry->pCharacter && plentry->pQueueEntry->pCharacter && pentry->pCharacter == plentry->pQueueEntry->pCharacter)
                    plentry->pQueueEntry->Canceled = true;
            }

            plentry = plentry->pNext;
        }
    } // end if (url)

    // Create a new sprite of desired type.
    if (pmovieDef)
    {
        // CharId of loaded clip is the same as the old one
        // Also mark clip as loadedSeparately, so that _lockroot works.
        pnewChar = *GHEAP_NEW(pHeap) GFxSprite(pmovieDef->GetDataDef(), pmovieDef, this,
                                               pparent, newCharId, true);
    }
    bool charIsLoadedSuccessfully = (pnewChar.GetPtr() != NULL);

    if (pentry->Level == -1)
    {
        // Nested clip loading; replace poldChar.
        GASSERT(poldChar);

        // If that failed, create an empty sprite. This also handles unloadMovie().
        if (!pnewChar)
        {
            GFxCharacterCreateInfo ccinfo =
                pparent->GetResourceMovieDef()->GetCharacterCreateInfo
                (GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip));
            pnewChar = *(GFxSprite*)ccinfo.pCharDef->CreateCharacterInstance(pparent, newCharId,
                                                                             ccinfo.pBindDefImpl);
        }
        // And attach a new character in place of an old one.
        if (pnewChar)
        {
            pnewChar->AddToPlayList(this);
            //AB: we need to replicate the same CreateFrame, Depth and the Name.
            // Along with replicated Id this is important to maintain the loaded
            // movie on the stage. Otherwise, once the parent clip reaches the end
            // of its timeline the original movieclip will be recreated. Flash
            // doesn't recreate the original movie clip in this case, and it also
            // doesn't stop timeline animation for the target movieclip. The only way
            // to achieve both of these features is to copy Id, CreateFrame, Depth and
            // Name from the old character to the new one.
            // See last_to_first_frame_mc_no_reloadX.fla and 
            // test_timeline_anim_after_loadmovie.swf
            pnewChar->SetCreateFrame(poldChar->GetCreateFrame());
            pnewChar->SetDepth(poldChar->GetDepth());
            if (!poldChar->HasInstanceBasedName())
                pnewChar->SetName(poldChar->GetName());

            pparent->ReplaceChildCharacter(poldChar, pnewChar);
            // In case permanent variables were assigned.. check them.
            ResolveStickyVariables(pnewChar);

            // need to update the optimized play list
            pnewChar->ModifyOptimizedPlayListLocal<GFxSprite>(this);
        }
    }
    else
    {
        // Level loading.
        GASSERT(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag);           
        if (pnewChar)
        {
            pnewChar->SetLevel(pentry->Level);
            SetLevelMovie(pentry->Level, pnewChar);
            // we will execute action script for frame 0 ourself here
            // this is needed because this script has to be executed 
            // before calling OnLoadInit
            G_SetFlag<Flag_LevelClipsChanged>(Flags, false);
            // Check if permanent/sticky variables were assigned to level.
            ResolveStickyVariables(pnewChar);
        }

        if (!pLevel0Movie && plog)
        {
            plog->LogScriptWarning("_level0 unloaded - no further playback possible\n");
            return;
        }
    }

    GASSERT(pLevel0Movie);
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    GASSERT(penv);
    //GASSERT(pentry->MovieClipLoaderHolder.ToObject()->GetObjectType() == GASObjectInterface::Object_MovieClipLoader);
    GASMovieClipLoader* pmovieClipLoader = 
        static_cast<GASMovieClipLoader*>(pentry->MovieClipLoaderHolder.ToObject(penv));

    // Notify MovieClipLoader listeners. TODO: should be done in right places in right time.
    if (charIsLoadedSuccessfully)
    {
        if (pmovieClipLoader)
        {
            pmovieClipLoader->NotifyOnLoadStart(penv, pnewChar);
            pmovieClipLoader->NotifyOnLoadProgress(penv, pnewChar, filelength, filelength);
            pmovieClipLoader->NotifyOnLoadComplete(penv, pnewChar, 0);
        }
        // Run actions on Frame 0. This should be done before calling OnLoadInit
        //!AB: note, we can't use OnEventLoad here, need to use ExecuteFrame0Events. 
        // ExecuteFrame0Events executes onLoad event AFTER the first frame and Flash
        // does exactly the same.
        //pnewChar->OnEventLoad();
        pnewChar->ExecuteFrame0Events();
        DoActions();
        if (pmovieClipLoader)
            pmovieClipLoader->NotifyOnLoadInit(penv, pnewChar);
    } 
    else 
    {
        if (pmovieClipLoader)
        {
            if (url.GetLength())
            {
                // No new char is loaded - assuming the error occurs
                pmovieClipLoader->NotifyOnLoadError(penv, pnewChar, "URLNotFound", 0);
                // are there any other error codes? (AB)
            }
            else 
            {
                pmovieClipLoader->NotifyOnLoadError(penv, pnewChar, "Unknown error", 0);
            }
        }
    }
    if (url.GetLength() == 0)
    {
        // if unloadMovie then force oldChar to be released now and
        // force collect to flush text format cache and invoke garbage collection.
        poldChar = NULL;
        MemContext->LimHandler.Collect(pHeap);
    }
}


#ifndef GFC_NO_XML_SUPPORT
void    GFxMovieRoot::ProcessLoadXML(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& level0Path)
{
    GASSERT(pentry);
    GASSERT((pentry->Type & GFxLoadQueueEntry::LTF_XMLFlag) != 0);

    if (pentry->URL.GetLength())
    {
        GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadXML, pentry->URL, level0Path);
        GString                   fileName;
        pls->BuildURL(&fileName, loc);
        pentry->XMLHolder.Loader->Load(fileName.ToCStr(), pls->GetFileOpener());
        GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
        pentry->XMLHolder.Loader->InitASXml(penv, pentry->XMLHolder.ASObj.ToObject(penv));
    }
}
#endif


#ifndef GFC_NO_CSS_SUPPORT
void    GFxMovieRoot::ProcessLoadCSS(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& level0Path)
{
    GASSERT(pentry);
    GASSERT((pentry->Type & GFxLoadQueueEntry::LTF_CSSFlag) != 0);

    if (pentry->URL.GetLength())
    {
        GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadCSS, pentry->URL, level0Path);
        GString                   fileName;
        pls->BuildURL(&fileName, loc);
        pentry->CSSHolder.Loader->Load(fileName.ToCStr(), pls->GetFileOpener());
        GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
        pentry->CSSHolder.Loader->Init(penv, pentry->CSSHolder.ASObj.ToObject(penv));
    }
}
#endif

#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
void    GFxMovieRoot::ProcessLoadVars(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& level0Path)
{
    GASSERT(pentry);
    GASSERT((pentry->Type & GFxLoadQueueEntry::LTF_VarsFlag) != 0);

    GString data;
    SInt      fileLen = 0;
    if (pentry->URL.GetLength())
    {
        bool userVarsProtocol = 0;

        // @TODO logic to determine if a user protocol was used

        if (userVarsProtocol)
        {
            // @TODO
        }
        else
        {
            GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadVars, pentry->URL, level0Path);
            GString                   fileName;
            pls->BuildURL(&fileName, loc);

            // File loading protocol
            GPtr<GFile> pfile;
            pfile = *pls->OpenFile(fileName.ToCStr());
            if (pfile)
            {
                if (pentry->LoadVarsHolder.IsObject())
                {
                    if (!GFx_ReadLoadVariables(pfile, &data, &fileLen))
                        pentry->LoadVarsHolder.DropRefs();
                }
                else 
                {
                    GFx_ReadLoadVariables(pfile, &data, &fileLen);
                }
            }
        }
    }
    LoadVars(pentry, pls, data, fileLen);
}

void GFxMovieRoot::LoadVars(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GString& data, SInt fileLen)
{
    if (pentry->LoadVarsHolder.ToObject(NULL))
    {
        GASSERT(pentry->LoadVarsHolder.ToObject(NULL)->GetObjectType() == GASObjectInterface::Object_LoadVars);
        GASLoadVarsObject* ploadVars = 
            static_cast<GASLoadVarsObject*>(pentry->LoadVarsHolder.ToObject(NULL));
        GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
        GASString str = penv->CreateString(data);
        ploadVars->SetLoadedBytes(GASNumber(fileLen));
        ploadVars->NotifyOnData(penv, str);
    }
    else 
    {
        GPtr<GFxASCharacter> pchar;
        if (pentry->Level == -1)
        {
            // character loading
            GASSERT(pentry->pCharacter);
            pchar = pentry->pCharacter->ResolveCharacter(this);
            GASSERT(pchar);
        }
        else
        {
            // Level loading
            GASSERT(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag);           
            // use MovieLevels to get the appropriate level, and
            // add the variables into the level's sprite
            // find the right level movie
            pchar = GetLevelMovie(pentry->Level);

            // If level doesn't exist, create one using an empty MovieDef.
            if (!pchar)
                pchar = *CreateEmptySprite(pls, pentry->Level);
            if (!pchar)
                return;
        }
        if(pchar->GetObjectType() == GASObjectInterface::Object_LoadVars)
        {
            GASLoadVarsObject* lvobj = (GASLoadVarsObject*) pchar.GetPtr();
            lvobj->SetLoadedBytes(GASNumber(fileLen));
        }
        GASLoadVarsProto::LoadVariables(pLevel0Movie->GetASEnvironment(), pchar, data);
    }
}

GFxSprite* GFxMovieRoot::CreateEmptySprite(GFxLoadStates* pls, SInt level)
{
    // Technically this is not necessary since empty 'DataDefs' are not stored 
    // in the library right now, but the argument must be passed for consistency.
    // TBD: Perhaps we could hash it globally, since there should be only one?
    GFxResourceKey createKey = GFxMovieDataDef::CreateMovieFileKey("", 0, 0, 0, 0);

    // Create a MovieDataDef containing our image (and an internal corresponding ShapeDef).
    GPtr<GFxMovieDataDef> pemptyDataDef = 
        *GFxMovieDataDef::Create(createKey, GFxMovieDataDef::MT_Empty, "", pHeap);
        
    GPtr<GFxMovieDefImpl> pemptyMovieImpl;
    if (pemptyDataDef)
    {
        pemptyDataDef->InitEmptyMovieDef();
        pls->SetRelativePathForDataDef(pemptyDataDef);
        pemptyMovieImpl = 
            *GHEAP_NEW(pHeap) GFxMovieDefImpl(pemptyDataDef, pls->GetBindStates(), 
                                              pls->pLoaderImpl, GFxLoader::LoadAll, 
                                              pStateBag->GetDelegate(), pHeap, true);
    }
    if (!pemptyMovieImpl)
        return NULL;

    GFxSprite* pnewChar = GHEAP_NEW(pHeap) GFxSprite(pemptyDataDef, pemptyMovieImpl, this,
                                                     NULL, GFxResourceId(), true);
    pnewChar->SetLevel(level);
    SetLevelMovie(level, pnewChar);
    return pnewChar;
}
#endif //#ifndef GFC_NO_FXPLAYER_AS_LOADVARS

// Processes the load queue handling load/unload instructions.  
void    GFxMovieRoot::ProcessLoadQueue()
{
    while(pLoadQueueHead)
    {
        // Remove from queue.
        GFxLoadQueueEntry *pentry = pLoadQueueHead;
        pLoadQueueHead = pentry->pNext;

        // Capture load states - constructor copies their values from pStateBag;
        // this means that states used for 'loadMovie' can be overridden per movie view.
        // This means that state inheritance chain (GFxLoader -> GFxMovieDefImpl -> GFxMovieRoot)
        // takes effect here, so setting new states in any of above can have effect
        // on loadMovie calls. Note that this is different from import logic, which
        // always uses exactly same states as importee. Such distinction is by design.
        GPtr<GFxLoadStates>  pls = *new GFxLoadStates(pLevel0Def->pLoaderImpl, pStateBag);

        // Obtain level0 path before it has a chance to be unloaded below.
        GString level0Path;
        GetLevel0Path(&level0Path);

        if ((pentry->Type & GFxLoadQueueEntry::LTF_VarsFlag) != 0)
        {
#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
            ProcessLoadVars(pentry, pls, level0Path);
#endif //#ifndef GFC_NO_FXPLAYER_AS_LOADVARS
        }
#ifndef GFC_NO_XML_SUPPORT
        else if ((pentry->Type & GFxLoadQueueEntry::LTF_XMLFlag) != 0)
            ProcessLoadXML(pentry, pls, level0Path);
#endif
#ifndef GFC_NO_CSS_SUPPORT
        else if ((pentry->Type & GFxLoadQueueEntry::LTF_CSSFlag) != 0)
            ProcessLoadCSS(pentry, pls, level0Path);
#endif
        else
            ProcessLoadMovieClip(pentry, pls, level0Path);
        delete pentry;
    }

    GFxLoadQueueEntryMT* pentry = pLoadQueueMTHead;
    // First we need to make sure that our queue does not have any tasks with 
    // unfinished preloading task (which usually is just searching and opening a file). 
    // We need this check to avoid a race condition like this: let say we load 
    // A.swf into _level0 and B.swf into _level1 and proloading for A.swf takes more 
    // time then for B.swf . In this case B.swf will be loaded first and then unloaded 
    // by loading A.swf into _level0. This happens because we unload a level just after preloading
    // has finished and unloading of _level0 unloads all everything in the swf movie
    while(pentry)
    {
        if (!pentry->IsPreloadingFinished())
            return;
        pentry = pentry->pNext;
    }
    // Check status of all movies that are loading through the TaskManager if we have one.
    pentry = pLoadQueueMTHead;
    while(pentry)
    {
        if (pentry->LoadFinished()) 
        {
            GFxLoadQueueEntryMT* next = pentry->pNext;
            if (next)
                next->pPrev = pentry->pPrev;
            if (pentry->pPrev)
                pentry->pPrev->pNext = next;
            if (pLoadQueueMTHead == pentry) 
                pLoadQueueMTHead = next;
            delete pentry;
            pentry = next;
        } 
        else 
        {
            pentry = pentry->pNext;
        }
    }
}

// Fills in a file system path relative to _level0.
bool    GFxMovieRoot::GetLevel0Path(GString *ppath) const
{       
    if (!pLevel0Movie)
    {
        ppath->Clear();
        return false;
    }

    // Get URL.
    *ppath = pLevel0Def->GetFileURL();

    // Extract path by clipping off file name.
    if (!GFxURLBuilder::ExtractFilePath(ppath))
    {
        ppath->Clear();
        return false;
    }
    return true;
}



// *** Drag State support


void GFxMovieRoot::DragState::InitCenterDelta(bool lockCenter)
{
    LockCenter = lockCenter;

    if (!LockCenter)
    {
        typedef GRenderer::Matrix   Matrix;                     

        // We are not centering on the object, so record relative delta         
        Matrix          parentWorldMat;
        GFxCharacter*   pchar = pCharacter;
        if (pchar->GetParent())
            parentWorldMat = pchar->GetParent()->GetWorldMatrix();

        // Mouse location
        const GFxMouseState* pmouseState = pchar->GetMovieRoot()->GetMouseState(0); // only mouse id = 0 is supp
        GASSERT(pmouseState);
        GPointF worldMouse(pmouseState->GetLastPosition());

        GPointF parentMouse;
        // Transform to parent coordinates [world -> parent]
            parentWorldMat.TransformByInverse(&parentMouse, worldMouse);

#ifndef GFC_NO_3D
        if (pchar->Is3D(true))
        {
            const GMatrix3D     *pPersp = pchar->GetPerspective3D(true);
            const GMatrix3D     *pView = pchar->GetView3D(true);
            if (pPersp)
                pchar->GetMovieRoot()->ScreenToWorld.SetPerspective(*pPersp);
            if (pView)
                pchar->GetMovieRoot()->ScreenToWorld.SetView(*pView);
            pchar->GetMovieRoot()->ScreenToWorld.SetWorld(pchar->GetParent()->GetWorldMatrix3D());
            pchar->GetMovieRoot()->ScreenToWorld.GetWorldPoint(&parentMouse);
        }
#endif

        // Calculate the relative mouse offset, so that we can apply adjustment
        // before bounds checks.
        Matrix  local = pchar->GetMatrix();
        CenterDelta.x = local.M_[0][2] - parentMouse.x;
        CenterDelta.y = local.M_[1][2] - parentMouse.y;
    }
}


// *** Action List management
GFxMovieRoot::ActionQueueType::ActionQueueType(GMemoryHeap *pheap)
    : pHeap(pheap)
{
    LastSessionId = 0;
    ModId = 1;
    pFreeEntry = NULL;
    CurrentSessionId = ++LastSessionId;
    FreeEntriesCount = 0;
}

GFxMovieRoot::ActionQueueType::~ActionQueueType()
{
    Clear();

    // free pFreeEntry chain
    for(ActionEntry* pcur = pFreeEntry; pcur; )
    {
        ActionEntry* pnext = pcur->pNextEntry;
        delete pcur;
        pcur = pnext;
    }
}

GFxMovieRoot::ActionEntry*  GFxMovieRoot::ActionQueueType::InsertEntry(GFxActionPriority::Priority prio)
{
    ActionEntry *p = 0;
    if (pFreeEntry)
    {
        p = pFreeEntry;
        pFreeEntry = pFreeEntry->pNextEntry;
        p->pNextEntry = 0;
        --FreeEntriesCount;
    }
    else
    {
        if ((p = GHEAP_NEW(GetMovieHeap()) ActionEntry)==0)
            return 0;
    }

    ActionQueueEntry& actionQueueEntry = Entries[prio];

    // Insert and move insert pointer to us.
    if (actionQueueEntry.pInsertEntry == 0) // insert at very beginning
    {
        p->pNextEntry                   = actionQueueEntry.pActionRoot;
        actionQueueEntry.pActionRoot    = p;
    }
    else
    {
        p->pNextEntry = actionQueueEntry.pInsertEntry->pNextEntry;
        actionQueueEntry.pInsertEntry->pNextEntry = p;
    }
    actionQueueEntry.pInsertEntry   = p;
    if (p->pNextEntry == NULL)
        actionQueueEntry.pLastEntry = p;
    p->SessionId = CurrentSessionId;
    ++ModId;

    // Done
    return p;
}

GFxMovieRoot::ActionEntry* GFxMovieRoot::ActionQueueType::FindEntry
    (GFxActionPriority::Priority prio, const ActionEntry& entry)
{
    ActionQueueEntry& queue = Entries[prio];
    ActionEntry* pentry = queue.pActionRoot;
    for (; pentry != NULL; pentry = pentry->pNextEntry)
    {
        if (*pentry == entry)
            return pentry;
    }
    return NULL;
}

void GFxMovieRoot::ActionQueueType::AddToFreeList(GFxMovieRoot::ActionEntry* pentry)
{
    pentry->ClearAction();
    if (FreeEntriesCount < 50)
    {
        pentry->pNextEntry = pFreeEntry;
        pFreeEntry = pentry;
        ++FreeEntriesCount;
    }
    else
        delete pentry;
}

void    GFxMovieRoot::ActionQueueType::Clear()
{   
    GFxMovieRoot::ActionQueueIterator iter(this);
    while(iter.getNext())
        ;
}

UInt GFxMovieRoot::ActionQueueType::StartNewSession(UInt* pprevSessionId) 
{ 
    if (pprevSessionId) *pprevSessionId = CurrentSessionId;
    CurrentSessionId = ++LastSessionId; 
    return CurrentSessionId; 
}

GFxMovieRoot::ActionQueueIterator::ActionQueueIterator(GFxMovieRoot::ActionQueueType* pactionQueue)
{
    GASSERT(pactionQueue);
    pActionQueue    = pactionQueue;
    ModId           = 0;
    CurrentPrio     = 0;
    pLastEntry      = 0;
}

GFxMovieRoot::ActionQueueIterator::~ActionQueueIterator()
{
    if (pLastEntry)
        pActionQueue->AddToFreeList(pLastEntry); // release entry
}

const GFxMovieRoot::ActionEntry* GFxMovieRoot::ActionQueueIterator::getNext()
{
    if (pActionQueue->ModId != ModId)
    {
        // new actions were added - restart
        CurrentPrio = 0;
        ModId = pActionQueue->ModId;
    }
    ActionEntry* pcurEntry = pActionQueue->Entries[CurrentPrio].pActionRoot;
    if (!pcurEntry)
    {
        while(pcurEntry == NULL && ++CurrentPrio < AP_Count)
        {
            pcurEntry = pActionQueue->Entries[CurrentPrio].pActionRoot;
        }
    }
    if (pcurEntry)
    {
        if (pcurEntry == pActionQueue->Entries[CurrentPrio].pInsertEntry)
            pActionQueue->Entries[CurrentPrio].pInsertEntry = pcurEntry->pNextEntry;
        pActionQueue->Entries[CurrentPrio].pActionRoot = pcurEntry->pNextEntry;
        pcurEntry->pNextEntry = NULL;
    }
    if (pActionQueue->Entries[CurrentPrio].pActionRoot == NULL)
    {
        pActionQueue->Entries[CurrentPrio].pInsertEntry = NULL;
        pActionQueue->Entries[CurrentPrio].pLastEntry = NULL;
    }
    if (pLastEntry)
        pActionQueue->AddToFreeList(pLastEntry); // release entry
    pLastEntry = pcurEntry;
    return pcurEntry;
}

GFxMovieRoot::ActionQueueSessionIterator::ActionQueueSessionIterator(struct GFxMovieRoot::ActionQueueType* pactionQueue, UInt sessionId) :
ActionQueueIterator(pactionQueue), SessionId(sessionId)
{
}

const GFxMovieRoot::ActionEntry* GFxMovieRoot::ActionQueueSessionIterator::getNext()
{
    if (pActionQueue->ModId != ModId)
    {
        // new actions were added - restart
        CurrentPrio = 0;
        ModId = pActionQueue->ModId;
    }
    ActionEntry* pcurEntry = NULL;
    for(;CurrentPrio < AP_Count; ++CurrentPrio)
    {
        pcurEntry = pActionQueue->Entries[CurrentPrio].pActionRoot;
        ActionEntry* pprevEntry = NULL;

        // now search for the node with appropriate session id. Once it is found - 
        // save it and remove from the list
        while(pcurEntry && pcurEntry->SessionId != SessionId)
        {
            pprevEntry = pcurEntry;
            pcurEntry = pcurEntry->pNextEntry;
        }

        if (pcurEntry)
        {
            if (!pprevEntry)
                pActionQueue->Entries[CurrentPrio].pActionRoot = pcurEntry->pNextEntry;
            else
                pprevEntry->pNextEntry = pcurEntry->pNextEntry;

            if (!pcurEntry->pNextEntry)
                pActionQueue->Entries[CurrentPrio].pLastEntry = pprevEntry;

            if (pcurEntry == pActionQueue->Entries[CurrentPrio].pInsertEntry)
            {
                if (!pcurEntry->pNextEntry)
                    pActionQueue->Entries[CurrentPrio].pInsertEntry = pprevEntry;
                else
                    pActionQueue->Entries[CurrentPrio].pInsertEntry = pcurEntry->pNextEntry;
            }

            pcurEntry->pNextEntry = NULL;
            break;
        }
    }
    if (pLastEntry)
        pActionQueue->AddToFreeList(pLastEntry); // release entry
    pLastEntry = pcurEntry;
    return pcurEntry;
}


void    GFxMovieRoot::DoActions()
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer actionsTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_DoActions, Amp_Profile_Level_Low);
#endif

    // Execute the actions in the action list, in the given environment.
    // NOTE: Actions can add other actions to the end of the list while running.
    GFxMovieRoot::ActionQueueIterator iter(&ActionQueue);
    const ActionEntry* paction;
    while((paction = iter.getNext()) != NULL)
    {
        paction->Execute(this);
    }
}

void    GFxMovieRoot::DoActionsForSession(UInt sessionId)
{
    // Execute the actions in the action list, in the given environment.
    // NOTE: Actions can add other actions to the end of the list while running.
    GFxMovieRoot::ActionQueueSessionIterator iter(&ActionQueue, sessionId);
    const ActionEntry* paction;
    while((paction = iter.getNext()) != NULL)
    {
        paction->Execute(this);
    }
}

// Executes actions in this entry
void    GFxMovieRoot::ActionEntry::Execute(GFxMovieRoot *proot) const
{   
    GUNUSED(proot);
    // pCharacter might be null, if only InsertEmptyAction was used, without
    // subsequent call to SetAction.
    if (!pCharacter || pCharacter->IsUnloaded())
        return;

    switch(Type)
    {
    case Entry_Buffer:      
        // Sprite is already AddRefd' so it's guaranteed to stick around.
        pCharacter->ExecuteBuffer(pActionBuffer);
        break;

    case Entry_Event:       
        pCharacter->ExecuteEvent(EventId);      
        break;

    case Entry_Function:        
        pCharacter->ExecuteFunction(Function, FunctionParams);      
        break;

    case Entry_CFunction:        
        pCharacter->ExecuteCFunction(CFunction, FunctionParams);      
        break;

    case Entry_None:
        break;
    }
}


// *** GFxMovieRoot's GFxMovieView interface implementation


void    GFxMovieRoot::SetViewport(const GViewport& viewDesc)
{
    SInt prevLeft = Viewport.Left;
    SInt prevTop  = Viewport.Top;
    SInt prevWidth = Viewport.Width;
    SInt prevHeight = Viewport.Height;
    Float prevScale = Viewport.Scale;
    Float prevRatio = Viewport.AspectRatio;

    G_SetFlag<Flag_ViewportSet>(Flags, true);
    Viewport        = viewDesc;

    if ((Viewport.BufferHeight <= 0 || Viewport.BufferWidth <= 0) && 
        Viewport.Width > 4 && Viewport.Height > 4)  // PPS: Additional check for minimized state
        GFC_DEBUG_WARNING(1, "GFxMovieRoot::SetViewport: buffer size not set");

    GRectF prevVisRect = VisibleFrameRect;
    UpdateViewport();

    if (prevVisRect != VisibleFrameRect ||
        (ViewScaleMode == SM_NoScale && 
            (prevWidth != Viewport.Width || prevHeight != Viewport.Height || 
             prevLeft  != Viewport.Left  || prevTop    != Viewport.Top ||
             prevScale != Viewport.Scale || prevRatio != Viewport.AspectRatio)) ||
        (ViewScaleMode != SM_ExactFit && 
            (prevWidth != Viewport.Width || 
             prevHeight != Viewport.Height || prevRatio != Viewport.AspectRatio)))
    {
#ifndef GFC_NO_FXPLAYER_AS_STAGE
        if (GetLevelMovie(0))
        {
            // queue onResize
            GFxMovieRoot::ActionEntry* pe = InsertEmptyAction(GFxMovieRoot::AP_Frame);
            if (pe) pe->SetAction(GetLevelMovie(0), GASStageCtorFunction::NotifyOnResize, NULL);
        }
#endif // GFC_NO_FXPLAYER_AS_STAGE
    }
}

void    GFxMovieRoot::UpdateViewport()
{
    GRectF prevVisibleFrameRect = VisibleFrameRect;
    Float prevViewOffsetX       = ViewOffsetX;
    Float prevViewOffsetY       = ViewOffsetY;
    Float prevViewScaleX        = ViewScaleX;
    Float prevViewScaleY        = ViewScaleY;
    Float prevPixelScale        = PixelScale;

    // calculate frame rect to be displayed and update    
    if (pLevel0Def)
    {
        // We need to calculate VisibleFrameRect depending on scaleMode and alignment. Alignment is significant only for NoScale mode; 
        // otherwise it is ignored. 
        // Renderer will use VisibleFrameRect to inscribe it into Viewport's rectangle. VisibleFrameRect coordinates are in twips,
        // Viewport's coordinates are in pixels. For simplest case, for ExactFit mode VisibleFrameRect is equal to FrameSize, thus,
        // viewport is filled by the whole scene (the original aspect ratio is ignored in this case, the aspect ratio of viewport
        // is used instead).
        // ViewOffsetX and ViewOffsetY is calculated in stage pixels (not viewport's ones).

        // Viewport rectangle recalculated to twips.
        GRectF viewportRect(PixelsToTwips(Float(Viewport.Left)), 
            PixelsToTwips(Float(Viewport.Top)), 
            PixelsToTwips(Float(Viewport.Left + Viewport.Width)), 
            PixelsToTwips(Float(Viewport.Top + Viewport.Height)));
        const Float viewWidth = viewportRect.Width();
        const Float viewHeight = viewportRect.Height();
        const Float frameWidth = pLevel0Def->GetFrameRectInTwips().Width();
        const Float frameHeight = pLevel0Def->GetFrameRectInTwips().Height();
        switch(ViewScaleMode)
        {
        case SM_NoScale:
            {
                // apply Viewport.Scale and .AspectRatio first
                const Float scaledViewWidth  = viewWidth * Viewport.AspectRatio * Viewport.Scale;
                const Float scaledViewHieght = viewHeight * Viewport.Scale;

                // calculate a VisibleFrameRect
                switch(ViewAlignment)
                {
                case Align_Center:
                    // We need to round centering to pixels in noScale mode, otherwise we lose pixel center alignment.
                    VisibleFrameRect.Left = (Float)PixelsToTwips((int)TwipsToPixels(frameWidth/2 - scaledViewWidth/2));
                    VisibleFrameRect.Top  = (Float)PixelsToTwips((int)TwipsToPixels(frameHeight/2 - scaledViewHieght/2));
                    break;
                case Align_TopLeft:
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top  = 0;
                    break;
                case Align_TopCenter:
                    VisibleFrameRect.Left = (Float)PixelsToTwips((int)TwipsToPixels(frameWidth/2 - scaledViewWidth/2));
                    VisibleFrameRect.Top  = 0;
                    break;
                case Align_TopRight:
                    VisibleFrameRect.Left = frameWidth - scaledViewWidth;
                    VisibleFrameRect.Top  = 0;
                    break;
                case Align_BottomLeft:
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top  = frameHeight - scaledViewHieght;
                    break;
                case Align_BottomCenter:
                    VisibleFrameRect.Left = (Float)PixelsToTwips((int)TwipsToPixels(frameWidth/2 - scaledViewWidth/2));
                    VisibleFrameRect.Top  = frameHeight - scaledViewHieght;
                    break;
                case Align_BottomRight:
                    VisibleFrameRect.Left = frameWidth - scaledViewWidth;
                    VisibleFrameRect.Top  = frameHeight - scaledViewHieght;
                    break;
                case Align_CenterLeft:
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top  = (Float)PixelsToTwips((int)TwipsToPixels(frameHeight/2 - scaledViewHieght/2));
                    break;
                case Align_CenterRight:
                    VisibleFrameRect.Left = frameWidth - scaledViewWidth;
                    VisibleFrameRect.Top  = (Float)PixelsToTwips((int)TwipsToPixels(frameHeight/2 - scaledViewHieght/2));
                    break;
                }
                VisibleFrameRect.SetWidth(scaledViewWidth);
                VisibleFrameRect.SetHeight(scaledViewHieght);
                ViewOffsetX = TwipsToPixels(VisibleFrameRect.Left);
                ViewOffsetY = TwipsToPixels(VisibleFrameRect.Top);
                ViewScaleX = Viewport.AspectRatio * Viewport.Scale;
                ViewScaleY = Viewport.Scale;
                break;
            }
        case SM_ShowAll: 
        case SM_NoBorder: 
            {
                const Float viewWidthWithRatio = viewWidth * Viewport.AspectRatio;
                // For ShowAll and NoBorder we need to apply AspectRatio to viewWidth in order
                // to calculate correct VisibleFrameRect and scales.
                // Scale is ignored for these modes.
                if ((ViewScaleMode == SM_ShowAll && viewWidthWithRatio/frameWidth < viewHeight/frameHeight) ||
                    (ViewScaleMode == SM_NoBorder && viewWidthWithRatio/frameWidth > viewHeight/frameHeight))
                {
                    Float visibleHeight = frameWidth * viewHeight / viewWidthWithRatio;    
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top = frameHeight/2 - visibleHeight/2;
                    VisibleFrameRect.SetWidth(frameWidth);
                    VisibleFrameRect.SetHeight(visibleHeight);

                    ViewOffsetX = 0;
                    ViewOffsetY = TwipsToPixels(VisibleFrameRect.Top);
                    ViewScaleX = viewWidth ? (frameWidth / viewWidth) : 0.0f;
                    ViewScaleY = ViewScaleX / Viewport.AspectRatio;
                }
                else 
                {
                    Float visibleWidth = frameHeight * viewWidthWithRatio / viewHeight;    
                    VisibleFrameRect.Left = frameWidth/2 - visibleWidth/2;
                    VisibleFrameRect.Top = 0;
                    VisibleFrameRect.SetWidth(visibleWidth);
                    VisibleFrameRect.SetHeight(frameHeight);

                    ViewOffsetX = TwipsToPixels(VisibleFrameRect.Left);
                    ViewOffsetY = 0;
                    ViewScaleY = viewHeight ? (frameHeight / viewHeight) : 0.0f;
                    ViewScaleX = ViewScaleY * Viewport.AspectRatio;
                }
            }
            break;
        case SM_ExactFit: 
            // AspectRatio and Scale is ignored for this mode.
            VisibleFrameRect.Left = VisibleFrameRect.Top = 0;
            VisibleFrameRect.SetWidth(frameWidth);
            VisibleFrameRect.SetHeight(frameHeight);
            ViewOffsetX = ViewOffsetY = 0;
            ViewScaleX = viewWidth  ? (VisibleFrameRect.Width() / viewWidth) : 0.0f;
            ViewScaleY = viewHeight ? (VisibleFrameRect.Height() / viewHeight) : 0.0f;
            break;
        }
        PixelScale = G_Max((ViewScaleX == 0) ? 0.005f : 1.0f/ViewScaleX, 
                           (ViewScaleY == 0) ? 0.005f : 1.0f/ViewScaleY);    
    }
    else
    {
        ViewOffsetX = ViewOffsetY = 0;
        ViewScaleX = ViewScaleY = 1.0f;
        PixelScale = GFC_TWIPS_TO_PIXELS(20.0f);
    }
    //  RendererInfo.ViewportScale = PixelScale;

    ViewportMatrix = GMatrix2D::Translation(-VisibleFrameRect.Left, -VisibleFrameRect.Top);
    ViewportMatrix.AppendScaling(Viewport.Width / VisibleFrameRect.Width(),
                                 Viewport.Height / VisibleFrameRect.Height());
    ViewportMatrix.AppendTranslation(Float(Viewport.Left), Float(Viewport.Top));
  //  RendererInfo.ViewportScale = PixelScale;

    if (prevVisibleFrameRect != VisibleFrameRect || prevViewOffsetX != ViewOffsetX ||
        prevViewOffsetY      != ViewOffsetY      || prevViewScaleX  != ViewScaleX  ||
        prevViewScaleY       != ViewScaleY       || prevPixelScale  != PixelScale)
    {
        SetDirtyFlag();
#ifndef GFC_NO_3D
        UpdateViewAndPerspective();
#endif
    }
}

void    GFxMovieRoot::GetViewport(GViewport *pviewDesc) const
{
    *pviewDesc = Viewport;
}

void    GFxMovieRoot::SetViewScaleMode(ScaleModeType scaleMode)
{
    ViewScaleMode = scaleMode;
    SetViewport(Viewport);
}

void    GFxMovieRoot::SetViewAlignment(AlignType align)
{
    ViewAlignment = align;
    SetViewport(Viewport);
}

GFxASCharacter* GFxMovieRoot::GetTopMostEntity(const GPointF& mousePos, 
                                               UInt controllerIdx, 
                                               bool testAll, 
                                               const GFxASCharacter* ignoreMC)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer funcTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_GetTopMostEntity, Amp_Profile_Level_Low);
#endif

#ifndef GFC_NO_3D
    float w = (VisibleFrameRect.Right - VisibleFrameRect.Left);
    float h = (VisibleFrameRect.Bottom - VisibleFrameRect.Top);

    float nsx = 2.f * ((mousePos.x - PixelsToTwips(ViewOffsetX)) / w) - 1.0f;
    float nsy = 2.f * ((mousePos.y - PixelsToTwips(ViewOffsetY)) / h) - 1.0f;

    ScreenToWorld.SetNormalizedScreenCoords(nsx, nsy);    // -1 to 1
#endif

    GFxASCharacter* ptopMouseCharacter = 0;
    // look for chars marked as topmostLevel first.
    GFxCharacter::TopMostParams params(this, controllerIdx, testAll, ignoreMC);
    for (int i = (int)TopmostLevelCharacters.GetSize() - 1; i >= 0; --i)
    {
        GFxCharacter* pch = TopmostLevelCharacters[i];
        GASSERT(pch);

        GRenderer::Matrix matrix(pch->GetParent()->GetWorldMatrix());
        GPointF pp = matrix.TransformByInverse(mousePos);

        // need to test with 'testAll' = true first to detect any overlappings
        ptopMouseCharacter = pch->GetTopMostMouseEntity(pp, params);
        if (ptopMouseCharacter)
            break;
    }
    if (!ptopMouseCharacter)
    {
        for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
        {   
            GFxSprite* pmovie  = MovieLevels[movieIndex-1].pSprite;
            ptopMouseCharacter = pmovie->GetTopMostMouseEntity(mousePos, params);
            if (ptopMouseCharacter)
                break;
        }
    }
    return ptopMouseCharacter;
}

void GFxMovieRoot::GetStats(GStatBag* pbag, bool reset) 
{
    if (AdvanceStats)
    {
        AdvanceStats->GetStats(pbag, reset);
    }
    if (DisplayStats)
    {
        DisplayStats->GetStats(pbag, reset);
    }
}


template<typename T>
struct PtrGuard
{
    PtrGuard(T** ptr) : Ptr(ptr) {}
    ~PtrGuard() { if(*Ptr)  {  (*Ptr)->Release();  *Ptr = NULL; } }
    T** Ptr;
};

Float    GFxMovieRoot::Advance(Float deltaT, UInt frameCatchUpCount)
{   
    if (G_IsFlagSet<Flag_Paused>(Flags))
        return 0.05f;

    if (!MovieLevels.GetSize())
        return pLevel0Def ? (1.0f / pLevel0Def->GetFrameRate()) : 0.0f;
    // DBG
    //printf("GFxMovieRoot::Advance %d   -------------------\n", pLevel0Movie->GetLevelMovie(0)->ToSprite()->GetCurrentFrame());

#if !defined(GFC_NO_STAT)
    ScopeFunctionTimer advanceTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_Advance, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    if (deltaT < 0.0f)
    {
        GFC_DEBUG_WARNING(1, "GFxMovieView::Advance - negative deltaT argument specified");
        deltaT = 0.0f;
    }

    G_SetFlag<Flag_AdvanceCalled>(Flags, true);

    // Advance - Cache states for faster non-locked access.
    GPtr<GFxActionControl>           pac;

#ifndef GFC_NO_SOUND
    GFxState*                        pstates[11]    = {0,0,0,0,0,0,0,0,0,0,0};
    const static GFxState::StateType stateQuery[11] =
    { GFxState::State_UserEventHandler, GFxState::State_FSCommandHandler,
      GFxState::State_ExternalInterface, GFxState::State_ActionControl,
      GFxState::State_Log, GFxState::State_FontLib, GFxState::State_FontMap,
      GFxState::State_FontProvider, GFxState::State_Translator, GFxState::State_RenderConfig, GFxState::State_Audio};
    // Get multiple states at once to avoid extra locking.
    pStateBag->GetStatesAddRef(pstates, stateQuery, 11);
#else
    GFxState*                        pstates[10]    = {0,0,0,0,0,0,0,0,0,0};
    const static GFxState::StateType stateQuery[10] =
    { GFxState::State_UserEventHandler, GFxState::State_FSCommandHandler,
    GFxState::State_ExternalInterface, GFxState::State_ActionControl,
    GFxState::State_Log, GFxState::State_FontLib, GFxState::State_FontMap,
    GFxState::State_FontProvider, GFxState::State_Translator, GFxState::State_RenderConfig};
    // Get multiple states at once to avoid extra locking.
    pStateBag->GetStatesAddRef(pstates, stateQuery, 10);
#endif
  
    pUserEventHandler   = *(GFxUserEventHandler*)  pstates[0];
    pFSCommandHandler   = *(GFxFSCommandHandler*)  pstates[1];
    pExtIntfHandler     = *(GFxExternalInterface*) pstates[2];
    pac                 = *(GFxActionControl*)     pstates[3];
    pCachedLog          = *(GFxLog*)               pstates[4];

    if (pRenderConfig != pstates[9])
    {
        for (UInt i = 0; i < MovieLevels.GetSize(); i++)
            MovieLevels[i].pSprite->OnRendererChanged();
    }
    pRenderConfig       = *(GFxRenderConfig*)      pstates[9];

    // Check if FontLib, FontMap, FontProvider or Translator is changed
    UInt8 stateChangeFlag = 0u;
    {
        GPtr<GFxFontLib>      fl = *(GFxFontLib*)            pstates[5];
        GPtr<GFxFontMap>      fm = *(GFxFontMap*)            pstates[6];
        GPtr<GFxFontProvider> fp = *(GFxFontProvider*)       pstates[7];
        GPtr<GFxTranslator>   tr = *(GFxTranslator*)         pstates[8];
        stateChangeFlag = pFontManagerStates->CheckStateChange(fl,fm,fp,tr);
    }

    // Set flag indicating that the log is cached.
    G_SetFlag<Flag_CachedLogFlag>(Flags, true);
    
    if (pac)
    {
        G_SetFlag<Flag_VerboseAction>(Flags, (pac->GetActionFlags() & GFxActionControl::Action_Verbose) != 0);
        G_SetFlag<Flag_SuppressActionErrors>(Flags, (pac->GetActionFlags() & GFxActionControl::Action_ErrorSuppress) != 0);
        G_SetFlag<Flag_LogRootFilenames>(Flags, (pac->GetActionFlags() & GFxActionControl::Action_LogRootFilenames) != 0);
        G_SetFlag<Flag_LogLongFilenames>(Flags, (pac->GetActionFlags() & GFxActionControl::Action_LongFilenames) != 0);
        G_SetFlag<Flag_LogChildFilenames>(Flags, (pac->GetActionFlags() & GFxActionControl::Action_LogChildFilenames) != 0);
    }
    else
    {
        G_SetFlag<Flag_VerboseAction>(Flags, 0);
        G_SetFlag<Flag_SuppressActionErrors>(Flags, 0);
        G_SetFlag<Flag_LogRootFilenames>(Flags, 0);
        G_SetFlag<Flag_LogLongFilenames>(Flags, 0);
        G_SetFlag<Flag_LogChildFilenames>(Flags, 0);
    }

#ifndef GFC_NO_SOUND
    pAudio = (GFxAudioBase*) pstates[10];
    PtrGuard<GFxAudioBase> as_ptr(&pAudio);
    if (pAudio)
    {
        pSoundRenderer = pAudio->GetRenderer();
        if(pSoundRenderer)
            pSoundRenderer->AddRef();
    }
    PtrGuard<GSoundRenderer> sr_ptr(&pSoundRenderer);
#endif    
    // Capture frames loaded so that the value of a loaded frame will be consistent
    // for all root nodes that share the same MovieDefImpl. This ensures that if
    // there 'loadMovie' is used several times on the same progressively loaded SWF,
    // all instances will see the same progress state.

    GFxMovieDefRootNode *pdefNode = RootMovieDefNodes.GetFirst();
    while(!RootMovieDefNodes.IsNull(pdefNode))
    {        
        // Bytes loaded must always be grabbed before the frame, since there is a data
        // race between value returned 'getBytesLoaded' and the frame we can seek to.
        if (!pdefNode->ImportFlag)
        {
            pdefNode->BytesLoaded  = pdefNode->pDefImpl->GetBytesLoaded();
            pdefNode->LoadingFrame = pdefNode->pDefImpl->GetLoadingFrame();
        }

        // if FontLib, FontMap, FontProvider or Translator is changed we need to 
        // to clean FontManager cache
        if (stateChangeFlag)
            pdefNode->pFontManager->CleanCache();

        pdefNode = pdefNode->pNext;
    }

    // Notify TextField that states changed
    if (stateChangeFlag)
    {
        for (UInt i = 0; i < MovieLevels.GetSize(); i++)
            MovieLevels[i].pSprite->SetStateChangeFlags(stateChangeFlag);
    }

    // Shadowed value from GFxSprite::GetLoadingFrame would have been grabbed in
    // the RootMovieDefNodes traversal above, so it must come after it.
    if ((pLevel0Movie->GetLoadingFrame() == 0))
    {
        G_SetFlag<Flag_CachedLogFlag>(Flags, 0);
        return 0;
    }

    // Execute loading/frame0 events for root level clips, if necessary.
    if (G_IsFlagSet<Flag_LevelClipsChanged>(Flags) && (pLevel0Movie->GetLoadingFrame() > 0))
    {
        G_SetFlag<Flag_LevelClipsChanged>(Flags, false);      
        // Queue up load events, tags and actions for frame 0.
        for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
        {
            // ExecuteFrame0Events does an internal check, so this will only have an effect once.
            MovieLevels[movieIndex-1].pSprite->ExecuteFrame0Events();
        }

        // It is important to execute frame 0 events here before we get to AdvanceFrame() below,
        // so that actions like stop() are applied in timely manner.
        DoActions();
        ProcessUnloadQueue();
        ProcessLoadQueue();
    }

    // *** Advance the frame based on time

    TimeElapsed += UInt64(deltaT * 1000);
    TimeRemainder += deltaT;    

//if (TimeRemainder >= FrameTime)
//    MovieLevels[0].pSprite->DumpDisplayList(0, "_LEVEL");

    UInt64 advanceStart = GTimer::GetProfileTicks();
    Float minDelta = FrameTime;

    if (IntervalTimers.GetSize() > 0)
    {
        UPInt i, n;
        UInt needCompress = 0;
        for (i = 0, n = IntervalTimers.GetSize(); i < n; ++i)
        {
            if (IntervalTimers[i] && IntervalTimers[i]->IsActive())
            {
                IntervalTimers[i]->Invoke(this, FrameTime);

                Float delta = Float((IntervalTimers[i]->GetNextInvokeTime() - TimeElapsed))/1000.f;
                if (delta < minDelta)
                    minDelta = delta;
            }
            else
                ++needCompress;
        }
        if (needCompress)
        {
            n = IntervalTimers.GetSize(); // size could be changed after Invoke
            UInt j;
            // remove empty entries
            for (i = j = 0; i < n; ++i)
            {
                if (!IntervalTimers[j] || !IntervalTimers[j]->IsActive())
                {
                    delete IntervalTimers[j];
                    IntervalTimers.RemoveAt(j);
                }
                else
                    ++j;
            }
        }
    }
#ifndef GFC_NO_VIDEO
    if (VideoProviders.GetSize() > 0)
    {
        GArray<GFxVideoProvider*> remove_list;
        for(GHashSet<GPtr<GFxVideoProvider> >::Iterator it = VideoProviders.Begin(); it != VideoProviders.End(); ++it)
        {
            GPtr<GFxVideoProvider> pvideo = *it;
            // we have to call Progress method here and not from VideoObject AdvanceFrame method 
            // because there could be videos that are not attached to any VideoObject but still they 
            // have to progress (only sound in this case is produced)
            pvideo->Advance();
            if (pvideo->IsActive())
            {
                if( pvideo->GetFrameTime() < minDelta)
                    minDelta = pvideo->GetFrameTime();
            }
            else {
                pvideo->Close();
                remove_list.PushBack(pvideo);
            }
        }
        for(UPInt i = 0; i < remove_list.GetSize(); ++i)
            VideoProviders.Remove(remove_list[i]);
    }
#endif
#ifndef GFC_NO_SOUND
    if (pSoundRenderer)
    {
        Float delta = pSoundRenderer->Update();
        if (delta < minDelta)
            minDelta = delta;
    }
#endif

    ProcessInput();

   
    if (TimeRemainder >= FrameTime)
    {
        // Mouse, keyboard and timer handlers above may queue up actions which MUST be executed before the
        // Advance. For example, if a mouse event contains "gotoAndPlay(n)" action and Advance is
        // going to queue up a frame with "stop" action, then that gotoAndPlay will be incorrectly stopped.
        // By executing actions before the Advance we avoid queuing the stop-frame and gotoAndPlay will
        // work correctly. (AB, 12/13/06)
        DoActions();

        bool frameCatchUp = frameCatchUpCount > 0 || ForceFrameCatchUp > 0;
        do
        {
            if (frameCatchUp)
                TimeRemainder -= FrameTime;
            else
                TimeRemainder = (Float)fmod(TimeRemainder, FrameTime);

            Float framePos = (TimeRemainder >= FrameTime) ? 0.0f : (TimeRemainder / FrameTime);

            // Advance a frame.
            {
                AdvanceFrame(1, framePos);
#ifndef GFC_NO_VIDEO
                // we need to call advance for video players here also because when we synchronize
                // swf timeline and video frames we need to notify video players that our the timer has changed.
                if (VideoProviders.GetSize() > 0)
                {
                    GArray<GFxVideoProvider*> remove_list;
                    for(GHashSet<GPtr<GFxVideoProvider> >::Iterator it = VideoProviders.Begin(); it != VideoProviders.End(); ++it)
                    {
                        GPtr<GFxVideoProvider> pvideo = *it;
                        // we have to call Progress method here and not from VideoObject AdvanceFrame method 
                        // because there could be videos that are not attached to any VideoObject but still they 
                        // have to progress (only sound in this case is produced)
                        pvideo->Advance();
                        if (pvideo->IsActive())
                        {
                            if( pvideo->GetFrameTime() < minDelta)
                                minDelta = pvideo->GetFrameTime();
                        }
                        else {
                            pvideo->Close();
                            remove_list.PushBack(pvideo);
                        }
                    }
                    for(UPInt i = 0; i < remove_list.GetSize(); ++i)
                        VideoProviders.Remove(remove_list[i]);
                }
#endif
            }

            // Execute actions queued up due to actions and mouse.
            DoActions();
            ProcessUnloadQueue();
            ProcessLoadQueue();
            if (ForceFrameCatchUp > 0)
                ForceFrameCatchUp--;

        } while((frameCatchUpCount-- > 0 && TimeRemainder >= FrameTime) || ForceFrameCatchUp > 0);
        // Force GetTopmostMouse update in next Advance so that buttons detect change, if any.
        G_SetFlag<Flag_NeedMouseUpdate>(Flags, true);

        // Let refcount collector to do its job
#ifndef GFC_NO_GC
        MemContext->ASGC->AdvanceFrame(&NumAdvancesSinceCollection, &LastCollectionFrame);
#endif
    }
    else
    {
        // Fractional advance only.
        Float framePos = TimeRemainder / FrameTime;
        AdvanceFrame(0, framePos);

        TimeRemainder = (Float)fmod(TimeRemainder, FrameTime);

        // Technically AdvanceFrame(0) above should not generate any actions.
        // However, we need to execute actions queued up due to mouse.
        DoActions();
        ProcessUnloadQueue();
        ProcessLoadQueue();
    }    

    ResetTabableArrays();
    G_SetFlag<Flag_CachedLogFlag>(Flags, false);

    // DBG
    //printf("GFxMovieRoot::Advance %d   ^^^^^^^^^^^^^^^^^\n", pLevel0Movie->GetLevelMovie(0)->ToSprite()->GetCurrentFrame());
    UInt64 advanceStop = GTimer::GetProfileTicks();
    minDelta -= (advanceStop - advanceStart)/1000000.0f;
    if (minDelta < 0.0f)
        minDelta = 0.0f;
    return G_Min(minDelta, FrameTime - TimeRemainder);
}

#ifdef GFC_USE_OLD_ADVANCE
void GFxMovieRoot::AdvanceFrame(bool nextFrame, Float framePos)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timelineTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_AdvanceFrame);
#endif
    for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
    {
        GFxSprite* pmovie = MovieLevels[movieIndex-1].pSprite;
        pmovie->AdvanceFrame(nextFrame, framePos);
    }
}

void GFxMovieRoot::ReleaseUnloadList()
{
}
#else
void GFxMovieRoot::ReleaseUnloadList()
{
    GFxASCharacter* pnextCh = NULL;
    for (GFxASCharacter* pcurCh = pUnloadListHead; pcurCh; pcurCh = pnextCh)
    {
        GASSERT(pcurCh->IsUnloaded());
        GASSERT(!pcurCh->IsOptAdvancedListFlagSet());

        pnextCh = pcurCh->pNextUnloaded;
        pcurCh->pNextUnloaded = NULL;
        pcurCh->Release();
    }
    pUnloadListHead = NULL;
}

void GFxMovieRoot::AdvanceFrame(bool nextFrame, Float framePos)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timelineTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_AdvanceFrame, Amp_Profile_Level_Low);
#endif

#ifdef GFC_ADVANCE_EXECUTION_TRACE
    bool first = true;
#endif
    // If optimized advance list is invalidated then we need to 
    // recreate it. We need a separate loop iteration, since call to Advance
    // may modify the optimized advance list and this may cause incorrect behavior.
    if (IsOptAdvanceListInvalid())
    {
        pPlayListOptHead = NULL;
        G_SetFlag<Flag_OptimizedAdvanceListInvalid>(Flags, false);
        GFxASCharacter* pnextCh = NULL;
        for (GFxASCharacter* pcurCh = pPlayListHead; pcurCh; pcurCh = pnextCh)
        {
            pnextCh = pcurCh->pPlayNext;
            pcurCh->ClearOptAdvancedListFlag();
            pcurCh->pPlayNextOpt = NULL;

            // "unloaded" chars should be already removed.
            GASSERT(!pcurCh->IsUnloaded());

            if (pcurCh->CanAdvanceChar())
            {
                if (pcurCh->CheckAdvanceStatus(false) == 1)
                    pcurCh->AddToOptimizedPlayList(this);
                if (nextFrame || pcurCh->IsReqPartialAdvanceFlagSet())
                {
#ifdef GFC_ADVANCE_EXECUTION_TRACE
                    if (first)
                        printf("\n");
                    first = false;
                    printf("!------------- %s\n", pcurCh->GetCharacterHandle()->GetNamePath().ToCStr());
#endif
                    pcurCh->AdvanceFrame(nextFrame, framePos);          
                }
            }
        }
    }
    else
    {
        GFxASCharacter* pnextCh = NULL;
        for (GFxASCharacter* pcurCh = pPlayListOptHead; pcurCh; pcurCh = pnextCh)
        {
            GASSERT(!pcurCh->IsUnloaded());
            GASSERT(pcurCh->IsOptAdvancedListFlagSet());

            pnextCh = pcurCh->pPlayNextOpt;

            if (pcurCh->CanAdvanceChar() && (nextFrame || pcurCh->IsReqPartialAdvanceFlagSet()))
            {
#ifdef GFC_ADVANCE_EXECUTION_TRACE
                if (first)
                    printf("\n");
                first = false;
                printf("+------------- %s\n", pcurCh->GetCharacterHandle()->GetNamePath().ToCStr());
#endif
                pcurCh->AdvanceFrame(nextFrame, framePos);          
            }
        }
    }

#ifdef GFC_ADVANCE_EXECUTION_TRACE
    if (!first)
        printf("\n");
#endif
}
#endif

void GFxMovieRoot::ProcessInput()
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timelineTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_ProcessInput, Amp_Profile_Level_Low);
#endif

#if !defined(GFC_NO_KEYBOARD_SUPPORT) || !defined(GFC_NO_FXPLAYER_AS_MOUSE)
    // *** Handle keyboard / mice
    if (pLevel0Movie) 
    {
        ProcessFocusKeyInfo focusKeyInfo;
        GASEnvironment*     penv = pLevel0Movie->GetASEnvironment();
        GASStringContext*   psc = penv->GetSC();
        UInt32              miceProceededMask = 0;
        UInt32              miceSupportedMask = ((1U << MouseCursorCount) - 1);

        while (!InputEventsQueue.IsQueueEmpty())
        {
            const GFxInputEventsQueue::QueueEntry* qe = InputEventsQueue.GetEntry();
            if (qe->IsKeyEntry())
            {
                ProcessKeyboard(psc, qe, &focusKeyInfo);
            }
#ifndef GFC_NO_FXPLAYER_AS_MOUSE
            else if (qe->IsMouseEntry())
            {
                ProcessMouse(penv, qe, &miceProceededMask);
            }
#endif //GFC_NO_FXPLAYER_AS_MOUSE
        }

#ifndef GFC_NO_FXPLAYER_AS_MOUSE
        if (G_IsFlagSet<Flag_NeedMouseUpdate>(Flags) && (miceProceededMask & miceSupportedMask) != miceSupportedMask)
        {
            for (UInt mi = 0, mask = 1; mi < MouseCursorCount; ++mi, mask <<= 1)
            {
                if (!(miceProceededMask & mask))
                {
                    if (!MouseState[mi].IsActivated())
                        continue; // this mouse is not activated yet
                    MouseState[mi].ResetPrevButtonsState();
                    GPtr<GFxASCharacter> ptopMouseCharacter = 
                        GetTopMostEntity(MouseState[mi].GetLastPosition(), mi, false);
                    MouseState[mi].SetTopmostEntity(ptopMouseCharacter);    

                    // check for necessity of changing cursor
                    CheckMouseCursorType(mi, ptopMouseCharacter);

                    GFx_GenerateMouseButtonEvents(mi, &MouseState[mi], penv->CheckExtensions() ? 16 : 1);
                }
            }
        }
#else
        GUNUSED2(miceProceededMask, miceSupportedMask);
#endif //GFC_NO_FXPLAYER_AS_MOUSE

        FinalizeProcessFocusKey(&focusKeyInfo);
        G_SetFlag<Flag_NeedMouseUpdate>(Flags, false);
    }
#endif //if !defined(GFC_NO_KEYBOARD_SUPPORT) || !defined(GFC_NO_FXPLAYER_AS_MOUSE)
}

void GFxMovieRoot::ProcessKeyboard(GASStringContext* psc, 
                                   const GFxInputEventsQueue::QueueEntry* qe, 
                                   ProcessFocusKeyInfo* focusKeyInfo)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timelineTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_ProcessKeyboard, Amp_Profile_Level_Low);
#endif

#ifndef GFC_NO_KEYBOARD_SUPPORT
    int                 keyMask = 0;
    const GFxInputEventsQueue::QueueEntry::KeyEntry& keyEntry = qe->GetKeyEntry();
    // keyboard
    if (keyEntry.Code != 0)
    {
        // key down / up
        GFxEventId::IdCode eventIdCode;
        GFxEvent::EventType event;
        if (keyEntry.KeyIsDown)
        {
            eventIdCode = GFxEventId::Event_KeyDown;
            event       = GFxEvent::KeyDown;
        }
        else
        {
            eventIdCode = GFxEventId::Event_KeyUp;
            event       = GFxEvent::KeyUp;
        }
        GFxEventId eventId((UByte)eventIdCode, keyEntry.Code, keyEntry.AsciiCode, 
            keyEntry.WcharCode, keyEntry.KeyboardIndex);
        eventId.SpecialKeysState = keyEntry.SpecialKeysState;

        //printf ("Key %d (ASCII %d) is %s\n", code, asciiCode, (event == GFxEvent::KeyDown) ? "Down":"Up");

        for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
            MovieLevels[movieIndex-1].pSprite->PropagateKeyEvent(eventId, &keyMask);

        const GFxKeyboardState* keyboardState = GetKeyboardState(keyEntry.KeyboardIndex);
        GASSERT(keyboardState);
        keyboardState->NotifyListeners(psc, keyEntry.Code, keyEntry.AsciiCode, keyEntry.WcharCode, event);

        if (!IsDisableFocusKeys())
            ProcessFocusKey(event, keyEntry, focusKeyInfo);
    }
    else if (keyEntry.WcharCode != 0)
    {
        // char
        FocusGroupDescr& focusGroup = GetFocusGroup(keyEntry.KeyboardIndex);
        GPtr<GFxASCharacter> curFocused = focusGroup.LastFocused;
        if (curFocused)
        {
            curFocused->OnCharEvent(keyEntry.WcharCode, keyEntry.KeyboardIndex);
        }
    }
#else
    GUNUSED(psc);
#endif //GFC_NO_KEYBOARD_SUPPORT
}

void GFxMovieRoot::ProcessMouse(GASEnvironment* penv, const GFxInputEventsQueue::QueueEntry* qe, UInt32* miceProceededMask)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer timelineTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_ProcessMouse, Amp_Profile_Level_Low);
#endif

    // Proceed mouse
    *miceProceededMask |= (1 << qe->GetMouseEntry().MouseIndex);

    UInt mi = qe->GetMouseEntry().MouseIndex;
    MouseState[mi].UpdateState(*qe);

    GPtr<GFxASCharacter> ptopMouseCharacter = GetTopMostEntity(qe->GetMouseEntry().GetPosition(), mi, false);
    MouseState[mi].SetTopmostEntity(ptopMouseCharacter);    

    /*//Debug code
    if (lastState != (MouseButtons & 1)) 
    {
    if (ptopMouseCharacter)  //?
    printf("! %s\n", ptopMouseCharacter->GetCharacterHandle()->GetNamePath().ToCStr());
    } 
    if (ptopMouseCharacter)  //?
    {
    printf("? %s\n", ptopMouseCharacter->GetCharacterHandle()->GetNamePath().ToCStr());
    }*/ 

#ifndef GFC_NO_IME_SUPPORT
    UInt buttonsState = MouseState[mi].GetButtonsState();
    // notify IME about mouse down/up
    if (qe->GetMouseEntry().IsButtonsStateChanged())
    {
        GPtr<GFxIMEManager> pIMEManager = GetIMEManager();
        if (pIMEManager && pIMEManager->IsMovieActive(this))
        {
            if (qe->GetMouseEntry().IsAnyButtonPressed())
                pIMEManager->OnMouseDown(this, buttonsState, ptopMouseCharacter);
            else
                pIMEManager->OnMouseUp(this, buttonsState, ptopMouseCharacter);
        }
    }
#endif //#ifndef GFC_NO_IME_SUPPORT

    // Send mouse events.
    GFxEventId::IdCode buttonEvent = GFxEventId::Event_Invalid;
    if (qe->GetMouseEntry().IsButtonsStateChanged() && qe->GetMouseEntry().IsLeftButton())
    {
        buttonEvent = (qe->GetMouseEntry().IsAnyButtonPressed()) ?
            GFxEventId::Event_MouseDown : GFxEventId::Event_MouseUp;
    }

    for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
    {
        GFxSprite* pmovie = MovieLevels[movieIndex-1].pSprite;

        // Send mouse up/down events.
        if (buttonEvent != GFxEventId::Event_Invalid)
            pmovie->PropagateMouseEvent(GFxButtonEventId(buttonEvent, mi));
        // Send move.
        if (MouseState[mi].IsMouseMoved())
            pmovie->PropagateMouseEvent(GFxButtonEventId(GFxEventId::Event_MouseMove, mi));
    }
    if (MouseState[mi].IsMouseMoved())
    {
        if (!IsDisableFocusAutoRelease())
            HideFocusRect(mi);
    }

    if (qe->GetMouseEntry().IsMouseWheel())
    {
        // no need to propagate the mouse wheel, just sent 
        if (ptopMouseCharacter)
            ptopMouseCharacter->OnMouseWheelEvent(qe->GetMouseEntry().WheelScrollDelta);
    }

    if (pASMouseListener && !pASMouseListener->IsEmpty() && 
        (MouseState[mi].IsMouseMoved() || qe->GetMouseEntry().IsButtonsStateChanged() || 
        qe->GetMouseEntry().IsMouseWheel()))
    {
        // Notify Mouse class' AS listeners

        bool extensions = penv->CheckExtensions();
        if (MouseState[mi].IsMouseMoved())
            pASMouseListener->OnMouseMove(penv, mi);
        if (qe->GetMouseEntry().IsMouseWheel() || qe->GetMouseEntry().IsButtonsStateChanged())
        {
            GPtr<GFxASCharacter> ptopMouseCharacter = GetTopMostEntity(qe->GetMouseEntry().GetPosition(), mi, true);
            if (qe->GetMouseEntry().IsMouseWheel())
                pASMouseListener->OnMouseWheel(penv, mi, qe->GetMouseEntry().WheelScrollDelta, ptopMouseCharacter);
            if (qe->GetMouseEntry().IsButtonsStateChanged())
            {
                for (UInt mask = 1, buttonId = 1; GFxMouseState::MouseButton_AllMask & mask; mask <<= 1, ++buttonId)
                {
                    if (qe->GetMouseEntry().ButtonsState & mask)
                    {
                        if (qe->GetMouseEntry().IsAnyButtonPressed())
                            pASMouseListener->OnMouseDown(penv, mi, buttonId, ptopMouseCharacter);
                        else
                            pASMouseListener->OnMouseUp(penv, mi, buttonId, ptopMouseCharacter);
                    }
                    if (!extensions) break; // support only left button, if extensions are not enabled
                }
            }
        }
    }

    // check for necessity of changing cursor
    CheckMouseCursorType(mi, ptopMouseCharacter);

    // Send onKillFocus, if necessary
    if (qe->GetMouseEntry().IsAnyButtonPressed() && qe->GetMouseEntry().IsLeftButton())
    {
        GPtr<GFxASCharacter> curFocused = GetFocusGroup(mi).LastFocused;
        if (ptopMouseCharacter != curFocused)
        {
            if (curFocused && curFocused->DoesAcceptMouseFocus())
            { 
                // if left mouse button was clicked on canvas or on character that doesn't 
                // accept mouse focus (the only character accepts mouse focus is selectable
                // textfield) then need to queue up the "losing focus" event. Otherwise, 
                // transfer the focus.

                if (!ptopMouseCharacter || !ptopMouseCharacter->DoesAcceptMouseFocus())
                    QueueSetFocusTo(NULL, ptopMouseCharacter, mi, GFx_FocusMovedByMouse);
                else
                    QueueSetFocusTo(ptopMouseCharacter, ptopMouseCharacter, mi, GFx_FocusMovedByMouse);
            }
            else if (ptopMouseCharacter && ptopMouseCharacter->DoesAcceptMouseFocus())
                QueueSetFocusTo(ptopMouseCharacter, ptopMouseCharacter, mi, GFx_FocusMovedByMouse);
        }
    }

    //!AB: button's events, such as onPress/onRelease should be fired AFTER
    // appropriate mouse events (onMouseDown/onMouseUp).
    GFx_GenerateMouseButtonEvents(mi, &MouseState[mi], penv->CheckExtensions() ? 16 : 1);
}

void GFxMovieRoot::ProcessUnloadQueue()
{
    // process unload queue
    if (pUnloadListHead)
    {
        GFxASCharacter* pnextCh = NULL;
        for (GFxASCharacter* pcurCh = pUnloadListHead; pcurCh; pcurCh = pnextCh)
        {
            GASSERT(pcurCh->IsUnloaded());
            GASSERT(!pcurCh->IsOptAdvancedListFlagSet());
#ifdef GFC_BUILD_DEBUG
            GFxASCharacter* psavedParent = pcurCh->GetParent();
#endif

            pnextCh = pcurCh->pNextUnloaded;
            pcurCh->pNextUnloaded = NULL;
            pcurCh->OnEventUnload();

            // Remove from parent's display list
            GFxASCharacter* pparent = pcurCh->GetParent();

#ifdef GFC_BUILD_DEBUG
            GASSERT(psavedParent == pparent);
#endif

            if (pparent)
                pparent->DetachChild(pcurCh);

            pcurCh->Release();
        }
        pUnloadListHead = NULL;
    }
}

void GFxMovieRoot::CheckMouseCursorType(UInt mouseIdx, GFxASCharacter* ptopMouseCharacter)
{
    if (MouseState[mouseIdx].IsTopmostEntityChanged())
    {
        UInt newCursorType = GFxMouseCursorEvent::ARROW;
        if (ptopMouseCharacter)
            newCursorType = ptopMouseCharacter->GetCursorType();
        ChangeMouseCursorType(mouseIdx, newCursorType);
    }
}

void GFxMovieRoot::ChangeMouseCursorType(UInt mouseIdx, UInt newCursorType)
{
#ifndef GFC_NO_FXPLAYER_AS_MOUSE
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    if (G_IsFlagSet<Flag_SetCursorTypeFuncOverloaded>(Flags) && penv->CheckExtensions())
    {
        // extensions enabled and Mouse.setCursorType function was overloaded
        // via ActionScript.
        // find Mouse.setCursorType and invoke it
        // Note, we need to invoke Mouse.setCursorType even if cursor type is the
        // same as before: this will allow to track changes in cursor position from
        // the custom setCursorType.
        GASValue objVal;
        if (penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Mouse), &objVal))
        {
            GASObject* pobj = objVal.ToObject(penv);
            if (pobj)
            {
                GASValue scval;
                if (pobj->GetMember(penv, penv->GetBuiltin(GASBuiltin_setCursorType), &scval))
                {
                    GASFunctionRef funcRef = scval.ToFunction(penv);
                    if (!funcRef.IsNull())
                    {
                        GASValue res;
                        penv->Push(GASNumber(mouseIdx));
                        penv->Push(GASNumber(newCursorType));
                        funcRef.Invoke(GASFnCall(&res, objVal, penv, 2, penv->GetTopIndex()));
                        penv->Drop2();    
                    }
                }
            }
        }
    }
    else
    {
        // extensions disabled; just call static method GASMouseCtorFunction::SetCursorType
        if (newCursorType != MouseState[mouseIdx].GetCursorType())
            GASMouseCtorFunction::SetCursorType(this, mouseIdx, newCursorType);
    }
#endif
    MouseState[mouseIdx].SetCursorType(newCursorType);
}

void GFxMovieRoot::Release()
{
    if ((GAtomicOps<SInt>::ExchangeAdd_NoSync(&RefCount, -1) - 1) == 0)
    {
        // Since deleting the memory context releases the heap,
        // we AddRef the memory context so that it doesn't get deleted before this object
        GPtr<MemoryContextImpl> memContext = MemContext;
        delete this;
        // now it's safe to delete the context
    }
}


UInt  GFxMovieRoot::HandleEvent(const GFxEvent &event)
{
    if (!IsMovieFocused() && event.Type != GFxEvent::SetFocus)
        return HE_NotHandled;

    union
    {
        const GFxEvent *        pevent;
        const GFxKeyEvent *     pkeyEvent;
        const GFxMouseEvent *   pmouseEvent;
        const GFxCharEvent *    pcharEvent;
        const GFxIMEEvent *     pimeEvent;
        const GFxSetFocusEvent* pfocusEvent;
    };
    pevent = &event;

    // handle set focus event for movie
    if (event.Type == GFxEvent::SetFocus)
    {
#ifndef GFC_NO_KEYBOARD_SUPPORT
        // Handle special keys state, if initialized
        for (UInt i = 0; i < GFC_MAX_KEYBOARD_SUPPORTED; ++i)
        {
            if (pfocusEvent->SpecialKeysStates[i].IsInitialized())
            {
                GFxKeyboardState* keyboardState = GetKeyboardState(i);
                GASSERT(keyboardState);
                keyboardState->SetKeyToggled(GFxKey::NumLock, 
                    pfocusEvent->SpecialKeysStates[i].IsNumToggled());
                keyboardState->SetKeyToggled(GFxKey::CapsLock, 
                    pfocusEvent->SpecialKeysStates[i].IsCapsToggled());
                keyboardState->SetKeyToggled(GFxKey::ScrollLock, 
                    pfocusEvent->SpecialKeysStates[i].IsScrollToggled());
            }
        }
#endif
        // Set special key states
        OnMovieFocus(true);
        return HE_Handled;
    }
    else if (event.Type == GFxEvent::KeyDown || event.Type == GFxEvent::KeyUp)
    {
#ifndef GFC_NO_KEYBOARD_SUPPORT
        // Handle special keys state, if initialized
        GFxKeyboardState* keyboardState = GetKeyboardState(pkeyEvent->KeyboardIndex);
        GFC_DEBUG_WARNING(!keyboardState, "GFxKeyEvent contains wrong index of keyboard");
        if (keyboardState && pkeyEvent->SpecialKeysState.IsInitialized())
        {
            keyboardState->SetKeyToggled(GFxKey::NumLock, 
                pkeyEvent->SpecialKeysState.IsNumToggled());
            keyboardState->SetKeyToggled(GFxKey::CapsLock, 
                pkeyEvent->SpecialKeysState.IsCapsToggled());
            keyboardState->SetKeyToggled(GFxKey::ScrollLock, 
                pkeyEvent->SpecialKeysState.IsScrollToggled());
        }
#endif
    }

    // Process the event type

    switch(event.Type)
    {
        // Mouse.
    case GFxEvent::MouseMove:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        if (pmouseEvent->MouseIndex < MouseCursorCount)
        {
            GPointF pt(PixelsToTwips(pmouseEvent->x * ViewScaleX + ViewOffsetX),
                PixelsToTwips(pmouseEvent->y * ViewScaleY + ViewOffsetY));
            InputEventsQueue.AddMouseMove(pmouseEvent->MouseIndex, pt);
            return HE_Completed;
        }
        break;

    case GFxEvent::MouseUp:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        if (pmouseEvent->MouseIndex < MouseCursorCount)
        {
            GPointF pt(PixelsToTwips(pmouseEvent->x * ViewScaleX + ViewOffsetX),
                PixelsToTwips(pmouseEvent->y * ViewScaleY + ViewOffsetY));
            InputEventsQueue.AddMouseRelease(pmouseEvent->MouseIndex, pt, (1 << pmouseEvent->Button));
            return HE_Completed;
        }
        break;

    case GFxEvent::MouseDown:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        if (pmouseEvent->MouseIndex < MouseCursorCount)
        {
            GPointF pt(PixelsToTwips(pmouseEvent->x * ViewScaleX + ViewOffsetX),
                PixelsToTwips(pmouseEvent->y * ViewScaleY + ViewOffsetY));
            InputEventsQueue.AddMousePress(pmouseEvent->MouseIndex, pt, (1 << pmouseEvent->Button));
            return HE_Completed;
        }
        break;

    case GFxEvent::MouseWheel:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        if (pmouseEvent->MouseIndex < MouseCursorCount)
        {
            GPointF pt(PixelsToTwips(pmouseEvent->x * ViewScaleX + ViewOffsetX),
                PixelsToTwips(pmouseEvent->y * ViewScaleY + ViewOffsetY));
            InputEventsQueue.AddMouseWheel(pmouseEvent->MouseIndex, pt, (SInt)pmouseEvent->ScrollDelta);
            return HE_Completed;
        }
        break;


#ifndef GFC_NO_KEYBOARD_SUPPORT
        // Keyboard.
    case GFxEvent::KeyDown:
        {
            GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxKeyEvent),
                "GFxKeyEvent class required for keyboard events");

            GFxKeyboardState* keyboardState = GetKeyboardState(pkeyEvent->KeyboardIndex);
            if (keyboardState)
            {
                keyboardState->SetKeyDown(pkeyEvent->KeyCode, pkeyEvent->AsciiCode, pkeyEvent->SpecialKeysState);
            }
            InputEventsQueue.AddKeyDown((short)pkeyEvent->KeyCode, pkeyEvent->AsciiCode, 
                pkeyEvent->SpecialKeysState, pkeyEvent->KeyboardIndex);
            if (pkeyEvent->WcharCode >= 32 && pkeyEvent->WcharCode != 127)
            {
                InputEventsQueue.AddCharTyped(pkeyEvent->WcharCode, pkeyEvent->KeyboardIndex);
            }
            return HE_Completed;
        }

    case GFxEvent::KeyUp:
        {
            GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxKeyEvent),
                "GFxKeyEvent class required for keyboard events");

            GFxKeyboardState* keyboardState = GetKeyboardState(pkeyEvent->KeyboardIndex);
            if (keyboardState)
            {
                keyboardState->SetKeyUp(pkeyEvent->KeyCode, pkeyEvent->AsciiCode, pkeyEvent->SpecialKeysState);
            }
            InputEventsQueue.AddKeyUp((short)pkeyEvent->KeyCode, pkeyEvent->AsciiCode, 
                pkeyEvent->SpecialKeysState, pkeyEvent->KeyboardIndex);
            return HE_Completed;
        }

    case GFxEvent::CharEvent:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxCharEvent),
            "GFxCharEvent class required for keyboard events");

        GFC_DEBUG_WARNING(!GetKeyboardState(pcharEvent->KeyboardIndex), "GFxCharEvent contains wrong index of keyboard");

        InputEventsQueue.AddCharTyped(pcharEvent->WcharCode, pcharEvent->KeyboardIndex);
        return HE_Completed;
#else
    case GFxEvent::KeyDown:
    case GFxEvent::KeyUp:
    case GFxEvent::CharEvent:
        GFC_DEBUG_WARNING(1, "Keyboard support is disabled due to GFC_NO_KEYBOARD_SUPPORT macro.");
        break;
#endif //GFC_NO_KEYBOARD_SUPPORT

#ifndef GFC_NO_IME_SUPPORT
    case GFxEvent::IMEEvent:
        {
            // IME events should be handled immediately
            GPtr<GFxIMEManager> pIMEManager = GetIMEManager();
            if (pIMEManager)
            {
                return pIMEManager->HandleIMEEvent(this, *pimeEvent);
            }
#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
            else
            {
                return HandleKoreanIME(*pimeEvent);
            }
#endif // GFC_NO_BUILTIN_KOREAN_IME
        }
        break;
#else //#ifndef GFC_NO_IME_SUPPORT
	GUNUSED(pimeEvent);
#endif

    case GFxEvent::KillFocus:
        OnMovieFocus(false);
        break;

    default:
        break;
    }
    return HE_NotHandled;
}

// The host app uses this to tell the GFxMovieSub where the
// user's mouse pointer is.
// Note, this function is deprecated, use HandleEvent instead.
void    GFxMovieRoot::NotifyMouseState(Float x, Float y, UInt buttons, UInt mouseIndex)
{
    Float mx  = (x * ViewScaleX + ViewOffsetX);
    Float my  = (y * ViewScaleY + ViewOffsetY);
    GPointF     pt(PixelsToTwips(mx), PixelsToTwips(my));

    if (mouseIndex < MouseCursorCount)
    {
        //MouseState[mouseIndex].ResetQueue();
        //GPointF p = MouseState[mouseIndex].GetLastPosition();
        //if (pt != p)
        InputEventsQueue.AddMouseMove(mouseIndex, pt);
        UInt lastButtons = MouseState[mouseIndex].GetButtonsState();
        for (UInt i = 0, mask = 1; i < GFxMouseState::MouseButton_MaxNum; i++, mask <<= 1)
        {
            if ((buttons & mask) && !(lastButtons & mask))
                InputEventsQueue.AddMousePress(mouseIndex, pt, (buttons & mask));
            else if ((lastButtons & mask) && !(buttons & mask))
                InputEventsQueue.AddMouseRelease(mouseIndex, pt, (lastButtons & mask));
        }
    }
}

void GFxMovieRoot::GetMouseState(UInt mouseIndex, Float* x, Float* y, UInt* buttons)
{
    GASSERT(mouseIndex < GFC_MAX_MICE_SUPPORTED);
    if (mouseIndex < MouseCursorCount)
    {
        GPointF p = MouseState[mouseIndex].GetLastPosition();
        // recalculate coords back to window
        p.x = (TwipsToPixels(p.x) - ViewOffsetX) / ViewScaleX;
        p.y = (TwipsToPixels(p.y) - ViewOffsetY) / ViewScaleY;

        if (x)
            *x = p.x;
        if (y)
            *y = p.y;
        if (buttons)
            *buttons = MouseState[mouseIndex].GetButtonsState();
    }
}

bool    GFxMovieRoot::HitTest(Float x, Float y, HitTestType testCond, UInt controllerIdx)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer hitTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_HitTest, Amp_Profile_Level_Low);
#endif

    Float mx  = (x * ViewScaleX + ViewOffsetX);
    Float my  = (y * ViewScaleY + ViewOffsetY);

#ifndef GFC_NO_3D
    float w = (VisibleFrameRect.Right - VisibleFrameRect.Left);
    float h = (VisibleFrameRect.Bottom - VisibleFrameRect.Top);

    float nsx = 2.f * (PixelsToTwips(mx) / w - 0.5f);       // 2.f * ((mx - PixelsToTwips(ViewOffsetX)) / w) - 1.0f;
    float nsy = 2.f * (PixelsToTwips(my) / h - 0.5f);       // 2.f * ((my - PixelsToTwips(ViewOffsetY)) / h) - 1.0f;

//    GetLog()->LogMessage("nsx=%.2f, nsy=%.2f\n", nsx, nsy);
    ScreenToWorld.SetNormalizedScreenCoords(nsx, nsy);    // -1 to 1
#endif

    for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
    {
        GFxSprite*  pmovie  = MovieLevels[movieIndex-1].pSprite;

        GRectF      movieLocalBounds = pmovie->GetBounds(GRenderer::Matrix());

        GPointF     pt(PixelsToTwips(mx), PixelsToTwips(my));

        GPointF     ptMv = pmovie->GetWorldMatrix().TransformByInverse(pt);

        if (
#ifndef GFC_NO_3D
            pmovie->Has3D() || 
#endif
            movieLocalBounds.Contains(ptMv))
        {
            switch(testCond)
            {
            case HitTest_Bounds:
                if (pmovie->PointTestLocal(ptMv, 0))
                    return 1;
                break;
            case HitTest_Shapes:
                if (pmovie->PointTestLocal(ptMv, GFxCharacter::HitTest_TestShape))
                    return 1;
                break;
            case HitTest_ButtonEvents:
                {
                    GFxCharacter::TopMostParams p(this, controllerIdx);
                    if (pmovie->GetTopMostMouseEntity(ptMv, p))
                        return 1;
                    break;
                }
            case HitTest_ShapesNoInvisible:
                if (pmovie->PointTestLocal(ptMv, 
                    GFxCharacter::HitTest_TestShape | GFxCharacter::HitTest_IgnoreInvisible))
                    return 1;
                break;
            }
        }
    }
    return 0;
}


bool    GFxMovieRoot::HitTest3D(GPoint3F *ptout, Float x, Float y, UInt controllerIdx)
{
#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer hitTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_HitTest, Amp_Profile_Level_Low);
#endif
 
    Float mx  = PixelsToTwips(x * ViewScaleX + ViewOffsetX);
    Float my  = PixelsToTwips(y * ViewScaleY + ViewOffsetY);

#ifndef GFC_NO_3D
    float w = (VisibleFrameRect.Right - VisibleFrameRect.Left);
    float h = (VisibleFrameRect.Bottom - VisibleFrameRect.Top);

    float nsx = 2.f * (mx / w - 0.5f);
    float nsy = 2.f * (my / h - 0.5f);

    ScreenToWorld.SetNormalizedScreenCoords(nsx, nsy);    // -1 to 1
#endif

    for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
    {
        GFxSprite*  pmovie  = MovieLevels[movieIndex-1].pSprite;
        GRectF      movieLocalBounds = pmovie->GetBounds(GRenderer::Matrix());
        GPointF     pt(mx, my);
        GPointF     ptMv = pmovie->GetWorldMatrix().TransformByInverse(pt);

        if (
#ifndef GFC_NO_3D
            pmovie->Has3D() || 
#endif
            movieLocalBounds.Contains(ptMv))
        {
            GFxASCharacter* pchar = pmovie->GetMovieRoot()->GetTopMostEntity(pt, controllerIdx, true, NULL);    //this);
            if (pchar)
            {
#ifndef GFC_NO_3D
                if (pchar->Is3D(true))
                {
                    GPointF objHitPoint = ScreenToWorld.GetLastWorldPoint();
                    *ptout = pchar->GetWorldMatrix3D().Transform(GPoint3F(objHitPoint.x, objHitPoint.y, 0));
                    ptout->x = TwipsToPixels(ptout->x);
                    ptout->y = TwipsToPixels(ptout->y);
//                    pmovie->GetLog()->LogMessage("Hit '%s', x=%f, y=%f, z=%f\n", pchar->GetName().GetBuffer(), ptout->x, ptout->y, ptout->z);
                }
                else
                {
                    ptout->x = x;
                    ptout->y = y;
                    ptout->z = 0;
                }
#endif
                return 1;
            }
        }
    }
    return 0;
}

bool    GFxMovieRoot::AttachDisplayCallback(const char* pathToObject, void (GCDECL *callback)(void*), void* userPtr)
{   
    if (!pLevel0Movie)
        return 0;

    GASEnvironment*     penv    = pLevel0Movie->GetASEnvironment();
    GASValue            obj;
    if (penv->GetVariable(penv->CreateString(pathToObject), &obj))
    {
        GFxASCharacter*     pchar   = obj.ToASCharacter(penv);

        if (pchar)
        {
            pchar->SetDisplayCallback(callback, userPtr);
            return true;
        }
    }
    return false;
}


int    GFxMovieRoot::AddIntervalTimer(GASIntervalTimer *timer)
{
    timer->SetId(++LastIntervalTimerId);
    IntervalTimers.PushBack(timer);
    return LastIntervalTimerId;
}

void    GFxMovieRoot::ClearIntervalTimer(int timerId)
{
    for (UPInt i = 0, n = IntervalTimers.GetSize(); i < n; ++i)
    {
        if (IntervalTimers[i] && IntervalTimers[i]->GetId() == timerId)
        {
            // do not remove the timer's array's entry here, just null it;
            // it will be removed later in Advance.
            IntervalTimers[i]->Clear();
            return;
        }
    }
}

void    GFxMovieRoot::ShutdownTimers()
{
    for (UPInt i = 0, n = IntervalTimers.GetSize(); i < n; ++i)
    {
        delete IntervalTimers[i];
    }
    IntervalTimers.Clear();
}

#ifndef GFC_NO_VIDEO
void GFxMovieRoot::AddVideoProvider(GFxVideoProvider* pvideo)
{
    if (pvideo)
        VideoProviders.Add(pvideo);
}
void GFxMovieRoot::RemoveVideoProvider(GFxVideoProvider* pvideo)
{
    VideoProviders.Remove(pvideo);
}
#endif

// *** GFxMovieRoot's GFxMovie interface implementation

// These methods mostly delegate to _level0 movie.

GFxMovieDef*    GFxMovieRoot::GetMovieDef() const
{   
    return pLevel0Def;
}
UInt    GFxMovieRoot::GetCurrentFrame() const
{
    return pLevel0Movie ? pLevel0Movie->GetCurrentFrame() : 0;
}
bool    GFxMovieRoot::HasLooped() const
{
    return pLevel0Movie ? pLevel0Movie->HasLooped() : 0; 
}
void    GFxMovieRoot::Restart()                         
{       
    if (pLevel0Movie)
    {
        GPtr<GFxMovieDefImpl> prootMovieDef = pLevel0Movie->GetResourceMovieDef();

        // It is important to destroy the sprite before the global context,
        // so that is is not used from OnEvent(unload) in sprite destructor
        // NOTE: We clear the list here first because users can store pointers in _global,
        // which would cause pMovie assignment to not release it early (avoid "aeon.swf" crash).
        SInt i;
        for (i = (SInt)MovieLevels.GetSize() - 1; i >= 0; i--)
            ReleaseLevelMovie(i);
        // Release all refs.
        MovieLevels.Clear();

        // clean up threaded load queue before freeing
        GFxLoadQueueEntryMT* plq = pLoadQueueMTHead;
        UInt plqCount = 0;
        while (plq)
        {
            plqCount++;
            plq->Cancel();
            plq = plq->pNext;
        }
        // wait until all threaded loaders exit
        // this is required to avoid losing the movieroot
        // heap before the load tasks complete.
        UInt plqDoneCount = 0;
        while (plqCount > plqDoneCount)
        {
            plqDoneCount = 0;
            plq = pLoadQueueMTHead;
            while (plq)
            {
                if (plq->LoadFinished())
                    plqDoneCount++;
                plq = plq->pNext;
            }
        }

        // free load queue
        while(pLoadQueueHead)
        {
            // Remove from queue.
            GFxLoadQueueEntry *pentry = pLoadQueueHead;
            pLoadQueueHead = pentry->pNext;
            delete pentry;
        }
        while(pLoadQueueMTHead)
        {
            GFxLoadQueueEntryMT *pentry = pLoadQueueMTHead;
            pLoadQueueMTHead = pentry->pNext;
            delete pentry;
        }
        pLoadQueueHead      = 0;
        pLoadQueueMTHead    = 0;

        ReleaseUnloadList();
        InvalidateOptAdvanceList();
        pPlayListHead = pPlayListOptHead = NULL;

#ifndef GFC_NO_IME_SUPPORT
        // clean up IME manager
        bool wasIMEActive = false;

        GPtr<GFxIMEManager> pIMEManager = GetIMEManager();
        if (pIMEManager)
        {
            if (pIMEManager->IsMovieActive(this))
            {
                wasIMEActive = true;
                pIMEManager->SetActiveMovie(NULL);
            }
        }

        // delete params of candidate list so they could be recreated
        delete pIMECandidateListStyle;
        pIMECandidateListStyle = NULL;

#endif //#ifndef GFC_NO_IME_SUPPORT

#ifndef GFC_NO_VIDEO
        VideoProviders.Clear();
#endif
#ifndef GFC_NO_SOUND
        GFxSprite* psprite = GetLevelMovie(0);
        if (psprite)
            psprite->StopActiveSounds();
#endif

        // Unregister all classes
        pGlobalContext->UnregisterAllClasses();
        pASMouseListener = NULL;
        delete pRetValHolder;pRetValHolder = NULL;

        // Reset focus states
        ResetFocusStates();

        G_Set3WayFlag<Shift_DisableFocusAutoRelease>(Flags, 0);
        G_Set3WayFlag<Shift_AlwaysEnableFocusArrowKeys>(Flags, 0);
        G_Set3WayFlag<Shift_AlwaysEnableKeyboardPress>(Flags, 0);
        G_Set3WayFlag<Shift_DisableFocusRolloverEvent>(Flags, 0);
        G_Set3WayFlag<Shift_DisableFocusKeys>(Flags, 0);
        MemContext->LimHandler.Reset(pHeap);
        ResetMouseState();

        // Recreate global context
        pGlobalContext->PreClean(true); // clean with preserving builtin gfxPlayer, gfxArg, gfxLanguage props

#ifndef GFC_NO_GC
        // this collect should collect everything from old global context; do not
        // remove!
        MemContext->ASGC->ForceCollect();
#endif

        pGlobalContext->Init(this);
        pRetValHolder = GHEAP_NEW(pHeap) ReturnValueHolder(pGlobalContext);

        // Note: Log is inherited dynamically from Def, so we don't set it here
        GPtr<GFxSprite> prootMovie = 
            *GHEAP_NEW(pHeap) GFxSprite(prootMovieDef->GetDataDef(), prootMovieDef,
                                        this, NULL, GFxResourceId());

        if (!prootMovie)
        {
            return;
        }

        // reset mouse state and cursor
        if (pUserEventHandler)
        {
            for (UInt i = 0; i < MouseCursorCount; ++i)
            {
                pUserEventHandler->HandleEvent(this, GFxMouseCursorEvent(GFxEvent::DoShowMouse, i));
                pUserEventHandler->HandleEvent(this, GFxMouseCursorEvent(GFxMouseCursorEvent::ARROW, i));
            }
        }
        // Reset invoke aliases set by ExternalInterface.addCallback
        delete pInvokeAliases; pInvokeAliases = NULL;

        // Assign level and _level0 name.
        prootMovie->SetLevel(0);
        SetLevelMovie(0, prootMovie);

        // Register aux AS classes defined in external libraries
        RegisterAuxASClasses();

        // In case permanent variables were assigned.. check them.
        ResolveStickyVariables(prootMovie);

        // reset keyboard state
        ResetKeyboardState();

#ifndef GFC_NO_IME_SUPPORT
        if (wasIMEActive)
            pIMEManager->SetActiveMovie(this);
#endif //#ifndef GFC_NO_IME_SUPPORT

        Advance(0.0f, 0);
        SetDirtyFlag();

#ifndef GFC_NO_GC
        MemContext->ASGC->ForceCollect();
#endif
    }
}

void    GFxMovieRoot::ResetMouseState()
{
    for (UInt i = 0; i < sizeof(MouseState)/sizeof(MouseState[0]); ++i)
    {
        MouseState[i].ResetState();
    }
}

void    GFxMovieRoot::ResetKeyboardState()
{
#ifndef GFC_NO_KEYBOARD_SUPPORT
    for (UInt i = 0; i < sizeof(KeyboardState)/sizeof(KeyboardState[0]); ++i)
    {
        KeyboardState[i].ResetState();
    }
#endif
}

#ifndef GFC_NO_KEYBOARD_SUPPORT
void    GFxMovieRoot::SetKeyboardListener(GFxKeyboardState::IListener* l)
{
    for (UInt i = 0; i < sizeof(KeyboardState)/sizeof(KeyboardState[0]); ++i)
    {
        KeyboardState[i].SetListener(l);
    }
}
#endif

void    GFxMovieRoot::GotoFrame(UInt targetFrameNumber) 
{   // 0-based!!
    if (pLevel0Movie) pLevel0Movie->GotoFrame(targetFrameNumber); 
}

bool    GFxMovieRoot::GotoLabeledFrame(const char* label, SInt offset)
{
    if (!pLevel0Movie)
        return 0;

    UInt    targetFrame = GFC_MAX_UINT;
    if (pLevel0Def->GetDataDef()->GetLabeledFrame(label, &targetFrame, 0))
    {
        GotoFrame((UInt)((SInt)targetFrame + offset));
        return 1;
    }
    else
    {
        if (GetLog())
            GetLog()->LogScriptError("Error: MovieImpl::GotoLabeledFrame('%s') unknown label\n", label);
        return 0;
    }
}

#ifndef GFC_NO_3D
// force the movie to be fully 3D
void GFxMovieRoot::Make3D()
{
    for (UInt movieIndex = 0; movieIndex < MovieLevels.GetSize(); movieIndex++)
    {
        GFxSprite* pmovie = MovieLevels[movieIndex].pSprite;
        pmovie->CreateMatrix3D();       // create a 3D matrix if necessary
    }
}

// recompute view and perspective if necessary (usually when visible frame rect changes)
void GFxMovieRoot::UpdateViewAndPerspective()        
{ 
    if (pViewMatrix3D == NULL)
        pViewMatrix3D = GHEAP_NEW(pHeap) GMatrix3DNewable;
    if (pPerspectiveMatrix3D == NULL)
        pPerspectiveMatrix3D = GHEAP_NEW(pHeap) GMatrix3DNewable;
    if (GetRenderer())
        GetRenderer()->MakeViewAndPersp3D(VisibleFrameRect, *pViewMatrix3D, *pPerspectiveMatrix3D, PerspFOV);

    for (UInt movieIndex = 0; movieIndex < MovieLevels.GetSize(); movieIndex++)
    {
        GFxSprite* pmovie = MovieLevels[movieIndex].pSprite;
        if (pmovie)
            pmovie->UpdateViewAndPerspective();
    }
}

// Set view and projection matrices
void    GFxMovieRoot::Setup3DDisplay(GFxDisplayContext &context)
{
    GRenderer *pRenderer = context.GetRenderer();
    if (pViewMatrix3D == NULL || pPerspectiveMatrix3D == NULL)
    {
        GMatrix3D matView;
        GMatrix3D matPersp;
        pRenderer->MakeViewAndPersp3D(VisibleFrameRect, matView, matPersp, PerspFOV);

        // set view matrix
        if (pViewMatrix3D == NULL)
        {
            pViewMatrix3D = GHEAP_NEW(pHeap) GMatrix3DNewable;
            *pViewMatrix3D = matView;    
        }

        // set perspective matrix
        if (pPerspectiveMatrix3D == NULL)
        {
            pPerspectiveMatrix3D = GHEAP_NEW(pHeap) GMatrix3DNewable;
            *pPerspectiveMatrix3D = matPersp;
        }
    }

    context.pViewMatrix3D = pViewMatrix3D;
    pRenderer->SetView3D(*pViewMatrix3D);
    ScreenToWorld.SetView(*pViewMatrix3D);

    context.pPerspectiveMatrix3D = pPerspectiveMatrix3D;
    pRenderer->SetPerspective3D(*pPerspectiveMatrix3D);
    ScreenToWorld.SetPerspective(*pPerspectiveMatrix3D);
}
#endif       

void    GFxMovieRoot::Display()
{
    // If there is no _level0, there are no other levels either.
    if (!pLevel0Movie)
        return;
    
#if !defined(GFC_NO_STAT)
    ScopeFunctionTimer displayTimer(DisplayStats, NativeCodeSwdHandle, Func_GFxMovieRoot_Display, Amp_Profile_Level_Low);
#endif

    // Warn in case the user called Display() before Advance().
    GFC_DEBUG_WARNING(!G_IsFlagSet<Flag_AdvanceCalled>(Flags), "GFxMovieView::Display called before Advance - no rendering will take place");

    bool    haveVisibleMovie = 0;

    for (UInt movieIndex = 0; movieIndex < MovieLevels.GetSize(); movieIndex++)
    {
        if (MovieLevels[movieIndex].pSprite->GetVisible())
        {
            haveVisibleMovie = 1;
            break;
        }
    }

    // Avoid rendering calls if no movies are visible.
    if (!haveVisibleMovie)
        return;
    // DBG
    //printf("GFxMovieRoot::Display %d   -----------------------\n", pLevel0Movie->GetLevelMovie(0)->ToSprite()->GetCurrentFrame());

    bool ampProfiling = false;
#ifdef GFX_AMP_SERVER
    ampProfiling = GFxAmpServer::GetInstance().IsProfiling();
#endif
    GFxDisplayContext context(pStateBag, 
                              pLevel0Def->GetWeakLib(),
                              &pLevel0Def->GetResourceBinding(),
                              GetPixelScale(),
                              ViewportMatrix, ampProfiling ? DisplayStats : NULL);

    //GRenderer::Matrix matrix = GMatrix2D::Identity;
    //context.pParentMatrix = &matrix;

    // Need to check RenderConfig before GetRenderer.
    if (!context.GetRenderConfig() || !context.GetRenderer())
    {
        GFC_DEBUG_WARNING(1, "GFxMovie::Display failed, no renderer specified");
        return;
    }

    // Cache log to avoid expensive state access.
    G_SetFlag<Flag_CachedLogFlag>(Flags, true);
    pCachedLog    = context.pLog;

    context.GetRenderer()->BeginDisplay(BackgroundColor, Viewport, VisibleFrameRect.Left, VisibleFrameRect.Right, VisibleFrameRect.Top, VisibleFrameRect.Bottom);

#ifndef GFC_NO_3D
    Setup3DDisplay(context);
#endif

#ifndef GFC_NO_VIDEO
    if (VideoProviders.GetSize() > 0)
    {
        ScopeFunctionTimer displayVideoTimer(DisplayStats, NativeCodeSwdHandle, Func_GFxVideoProviderDisplay, Amp_Profile_Level_Low);

        for(GHashSet<GPtr<GFxVideoProvider> >::Iterator it = VideoProviders.Begin(); it != VideoProviders.End(); ++it)
        {
            GPtr<GFxVideoProvider> pvideo = *it;
            // we have to call Progress method here and not from VideoObject AdvanceFrame method 
            // because there could be videos that are not attached to any VideoObject but still they 
            // have to progress (only sound in this case is produced)
            pvideo->Display(context.GetRenderer());
        }
    }
#endif
    // Display all levels. _level0 is at the bottom.
    for (UInt movieIndex = 0; movieIndex < MovieLevels.GetSize(); movieIndex++)
    {
        GFxSprite* pmovie = MovieLevels[movieIndex].pSprite;
        pmovie->Display(context);
    }
    DisplayTopmostLevelCharacters(context);

    // Display focus rect
    DisplayFocusRect(context);

    context.GetRenderer()->EndDisplay();
    context.pMeshCacheManager->EndDisplay();

    G_SetFlag<Flag_CachedLogFlag>(Flags, false);

    // DBG
    //printf("GFxMovieRoot::Display %d   ^^^^^^^^^^^^^^^^^^^^^\n", pLevel0Movie->GetLevelMovie(0)->ToSprite()->GetCurrentFrame());

#ifdef GFC_BUILD_LOGO
    GFx_DisplayLogo(context.GetRenderer(), Viewport, pDummyImage);
#endif
}

void    GFxMovieRoot::SetPause(bool pause)
{
    if ((G_IsFlagSet<Flag_Paused>(Flags) && pause) || (!G_IsFlagSet<Flag_Paused>(Flags) && !pause))
        return;
    G_SetFlag<Flag_Paused>(Flags, pause);
    if (pause)
        PauseTickMs = GTimer::GetTicks()/1000;
    else
        StartTickMs += (GTimer::GetTicks()/1000 - PauseTickMs); 

#ifndef GFC_USE_OLD_ADVANCE
    GFxASCharacter* pnextCh = NULL;
    for (GFxASCharacter* pcurCh = pPlayListHead; pcurCh; pcurCh = pnextCh)
    {
        pnextCh = pcurCh->pPlayNext;
        pcurCh->SetPause(pause);
    }
#endif // GFC_USE_OLD_ADVANCE
#ifndef GFC_NO_VIDEO
    if (VideoProviders.GetSize() > 0)
    {
        GArray<GFxVideoProvider*> remove_list;
        for(GHashSet<GPtr<GFxVideoProvider> >::Iterator it = VideoProviders.Begin(); it != VideoProviders.End(); ++it)
        {
            GPtr<GFxVideoProvider> pvideo = *it;
            pvideo->Pause(pause);
        }
    }
#endif
}

bool GFxMovieRoot::IsPaused() const
{
    return G_IsFlagSet<Flag_Paused>(Flags);
}

void    GFxMovieRoot::SetPlayState(PlayState s)
{   
    if (pLevel0Movie)
        pLevel0Movie->SetPlayState(s); 
}
GFxMovie::PlayState GFxMovieRoot::GetPlayState() const
{   
    if (!pLevel0Movie) return Stopped;
    return pLevel0Movie->GetPlayState(); 
}

void    GFxMovieRoot::SetVisible(bool visible)
{
    if (pLevel0Movie) pLevel0Movie->SetVisible(visible);
}
bool    GFxMovieRoot::GetVisible() const
{
    return pLevel0Movie ? pLevel0Movie->GetVisible() : 0;
}

// Action Script access.

bool GFxMovieRoot::GetVariable(GFxValue *pval, const char* ppathToVar) const
{
    if (!pLevel0Movie || !pval)
        return 0;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer getTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_GetVariable, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue retVal;
    if (penv->GetVariable(path, &retVal))
    {
        ASValue2GFxValue(penv, retVal, pval);
        return true;
    }
    return false;
}

bool GFxMovieRoot::SetVariable(const char* ppathToVar, const GFxValue& value, SetVarType setType)
{
    if (!pLevel0Movie)
        return 0;
    if (!ppathToVar)
    {
        if (GetLog())
            GetLog()->LogError("Error: NULL pathToVar passed to SetVariable/SetDouble()\n");
        return 0;
    }

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer setTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_SetVariable, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue val;
    GFxValue2ASValue(value, &val);
    bool setResult = pLevel0Movie->GetASEnvironment()->SetVariable(path, val, NULL, 
        (setType == SV_Normal));

    if ( (!setResult && (setType != SV_Normal)) ||
        (setType == SV_Permanent) )
    {
        // Store in sticky hash.
        AddStickyVariable(path, val, setType);
    }
    return setResult;
}

bool GFxMovieRoot::SetVariableArray(SetArrayType type, const char* ppathToVar,
                                    UInt index, const void* pdata, UInt count, SetVarType setType)
{
    if (!pLevel0Movie)
        return false;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer setTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_SetVariableArray, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GPtr<GASArrayObject> parray;
    GASValue retVal;
    if(penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
            parray = static_cast<GASArrayObject*>(pobj);
    }

    if (!parray)
    {
        parray = *GHEAP_NEW(pHeap) GASArrayObject(pLevel0Movie->GetASEnvironment());
    }
    if (count + index > UInt(parray->GetSize()))
        parray->Resize(count + index);

    switch(type)
    {
    case SA_Int:
        {
            const int* parr = static_cast<const int*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(parr[i]));
            }
            break;
        }
    case SA_Float:
        {
            const Float* parr = static_cast<const Float*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(GASNumber(parr[i])));
            }
            break;
        }
    case SA_Double:
        {
            const Double* parr = static_cast<const Double*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(GASNumber(parr[i])));
            }
            break;
        }
    case SA_Value:
        {
            const GFxValue* parr = static_cast<const GFxValue*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                GASValue asVal;
                GFxValue2ASValue(parr[i], &asVal);
                parray->SetElement(i + index, asVal);
            }
            break;
        }
    case SA_String:
        {
            const char* const* parr = static_cast<const char* const*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(pGlobalContext->CreateString(parr[i])));
            }
            break;
        }
    case SA_StringW:
        {
            const wchar_t* const* parr = static_cast<const wchar_t* const*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(pGlobalContext->CreateString(parr[i])));
            }
            break;
        }
    }

    GASValue val;
    val.SetAsObject(parray);

    bool setResult = pLevel0Movie->GetASEnvironment()->SetVariable(path, val, NULL, 
        (setType == SV_Normal));

    if ( (!setResult && (setType != SV_Normal)) ||
        (setType == SV_Permanent) )
    {
        // Store in sticky hash.
        AddStickyVariable(path, val, setType);
    }
    return setResult;
}

bool GFxMovieRoot::SetVariableArraySize(const char* ppathToVar, UInt count, SetVarType setType)
{
    if (!pLevel0Movie)
        return false;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer setTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_SetVariableArraySize, Amp_Profile_Level_Low);
#endif

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GPtr<GASArrayObject> parray;
    GASValue retVal;
    if(penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
        {
            // Array already exists - just resize
            parray = static_cast<GASArrayObject*>(pobj);
            if (count != UInt(parray->GetSize()))
                parray->Resize(count);
            return true;
        }
    }

    // no array found - allocate new one, resize and set
    parray = *GHEAP_NEW(pHeap) GASArrayObject(pLevel0Movie->GetASEnvironment());
    parray->Resize(count);
    GASValue val;
    val.SetAsObject(parray);

    bool setResult = pLevel0Movie->GetASEnvironment()->SetVariable(path, val, NULL, 
        (setType == SV_Normal));

    if ( (!setResult && (setType != SV_Normal)) ||
        (setType == SV_Permanent) )
    {
        // Store in sticky hash.
        AddStickyVariable(path, val, setType);
    }
    return setResult;
}

UInt GFxMovieRoot::GetVariableArraySize(const char* ppathToVar)
{
    if (!pLevel0Movie)
        return 0;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer getTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_GetVariableArraySize, Amp_Profile_Level_Low);
#endif

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue retVal;
    if (penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
        {
            GASArrayObject* parray = static_cast<GASArrayObject*>(pobj);
            return parray->GetSize();
        }
    }
    return 0;
}

bool GFxMovieRoot::GetVariableArray(SetArrayType type, const char* ppathToVar,
                                    UInt index, void* pdata, UInt count)
{
    if (!pLevel0Movie)
        return false;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer getTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_GetVariableArray, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue retVal;
    if(penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject(penv);
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
        {
            pRetValHolder->ResetPos();
            pRetValHolder->ResizeStringArray(0);

            GASArrayObject* parray = static_cast<GASArrayObject*>(pobj);
            UInt arrSize = parray->GetSize();
            switch(type)
            {
            case SA_Int:
                {
                    int* parr = static_cast<int*>(pdata);
                    for (UInt i = 0, n = G_Min(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                            parr[i] = (int)pval->ToNumber(penv);
                        else
                            parr[i] = 0;
                    }
                }
                break;
            case SA_Float:
                {
                    Float* parr = static_cast<Float*>(pdata);
                    for (UInt i = 0, n = G_Min(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                            parr[i] = (Float)pval->ToNumber(penv);
                        else
                            parr[i] = 0;
                    }
                }
                break;
            case SA_Double:
                {
                    Double* parr = static_cast<Double*>(pdata);
                    for (UInt i = 0, n = G_Min(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                            parr[i] = (Double)pval->ToNumber(penv);
                        else
                            parr[i] = 0;
                    }
                }
                break;
            case SA_Value:
                {
                    GFxValue* parr = static_cast<GFxValue*>(pdata);
                    for (UInt i = 0, n = G_Min(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        GFxValue* pdestVal = &parr[i];
                        pdestVal->SetUndefined();
                        if (pval)
                            ASValue2GFxValue(penv, *pval, pdestVal);
                        else
                            pdestVal->SetUndefined();
                    }
                }
                break;
            case SA_String:
                {
                    const char** parr = static_cast<const char**>(pdata);
                    UInt n = G_Min(arrSize, count);
                    pRetValHolder->ResizeStringArray(n);
                    for (UInt i = 0; i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                        {
                            GASString str = pval->ToString(penv);
                            parr[i] = str.ToCStr();
                            pRetValHolder->StringArray[pRetValHolder->StringArrayPos++] = str;   
                        }
                        else
                            parr[i] = NULL;
                    }
                }
                break;
            case SA_StringW:
                {
                    const wchar_t** parr = static_cast<const wchar_t**>(pdata);
                    // pre-calculate the size of required buffer
                    UInt i, bufSize = 0;
                    UInt n = G_Min(arrSize, count);
                    pRetValHolder->ResizeStringArray(n);
                    for (i = 0; i < arrSize; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                        {
                            GASString str = pval->ToString(penv);
                            pRetValHolder->StringArray[i] = str;
                            bufSize += str.GetLength()+1;
                        }
                    }
                    wchar_t* pwBuffer = (wchar_t*)pRetValHolder->PreAllocateBuffer(bufSize * sizeof(wchar_t));

                    for (i = 0; i < n; ++i)
                    {
                        const char* psrcStr = pRetValHolder->StringArray[i].ToCStr();
                        wchar_t* pwdstStr = pwBuffer;
                        UInt32 code;
                        while ((code = GUTF8Util::DecodeNextChar(&psrcStr)) != 0)
                        {
                            *pwBuffer++ = (wchar_t)code;
                        }
                        *pwBuffer++ = 0;

                        parr[i] = pwdstStr;
                    }
                    pRetValHolder->ResizeStringArray(0);
                }
                break;
            }
            return true;
        }
    }
    return false;
}

void GFxMovieRoot::AddStickyVariable(const GASString& fullPath, const GASValue &val, SetVarType setType)
{
    // Parse the path first.
    GASStringContext sc(pGlobalContext, 8);
    GASString   path(sc.GetBuiltin(GASBuiltin_empty_));
    GASString   name(sc.GetBuiltin(GASBuiltin_empty_));

    if (!GASEnvironment::ParsePath(&sc, fullPath, &path, &name))
    {
        if (name.IsEmpty())
            return;

        // This can happen if a sticky variable did not have a path. In that
        // case assume it is _level0.
        path = sc.GetBuiltin(GASBuiltin__level0);
    }
    else
    {
        // TBD: Verify/clean up path integrity?
        //  -> no _parent, _global, "..", ".", "this"
        //  -> _root converts to _level0
        // "/" converts to "."
        bool    needLevelPrefix = 0;

        if (path.GetSize() >= 5)
        {           
            if (!memcmp(path.ToCStr(), "_root", 5))
            {
                // Translate _root.
                path = sc.GetBuiltin(GASBuiltin__level0) + path.Substring(5, path.GetLength());
            }

            // Warn about invalid path components in debug builds.
#ifdef  GFC_BUILD_DEBUG         
            const char *p = strstr(path.ToCStr(), "_parent");
            if (!p) p = strstr(path.ToCStr(), "_global");

            if (p)
            {
                if ((p == path.ToCStr()) || *(p-1) == '.')
                {
                    GFC_DEBUG_WARNING(1, "SetVariable - Sticky/Permanent variable path can not include _parent or _global");
                }
            }
#endif

            // Path must start with _levelN.
            if (memcmp(path.ToCStr(), "_level", 6) != 0)
            {
                needLevelPrefix = 1;
            }
        }
        else
        {
            needLevelPrefix = 1;
        }

        if (needLevelPrefix)
            path = sc.GetBuiltin(GASBuiltin__level0dot_) + path;
    }

    StickyVarNode* pnode = 0;
    StickyVarNode* p = 0;

    if (StickyVariables.Get(path, &pnode) && pnode)
    {
        p = pnode;

        while (p)
        {
            // If there is a name match, overwrite value.
            if (p->Name == name)
            {
                p->Value     = val;
                if (!p->Permanent) // Permanent sticks.
                    p->Permanent = (setType == SV_Permanent);
                return;
            }
            p = p->pNext;
        };

        // Not found, add new node
        if ((p = GHEAP_NEW(pHeap) StickyVarNode(name, val, (setType == SV_Permanent)))!=0)
        {          
            // Link prev node to us: order does not matter.
            p->pNext = pnode->pNext;
            pnode->pNext = p;
        }
        // Done.
        return;
    }

    // Node does not exit, create it
    if ((p = GHEAP_NEW(pHeap) StickyVarNode(name, val, (setType == SV_Permanent)))!=0)
    {        
        // Save node.
        StickyVariables.Set(path, p);
    }
}

void    GFxMovieRoot::ResolveStickyVariables(GFxASCharacter *pcharacter)
{
    GASSERT(pcharacter);

    const GASString &path  = pcharacter->GetCharacterHandle()->GetNamePath();
    StickyVarNode*   pnode = 0;
    StickyVarNode*   p, *pnext;

    if (StickyVariables.Get(path, &pnode))
    {       
        // If found, add variables.     
        // We also remove sticky nodes while keeping permanent ones.
        StickyVarNode*  ppermanent = 0;
        StickyVarNode*  ppermanentTail = 0;

        p = pnode;

        while (p)
        {
            pcharacter->SetMember(pcharacter->GetASEnvironment(), p->Name, p->Value);
            pnext = p->pNext;

            if (p->Permanent)
            {
                // If node is permanent, create a permanent-only linked list.
                if (!ppermanent)
                {
                    ppermanent = p;
                }
                else
                {
                    ppermanentTail->pNext = p;
                }

                ppermanentTail = p;
                p->pNext = 0;
            }
            else
            {
                // If not permanent, delete this node.
                delete p;
            }

            p = pnext;
        }


        if (ppermanent)
        {
            // If permanent list was formed, keep it.
            if (ppermanent != pnode)
                StickyVariables.Set(path, ppermanent);
        }
        else
        {
            // Otherwise delete hash key.
            StickyVariables.Remove(path);
        }
    }
}

void    GFxMovieRoot::ClearStickyVariables()
{
    GASStringHash<StickyVarNode*>::Iterator ihash = StickyVariables.Begin();

    // Travers hash and delete nodes.
    for( ;ihash != StickyVariables.End(); ++ihash)
    {
        StickyVarNode* pnode = ihash->Second;
        StickyVarNode* p;

        while (pnode)
        {
            p = pnode;
            pnode = p->pNext;
            delete p;
        }
    }

    StickyVariables.Clear();
}

bool    GFxMovieRoot::IsAvailable(const char* pathToVar) const
{
    if (!pLevel0Movie)
        return 0;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(pathToVar));

    return penv->IsAvailable(path);
}

bool        GFxMovieRoot::Invoke(const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs)
{
    return Invoke(pLevel0Movie, pmethodName, presult, pargs, numArgs);
}

// For ActionScript interfacing convenience.
bool        GFxMovieRoot::Invoke(GFxSprite* thisSpr, const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs)
{
    if (!thisSpr) return 0;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer invokeTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_Invoke, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASValue resultVal;
    GASEnvironment* penv = thisSpr->GetASEnvironment();
    GASSERT(penv);

    // push arguments directly into environment
    for (int i = (int)numArgs - 1; i >= 0; --i)
    {
        GASSERT(pargs); // should be checked only if numArgs > 0
        const GFxValue& gfxVal = pargs[i];
        GASValue asval;
        GFxValue2ASValue(gfxVal, &asval);
        penv->Push(asval);
    }
    bool retVal;

    // try to resolve alias
    InvokeAliasInfo* palias;
    if (pInvokeAliases && (palias = ResolveInvokeAlias(pmethodName)) != NULL)
        retVal = InvokeAlias(pmethodName, *palias, &resultVal, numArgs);
    else
        retVal = thisSpr->Invoke(pmethodName, &resultVal, numArgs);
    penv->Drop(numArgs); // release arguments
    if (retVal && presult)
    {
        // convert result to GFxValue
        ASValue2GFxValue(penv, resultVal, presult);
    }
    return retVal;
}

bool        GFxMovieRoot::Invoke(const char* pmethodName, GFxValue *presult, const char* pargFmt, ...)
{
    if (!pLevel0Movie) return 0;

#ifdef GFX_AMP_SERVER
    ScopeFunctionTimer invokeTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_Invoke, Amp_Profile_Level_Low);
#endif

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    GASValue resultVal;
    va_list args;
    va_start(args, pargFmt);
    
    bool retVal;
    // try to resolve alias
    InvokeAliasInfo* palias;
    if (pInvokeAliases && (palias = ResolveInvokeAlias(pmethodName)) != NULL)
        retVal = InvokeAliasArgs(pmethodName, *palias, &resultVal, pargFmt, args);
    else
        retVal = pLevel0Movie->InvokeArgs(pmethodName, &resultVal, pargFmt, args);
    va_end(args);
    if (retVal && presult)
    {
        // convert result to GFxValue
        ASValue2GFxValue(pLevel0Movie->GetASEnvironment(), resultVal, presult);
    }
    return retVal;
}


bool        GFxMovieRoot::InvokeArgs(const char* pmethodName, GFxValue *presult, const char* pargFmt, va_list args)
{
    if (pLevel0Movie)
    {
#ifdef GFX_AMP_SERVER
        ScopeFunctionTimer invokeTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_InvokeArgs, Amp_Profile_Level_Low);
#endif

        // Need to restore high precision mode of FPU for X86 CPUs.
        // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
        // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
        // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
        GFxDoublePrecisionGuard dpg;

        GASValue resultVal;
        bool retVal;
        // try to resolve alias
        InvokeAliasInfo* palias;
        if (pInvokeAliases && (palias = ResolveInvokeAlias(pmethodName)) != NULL)
            retVal = InvokeAliasArgs(pmethodName, *palias, &resultVal, pargFmt, args);
        else
            retVal = pLevel0Movie->InvokeArgs(pmethodName, &resultVal, pargFmt, args);
        if (retVal && presult)
        {
            // convert result to GFxValue
            ASValue2GFxValue(pLevel0Movie->GetASEnvironment(), resultVal, presult);
        }
        return retVal;
    }
    return false;
}

bool GFxMovieRoot::InvokeAlias(const char* pmethodName, const InvokeAliasInfo& alias, GASValue *presult, UInt numArgs)
{
    if (pLevel0Movie)
    {
#ifdef GFX_AMP_SERVER
        ScopeFunctionTimer invokeTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_InvokeAlias, Amp_Profile_Level_Low);
#endif

        GPtr<GASObject> pobj = alias.ThisObject; // make a copy to hold ref on object
        GPtr<GFxASCharacter> pchar;
        if (alias.ThisChar)
        {
            pchar = alias.ThisChar->ResolveCharacter(this);
        }
        GASObjectInterface* pthis = 
            (pobj) ? (GASObjectInterface*)pobj.GetPtr() : (GASObjectInterface*)pchar.GetPtr();
        GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
        return GAS_Invoke(alias.Function, presult, pthis, penv, numArgs, penv->GetTopIndex(), pmethodName);
    }
    return false;
}

bool GFxMovieRoot::InvokeAliasArgs(const char* pmethodName, const InvokeAliasInfo& alias, GASValue *presult, const char* methodArgFmt, va_list args)
{
    if (pLevel0Movie)
    {
#ifdef GFX_AMP_SERVER
        ScopeFunctionTimer invokeTimer(AdvanceStats, NativeCodeSwdHandle, Func_GFxMovieRoot_InvokeAliasArgs, Amp_Profile_Level_Low);
#endif

        GPtr<GASObject> pobj = alias.ThisObject; // make a copy to hold ref on object
        GPtr<GFxASCharacter> pchar;
        if (alias.ThisChar)
        {
            pchar = alias.ThisChar->ResolveCharacter(this);
        }
        GASObjectInterface* pthis = 
            (pobj) ? (GASObjectInterface*)pobj.GetPtr() : (GASObjectInterface*)pchar.GetPtr();
        GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
        return GAS_InvokeParsed(alias.Function, presult, pthis, penv, methodArgFmt, args, pmethodName);
    }
    return false;
}


void GFxMovieRoot::SetExternalInterfaceRetVal(const GFxValue& retVal)
{
    GFxValue2ASValue(retVal, &ExternalIntfRetVal);
}

void GFxMovieRoot::AddInvokeAlias(const GASString& alias, 
                                  GFxCharacterHandle* pthisChar, 
                                  GASObject* pthisObj, 
                                  const GASFunctionRef& func)
{
    if (!pInvokeAliases)
        pInvokeAliases = new GASStringHash<InvokeAliasInfo>();
    InvokeAliasInfo aliasInfo;
    aliasInfo.ThisObject = pthisObj;
    aliasInfo.ThisChar   = pthisChar;
    aliasInfo.Function   = func;
    pInvokeAliases->Set(alias, aliasInfo);
}

void        GFxMovieRoot::OnMovieFocus(bool set)
{
#ifndef GFC_NO_KEYBOARD_SUPPORT
    if (!set)
    {
        ResetKeyboardState();
        ResetMouseState();
    }
#endif //#ifndef GFC_NO_KEYBOARD_SUPPORT
    G_SetFlag<Flag_MovieIsFocused>(Flags, set);
#ifndef GFC_NO_IME_SUPPORT
    GPtr<GFxIMEManager> pIMEManager = GetIMEManager();
    if (pIMEManager)
    {
        pIMEManager->SetActiveMovie(this);
    }
#endif //#ifndef GFC_NO_IME_SUPPORT
}

GFxMovieRoot::InvokeAliasInfo* GFxMovieRoot::ResolveInvokeAlias(const char* pstr) const
{
    if (pInvokeAliases && pLevel0Movie)
    {
        GASString str = pLevel0Movie->GetASEnvironment()->CreateString(pstr);
        return pInvokeAliases->Get(str);
    }
    return NULL;
}

// Focus management
class GASTabIndexSortFunctor
{
public:
    inline bool operator()(const GFxASCharacter* a, const GFxASCharacter* b) const
    {
        return (a->GetTabIndex() < b->GetTabIndex());
    }
};

class GASAutoTabSortFunctor
{
    enum
    {
        Epsilon = 20
    };
public:
    inline bool operator()(const GFxASCharacter* a, const GFxASCharacter* b) const
    {
        GFxCharacter::Matrix ma = a->GetLevelMatrix();
        GFxCharacter::Matrix mb = b->GetLevelMatrix();
        GRectF aRect  = ma.EncloseTransform(a->GetFocusRect());
        GRectF bRect  = mb.EncloseTransform(b->GetFocusRect());

        //@DBG
        //printf("Comparing %s with %s\n", a->GetCharacterHandle()->GetNamePath().ToCStr(), b->GetCharacterHandle()->GetNamePath().ToCStr());

        GPointF centerA = aRect.Center();
        GPointF centerB = bRect.Center();

        if (G_Abs(aRect.Top - bRect.Top) <= Epsilon ||
            G_Abs(aRect.Bottom - bRect.Bottom) <= Epsilon ||
            G_Abs(centerA.y - centerB.y) <= Epsilon)
        {
            // same row
            //@DBG
            //printf ("   same row, less? %d, xA = %f, xB = %f\n", int(centerA.x < centerB.x), centerA.x , centerB.x);
            return centerA.x < centerB.x;
        }
        //@DBG
        //printf ("   less? %d, yA = %f, yB = %f\n", int(centerA.y < centerB.y), centerA.y , centerB.y);
        return centerA.y < centerB.y;
    }
};

void GFxMovieRoot::ResetTabableArrays()
{
    for (UInt i = 0; i < FocusGroupsCnt; ++i)
    {
        FocusGroups[i].ResetTabableArray();
    }
}

void GFxMovieRoot::ResetFocusStates()
{
    for (UInt i = 0; i < FocusGroupsCnt; ++i)
    {
        FocusGroups[i].ModalClip.Clear();
        FocusGroups[i].ResetTabableArray();
        FocusGroups[i].LastFocusKeyCode = 0;
    }
}

bool GFxMovieRoot::SetControllerFocusGroup(UInt controllerIdx, UInt focusGroupIndex)
{
    if (controllerIdx >= GFX_MAX_CONTROLLERS_SUPPORTED || 
        focusGroupIndex >= GFX_MAX_CONTROLLERS_SUPPORTED)
        return false;
    FocusGroupIndexes[controllerIdx] = (UInt8)focusGroupIndex;
    if (focusGroupIndex >= FocusGroupsCnt)
        FocusGroupsCnt = focusGroupIndex + 1;
    return true;
}

UInt GFxMovieRoot::GetControllerFocusGroup(UInt controllerIdx) const
{
    if (controllerIdx >= GFX_MAX_CONTROLLERS_SUPPORTED)
        return false;
    return FocusGroupIndexes[controllerIdx];
}

void GFxMovieRoot::FillTabableArray(const ProcessFocusKeyInfo* pfocusInfo)
{
    GASSERT(pfocusInfo);

    FocusGroupDescr& focusGroup = *pfocusInfo->pFocusGroup;
    if (pfocusInfo->InclFocusEnabled && 
        (focusGroup.TabableArrayStatus & FocusGroupDescr::TabableArray_Initialized) && 
        !(focusGroup.TabableArrayStatus & FocusGroupDescr::TabableArray_WithFocusEnabled))
        focusGroup.ResetTabableArray(); // need to refill array with focusEnabled chars
    if (!(focusGroup.TabableArrayStatus & FocusGroupDescr::TabableArray_Initialized))
    {
        GFxSprite* modalClip    = NULL;
        //bool tabIndexed         = false;
        GFxASCharacter::FillTabableParams p;
        p.Array = &focusGroup.TabableArray;
        p.InclFocusEnabled = pfocusInfo->InclFocusEnabled;
        if ((modalClip = focusGroup.GetModalClip(this)) == NULL)
        {
            // fill array by focusable characters
            for (int movieIndex = (int)MovieLevels.GetSize(); movieIndex > 0; movieIndex--)
            {
                MovieLevels[movieIndex-1].pSprite->FillTabableArray(&p);
            }
        }
        else
        {
            // fill array by focusable characters but only by children of ModalClip
            modalClip->FillTabableArray(&p);
        }

        if (p.TabIndexed)
        {
            // sort by tabIndex if tabIndexed == true
            static GASTabIndexSortFunctor sf;
            G_QuickSort(focusGroup.TabableArray, sf);
        }
        else
        {
            // sort for automatic order
            static GASAutoTabSortFunctor sf;
            G_QuickSort(focusGroup.TabableArray, sf);
        }
        focusGroup.TabableArrayStatus = FocusGroupDescr::TabableArray_Initialized;
        if (pfocusInfo->InclFocusEnabled)
            focusGroup.TabableArrayStatus |= FocusGroupDescr::TabableArray_WithFocusEnabled;
    }
}

void GFxMovieRoot::InitFocusKeyInfo(ProcessFocusKeyInfo* pfocusInfo, 
                                    const GFxInputEventsQueue::QueueEntry::KeyEntry& keyEntry,
                                    bool inclFocusEnabled,
                                    FocusGroupDescr* pfocusGroup)
{
    if (!pfocusInfo->Initialized)
    {
        FocusGroupDescr& focusGroup = (!pfocusGroup) ? GetFocusGroup(keyEntry.KeyboardIndex) : *pfocusGroup;
        pfocusInfo->pFocusGroup     = &focusGroup;
        pfocusInfo->PrevKeyCode     = focusGroup.LastFocusKeyCode;
        pfocusInfo->Prev_aRect      = focusGroup.LastFocusedRect;
        pfocusInfo->InclFocusEnabled= inclFocusEnabled;
        pfocusInfo->ManualFocus     = false;
        pfocusInfo->KeyboardIndex   = keyEntry.KeyboardIndex;

        FillTabableArray(pfocusInfo);

        //@DBG Debug code, do not remove!
        //for(int ii = 0; ii < tabableArraySize; ++ii)
        //{
        //printf("Focus[%d] = %s\n", ii, pfocusInfo->TabableArray[ii]->GetCharacterHandle()->GetNamePath().ToCStr());
        //}

        pfocusInfo->CurFocusIdx = -1;
        pfocusInfo->CurFocused  = focusGroup.LastFocused;
        if (pfocusInfo->CurFocused)
        {
            // find the index of currently focused
            for (UPInt i = 0; i < focusGroup.TabableArray.GetSize(); ++i)
            {
                if (focusGroup.TabableArray[i] == pfocusInfo->CurFocused)
                {
                    pfocusInfo->CurFocusIdx = (int)i;
                    break;
                }
            }
        }
        pfocusInfo->Initialized = true;
    }
}

void GFxMovieRoot::ProcessFocusKey(GFxEvent::EventType event, 
                                   const GFxInputEventsQueue::QueueEntry::KeyEntry& keyEntry, 
                                   ProcessFocusKeyInfo* pfocusInfo)
{
    if (event == GFxEvent::KeyDown)
    {
        if (keyEntry.Code == GFxKey::Tab || 
            ((IsFocusRectShown(keyEntry.KeyboardIndex) || pfocusInfo->ManualFocus) && 
            (keyEntry.Code == GFxKey::Left || keyEntry.Code == GFxKey::Right || 
             keyEntry.Code == GFxKey::Up || keyEntry.Code == GFxKey::Down)))
        {
            // init focus info if it is not initialized yet
            InitFocusKeyInfo(pfocusInfo, keyEntry, false);
            const FocusGroupDescr& focusGroup = *pfocusInfo->pFocusGroup;
            int tabableArraySize = (int)focusGroup.TabableArray.GetSize();
            if (keyEntry.Code == GFxKey::Tab)
            {
                int cnt;
                int curFocusIdx = pfocusInfo->CurFocusIdx;
                pfocusInfo->CurFocusIdx = -1;
                for (cnt = 0; cnt < tabableArraySize; ++cnt)
                {
                    if (GFxSpecialKeysState(keyEntry.SpecialKeysState).IsShiftPressed())
                    {
                        if (--curFocusIdx < 0)
                            curFocusIdx = tabableArraySize-1;
                    }
                    else
                    {
                        if (++curFocusIdx >= tabableArraySize)
                            curFocusIdx = 0;
                    }
                    // check if candidate is tabable, or if manual focus mode is on. Break, if so.
                    if (focusGroup.TabableArray[curFocusIdx] && 
                        (pfocusInfo->InclFocusEnabled || focusGroup.TabableArray[curFocusIdx]->IsTabable()) &&
                        focusGroup.TabableArray[curFocusIdx]->IsFocusAllowed(this, pfocusInfo->KeyboardIndex))
                    {
                        pfocusInfo->CurFocusIdx = curFocusIdx;
                        break;
                    }
                } 
                SetDirtyFlag();
            }
            else if (pfocusInfo->CurFocused)
            {
                if (pfocusInfo->CurFocused->IsFocusRectEnabled() || IsAlwaysEnableFocusArrowKeys())
                {
                    GFxCharacter::Matrix ma = pfocusInfo->CurFocused->GetLevelMatrix();
                    // aRect represents the rectangle of currently focused character.
                    GRectF aRect  = ma.EncloseTransform(pfocusInfo->CurFocused->GetFocusRect());

                    // We need to adjust the existing source rectangle (aRect). If we have
                    // saved previous rectangle and the direction if focus moving is the same
                    // (for example, we pressed the up/down arrow key again), then we will use
                    // Left/Right coordinates from the saved rectangle to maintain the same Y-axis.
                    // Same will happen if we move right/left, we will use original Top/Bottom
                    // to keep X-axis.
                    if (pfocusInfo->PrevKeyCode == keyEntry.Code)
                    {
                        if (keyEntry.Code == GFxKey::Up || keyEntry.Code == GFxKey::Down)
                        {
                            aRect.Left      = pfocusInfo->Prev_aRect.Left;
                            aRect.Right     = pfocusInfo->Prev_aRect.Right;
                        }
                        else if (keyEntry.Code == GFxKey::Right || keyEntry.Code == GFxKey::Left)
                        {
                            aRect.Top       = pfocusInfo->Prev_aRect.Top;
                            aRect.Bottom    = pfocusInfo->Prev_aRect.Bottom;
                        }
                    }
                    else
                    {
                        pfocusInfo->Prev_aRect  = aRect;
                        pfocusInfo->PrevKeyCode = keyEntry.Code;
                    }

                    if (keyEntry.Code == GFxKey::Right || keyEntry.Code == GFxKey::Left)
                    {
                        SetDirtyFlag();
                        int newFocusIdx = pfocusInfo->CurFocusIdx;
                        GRectF newFocusRect;
                        newFocusRect.Left = newFocusRect.Right = (keyEntry.Code == GFxKey::Right) ? Float(INT_MAX) : Float(INT_MIN);
                        newFocusRect.Top = newFocusRect.Bottom = Float(INT_MAX);
                        // find nearest from right or left side

                        bool hitStipe = false;
                        for (int i = 0; i < tabableArraySize - 1; ++i)
                        {
                            newFocusIdx = (keyEntry.Code == GFxKey::Right) ? newFocusIdx + 1 : newFocusIdx - 1;
                            if (newFocusIdx >= tabableArraySize)
                                newFocusIdx = 0;
                            else if (newFocusIdx < 0)
                                newFocusIdx = tabableArraySize - 1;
                            GPtr<GFxASCharacter> b = focusGroup.TabableArray[newFocusIdx];
                            if ((!pfocusInfo->InclFocusEnabled && !b->IsTabable()) ||
                                !b->IsFocusAllowed(this, pfocusInfo->KeyboardIndex))
                            {
                                // If this is not for manual focus and not tabable - ignore.
                                continue;
                            }
                            GFxCharacter::Matrix mb = b->GetLevelMatrix();
                            GRectF bRect  = mb.EncloseTransform(b->GetFocusRect());

                            bool  curHitStripe    = false;
                            // check the "stripe zone"

                            GRectF stripeRect;
                            if (keyEntry.Code == GFxKey::Right)
                                stripeRect = GRectF(aRect.Right+1, aRect.Top, GFC_MAX_FLOAT, aRect.Bottom);
                            else  // Left
                                stripeRect = GRectF(GFC_MIN_FLOAT, aRect.Top, aRect.Left-1, aRect.Bottom);
                            // Check, if the current character ("b") is in the stripe zone or not.
                            if (bRect.Intersects(stripeRect))
                            {
                                GRectF intersectionRect = bRect;
                                intersectionRect.Intersect(stripeRect);
                                //@DBG
                                //printf("Intersection height is %f\n", intersectionRect.Height());
                                if (intersectionRect.Height() >= 40) // 2 pixels threshold
                                    curHitStripe    = true;
                            }
                            if (curHitStripe)
                            {
                                if (!hitStipe)
                                { // first hit - save the current char index and rect
                                    pfocusInfo->CurFocusIdx = newFocusIdx;
                                    newFocusRect            = bRect;
                                    hitStipe                = true;

                                    //@DBG
                                    //printf("InitFocus = %s\n",  
                                    //    b->GetCharacterHandle()->GetNamePath().ToCStr());
                                    continue;
                                }
                            }
                            // if we already hit stripe once - ignore all chars NOT in the stripe zone
                            if (hitStipe && !curHitStripe)
                                continue;

                            GPointF vector1, vector2;
                            // lets calculate the distance from the adjusted rectangle of currently
                            // focused character to the character currently being iterated, and compare
                            // the distance with the distance to existing candidate.
                            // The distance being calculated is from the middle of the right side (if "right" 
                            // key is down) to the middle of the left side, or other way around.
                            if (keyEntry.Code == GFxKey::Right)
                            {
                                GPointF ptBeg(aRect.Right, aRect.Top + aRect.Height()/2);
                                GRectF brect(bRect), newrect(newFocusRect);
                                brect.HClamp  (aRect.Right, bRect.Right);
                                newrect.HClamp(aRect.Right, newFocusRect.Right);
                                if (!brect.IsNormal() || G_IRound(TwipsToPixels(brect.Width())) <= 3) // threshold 3 pixels
                                    continue;
                                GPointF ptEnd1(brect.Left, bRect.Top + bRect.Height()/2);
                                GPointF ptEnd2(newrect.Left, newFocusRect.Top + newFocusRect.Height()/2);

                                vector1.x = floorf(TwipsToPixels(ptEnd1.x - ptBeg.x)); 
                                vector1.y = floorf(TwipsToPixels(ptEnd1.y - ptBeg.y));
                                vector2.x = floorf(TwipsToPixels(ptEnd2.x - ptBeg.x)); 
                                vector2.y = floorf(TwipsToPixels(ptEnd2.y - ptBeg.y));
                                if (vector1.x < 0) // negative, means it is not at the right
                                    continue;
                            }
                            else // left
                            {
                                GPointF ptBeg(aRect.Left, aRect.Top + aRect.Height()/2);
                                GRectF brect(bRect), newrect(newFocusRect);
                                brect.HClamp  (bRect.Left, aRect.Left);
                                newrect.HClamp(newFocusRect.Left, aRect.Left);
                                if (!brect.IsNormal() || G_IRound(TwipsToPixels(brect.Width())) <= 3) // threshold 3 pixels
                                    continue;
                                GPointF ptEnd1(brect.Right, bRect.Top + bRect.Height()/2);
                                GPointF ptEnd2(newrect.Right, newFocusRect.Top + newFocusRect.Height()/2);

                                vector1.x = floorf(TwipsToPixels(ptEnd1.x - ptBeg.x)); 
                                vector1.y = floorf(TwipsToPixels(ptEnd1.y - ptBeg.y));
                                vector2.x = floorf(TwipsToPixels(ptEnd2.x - ptBeg.x)); 
                                vector2.y = floorf(TwipsToPixels(ptEnd2.y - ptBeg.y));
                                if (vector1.x > 0) // positive, means it is not at the left
                                    continue;
                            }
                            //@DBG
                            //printf("Checking for %s, vec1(%d,%d), vec2(%d,%d)\n",  
                            //    b->GetCharacterHandle()->GetNamePath().ToCStr(),
                            //    (SInt)vector1.x, (SInt)vector1.y, (SInt)vector2.x, (SInt)vector2.y);
                            // Check, if the character in the "stripe-zone". If yes - check, is the new char
                            // closer by the 'x' coordinate or not. If 'x' coordinate is the same - check if it is 
                            // close by the 'y' coordinate. Update 'newFocus' item if so.
                            // If stripe is not hit, then just measure the distance to the new char and
                            // update the 'newFocus' item if it is closer than previous one.
                            if ((hitStipe && 
                                 (G_Abs(vector1.x) < G_Abs(vector2.x) || (vector1.x == vector2.x && G_Abs(vector1.y) < G_Abs(vector2.y)))) || 
                                (!hitStipe &&
                                (vector1.x*vector1.x + vector1.y*vector1.y < 
                                vector2.x*vector2.x + vector2.y*vector2.y)))
                            {
                                //@DBG
                                //printf("   newFocus = %s, vec1(%d,%d), vec2(%d,%d)\n",  
                                //    b->GetCharacterHandle()->GetNamePath().ToCStr(),
                                //    (SInt)vector1.x, (SInt)vector1.y, (SInt)vector2.x, (SInt)vector2.y);
                                pfocusInfo->CurFocusIdx = newFocusIdx;
                                newFocusRect            = bRect;
                            }
                        }
                    }
                    else if (keyEntry.Code == GFxKey::Up || keyEntry.Code == GFxKey::Down)
                    {
                        SetDirtyFlag();
                        int newFocusIdx = pfocusInfo->CurFocusIdx;
                        GRectF newFocusRect(0);
                        newFocusRect.Left = Float(INT_MAX);
                        newFocusRect.Top = (keyEntry.Code == GFxKey::Down) ? Float(INT_MAX) : Float(INT_MIN);
                        // find nearest from top and bottom side
                        // The logic is as follows:
                        // 1. The highest priority characters are ones, which boundary rectangles intersect with the "stripe zone" 
                        // above or below the currently selected character. I.e. for Down key the "stripe zone" will be 
                        // ((aRect.Left, aRect.Right), (aRect.Bottom, Infinity)).
                        //   a) if there are more than one characters in the "stripe zone", then the best candidate should
                        //      have shortest distance by Y axis.
                        // 2. Otherwise, the closest character will be chosen by comparing Y-distance and only then X-distance.
                        bool hitStipe = false;

                        for (int i = 0; i < tabableArraySize - 1; ++i)
                        {
                            newFocusIdx = (keyEntry.Code == GFxKey::Down) ? newFocusIdx + 1 : newFocusIdx - 1;
                            if (newFocusIdx >= tabableArraySize)
                                newFocusIdx = 0;
                            else if (newFocusIdx < 0)
                                newFocusIdx = tabableArraySize - 1;
                            GPtr<GFxASCharacter> b  = focusGroup.TabableArray[newFocusIdx];
                            if ((!pfocusInfo->InclFocusEnabled && !b->IsTabable()) ||
                                !b->IsFocusAllowed(this, pfocusInfo->KeyboardIndex))
                            {
                                // If this is not for manual focus and not tabable - ignore.
                                continue;
                            }
                            GFxCharacter::Matrix mb = b->GetLevelMatrix();
                            GRectF bRect  = mb.EncloseTransform(b->GetFocusRect());

                            bool  curHitStripe    = false;
                            // check the "stripe zone"
                            GRectF stripeRect;
                            if (keyEntry.Code == GFxKey::Down)
                                stripeRect = GRectF(aRect.Left, aRect.Bottom + 1, aRect.Right, GFC_MAX_FLOAT);
                            else // Up
                                stripeRect = GRectF(aRect.Left, GFC_MIN_FLOAT, aRect.Right, aRect.Top - 1);
                            //@DBG
                            //printf("bRect = %s\n",  b->GetCharacterHandle()->GetNamePath().ToCStr());
                            if (bRect.Intersects(stripeRect))
                            {
                                GRectF intersectionRect = bRect;
                                intersectionRect.Intersect(stripeRect);
                                //@DBG
                                //printf("Intersection width is %f\n", intersectionRect.Width());
                                if (intersectionRect.Width() >= 40) // 2 pixels threshold
                                    curHitStripe    = true;
                            }
                            //@DBG
                            //printf ("curHitStripe is %d\n", (int)curHitStripe);
                            if (curHitStripe)
                            {
                                if (!hitStipe)
                                { // first hit - save the current char index and rect
                                    pfocusInfo->CurFocusIdx = newFocusIdx;
                                    newFocusRect = bRect;
                                    hitStipe = true;

                                    //@DBG
                                    //printf("InitFocus = %s\n",  
                                    //    b->GetCharacterHandle()->GetNamePath().ToCStr());
                                    continue;
                                }
                            }
                            // if we already hit stripe once - ignore all chars NOT in the stripe zone
                            if (hitStipe && !curHitStripe)
                                continue;

                            GPointF vector1, vector2;
                            // lets calculate the distance from the adjusted rectangle of currently
                            // focused character to the character currently being iterated, and compare
                            // the distance with the distance to existing candidate.
                            // The distance being calculated is from the middle of the top side (if "up" 
                            // key is down) to the middle of the bottom side, or other way around.
                            if (keyEntry.Code == GFxKey::Up)
                            {
                                GPointF ptBeg(aRect.Left + aRect.Width()/2, aRect.Top);
                                GRectF brect(bRect), newrect(newFocusRect);
                                brect.VClamp  (bRect.Top, aRect.Top);
                                newrect.VClamp(newFocusRect.Top, aRect.Top);
                                if (!brect.IsNormal() || G_IRound(TwipsToPixels(brect.Height())) <= 3) // threshold 3 pixels
                                    continue;
                                GPointF ptEnd1(brect.Left + brect.Width()/2, brect.Bottom);
                                GPointF ptEnd2(newrect.Left + newrect.Width()/2, newrect.Bottom);
                                
                                vector1.x = floorf(TwipsToPixels(ptEnd1.x - ptBeg.x)); 
                                vector1.y = floorf(TwipsToPixels(ptEnd1.y - ptBeg.y));
                                vector2.x = floorf(TwipsToPixels(ptEnd2.x - ptBeg.x)); 
                                vector2.y = floorf(TwipsToPixels(ptEnd2.y - ptBeg.y));
                                if (vector1.y > 0) // positive, means it is not at the top
                                    continue;
                            }
                            else // down
                            {
                                GPointF ptBeg(aRect.Left + aRect.Width()/2, aRect.Bottom);
                                GRectF brect(bRect), newrect(newFocusRect);
                                brect.VClamp  (aRect.Bottom, bRect.Bottom);
                                newrect.VClamp(aRect.Bottom, newFocusRect.Bottom);
                                if (!brect.IsNormal() || G_IRound(TwipsToPixels(brect.Height())) <= 3) // threshold 3 pixels
                                    continue;
                                GPointF ptEnd1(brect.Left + brect.Width()/2, brect.Top);
                                GPointF ptEnd2(newrect.Left + newrect.Width()/2, newrect.Top);
                                
                                vector1.x = floorf(TwipsToPixels(ptEnd1.x - ptBeg.x)); 
                                vector1.y = floorf(TwipsToPixels(ptEnd1.y - ptBeg.y));
                                vector2.x = floorf(TwipsToPixels(ptEnd2.x - ptBeg.x)); 
                                vector2.y = floorf(TwipsToPixels(ptEnd2.y - ptBeg.y));
                                if (vector1.y < 0) // negative, means it is not at the down
                                    continue;
                            }
                            //@DBG
                            //printf("Checking for %s, vec1(%d,%d), vec2(%d,%d)\n",  
                            //    b->GetCharacterHandle()->GetNamePath().ToCStr(),
                            //    (SInt)vector1.x, (SInt)vector1.y, (SInt)vector2.x, (SInt)vector2.y);
                            // Check, if the character in the "stripe-zone". If yes - check, is the new char
                            // closer by the 'y' coordinate or not. If 'x' coordinate is the same - check if it is 
                            // close by the 'x' coordinate. Update 'newFocus' item if so.
                            // If stripe is not hit, then just measure the distance to the new char and
                            // update the 'newFocus' item if it is closer than previous one.
                            if ((hitStipe && 
                                (G_Abs(vector1.y) < G_Abs(vector2.y) || (vector1.y == vector2.y && G_Abs(vector1.x) < G_Abs(vector2.x)))) || 
                                (!hitStipe &&
                                (vector1.x*vector1.x + vector1.y*vector1.y < 
                                vector2.x*vector2.x + vector2.y*vector2.y)))
                            {
                                //@DBG
                                //printf("   newFocus = %s, vec1(%d,%d), vec2(%d,%d)\n",  
                                //    b->GetCharacterHandle()->GetNamePath().ToCStr(),
                                //    (SInt)vector1.x, (SInt)vector1.y, (SInt)vector2.x, (SInt)vector2.y);
                                pfocusInfo->CurFocusIdx = newFocusIdx;
                                newFocusRect = bRect;
                            }
                        }
                    }
                }
            }
            if (pfocusInfo->CurFocusIdx >= 0 && pfocusInfo->CurFocusIdx < tabableArraySize)
            {
                pfocusInfo->CurFocused = focusGroup.TabableArray[pfocusInfo->CurFocusIdx];
            }
            else
            {
                pfocusInfo->CurFocused = NULL;
            }
        }
    }
}

void GFxMovieRoot::FinalizeProcessFocusKey(ProcessFocusKeyInfo* pfocusInfo)
{
    FocusGroupDescr& focusGroup = GetFocusGroup(pfocusInfo->KeyboardIndex);
    if (pfocusInfo && pfocusInfo->Initialized && 
        (focusGroup.TabableArrayStatus & FocusGroupDescr::TabableArray_Initialized))
    {
        GFxASCharacter* psetFocusToCh;
        if (pfocusInfo->CurFocusIdx >= 0 && pfocusInfo->CurFocusIdx < (int)focusGroup.TabableArray.GetSize())
            psetFocusToCh = focusGroup.TabableArray[pfocusInfo->CurFocusIdx];
        else
        {
            // if CurFocusIdx is out of the TabableArray then do nothing. This may happen
            // when TabableArray is empty; in this case we do not need to transfer focus
            // anywhere.
            return; 
        }

        GPtr<GFxASCharacter> lastFocused = focusGroup.LastFocused;

        if (lastFocused != psetFocusToCh)
        {
            // keep tracking direction of focus movement
            focusGroup.LastFocusKeyCode = pfocusInfo->PrevKeyCode;
            focusGroup.LastFocusedRect  = pfocusInfo->Prev_aRect;

            QueueSetFocusTo(psetFocusToCh, NULL, pfocusInfo->KeyboardIndex, GFx_FocusMovedByKeyboard);
            if (!psetFocusToCh || psetFocusToCh->GetObjectType() != GFxASCharacter::Object_TextField)
                focusGroup.FocusRectShown = true;
            else
                focusGroup.FocusRectShown = false;
            SetDirtyFlag();
        }
    }
}

void GFxMovieRoot::ActivateFocusCapture(UInt controllerIdx)
{
    ProcessFocusKeyInfo focusKeyInfo;
    GFxInputEventsQueue::QueueEntry::KeyEntry keyEntry;
    keyEntry.Code = GFxKey::Tab;
    keyEntry.SpecialKeysState = 0;
    keyEntry.KeyboardIndex = (UInt8)controllerIdx;
    ProcessFocusKey(GFxKeyEvent::KeyDown, keyEntry, &focusKeyInfo);
    FinalizeProcessFocusKey(&focusKeyInfo);
}

void GFxMovieRoot::SetModalClip(GFxSprite* pmovie, UInt controllerIdx)
{
    FocusGroupDescr& focusGroup = GetFocusGroup(controllerIdx);

    if (!pmovie)
        focusGroup.ModalClip = NULL;
    else
        focusGroup.ModalClip = pmovie->GetCharacterHandle();
}

GFxSprite* GFxMovieRoot::GetModalClip(UInt controllerIdx)
{
    FocusGroupDescr& focusGroup = GetFocusGroup(controllerIdx);
    return focusGroup.GetModalClip(this);
}

UInt32 GFxMovieRoot::GetControllerMaskByFocusGroup(UInt focusGroupIndex)
{
    UInt32 v = 0;
    for (UInt f = 0, mask = 0x1; f < GFX_MAX_CONTROLLERS_SUPPORTED; ++f, mask <<= 1)
    {
        if (FocusGroupIndexes[f] == focusGroupIndex)
            v |= mask;
    }
    return v;
}

void GFxMovieRoot::DisplayFocusRect(const GFxDisplayContext& context)
{
    for (UInt f = 0; f < FocusGroupsCnt; ++f)
    {
        GPtr<GFxASCharacter> curFocused = FocusGroups[f].LastFocused;
        if (curFocused  && FocusGroups[f].FocusRectShown && curFocused->IsFocusRectEnabled())
        {
            GMatrix3D           m3d;
            GRenderer::Matrix   mat = curFocused->GetWorldMatrix();         
            GRectF focusLocalRect = curFocused->GetFocusRect();

            if (focusLocalRect.IsNull())
                return;
#ifndef GFC_NO_3D
            m3d = curFocused->GetWorldMatrix3D();
#else
            m3d = GMatrix3D(mat);
#endif
            // Do viewport culling if bounds are available.
            if (!VisibleFrameRect.Intersects(m3d.EncloseTransform(focusLocalRect)))
                if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                    return;

            GRenderer* prenderer   = context.GetRenderer();

            GRectF focusWorldRect = m3d.EncloseTransform(focusLocalRect);
            focusWorldRect.Expand(f*20.f, f*20.f);

            prenderer->SetCxform(GRenderer::Cxform()); // reset color transform for focus rect
            prenderer->SetMatrix(GRenderer::Matrix()); // reset matrix to work in world coords


            GPointF         coords[4];
            static const UInt16 indices[24] = { 
                0, 1, 2,    2, 1, 3,    2, 4, 5,    5, 4, 6,    7, 3, 8,    8, 3, 9,    5, 9, 10,   10, 9, 11 
            };
#define LIMIT_COORD(c) ((c>32767)?32767.f:((c < -32768)?-32768.f:c))
#define LIMITED_POINT(p) GPointF(LIMIT_COORD(p.x), LIMIT_COORD(p.y))

            coords[0] = LIMITED_POINT(focusWorldRect.TopLeft());
            coords[1] = LIMITED_POINT(focusWorldRect.TopRight());
            coords[2] = LIMITED_POINT(focusWorldRect.BottomRight());
            coords[3] = LIMITED_POINT(focusWorldRect.BottomLeft());

            GRenderer::VertexXY16i icoords[12];
            // Strip (fill in)
            icoords[0].x = (SInt16) coords[0].x;      icoords[0].y = (SInt16) coords[0].y;        // 0
            icoords[1].x = (SInt16) coords[1].x;      icoords[1].y = (SInt16) coords[1].y;        // 1
            icoords[2].x = (SInt16) coords[0].x;      icoords[2].y = (SInt16) coords[0].y + 40;   // 2
            icoords[3].x = (SInt16) coords[1].x;      icoords[3].y = (SInt16) coords[1].y + 40;   // 3
            icoords[4].x = (SInt16) coords[0].x + 40; icoords[4].y = (SInt16) coords[0].y + 40;   // 4
            icoords[5].x = (SInt16) coords[3].x;      icoords[5].y = (SInt16) coords[3].y - 40;   // 5
            icoords[6].x = (SInt16) coords[3].x + 40; icoords[6].y = (SInt16) coords[3].y - 40;   // 6
            icoords[7].x = (SInt16) coords[1].x - 40; icoords[7].y = (SInt16) coords[1].y + 40;   // 7
            icoords[8].x = (SInt16) coords[2].x - 40; icoords[8].y = (SInt16) coords[2].y - 40;   // 8
            icoords[9].x = (SInt16) coords[2].x;      icoords[9].y = (SInt16) coords[2].y - 40;   // 9 
            icoords[10].x = (SInt16) coords[3].x;     icoords[10].y = (SInt16) coords[3].y;       // 10
            icoords[11].x = (SInt16) coords[2].x;     icoords[11].y = (SInt16) coords[2].y;       // 11

            prenderer->SetVertexData(icoords, 12, GRenderer::Vertex_XY16i);

            UInt32 colorValue = 0xFFFF00;
            colorValue ^= (f * 0x1080D0);

            prenderer->FillStyleColor(colorValue | 0xFF000000);

            // Fill the inside
            prenderer->SetIndexData(indices, 24, GRenderer::Index_16);
            prenderer->DrawIndexedTriList(0, 0, 12, 0, 8);

            // Done
            prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
            prenderer->SetIndexData(0, 0, GRenderer::Index_None);
        }
    }
}

void GFxMovieRoot::HideFocusRect(UInt controllerIdx)
{
    FocusGroupDescr& focusGroup = GetFocusGroup(controllerIdx);
    if (focusGroup.FocusRectShown)
    {
        GPtr<GFxASCharacter> curFocused = focusGroup.LastFocused;
        if (curFocused && curFocused->GetParent())
            if (!curFocused->OnLosingKeyboardFocus(NULL, controllerIdx))
                return; // focus loss was prevented
    }
    focusGroup.FocusRectShown = false;
}

void GFxMovieRoot::SetFocusTo(GFxASCharacter* ch, UInt controllerIdx)
{
    // the order of events, if Selection.setFocus is invoked is as follows:
    // Instantly:
    // 1. curFocus.onKillFocus, curFocus = oldFocus
    // 2. curFocus = newFocus
    // 3. curFocus.onSetFocus, curFocus = newFocus
    // 4. Selection focus listeners, curFocus = newFocus
    // Queued:
    // 5. oldFocus.onRollOut, curFocus = newFocus
    // 6. newFocus.onRollOver, curFocus = newFocus
    FocusGroupDescr& focusGroup = GetFocusGroup(controllerIdx);
    GPtr<GFxASCharacter> curFocused = focusGroup.LastFocused;

    if (curFocused == ch) return;

    if (curFocused && curFocused->GetParent())
    {
        // queue onRollOut, step 5 (since it is queued up, we may do it before TransferFocus)
        if (!curFocused->OnLosingKeyboardFocus(ch, controllerIdx))
            return; // focus loss was prevented
    }

    // Do instant focus transfer (steps 1-4)
    TransferFocus(ch, controllerIdx, GFx_FocusMovedByKeyboard);

    // invoke onSetFocus for newly set LastFocused
    if (ch)
    {
        // queue onRollOver, step 6
        ch->OnGettingKeyboardFocus(controllerIdx);
    }
}

void GFxMovieRoot::QueueSetFocusTo(GFxASCharacter* ch, GFxASCharacter* ptopMostCh, UInt controllerIdx, GFxFocusMovedType fmt)
{
    // the order of events, if focus key is pressed is as follows:
    // 1. curFocus.onRollOut, curFocus = oldFocus
    // 2. newFocus.onRollOver, curFocus = oldFocus
    // 3. curFocus.onKillFocus, curFocus = oldFocus
    // 4. curFocus = newFocus
    // 5. curFocus.onSetFocus, curFocus = newFocus
    // 6. Selection focus listeners, curFocus = newFocus
    FocusGroupDescr& focusGroup = GetFocusGroup(controllerIdx);
    GPtr<GFxASCharacter> curFocused = focusGroup.LastFocused;

    if (curFocused == ch) 
        return;

#ifndef GFC_NO_IME_SUPPORT
    GPtr<GFxIMEManager> pIMEManager = GetIMEManager();
    if (pIMEManager)
    {
        // report about focus change to IME. IME may prevent focus changing by
        // returning previously focused character or grant by returning new
        // focusing one.
        ch = pIMEManager->HandleFocus(this, curFocused, ch, ptopMostCh);
        if (curFocused == ch) 
            return;
    }
#else
    GUNUSED(ptopMostCh);
#endif //#ifndef GFC_NO_IME_SUPPORT

    if (curFocused && curFocused->GetParent())
    {
        // queue onRollOut (step 1)
        if (!curFocused->OnLosingKeyboardFocus(ch, controllerIdx, fmt))
            return; // if focus loss was prevented - return
    }

    // invoke onSetFocus for newly set LastFocused
    if (ch)
    {
        // queue onRollOver (step 2)
        ch->OnGettingKeyboardFocus(controllerIdx);
    }

    // Queue setting focus to ch (steps 3-6)
    GASSelection::QueueSetFocus(GetLevelMovie(0)->GetASEnvironment(), ch, controllerIdx, fmt); 
}

// Instantly transfers focus w/o any queuing
void GFxMovieRoot::TransferFocus(GFxASCharacter* pNewFocus, UInt controllerIdx, GFxFocusMovedType fmt)
{
    FocusGroupDescr& focusGroup = GetFocusGroup(controllerIdx);
    GPtr<GFxASCharacter> curFocused = focusGroup.LastFocused;

    if (curFocused == pNewFocus) return;

    if (curFocused && curFocused->GetParent())
    {
        // invoke onKillFocus for LastFocused
        curFocused->OnFocus(GFxASCharacter::KillFocus, pNewFocus, controllerIdx, fmt);
    }

    focusGroup.LastFocused = pNewFocus;

    // invoke onSetFocus for newly set LastFocused
    if (pNewFocus)
    {
        pNewFocus->OnFocus(GFxASCharacter::SetFocus, curFocused, controllerIdx, fmt);
    }

    GASSelection::BroadcastOnSetFocus(GetLevelMovie(0)->GetASEnvironment(), curFocused, pNewFocus, controllerIdx);
}


void GFxMovieRoot::SetKeyboardFocusTo(GFxASCharacter* ch, UInt controllerIdx)
{
#ifndef GFC_NO_IME_SUPPORT
    GPtr<GFxIMEManager> pIMEManager = GetIMEManager();
    if (pIMEManager)
    {
        // report about focus change to IME. IME may prevent focus changing by
        // returning previously focused character or grant by returning new
        // focusing one.
        GPtr<GFxASCharacter> curFocused = GetFocusGroup(controllerIdx).LastFocused;
        ch = pIMEManager->HandleFocus(this, curFocused, ch, NULL);
    }
#endif //#ifndef GFC_NO_IME_SUPPORT
    FocusGroupDescr& focusGroup = GetFocusGroup(controllerIdx);

    if (!ch || ch->GetObjectType() != GFxASCharacter::Object_TextField)
        focusGroup.FocusRectShown = true;
    else
        focusGroup.FocusRectShown = false;
    
    focusGroup.ResetFocusDirection();
    SetFocusTo(ch, controllerIdx);
    // here is a difference with the Flash: in Flash, if you do Selection.setFocus on invisible character or
    // character with invisible parent then Flash sets the focus on it and shows the focusRect.
    // GFxPlayer sets the focus, but doesn't show the focusRect. Probably, it shouldn't set focus at all.
    // (AB, 02/22/07).
    if (focusGroup.FocusRectShown)
    {
        GFxASCharacter* cur = ch;
        for (; cur && cur->GetVisible(); cur = cur->GetParent())
            ;
        focusGroup.FocusRectShown = !cur;
    }
}

void GFxMovieRoot::ResetFocusForChar(GFxASCharacter* ch)
{
    for (UInt i = 0; i < FocusGroupsCnt; ++i)
    {
        if (FocusGroups[i].IsFocused(ch))
        {
            // send focus out event
            GPtr<GFxASCharacter> curFocused = FocusGroups[i].LastFocused;
            if (curFocused)
            {
                UInt32 m = GetControllerMaskByFocusGroup(i);
                UInt cc  = GetControllerCount();
                for (UInt j = 0; m != 0 && j < cc; ++j, m >>= 1)
                    SetFocusTo(NULL, j);
            }
            FocusGroups[i].ResetFocus();
        }
    }
}

bool GFxMovieRoot::IsFocused(const GFxASCharacter* ch) const
{
    for (UInt i = 0; i < FocusGroupsCnt; ++i)
    {
        if (FocusGroups[i].IsFocused(ch))
            return true;
    }
    return false;
}

void GFxMovieRoot::AddTopmostLevelCharacter(GFxASCharacter* pch)
{
    GASSERT(pch);
    // do not allow to mark _root/_levelN sprites as topmostLevel...
    if (pch->IsSprite() && pch->ToSprite()->IsLevelsMovie())
        return;

    // make sure this character is not in the list yet
    // we need insert the character at the right pos to maintain original order
    // of topmostLevel characters.
    // So, storing the char with lowest original depth first and the highest last.
    // All characters for higher _level will be stored in front of characters for lower _level.
    
    UPInt i = 0;
    if (TopmostLevelCharacters.GetSize() > 0)
    {
        GArrayDH<GFxCharacter*> chParents(GetHeap());
        GArrayDH<GFxCharacter*> curParents(GetHeap());
        // fill array of parents for the passed char
        GFxASCharacter* pchTopPar = NULL;
        for (GFxASCharacter* ppar = pch; ppar; ppar = ppar->GetParent())
        {
            chParents.PushBack(ppar);
            pchTopPar = ppar;
        }

        // find the position according to depth and level. Exhaustive search for now.
        for (UPInt n = TopmostLevelCharacters.GetSize(); i < n; ++i)
        {
            if (TopmostLevelCharacters[i] == pch)
                return;

            // fill array of parent for the current char
            curParents.Resize(0);
            GFxASCharacter* pcurTopPar = NULL;
            for (GFxASCharacter* ppar = TopmostLevelCharacters[i]; ppar; ppar = ppar->GetParent())
            {
                curParents.PushBack(ppar);
                pcurTopPar = ppar;
            }

            if (pcurTopPar == pchTopPar)
            {
                // compare parents, starting from the top ones
                SPInt chParIdx  = (SPInt)chParents.GetSize()-1;
                SPInt curParIdx = (SPInt)curParents.GetSize()-1;
                bool found = false, cancel_iteration = false;
                for(;chParIdx >= 0 && curParIdx >= 0; --chParIdx, --curParIdx)
                {
                    if (chParents[chParIdx] != curParents[curParIdx])
                    {
                        // parents are different: compare depths of first different parents then.
                        // Note, 'parents' arrays contain the characters itselves, so, there is no 
                        // need to test them separately.
                        GASSERT(chParents[chParIdx]->GetDepth() != curParents[curParIdx]->GetDepth());
                        if (chParents[chParIdx]->GetDepth() < curParents[curParIdx]->GetDepth())
                            found = true;
                        else
                            cancel_iteration = true;
                        break;
                    }
                }
                if (found)
                    break;
                else if (cancel_iteration)
                    continue;
            }
            else
            {
                GASSERT(static_cast<GFxSprite*>(pchTopPar)->IsLevelsMovie());
                GASSERT(static_cast<GFxSprite*>(pcurTopPar)->IsLevelsMovie());
                GASSERT(static_cast<GFxSprite*>(pcurTopPar)->GetLevel() != static_cast<GFxSprite*>(pchTopPar)->GetLevel());

                // different levels, compare their numbers
                if (static_cast<GFxSprite*>(pcurTopPar)->GetLevel() > static_cast<GFxSprite*>(pchTopPar)->GetLevel())
                    break; // stop, if we are done with our level
            }
        }
    }
    TopmostLevelCharacters.InsertAt(i, pch);
}

void GFxMovieRoot::RemoveTopmostLevelCharacter(GFxCharacter* pch)
{
    for (UPInt i = 0, n = TopmostLevelCharacters.GetSize(); i < n; ++i)
    {
        if (TopmostLevelCharacters[i] == pch)
        {
            TopmostLevelCharacters.RemoveAt(i);
            return;
        }
    }
}

void GFxMovieRoot::DisplayTopmostLevelCharacters(GFxDisplayContext &context) const
{
    GRenderer::Matrix* pom  = context.pParentMatrix;
    GRenderer::Cxform* pocf = context.pParentCxform;
#ifndef GFC_NO_3D
    GMatrix3D*        pom3d = context.pParentMatrix3D;
#endif
    for (UPInt i = 0, n = TopmostLevelCharacters.GetSize(); i < n; ++i)
    {
        GFxCharacter* pch = TopmostLevelCharacters[i];
        GASSERT(pch);
        GRenderer::Matrix       m = pch->GetParent()->GetWorldMatrix();
        GRenderer::Cxform      cf = pch->GetParent()->GetWorldCxform();
#ifndef GFC_NO_3D
        GMatrix3D             m3d = pch->GetParent()->GetWorldMatrix3D();

        context.pParentMatrix3D = &m3d;
#endif        
        context.pParentMatrix   = &m;
        context.pParentCxform   = &cf;
        pch->Display(context);
    }
    context.pParentMatrix   = pom;
    context.pParentCxform   = pocf;
#ifndef GFC_NO_3D
    context.pParentMatrix3D = pom3d;
#endif
}

// Sets style of candidate list. Invokes OnCandidateListStyleChanged callback.
void GFxMovieRoot::SetIMECandidateListStyle(const GFxIMECandidateListStyle& st)
{
#ifndef GFC_NO_IME_SUPPORT
    if (!pIMECandidateListStyle)
        pIMECandidateListStyle = GHEAP_NEW(pHeap) GFxIMECandidateListStyle(st);
    else
        *pIMECandidateListStyle = st;
#else
    GUNUSED(st);
#endif //#ifndef GFC_NO_IME_SUPPORT
}

// Gets style of candidate list
void GFxMovieRoot::GetIMECandidateListStyle(GFxIMECandidateListStyle* pst) const
{
#ifndef GFC_NO_IME_SUPPORT
    if (!pIMECandidateListStyle)
        *pst = GFxIMECandidateListStyle();
    else
        *pst = *pIMECandidateListStyle;
#else
    GUNUSED(pst);
#endif //#ifndef GFC_NO_IME_SUPPORT
}

bool GFxMovieRoot::GetDirtyFlag(bool doReset)
{
    bool dirtyFlag = G_IsFlagSet<Flag_DirtyFlag>(Flags);
    if (doReset)
        G_SetFlag<Flag_DirtyFlag>(Flags, false);
    return dirtyFlag;
}

GFxTextAllocator* GFxMovieRoot::GetTextAllocator()
{
    if (!MemContext->TextAllocator)
        MemContext->TextAllocator = *GHEAP_NEW(pHeap) GFxTextAllocator(pHeap);
    return MemContext->TextAllocator;
}

UInt64 GFxMovieRoot::GetASTimerMs() const
{
    GFxTestStream* pts = GetTestStream();
    UInt64 timerMs;
    if (pts)
    {
        if (pts->TestStatus == GFxTestStream::Record)
        {
            timerMs = GTimer::GetTicks()/1000 - StartTickMs;
            GLongFormatter f(timerMs);
            f.Convert();
            pts->SetParameter("timer", f.ToCStr());
        }
        else
        {
           GString tstr;
           pts->GetParameter("timer", &tstr);
           timerMs = G_atouq(tstr.ToCStr());
        }

    }
    else
        timerMs = GTimer::GetTicks()/1000 - StartTickMs;
    return timerMs;
}

UInt32 GFxMovieRoot::GetNumHiddenMovies() const
{
    UInt32 iNumHidden = 0;
    for (UPInt i = 0; i < MovieLevels.GetSize(); ++i)
    {
        if (MovieLevels[i].pSprite && !MovieLevels[i].pSprite->GetVisible())
        {
            ++iNumHidden;
        }
    }
    return iNumHidden;
}

UInt32 GFxMovieRoot::GetNumAlpha0Movies() const
{
    UInt32 iNumAlpha0 = 0;
    for (UPInt i = 0; i < MovieLevels.GetSize(); ++i)
    {
        if (MovieLevels[i].pSprite && MovieLevels[i].pSprite->GetVisible() && MovieLevels[i].pSprite->GetCxform().M_[3][0] < 0.01f)
        {
            ++iNumAlpha0;
        }
    }
    return iNumAlpha0;
}

UInt32 GFxMovieRoot::GetNumAnimatingHiddenMovies() const
{
    UInt32 iNumAnimating = 0;
    for (UPInt i = 0; i < MovieLevels.GetSize(); ++i)
    {
        if (MovieLevels[i].pSprite && !MovieLevels[i].pSprite->GetVisible() && MovieLevels[i].pSprite->GetPlayState() == Playing)
        {
            ++iNumAnimating;
        }
    }
    return iNumAnimating;
}

UInt32 GFxMovieRoot::GetNestedDepth() const
{
    UInt32 iMaxDepth = 0;
    for (UPInt i = 0; i < MovieLevels.GetSize(); ++i)
    {
        if (MovieLevels[i].Level > 0 )
        {
            iMaxDepth = G_Max(iMaxDepth, static_cast<UInt32>(MovieLevels[i].Level));
        }
    }
    return iMaxDepth;
}

UInt32 GFxMovieRoot::GetNumNestedObjects() const
{
    return (UInt32)(MovieLevels.GetSize() - 1);
}

GPointF GFxMovieRoot::TranslateToScreen(const GPointF& p, GRenderer::Matrix userMatrix)
{
    GRenderer::Matrix worldMatrix   = GetLevelMovie(0)->GetWorldMatrix();
    GRenderer::Matrix mat           = ViewportMatrix;

    mat.Prepend(userMatrix);
    mat.Prepend(worldMatrix);
    return mat.Transform(PixelsToTwips(p));
}

GRectF  GFxMovieRoot::TranslateToScreen(const GRectF& r, GRenderer::Matrix userMatrix)
{
    GRenderer::Matrix worldMatrix   = GetLevelMovie(0)->GetWorldMatrix();
    GRenderer::Matrix mat           = ViewportMatrix;

    mat.Prepend(userMatrix);
    mat.Prepend(worldMatrix);
    return mat.EncloseTransform(PixelsToTwips(r));
}

// pathToCharacter - path to a character, i.e. "_root.hud.mc";
// pt is in pixels, in coordinate space of the character specified by the pathToCharacter
// returning value is in pixels of screen.
bool GFxMovieRoot::TranslateLocalToScreen(const char* pathToCharacter, 
                                          const GPointF& pt, 
                                          GPointF* presPt, 
                                          GRenderer::Matrix userMatrix)
{
    GASSERT(presPt);
    // context of GFxMovieRoot
    GASEnvironment* penv = GetLevelMovie(0)->GetASEnvironment();
    GASValue res;
    bool found = penv->GetVariable(penv->CreateString(pathToCharacter), &res);
    if (!found)
        return false;// oops.
    GFxASCharacter* pchar = res.ToASCharacter(penv);
    // now we can get matrix from the character and translate the point
    if (!pchar)
        return false; // oops.
    // note, point coords should be in twips here!
    GPointF pointInTwips = PixelsToTwips(pt);

    GRenderer::Matrix worldMatrix   = pchar->GetWorldMatrix();
    GRenderer::Matrix mat           = ViewportMatrix;

    mat.Prepend(userMatrix);
    mat.Prepend(worldMatrix);
    *presPt = mat.Transform(pointInTwips);
    return true;
}

void   GFxMovieRoot::ForceCollectGarbage() 
{ 
#ifndef GFC_NO_GC
    MemContext->ASGC->ForceCollect();
#endif
}

void GFxMovieRoot::AddToPreDisplayList(GFxASCharacter* pch)
{
    GASSERT(!pch->IsInPreDisplayList());
    PreDisplayList.PushBack(pch);
}

void GFxMovieRoot::RemoveFromPreDisplayList(GFxASCharacter* pch)
{
    GASSERT(pch->IsInPreDisplayList());
    for (UPInt i = 0, n = PreDisplayList.GetSize(); i < n; ++i)
    {
        if (PreDisplayList[i] == pch)
        {
            PreDisplayList.RemoveAt(i);
            break;
        }
    }
}

void GFxMovieRoot::DisplayPrePass()
{
    if (PreDisplayList.GetSize() == 0)
        return;

#if !defined(GFC_NO_STAT)
    ScopeFunctionTimer displayTimer(DisplayStats, NativeCodeSwdHandle, Func_GFxMovieRoot_DisplayPrePass, Amp_Profile_Level_Low);
#endif

    // Warn in case the user called Display() before Advance().
    GFC_DEBUG_WARNING(!G_IsFlagSet<Flag_AdvanceCalled>(Flags), "GFxMovieView::DisplayPrePass called before Advance - no rendering will take place");

    bool ampProfiling = false;
#ifdef GFX_AMP_SERVER
    ampProfiling = GFxAmpServer::GetInstance().IsProfiling();
#endif

    GFxDisplayContext context(pStateBag, 
        pLevel0Def->GetWeakLib(),
        &pLevel0Def->GetResourceBinding(),
        GetPixelScale(),
        ViewportMatrix, ampProfiling ? DisplayStats : NULL, true);

    // Need to check RenderConfig before GetRenderer.
    if (!context.GetRenderConfig() || !context.GetRenderer())
    {
        GFC_DEBUG_WARNING(1, "GFxMovie::DisplayPrePass failed, no renderer specified");
        return;
    }

    context.GetRenderer()->BeginDisplay(   
        GColor(0),
        Viewport,
        VisibleFrameRect.Left, VisibleFrameRect.Right,
        VisibleFrameRect.Top, VisibleFrameRect.Bottom);

    GMatrix3D matView, matPersp;
#ifndef GFC_NO_3D
    context.GetRenderer()->MakeViewAndPersp3D(VisibleFrameRect, matView, matPersp, DEFAULT_FLASH_FOV, true /*invert Y */);
    context.GetRenderer()->SetView3D(matView);
    context.GetRenderer()->SetPerspective3D(matPersp);
#endif
    for (UPInt i = 0, n = PreDisplayList.GetSize(); i < n; ++i)
    {
        GFxASCharacter* pch = PreDisplayList[i];
        GASSERT(pch->IsInPreDisplayList());

        GFxASCharacter::CharFilterDesc* pFilters = pch->GetFilters();
        if (pFilters && pch->GetVisible())
        {
            GFxCharacter* pparent = pch->GetParent();
            GMatrix2D parentmatrix;
            GMatrix3D parentmatrix3D;
            if (pparent)
            {
#ifndef GFC_NO_3D
                if (pparent->Is3D(true))
                {
                    context.pParentMatrix = &GMatrix2D::Identity;
                    pparent->GetWorldMatrix3D(&parentmatrix3D);
                    context.pRealParentMatrix3D = &parentmatrix3D;
                    context.pParentMatrix3D = &GMatrix3D::Identity;
                }
                else
#endif
                {
                    pparent->GetWorldMatrix(&parentmatrix);
                    context.pParentMatrix = &parentmatrix;
                }
            }
            else
            {
                context.pParentMatrix = &GRenderer::Matrix::Identity;
#ifndef GFC_NO_3D
                context.pRealParentMatrix3D = 0;
#endif
            }
#ifndef GFC_NO_3D
            context.pViewMatrix3D = pch->GetView3D(true);
            context.pPerspectiveMatrix3D = pch->GetPerspective3D(true);
#endif
            //context.pParentMatrix3D = &GMatrix3D::Identity;

            // no need to set cxform since it is applied during main pass
            context.pParentCxform = &GRenderer::Cxform::Identity;

            pFilters->PrePassResult = 0;
            pch->Display(context);

            pFilters->FrameRect = context.mcRect;
            pFilters->Width = context.mcWidth;
            pFilters->Height = context.mcHeight;
            pFilters->PrePassResult = context.pFilterRTT;
            context.pFilterRTT = 0;
        }
    }

    context.GetRenderer()->EndDisplay();
}

bool GFxMovieRoot::FindExportedResource(GFxMovieDefImpl* localDef, GFxResourceBindData *presBindData, const GString& symbol)
{
    if (localDef->GetExportedResource(presBindData, symbol))
        return true;

    GFxMovieDefImpl* curDef = localDef;

    // not found in local def - look through "import parents" (movies which import the local one directly or indirectly)
    // Thus, if a moviedef was loaded by loadMovie it should look only inside the movie and ITS imports,
    // it shouldn't go to the movie that actually loaded it.
    GFxMovieDefRootNode *pdefNode = RootMovieDefNodes.GetFirst();
    while(!RootMovieDefNodes.IsNull(pdefNode))
    {      
        if (pdefNode->pDefImpl != localDef && pdefNode->pDefImpl->DoesDirectlyImport(curDef))
        {
            if (pdefNode->pDefImpl->GetExportedResource(presBindData, symbol))
                return true;
            curDef = pdefNode->pDefImpl;
        }
        pdefNode = pdefNode->pNext;
    }
    return false;
}

//
// ***** GFxSwfEvent
//
// For embedding event handlers in GFxPlaceObject2

void    GFxSwfEvent::Read(GFxStreamContext* psc, UInt32 flags)
{
    GASSERT(flags != 0);

    Event = GFxEventId(flags);

    UInt32  eventLength = psc->ReadU32();

    // if Event_KeyPress, read keyCode (UI8) and decrement eventLength.
    if (Event.Id & GFxEventId::Event_KeyPress)
    {
        Event.KeyCode = psc->ReadU8();
        --eventLength;
    }

    // Read the actions.
    // We need to allocate ActionBufferMemData from global heap, since it is possible 
    // to have a pointer to ActionBufferData after its MovieDataDef is unloaded (like, AS
    // class was registered in _global from SWF loaded by "loadMovie" and when the movie
    // is unloaded). So, we allocate all actions from global heap for now.
    pActionOpData = *GASActionBufferData::CreateNew();

    // Note, the Read will copy the Actions from StreamContext to newly allocated
    // buffer from global memory heap (see comment above why).
    pActionOpData->Read(psc, eventLength);

    if (pActionOpData->GetLength() != eventLength)
    {
        if (eventLength > pActionOpData->GetLength())
        {
            // This occasionally happens, so we need to adjust
            // the stream so the remaining entries are correctly loaded.
            UInt i, remainder = eventLength - pActionOpData->GetLength();        
            for (i = 0; i<remainder; i++)
                psc->ReadS8();

            // MA: Seems that ActionScript can actually jump outside of action buffer
            // bounds  into the binary content stored within other tags. Handling this
            // would complicate things. Ex: f1.swf 
            // For event tags this also means that data can actually be stored 
            // after ActionId == 0, hence we get this "remainder".
        }
        else
        {
            UPInt pos = psc->CurByteIndex;
            psc->CurByteIndex = pos - (pActionOpData->GetLength() - eventLength);
        }
    }
}


void    GFxSwfEvent::AttachTo(GFxASCharacter* ch) const
{
    // Create a function to execute the actions.
    if (pActionOpData && !pActionOpData->IsNull())
    {                    
        GASEnvironment*         penv  = ch->GetASEnvironment();
        GMemoryHeap*            pheap = penv->GetHeap();
        GPtr<GASActionBuffer>   pbuff = *GHEAP_NEW(pheap) GASActionBuffer(penv->GetSC(), pActionOpData);

        // we need to set a different type for special events, such as Initialize,
        // Construct, Load and Unload. These events behave differently when character
        // it unloading: the regular events' and action buffers' execution is terminated
        // if the event/action owner is going to be unloaded, but this is not true
        // for those events above.
        GASActionBuffer::ExecuteType et;
        switch(Event.Id)
        {
        case GFxEventId::Event_Initialize:
        case GFxEventId::Event_Construct:
        case GFxEventId::Event_Load:
        case GFxEventId::Event_Unload:
            et = GASActionBuffer::Exec_SpecialEvent;
            break;
        default:
            et = GASActionBuffer::Exec_Event;
        }
        GASValue method(GASFunctionRef(*GHEAP_NEW(pheap)
            GASAsFunctionObject(penv, pbuff, 0, pActionOpData->GetLength(),
                                0, et)));
        ch->SetClipEventHandlers(Event, method);
    }
}

//
// ***** GFxPlaceObjectUnpacked
//


// Place/move/whatever our object in the given pMovie.
void GFxPlaceObjectUnpacked::Execute(GFxSprite* m)
{
    GASEnvironment *penv = m->GetASEnvironment();
    GASString       sname(penv->GetBuiltin(GASBuiltin_empty_));
    m->AddDisplayObject(Pos, sname, NULL, 0, GFC_MAX_UINT, GFxDisplayList::Flags_PlaceObject);
}


//
// ***** GFxPlaceObject
//

GFxPlaceObject::GFxPlaceObject()
: HasCxForm(false) {}

GFxPlaceObject::~GFxPlaceObject() {}

UPInt    GFxPlaceObject::ComputeDataSize(GFxStream* pin)
{   
    int tagStart = pin->Tell();
    int tagEnd = pin->GetTagEndPosition();
    int len = tagEnd - tagStart;    

#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE
    if (pin->IsVerboseParse())
    {
        // Original PlaceObject tag; very simple.
        GFxCharPosInfo pos;
        pos.CharacterId = GFxResourceId(pin->ReadU16());
        pos.Depth       = pin->ReadU16();
        pin->ReadMatrix(&pos.Matrix_1);
        pin->LogParse("  CharId = %d\n"
            "  depth = %d\n"
            "  mat = \n",
            pos.CharacterId.GetIdIndex(), pos.Depth);
        pin->LogParseClass(pos.Matrix_1);
        if (pin->Tell() < pin->GetTagEndPosition())
        {
            pin->ReadCxformRgb(&pos.ColorTransform);
            pin->LogParse("  cxform:\n");
            pin->LogParseClass(pos.ColorTransform);
        }

        // reset stream position
        pin->SetPosition(tagStart);
    }
#endif

    return (UPInt)len;
}

GFxCharPosInfoFlags GFxPlaceObject::GetFlags() const
{
    return (UInt8) (GFxCharPosInfoFlags::Flags_HasCharacterId | 
                    GFxCharPosInfoFlags::Flags_HasDepth | 
                    GFxCharPosInfoFlags::Flags_HasMatrix |
                   ((HasCxForm) ? GFxCharPosInfoFlags::Flags_HasCxform : 0x0));
}

UInt16 GFxPlaceObject::GetDepth() const 
{
    GFxStreamContext    sc(pData);
    sc.Skip(2);
    return (sc.ReadU16());
}

// Place/move/whatever our object in the given pMovie.
void GFxPlaceObject::Execute(GFxSprite* m)
{
    GFxPlaceObject::UnpackedData data;
    Unpack(data);
    GASEnvironment *penv = m->GetASEnvironment();
    m->AddDisplayObject(data.Pos, penv->GetBuiltin(GASBuiltin_empty_), NULL, 0, GFC_MAX_UINT, GFxDisplayList::Flags_PlaceObject);
}

// Unpack data
void GFxPlaceObject::Unpack(GFxPlaceObjectBase::UnpackedData& data)
{
    GFxStreamContext    sc(pData);
    data.Name           = NULL;
    data.pEventHandlers = NULL;
    data.PlaceType      = Place_Add;
    data.Pos.SetCharacterIdFlag();
    data.Pos.CharacterId= GFxResourceId(sc.ReadU16());
    data.Pos.SetDepthFlag();
    data.Pos.Depth      = sc.ReadU16();
    data.Pos.SetMatrixFlag();
    sc.ReadMatrix(&data.Pos.Matrix_1);
    if (HasCxForm)
    {
        data.Pos.SetCxFormFlag();
        sc.ReadCxformRgb(&data.Pos.ColorTransform);
    }
}

void GFxPlaceObject::CheckForCxForm(UPInt dataSz)
{
    GFxStreamContext    sc(pData);
    GRenderer::Matrix   mat;
    sc.Skip(4);
    sc.ReadMatrix(&mat);
    HasCxForm = (sc.CurByteIndex < (dataSz-1));
}


//
// ***** GFxPlaceObject2
//

GFxPlaceObject2::~GFxPlaceObject2() 
{
    GFxPlaceObjectBase::EventArrayType* peh = NULL;
    GFxStreamContext    sc(pData);
    UByte   po2Flags = sc.ReadU8();
    if (po2Flags & PO2_HasActions)
    {
        peh = GetEventHandlersPtr(pData);
        if (peh)
        {
            for (UPInt i = 0, n = peh->GetSize(); i < n; i++)
            {
                delete (*peh)[i];
            }
            delete peh;
        }
    }
}

UPInt    GFxPlaceObject2::ComputeDataSize(GFxStream* pin, UInt movieVersion)
{   
    int tagStart = pin->Tell();
    int tagEnd = pin->GetTagEndPosition();
    int len = tagEnd - tagStart;

    GUNUSED(movieVersion);

#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE
    if (pin->IsVerboseParse())
    {
        UByte   po2Flags = pin->ReadU8();
        UInt32  allFlags = 0;
        char*   pname = NULL;

        GFxCharPosInfo pos;
        pos.Depth = pin->ReadU16();
        if (po2Flags & PO2_HasChar)
        {
            pos.SetCharacterIdFlag();
            pos.CharacterId = GFxResourceId(pin->ReadU16());
        }
        if (po2Flags & PO2_HasMatrix)
        {
            pos.SetMatrixFlag();
            pin->ReadMatrix(&pos.Matrix_1);
        }
        if (po2Flags & PO2_HasCxform)
        {
            pos.SetCxFormFlag();
            pin->ReadCxformRgba(&pos.ColorTransform);
        }
        if (po2Flags & PO2_HasRatio)
        {
            pos.SetRatioFlag();
            pos.Ratio = (Float)pin->ReadU16() / (Float)65535;
        }
        if (po2Flags & PO2_HasName)
            pname = pin->ReadString(pin->GetHeap());
        if (po2Flags & PO2_HasClipBracket)
        {
            pos.SetClipDepthFlag();
            pos.ClipDepth = pin->ReadU16(); 
        }
        if (po2Flags & PO2_HasActions)
        {
            UInt16  reserved = pin->ReadU16();
            GUNUSED(reserved);
            // The logical 'or' of all the following handlers.
            // I don't think we care about this...
            bool    u32Flags = (movieVersion >= 6);
            if (u32Flags)
                allFlags = pin->ReadU32();
            else
                allFlags = pin->ReadU16();
            GUNUSED(allFlags);

            // NOTE: The event handlers are not logged.
        }

        // Reset stream position
        pin->SetPosition(tagStart);

        // Separate parse logging to avoid extra tests/logic during execution.
        // Particularly important because VC++ does not compile out var-args.
        bool    hasChar         = (po2Flags & PO2_HasChar) != 0;
        bool    flagMove        = (po2Flags & PO2_FlagMove) != 0;
        pin->LogParse("  depth = %d\n", pos.Depth);
        if (po2Flags & PO2_HasChar)
            pin->LogParse("  char id = %d\n", pos.CharacterId.GetIdIndex());
        if (po2Flags & PO2_HasMatrix)
        {
            pin->LogParse("  mat:\n");
            pin->LogParseClass(pos.Matrix_1);
        }
        if (po2Flags & PO2_HasCxform)
        {
            pin->LogParse("  cxform:\n");
            pin->LogParseClass(pos.ColorTransform);
        }
        if (po2Flags & PO2_HasRatio)
            pin->LogParse("  ratio: %f\n", pos.Ratio);
        if (po2Flags & PO2_HasName)            
            pin->LogParse("  name = %s\n", pname ? pname : "<null>");
        if (po2Flags & PO2_HasClipBracket)            
            pin->LogParse("  ClipDepth = %d\n", pos.ClipDepth);
        if (po2Flags & PO2_HasActions)
            pin->LogParse("  actions: flags = 0x%X\n", (unsigned int)allFlags);
        if (hasChar && flagMove)
            pin->LogParse("    * (replace)\n");
        else if (!hasChar && flagMove)
            pin->LogParse("    * (move)\n");

        if (pname)
            GFREE(pname);

        // reset stream position
        pin->SetPosition(tagStart);
    }
#endif

    return (UPInt)len;
}

GFxCharPosInfoFlags GFxPlaceObject2::GetFlags() const
{
    GFxStreamContext    sc(pData);
    UByte po2Flags = sc.ReadU8();
    return (po2Flags & 0x5F);
}

UInt16 GFxPlaceObject2::GetDepth() const 
{
    GFxStreamContext    sc(pData);
    UByte po2Flags = sc.ReadU8();
    if (po2Flags & PO2_HasActions)
        sc.Skip(sizeof(GFxPlaceObjectBase::EventArrayType*));
    return (sc.ReadU16());
}

GFxPlaceObjectBase::PlaceActionType GFxPlaceObject2::GetPlaceType() const
{
    GFxStreamContext    sc(pData);
    UByte po2Flags = sc.ReadU8();
    bool    hasChar         = (po2Flags & PO2_HasChar) != 0;
    bool    flagMove        = (po2Flags & PO2_FlagMove) != 0;
    if (hasChar && flagMove)
        return GFxPlaceObject::Place_Replace;
    else if (!hasChar && flagMove)
        return GFxPlaceObject::Place_Move;
    return GFxPlaceObject::Place_Add;
}

// Place/move/whatever our object in the given pMovie.
void GFxPlaceObject2::ExecuteBase(GFxSprite* m, UInt8 version)
{
    GFxPlaceObject::UnpackedData data;
    UnpackBase(data, version);
    switch (data.PlaceType)
    {
    case GFxPlaceObject::Place_Add:
        // Original PlaceObject (tag 4) doesn't do replacement
        {
            GASEnvironment *penv = m->GetASEnvironment();
            GASString       sname((data.Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(data.Name));
            m->AddDisplayObject(data.Pos, sname, data.pEventHandlers, 0, GFC_MAX_UINT, GFxDisplayList::Flags_PlaceObject);
        }
        break;

    case GFxPlaceObject::Place_Move:
        m->MoveDisplayObject(data.Pos);
        break;

    case GFxPlaceObject::Place_Replace:
        {
            GASEnvironment *penv = m->GetASEnvironment();
            GASString       sname((data.Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(data.Name));
            m->ReplaceDisplayObject(data.Pos, sname);
        }        
        break;
    }
}

void GFxPlaceObject2::SetEventHandlersPtr(UByte* pdata, EventArrayType* peh)
{
#if defined(GFC_CPU_OTHER) || defined(GFC_CPU_MIPS)
    memcpy(pdata+1, &peh, sizeof(GFxPlaceObjectBase::EventArrayType*));
#else
    GFxPlaceObjectBase::EventArrayType** ptr = ((GFxPlaceObjectBase::EventArrayType**)(pdata+1));
    *ptr = peh;
#endif
}

GFxPlaceObjectBase::EventArrayType* GFxPlaceObject2::GetEventHandlersPtr(UByte* pdata)
{
#if defined(GFC_CPU_OTHER) || defined(GFC_CPU_MIPS)
    GFxPlaceObjectBase::EventArrayType* ptr;
    memcpy(&ptr, pdata+1, sizeof(GFxPlaceObjectBase::EventArrayType*));
    return ptr;
#else
    return *((GFxPlaceObjectBase::EventArrayType**)(pdata+1));
#endif
}

bool GFxPlaceObject2::HasEventHandlers(GFxStream* pin)
{
    UByte po2Flags = pin->ReadU8();
    pin->SetPosition(pin->Tell()-1);
    return ((po2Flags & PO2_HasActions) != 0);
}

void GFxPlaceObject2::RestructureForEventHandlers(UByte* pdata)
{
    // Move flags to byte 0
    pdata[0] = pdata[sizeof(GFxPlaceObjectBase::EventArrayType*)];
    // Zero event handler pointer
#if defined(GFC_CPU_OTHER) || defined(GFC_CPU_MIPS)
    memset(pdata+1, 0, sizeof(GFxPlaceObjectBase::EventArrayType*));
#else
    *((GFxPlaceObjectBase::EventArrayType**)(pdata+1)) = NULL;
#endif
}

GFxPlaceObjectBase::EventArrayType* GFxPlaceObject2::UnpackEventHandlers()
{
    GFxPlaceObjectBase::EventArrayType* peh = NULL;
    GFxStreamContext    sc(pData);
    UByte   po2Flags = sc.ReadU8();
    if (po2Flags & PO2_HasActions)
    {
        peh = GetEventHandlersPtr(pData);
        if (!peh)
        {
            GFxPlaceObject::UnpackedData data;
            Unpack(data); 
            peh = data.pEventHandlers;
        }
    }
    return peh;
}

void GFxPlaceObject2::UnpackBase(GFxPlaceObject::UnpackedData& data, UInt8 version)
{
    GFxStreamContext    sc(pData);
    UByte   po2Flags = sc.ReadU8();
    if (po2Flags & PO2_HasActions)
        sc.Skip(sizeof(GFxPlaceObjectBase::EventArrayType*));
    UInt32  allFlags = 0;
    data.Pos.Depth       = sc.ReadU16();
    if (po2Flags & PO2_HasChar)
    {
        data.Pos.SetCharacterIdFlag();
        data.Pos.CharacterId = GFxResourceId(sc.ReadU16());
    }
    if (po2Flags & PO2_HasMatrix)
    {
        data.Pos.SetMatrixFlag();
        sc.ReadMatrix(&data.Pos.Matrix_1);
    }
    if (po2Flags & PO2_HasCxform)
    {
        data.Pos.SetCxFormFlag();
        sc.ReadCxformRgba(&data.Pos.ColorTransform);
    }
    if (po2Flags & PO2_HasRatio)
    {
        data.Pos.SetRatioFlag();
        data.Pos.Ratio = (Float)sc.ReadU16() / (Float)65535;
    }
    if (po2Flags & PO2_HasName)
    {
        sc.Align();
        data.Name = (const char*)pData + sc.CurByteIndex;
        char c;
        while ( (c=sc.ReadU8()) != 0) {}
    }
    else
        data.Name = NULL;
    if (po2Flags & PO2_HasClipBracket)
    {
        data.Pos.SetClipDepthFlag();
        data.Pos.ClipDepth = sc.ReadU16(); 
    }
    GFxPlaceObjectBase::EventArrayType* peh = NULL;
    if (po2Flags & PO2_HasActions)
    {        
        peh = GetEventHandlersPtr(pData);
        if (!peh)
        {
            UInt16  reserved = sc.ReadU16();
            GUNUSED(reserved);
            // The logical 'or' of all the following handlers.
            // I don't think we care about this...
            bool    u32Flags = (version >= 6);
            if (u32Flags)
                allFlags = sc.ReadU32();
            else
                allFlags = sc.ReadU16();
            GUNUSED(allFlags);
            peh = GHEAP_NEW_ID(GMemory::pGlobalHeap, GFxStatMD_Tags_Mem) GFxPlaceObject::EventArrayType;
            // Read SwfEvents.
            for (;;)
            {
                // Read event.
                sc.Align();
                UInt32  thisFlags = 0;
                if (u32Flags)
                    thisFlags = sc.ReadU32();
                else                    
                    thisFlags = sc.ReadU16();
                if (thisFlags == 0)
                {
                    // Done with events.
                    break;
                }
                GFxSwfEvent*    ev = GHEAP_NEW_ID(GMemory::pGlobalHeap, GFxStatMD_Tags_Mem) GFxSwfEvent;
                ev->Read(&sc, thisFlags);
                peh->PushBack(ev);
            }
            SetEventHandlersPtr(pData, peh);
        }
    }
    data.pEventHandlers = peh;

    bool    hasChar         = (po2Flags & PO2_HasChar) != 0;
    bool    flagMove        = (po2Flags & PO2_FlagMove) != 0;
    data.PlaceType = GFxPlaceObject::Place_Add;
    if (hasChar && flagMove)
    {
        // Remove whatever's at Depth, and put GFxCharacter there.
        data.PlaceType = GFxPlaceObject::Place_Replace;
    }
    else if (!hasChar && flagMove)
    {
        // Moves the object at Depth to the new location.
        data.PlaceType = GFxPlaceObject::Place_Move;
    }
}


//
// ***** GFxPlaceObject3
//

GFxPlaceObject3::~GFxPlaceObject3() 
{
    GFxPlaceObjectBase::EventArrayType* peh = NULL;
    GFxStreamContext    sc(pData);
    UByte   po2Flags = sc.ReadU8();
    if (po2Flags & GFxPlaceObject2::PO2_HasActions)
    {
        peh = GFxPlaceObject2::GetEventHandlersPtr(pData);
        if (peh)
        {
            for (UPInt i = 0, n = peh->GetSize(); i < n; i++)
            {
                delete (*peh)[i];
            }
            delete peh;
        }
    }
}

UPInt    GFxPlaceObject3::ComputeDataSize(GFxStream* pin)
{   
    int tagStart = pin->Tell();
    int tagEnd = pin->GetTagEndPosition();
    int len = tagEnd - tagStart;

#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE
    if (pin->IsVerboseParse())
    {
        UByte   po2Flags = pin->ReadU8();
        UByte   po3Flags = pin->ReadU8();
        UInt32  allFlags = 0;
        char*   pname = NULL;

        GFxCharPosInfo pos;
        pos.Depth = pin->ReadU16();                
        if (po2Flags & GFxPlaceObject2::PO2_HasChar)
        {
            pos.SetCharacterIdFlag();
            pos.CharacterId = GFxResourceId(pin->ReadU16());
        }

        if (po2Flags & GFxPlaceObject2::PO2_HasMatrix)
        {
            pos.SetMatrixFlag();
            pin->ReadMatrix(&pos.Matrix_1);
        }
        if (po2Flags & GFxPlaceObject2::PO2_HasCxform)
        {
            pos.SetCxFormFlag();
            pin->ReadCxformRgba(&pos.ColorTransform);
        }

        if (po2Flags & GFxPlaceObject2::PO2_HasRatio)
        {
            pos.SetRatioFlag();
            pos.Ratio = (Float)pin->ReadU16() / (Float)65535;
        }
        if (po2Flags & GFxPlaceObject2::PO2_HasName)
            pname = pin->ReadString(pin->GetHeap());
        if (po2Flags & GFxPlaceObject2::PO2_HasClipBracket)
        {
            pos.SetClipDepthFlag();
            pos.ClipDepth = pin->ReadU16(); 
        }
        if (po3Flags & PO3_HasFilters)
        {
            pos.SetFiltersFlag();
            GFxFilterDesc filters[GFxFilterDesc::MaxFilters];
            GFx_LoadFilters(pin, filters, GFxFilterDesc::MaxFilters);
        }
        if (po3Flags & PO3_HasBlendMode)
        {
            UByte   blendMode = pin->ReadU8();
            if ((blendMode < 1) || (blendMode>14))
            {
                GFC_DEBUG_WARNING(1, "PlaceObject3::Read - loaded blend mode out of range");
                blendMode = 1;
            }
            // Assign the mode.
            pos.SetBlendModeFlag();
            pos.BlendMode = (UInt8) blendMode;
        }
        if (po3Flags & PO3_BitmapCaching)
        {
            pin->ReadU8();
        }
        if (po2Flags & GFxPlaceObject2::PO2_HasActions)
        {
            UInt16  reserved = pin->ReadU16();
            GUNUSED(reserved);
            allFlags = pin->ReadU32();
            GUNUSED(allFlags);

            // NOTE: The event handlers are not logged.
        }

        // Reset stream
        pin->SetPosition(tagStart);

        // Separate parse logging to avoid extra tests/logic during execution.
        // Particularly important because VC++ does not compile out var-args.
        bool    hasChar         = (po2Flags & GFxPlaceObject2::PO2_HasChar) != 0;
        bool    flagMove        = (po2Flags & GFxPlaceObject2::PO2_FlagMove) != 0;
        pin->LogParse("  depth = %d\n", pos.Depth);
        if (po2Flags & GFxPlaceObject2::PO2_HasChar)
            pin->LogParse("  char id = %d\n", pos.CharacterId.GetIdIndex());
        if (po2Flags & GFxPlaceObject2::PO2_HasMatrix)
        {
            pin->LogParse("  mat:\n");
            pin->LogParseClass(pos.Matrix_1);
        }
        if (po2Flags & GFxPlaceObject2::PO2_HasCxform)
        {
            pin->LogParse("  cxform:\n");
            pin->LogParseClass(pos.ColorTransform);
        }
        if (po2Flags & GFxPlaceObject2::PO2_HasRatio)
            pin->LogParse("  ratio: %f\n", pos.Ratio);
        if (po2Flags & GFxPlaceObject2::PO2_HasName)           
            pin->LogParse("  name = %s\n", pname ? pname : "<null>");
        if (po2Flags & GFxPlaceObject2::PO2_HasClipBracket)            
            pin->LogParse("  ClipDepth = %d\n", pos.ClipDepth);
        if (po3Flags & PO3_HasBlendMode)
            pin->LogParse("  blend mode = %d\n", pos.BlendMode);
        if (po2Flags & GFxPlaceObject2::PO2_HasActions)
            pin->LogParse("  actions: flags = 0x%X\n", (unsigned int)allFlags);
        if (hasChar && flagMove)
            pin->LogParse("    * (replace)\n");
        else if (!hasChar && flagMove)
            pin->LogParse("    * (move)\n");

        if (pname)
            GFREE(pname);

        // reset stream position
        pin->SetPosition(tagStart);
    }
#endif

    return (UPInt)len;
}


GFxCharPosInfoFlags GFxPlaceObject3::GetFlags() const
{
    GFxStreamContext    sc(pData);
    UByte po2Flags = sc.ReadU8();
    if (po2Flags & GFxPlaceObject2::PO2_HasActions)
        sc.Skip(sizeof(GFxPlaceObjectBase::EventArrayType*));
    UByte po3Flags = sc.ReadU8();
    return (UInt8)((po2Flags & 0x5f) | ((po3Flags & PO3_HasFilters) << 5) | ((po3Flags & PO3_HasBlendMode) << 6));
}
UInt16 GFxPlaceObject3::GetDepth() const 
{
    GFxStreamContext    sc(pData);
    UByte po2Flags = sc.ReadU8();
    if (po2Flags & GFxPlaceObject2::PO2_HasActions)
        sc.Skip(sizeof(GFxPlaceObjectBase::EventArrayType*));
    sc.Skip(1);
    return (sc.ReadU16());
}

GFxPlaceObjectBase::PlaceActionType GFxPlaceObject3::GetPlaceType() const
{
    GFxStreamContext    sc(pData);
    UByte po2Flags = sc.ReadU8();
    bool    hasChar         = (po2Flags & GFxPlaceObject2::PO2_HasChar) != 0;
    bool    flagMove        = (po2Flags & GFxPlaceObject2::PO2_FlagMove) != 0;
    if (hasChar && flagMove)
        return GFxPlaceObject::Place_Replace;
    else if (!hasChar && flagMove)
        return GFxPlaceObject::Place_Move;
    return GFxPlaceObject::Place_Add;
}

// Place/move/whatever our object in the given pMovie.
void GFxPlaceObject3::Execute(GFxSprite* m)
{
    GFxPlaceObject::UnpackedData data;
    Unpack(data);
    switch (data.PlaceType)
    {
    case GFxPlaceObject::Place_Add:
        // Original PlaceObject (tag 4) doesn't do replacement
        {
            GASEnvironment *penv = m->GetASEnvironment();
            GASString       sname((data.Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(data.Name));
            m->AddDisplayObject(data.Pos, sname, data.pEventHandlers, 0, GFC_MAX_UINT, GFxDisplayList::Flags_PlaceObject);
        }
        break;

    case GFxPlaceObject::Place_Move:
        m->MoveDisplayObject(data.Pos);
        break;

    case GFxPlaceObject::Place_Replace:
        {
            GASEnvironment *penv = m->GetASEnvironment();
            GASString       sname((data.Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(data.Name));
            m->ReplaceDisplayObject(data.Pos, sname);
        }        
        break;
    }
}

GFxPlaceObjectBase::EventArrayType* GFxPlaceObject3::UnpackEventHandlers()
{
    GFxPlaceObjectBase::EventArrayType* peh = NULL;
    GFxStreamContext    sc(pData);
    UByte   po2Flags = sc.ReadU8();
    if (po2Flags & GFxPlaceObject2::PO2_HasActions)
    {
        peh = GFxPlaceObject2::GetEventHandlersPtr(pData);
        if (!peh)
        {
            GFxPlaceObject::UnpackedData data;
            Unpack(data);
            peh = data.pEventHandlers;
        }
    }
    return peh;
}

void GFxPlaceObject3::Unpack(GFxPlaceObject::UnpackedData& data)
{
    GFxStreamContext    sc(pData);
    UByte   po2Flags = sc.ReadU8();
    if (po2Flags & GFxPlaceObject2::PO2_HasActions)
        sc.Skip(sizeof(GFxPlaceObjectBase::EventArrayType*));
    UByte   po3Flags = sc.ReadU8();
    UInt32  allFlags = 0;
    data.Pos.Depth       = sc.ReadU16();
    if (po2Flags & GFxPlaceObject2::PO2_HasChar)
    {
        data.Pos.SetCharacterIdFlag();
        data.Pos.CharacterId = GFxResourceId(sc.ReadU16());
    }
    if (po2Flags & GFxPlaceObject2::PO2_HasMatrix)
    {
        data.Pos.SetMatrixFlag();
        sc.ReadMatrix(&data.Pos.Matrix_1);
    }
    if (po2Flags & GFxPlaceObject2::PO2_HasCxform)
    {
        data.Pos.SetCxFormFlag();
        sc.ReadCxformRgba(&data.Pos.ColorTransform);
    }
    if (po2Flags & GFxPlaceObject2::PO2_HasRatio)
    {
        data.Pos.SetRatioFlag();
        data.Pos.Ratio = (Float)sc.ReadU16() / (Float)65535;
    }
    if (po2Flags & GFxPlaceObject2::PO2_HasName)
    {
        sc.Align();
        data.Name = (const char*)pData + sc.CurByteIndex;
        char c;
        while ( (c=sc.ReadU8()) != 0) {}
    }
    else
        data.Name = NULL;
    if (po2Flags & GFxPlaceObject2::PO2_HasClipBracket)
    {
        data.Pos.ClipDepth = sc.ReadU16(); 
        data.Pos.SetClipDepthFlag();
    }
    if (po3Flags & PO3_HasFilters)
    {
        data.Pos.SetFiltersFlag();
        // This loads only Blur and DropShadow/Glow filters
        GFxFilterDesc filters[GFxFilterDesc::MaxFilters];
        UInt numFilters = GFx_LoadFilters(&sc, filters, GFxFilterDesc::MaxFilters);
        if (numFilters)
        {
#if 0
            if (!data.Pos.TextFilter)
                data.Pos.TextFilter = *GHEAP_NEW_ID(GMemory::pGlobalHeap, GFxStatMD_Tags_Mem) GFxTextFilter;
            for (UInt i = 0; i < numFilters; ++i)
            {
                data.Pos.TextFilter->LoadFilterDesc(filters[i]);
            }
#else
            data.Pos.Filters.Clear();
            data.Pos.Filters.Append(filters, numFilters);
#endif
        }
    }
    if (po3Flags & PO3_HasBlendMode)
    {
        data.Pos.SetBlendModeFlag();
        UByte   blendMode = sc.ReadU8();
        if ((blendMode < 1) || (blendMode > 14))
        {
            GFC_DEBUG_WARNING1(1, "PlaceObject3::Read - loaded blend mode out of [0, 14] range : %d", blendMode);
            blendMode = 1;
        }
        // Assign the mode.
        data.Pos.BlendMode = (UInt8) blendMode;
    }
    if (po3Flags & PO3_BitmapCaching)
    {
        sc.ReadU8();
    }
    GFxPlaceObjectBase::EventArrayType* peh = NULL;
    if (po2Flags & GFxPlaceObject2::PO2_HasActions)
    {        
        peh = GFxPlaceObject2::GetEventHandlersPtr(pData);
        if (!peh)
        {
            UInt16  reserved = sc.ReadU16();
            GUNUSED(reserved);
            // The logical 'or' of all the following handlers.
            // I don't think we care about this...
            allFlags = sc.ReadU32();
            GUNUSED(allFlags);
            peh = GHEAP_NEW_ID(GMemory::pGlobalHeap, GFxStatMD_Tags_Mem) GFxPlaceObject::EventArrayType;
            // Read SwfEvents.
            for (;;)
            {
                // Read event.
                sc.Align();
                UInt32  thisFlags = 0;
                thisFlags = sc.ReadU32();
                if (thisFlags == 0)
                {
                    // Done with events.
                    break;
                }
                GFxSwfEvent*    ev = GHEAP_NEW_ID(GMemory::pGlobalHeap, GFxStatMD_Tags_Mem) GFxSwfEvent;
                ev->Read(&sc, thisFlags);
                peh->PushBack(ev);
            }
            GFxPlaceObject2::SetEventHandlersPtr(pData, peh);
        }
    }
    data.pEventHandlers = peh;

    bool    hasChar         = (po2Flags & GFxPlaceObject2::PO2_HasChar) != 0;
    bool    flagMove        = (po2Flags & GFxPlaceObject2::PO2_FlagMove) != 0;
    data.PlaceType = GFxPlaceObject::Place_Add;
    if (hasChar && flagMove)
    {
        // Remove whatever's at Depth, and put GFxCharacter there.
        data.PlaceType = GFxPlaceObject::Place_Replace;
    }
    else if (!hasChar && flagMove)
    {
        // Moves the object at Depth to the new location.
        data.PlaceType = GFxPlaceObject::Place_Move;
    }
}
//
// ***** GFxRemoveObject
//

void    GFxRemoveObject::Read(GFxLoadProcess* p)
{
    Id = p->ReadU16();
    Depth = p->ReadU16();
}

void    GFxRemoveObject::Execute(GFxSprite* m)
{
    m->RemoveDisplayObject(Depth, GFxResourceId(Id));
}

//
// ***** GFxRemoveObject2
//

void    GFxRemoveObject2::Read(GFxLoadProcess* p)
{
    Depth = p->ReadU16();
}

void    GFxRemoveObject2::Execute(GFxSprite* m)
{
    m->RemoveDisplayObject(Depth, GFxResourceId());
}

//
// ***** GFxSetBackgroundColor
//

void    GFxSetBackgroundColor::Execute(GFxSprite* m)
{
    //!AB: background should be set only once and only by the _level0 movie
    if (m->IsLevelsMovie() && m->GetLevel() == 0)
    {
        GFxMovieRoot *pmroot = m->GetMovieRoot();
        if (!pmroot->IsBackgroundSetByTag())
        {
            Float         alpha  = pmroot->GetBackgroundAlpha();
            Color.SetAlpha( UByte(gfrnd(alpha * 255.0f)) );
            pmroot->SetBackgroundColorByTag(Color);
        }
    }
}

void    GFxSetBackgroundColor::Read(GFxLoadProcess* p)
{
    p->GetStream()->ReadRgb(&Color);

    p->LogParse("  SetBackgroundColor: (%d %d %d)\n",
        (int)Color.GetRed(), (int)Color.GetGreen(), (int)Color.GetBlue());
}


// ****************************************************************************
// Helper function to load data from a file
//
bool GFx_ReadLoadVariables(GFile* pfile, GString* pdata, SInt* pfileLen)
{
    GASSERT(pfile);
    GASSERT(pdata);
    GASSERT(pfileLen);

    if ((*pfileLen = pfile->GetLength()) == 0)
        return false;

    GString str;

    UByte* td = (UByte*) GALLOC(*pfileLen, GStat_Default_Mem);
    pfile->Read(td, *pfileLen);

    wchar_t*            wcsptr = NULL;
    UByte*              ptextData = td;
    SInt                textLength = *pfileLen;

    // the following converts byte stream to appropriate endianness
    // for UTF16/UCS2 (wide char format)
    UInt16* prefix16 = (UInt16*)td;
    if (prefix16[0] == GByteUtil::BEToSystem((UInt16)0xFFFE)) // little endian
    {
        prefix16++;
        ptextData = (UByte*)prefix16;
        textLength = (*pfileLen / 2) - 1;
        if (sizeof(wchar_t) == 2)
        {
            for (SInt i=0; i < textLength; i++)
                prefix16[i] = (wchar_t)GByteUtil::LEToSystem(prefix16[i]);
        }
        else
        {
            // special case: create an aux buffer to hold the data
            wcsptr = (wchar_t*) GALLOC(textLength * sizeof(wchar_t), GStat_Default_Mem);
            for (SInt i=0; i < textLength; i++)
                wcsptr[i] = (wchar_t)GByteUtil::LEToSystem(prefix16[i]);
            ptextData = (UByte*)wcsptr;
        }
        str.AppendString( (const wchar_t*)ptextData, textLength );
    }
    else if (prefix16[0] == GByteUtil::BEToSystem((UInt16)0xFEFF)) // big endian
    {
        prefix16++;
        ptextData = (UByte*)prefix16;
        textLength = (*pfileLen / 2) - 1;
        if (sizeof(wchar_t) == 2)
        {
            for (SInt i=0; i < textLength; i++)
                prefix16[i] = GByteUtil::BEToSystem(prefix16[i]);
        }
        else
        {
            wcsptr = (wchar_t*) GALLOC(textLength * sizeof(wchar_t), GStat_Default_Mem);
            for (SInt i=0; i < textLength; i++)
                wcsptr[i] = GByteUtil::BEToSystem(prefix16[i]);
            ptextData = (UByte*)wcsptr;
        }
        str.AppendString( (const wchar_t*)ptextData, textLength );
    }
    else if (*pfileLen > 2 && td[0] == 0xEF && td[1] == 0xBB && td[2] == 0xBF)
    {
        // UTF-8 with explicit BOM
        ptextData += 3;
        textLength -= 3;
        str.AppendString( (const char*)ptextData, textLength );
    }
    else
    {
        str.AppendString( (const char*)ptextData, textLength );
    }

    if (wcsptr)
        GFREE(wcsptr);

    // following works directly on bytes
    GASGlobalContext::Unescape(str.ToCStr(), str.GetSize(), pdata);

    GFREE(td);
    return true;
}

///////////////////////////
// *** GFxMouseState
//
GFxMouseState::GFxMouseState()
{
    ResetState();
}

void GFxMouseState::ResetState()
{
    CursorType             = GFxMouseCursorEvent::ARROW;
    CurButtonsState        = PrevButtonsState = 0;
    LastPosition.x         = LastPosition.y   = 0;
    MouseInsideEntityLast  = false;
    TopmostEntityIsNull    = PrevTopmostEntityWasNull = false;
    Activated = MouseMoved = false;
}

void GFxMouseState::UpdateState(const GFxInputEventsQueue::QueueEntry& qe)
{
    Activated = true;
    PrevButtonsState = CurButtonsState;
    if (qe.GetMouseEntry().IsButtonsStateChanged())
    {
        if (qe.GetMouseEntry().IsAnyButtonReleased())
            CurButtonsState &= (~(qe.GetMouseEntry().ButtonsState & MouseButton_AllMask));
        else
            CurButtonsState |= (qe.GetMouseEntry().ButtonsState & MouseButton_AllMask);
    }

    // update MouseMoved and LastPos
    if (((SInt)qe.GetMouseEntry().GetPosition().x != (SInt)LastPosition.x || 
         (SInt)qe.GetMouseEntry().GetPosition().y != (SInt)LastPosition.y))
        MouseMoved = true;
    else
        MouseMoved = false;
    LastPosition = qe.GetMouseEntry().GetPosition();
}

//////////////////////////////////////////////////////////////////////////
//
// GFxInputEventsQueue
GFxInputEventsQueue::GFxInputEventsQueue():
    StartPos(0), UsedEntries(0), LastMousePosMask(0) 
{
}

const GFxInputEventsQueue::QueueEntry* GFxInputEventsQueue::PeekLastQueueEntry() const 
{ 
    if (UsedEntries == 0) 
        return NULL;
    GASSERT(StartPos < Queue_Length);
    UPInt idx = StartPos + UsedEntries - 1;
    if (idx >= Queue_Length)
        idx -= Queue_Length;
    GASSERT(idx < Queue_Length);
    return &Queue[idx];
}

GFxInputEventsQueue::QueueEntry* GFxInputEventsQueue::AddEmptyQueueEntry()
{
    if (UsedEntries == Queue_Length)
    {
        // queue is full!
        // just skip the first entry to make room for new entry
        ++StartPos;
        --UsedEntries;
        if (StartPos == Queue_Length)
            StartPos = 0;
    }
    GASSERT(UsedEntries < Queue_Length);
    UPInt idx = StartPos + UsedEntries;
    if (idx >= Queue_Length)
        idx -= Queue_Length;
    GASSERT(idx < Queue_Length);
    ++UsedEntries;
    return &Queue[idx];
}

void GFxInputEventsQueue::AddMouseMove(UInt mouseIndex, const GPointF& pos) 
{ 
    if (mouseIndex < GFC_MAX_MICE_SUPPORTED)
    {
        GASSERT(pos.x != GFC_MIN_FLOAT && pos.y != GFC_MIN_FLOAT);

        LastMousePosMask |= (1 << mouseIndex);
        LastMousePos[mouseIndex].x = pos.x;
        LastMousePos[mouseIndex].y = pos.y;
    }
}

void GFxInputEventsQueue::AddMouseButtonEvent(UInt mouseIndex, const GPointF& pos, UInt buttonsSt, UInt flags)
{
    // reset last position, since the record being added contains more recent position.
    if (pos.x != GFC_MIN_FLOAT)
        LastMousePosMask &= ~(1 << mouseIndex);

    QueueEntry* pqe           = AddEmptyQueueEntry();
    pqe->t                    = QueueEntry::QE_Mouse;
    QueueEntry::MouseEntry& mouseEntry = pqe->GetMouseEntry();
    mouseEntry.MouseIndex     = (UInt8)mouseIndex;
    mouseEntry.PosX           = pos.x;
    mouseEntry.PosY           = pos.y;
    mouseEntry.ButtonsState   = (ButtonsStateType)buttonsSt;
    mouseEntry.Flags          = (UInt8)flags;
}

void GFxInputEventsQueue::AddMouseWheel(UInt mouseIndex, const GPointF& pos, SInt delta) 
{ 
    // reset last position, since the record being added contains more recent position.
    if (pos.x != GFC_MIN_FLOAT)
        LastMousePosMask &= ~(1 << mouseIndex);

    QueueEntry* pqe             = AddEmptyQueueEntry();
    pqe->t                      = QueueEntry::QE_Mouse;
    QueueEntry::MouseEntry& mouseEntry = pqe->GetMouseEntry();
    mouseEntry.MouseIndex       = (UInt8)mouseIndex;
    mouseEntry.PosX             = pos.x;
    mouseEntry.PosY             = pos.y;
    mouseEntry.WheelScrollDelta = (SInt8)delta;
    mouseEntry.ButtonsState     = 0;
    mouseEntry.Flags            = MouseButton_Wheel;
}

void GFxInputEventsQueue::AddKeyEvent
    (short code, UByte ascii, UInt32 wcharCode, bool isKeyDown, GFxSpecialKeysState specialKeysState,
     UInt8 keyboardIndex)
{
    QueueEntry* pqe           = AddEmptyQueueEntry();
    pqe->t                    = QueueEntry::QE_Key;
    QueueEntry::KeyEntry& keyEntry = pqe->GetKeyEntry();
    keyEntry.Code             = code;
    keyEntry.AsciiCode        = ascii;
    keyEntry.WcharCode        = wcharCode;
    keyEntry.SpecialKeysState = specialKeysState.States;
    keyEntry.KeyboardIndex    = keyboardIndex;
    keyEntry.KeyIsDown        = isKeyDown;
}

const GFxInputEventsQueue::QueueEntry* GFxInputEventsQueue::GetEntry() 
{ 
    if (UsedEntries == 0) 
    {
        // a special case of simmulating mouse move events after queue is over.
        for (UInt i = 0, mask = 1; i < GFC_MAX_MICE_SUPPORTED; ++i, mask <<= 1)
        {
            if (LastMousePosMask & mask)
            {
                GASSERT(LastMousePos[i].x != GFC_MIN_FLOAT && LastMousePos[i].y != GFC_MIN_FLOAT);
                QueueEntry* pqe           = AddEmptyQueueEntry();
                pqe->t                    = QueueEntry::QE_Mouse;
                QueueEntry::MouseEntry& mouseEntry = pqe->GetMouseEntry();
                mouseEntry.MouseIndex     = (UInt8)i;
                mouseEntry.PosX           = LastMousePos[i].x;
                mouseEntry.PosY           = LastMousePos[i].y;
                mouseEntry.ButtonsState   = 0;
                mouseEntry.Flags          = MouseButton_Move;
                LastMousePosMask               &=  ~mask;
            }
        }
        if (UsedEntries == 0)
            return NULL;
    }
    GASSERT(StartPos < Queue_Length);
    UPInt idx = StartPos;

    ++StartPos;
    --UsedEntries;
    if (StartPos == Queue_Length)
        StartPos = 0;
    return &Queue[idx];
}

GFxSprite* GFxMovieRoot::FocusGroupDescr::GetModalClip(GFxMovieRoot* proot)
{
    if (ModalClip)
    {
        GPtr<GFxASCharacter> modalChar = ModalClip->ResolveCharacter(proot);
        if (modalChar)
        {
            GASSERT(modalChar->GetObjectType() == GASObject::Object_Sprite);
            return static_cast<GFxSprite*>(modalChar.GetPtr());
        }
    }
    return NULL;
}


/**********************************************************************

Filename    :   GFxIMEManager.cpp
Content     :   IME Manager base functinality
Created     :   Dec 17, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include <GFxIMEManager.h>

#ifndef GFC_NO_IME_SUPPORT
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "GFxText.h"
#include "GFxSprite.h"
#include "IME/GASIme.h"

// An implementation for GFxIMEManager class.

class GFxIMEManagerImpl : public GNewOverrideBase<GFxStatIME_Mem>
{
public:
    GFxMovieRoot*       pMovie;
    GPtr<GASTextField>  pTextField;
    SPInt               CursorPosition;
    GFxString           CandidateSwfPath;
    GFxString           CandidateSwfErrorMsg;
    bool                IMEDisabled;

    GFxIMEManagerImpl() : pMovie(NULL), pTextField(NULL) 
    { 
        IMEDisabled = false; 
        CandidateSwfPath = "IME.swf";
    }

    void SetActiveMovie(GFxMovieView* pmovie)
    {
        pMovie = static_cast<GFxMovieRoot*>(pmovie);
    }

    static void OnBroadcastSetConversionMode(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnSetConversionMode(fn.Env, fn.Arg(0).ToString(fn.Env));
    }

    static void OnBroadcastIMEConversion(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnIMEComposition(fn.Env, fn.Arg(0).ToString(fn.Env));
    }

    static void OnBroadcastSwitchLanguage(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnSwitchLanguage(fn.Env, fn.Arg(0).ToString(fn.Env));

    }

    static void OnBroadcastSetSupportedLanguages(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnSetSupportedLanguages(fn.Env, fn.Arg(0).ToString(fn.Env));

    }

    static void OnBroadcastSetCurrentInputLanguage(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnSetCurrentInputLang(fn.Env, fn.Arg(0).ToString(fn.Env));

    }

    static void OnBroadcastSetSupportedIMEs(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnSetSupportedIMEs(fn.Env, fn.Arg(0).ToString(fn.Env));
    }

    static void OnBroadcastSetIMEName(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnSetIMEName(fn.Env, fn.Arg(0).ToString(fn.Env));
    }

    static void OnBroadcastRemoveStatusWindow(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnRemoveStatusWindow(fn.Env);
    }

    static void OnBroadcastDisplayStatusWindow(const GASFnCall& fn)
    {
        if (!fn.Env)
            return;

        GASIme::BroadcastOnDisplayStatusWindow(fn.Env);
    }

    void BroadcastIMEConversion(const char* pString)
    {
        // We have to set up this queing since advance on the actionscript queue is called from a different
        // thread. 
        
        GASValueArray params;

        if (pMovie)
        {
            GASString compString = pMovie->pLevel0Movie->GetASEnvironment()->GetGC()->CreateString(pString);
            params.PushBack(GASValue(compString));
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastIMEConversion, &params);
        }
    }

    void BroadcastSwitchLanguage(const char* pString)
    {
        // We have to set up this queing since advance on the actionscript queue is called from a different
        // thread. 

        GASValueArray params;
        if (pMovie)
        {
            GASString compString = pMovie->pLevel0Movie->GetASEnvironment()->GetGC()->CreateString(pString);
            params.PushBack(GASValue(compString));
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastSwitchLanguage, &params);
        }
    }
    
    void BroadcastSetSupportedLanguages(const char* pString)
    {
        GASValueArray params;

        if (pMovie)
        {
            GASString supportedLanguages = pMovie->pLevel0Movie->GetASEnvironment()->GetGC()->CreateString(pString);
            params.PushBack(GASValue(supportedLanguages));
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastSetSupportedLanguages, &params);
        }
    }

    void BroadcastSetSupportedIMEs(const char* pString)
    {
        GASValueArray params;

        if (pMovie)
        {
            GASString supportedIMEs = pMovie->pLevel0Movie->GetASEnvironment()->GetGC()->CreateString(pString);
            params.PushBack(GASValue(supportedIMEs));
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastSetSupportedIMEs, &params);
        }
    }

    void BroadcastSetCurrentInputLanguage(const char* pString)
    {
        GASValueArray params;
        if (pMovie)
        {
            GASString currentInputLanguage = pMovie->pLevel0Movie->GetASEnvironment()->GetGC()->CreateString(pString);
            params.PushBack(GASValue(currentInputLanguage));
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastSetCurrentInputLanguage, &params);
        }   
    }

    void BroadcastSetIMEName(const char* pString)
    {
        GASValueArray params;
        if (pMovie)
        {
            GASString imeName = pMovie->pLevel0Movie->GetASEnvironment()->GetGC()->CreateString(pString);
            params.PushBack(GASValue(imeName));
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastSetIMEName, &params);
        }   
    }

    void BroadcastSetConversionStatus(const char* pString)
    {
        GASValueArray params;
        if (pMovie)
        {
            GASString convStatus = pMovie->pLevel0Movie->GetASEnvironment()->GetGC()->CreateString(pString);
            params.PushBack(GASValue(convStatus));
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastSetConversionMode, &params);
        }   
    }

    void BroadcastRemoveStatusWindow()
    {
        if (pMovie)
        {
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastRemoveStatusWindow, 0);
        }
    }

    void BroadcastDisplayStatusWindow()
    {
        if (pMovie)
        {
            pMovie->InsertEmptyAction()->SetAction(pMovie->pLevel0Movie, 
                GFxIMEManagerImpl::OnBroadcastDisplayStatusWindow, 0);
        }
    }

    void Reset() { pTextField = NULL; pMovie = NULL; }
};

GFxIMEManager::GFxIMEManager() : GFxState(State_IMEManager) 
{ 
    pImpl = new GFxIMEManagerImpl;
    UnsupportedIMEWindowsFlag = GFxIME_ShowUnsupportedIMEWindows;
}

GFxIMEManager::~GFxIMEManager()
{
    delete pImpl;
}

void GFxIMEManager::StartComposition()
{
    // we need to save the current position in text field
    if (pImpl->pMovie)
    {
        GPtr<GFxASCharacter> pfocusedCh = pImpl->pMovie->GetFocusedCharacter(0);
        if (pfocusedCh && pfocusedCh->GetObjectType() == GASObjectInterface::Object_TextField)
        {
            GPtr<GASTextField> ptextFld = static_cast<GASTextField*>(pfocusedCh.GetPtr());
            if (ptextFld->IsIMEEnabled())
            {
                pImpl->pTextField = ptextFld;

                // first of all, if we have an active selection - remove it
                pImpl->pTextField->ReplaceText(L"", pImpl->pTextField->GetBeginIndex(), 
                                               pImpl->pTextField->GetEndIndex());

                pImpl->CursorPosition = pImpl->pTextField->GetBeginIndex();
                pImpl->pTextField->SetSelection(pImpl->CursorPosition, pImpl->CursorPosition);
                pImpl->pTextField->CreateCompositionString();
            }
        }
    }
}

void GFxIMEManager::FinalizeComposition(const wchar_t* pstr, UPInt len)
{
    if (pImpl->pTextField)
    {
        pImpl->pTextField->CommitCompositionString(pstr, len);
        //pImpl->pTextField->SetCompositionStringHighlighting(true);
    }
    else
    {
        // a special case, used for Chinese New ChangJie and also (discovered 1/26/10) for new phonetic (also
		// known as New Zhu Yin or something similar sounding- also note that this bug for new phonetic
		// only appears after installing an chinese trad version of office 2003 on a chinese trad xp machine), 
		// if typing in English:
        // in this case, no start composition event is sent and whole
        // text is being sent through ime_finalize message. If we don't
        // have pTextField at this time we need to find currently focused
        // one and use it.
        if (pImpl->pMovie)
        {
            GPtr<GFxASCharacter> pfocusedCh = pImpl->pMovie->GetFocusedCharacter(0);
            if (pfocusedCh && pfocusedCh->GetObjectType() == GASObjectInterface::Object_TextField)
            {
                GASTextField* ptextFld = static_cast<GASTextField*>(pfocusedCh.GetPtr());
             //   ptextFld->ReplaceText(pstr, ptextFld->GetBeginIndex(), 
             //                         ptextFld->GetEndIndex(), len);
				
				if (ptextFld)
				{
					ptextFld->CreateCompositionString();
					ptextFld->SetCompositionStringText(pstr, G_wcslen(pstr));
					ptextFld->CommitCompositionString(pstr);
				}
            }
        }
    }
}

void GFxIMEManager::ClearComposition()
{
    if (pImpl->pTextField)
    {
        pImpl->pTextField->ClearCompositionString();
        //pImpl->pTextField->SetCompositionStringHighlighting(true);
    }
}

void GFxIMEManager::ReleaseComposition()
{
    if (pImpl->pTextField)
    {
        pImpl->pTextField->ReleaseCompositionString();
        //pImpl->pTextField->SetCompositionStringHighlighting(true);
    }
}

void GFxIMEManager::SetCompositionText(const wchar_t* pstr, UPInt len)
{
    if (pImpl->pTextField)
    {
        pImpl->pTextField->SetCompositionStringText(pstr, len);
    }
}

void GFxIMEManager::SetCompositionPosition()
{
    if (pImpl->pTextField)
    {
        UPInt pos = pImpl->pTextField->GetCaretIndex();
        pImpl->pTextField->SetCompositionStringPosition(pos);
    }
}

void GFxIMEManager::SetCursorInComposition(UPInt pos)
{
    if (pImpl->pTextField)
    {
        pImpl->pTextField->SetCursorInCompositionString(pos);
    }
}

void GFxIMEManager::SetWideCursor(bool ow)
{
    if (pImpl->pTextField)
    {
        pImpl->pTextField->SetWideCursor(ow);
        //pImpl->pTextField->SetCompositionStringHighlighting(!ow);
    }
}

void GFxIMEManager::OnOpenCandidateList()
{
    GASSERT(pImpl->pMovie);
    if (!pImpl->pMovie)
        return;


//  On Win Vista, it's possible for the candidate list to be loaded up before STARTCOMPOSITION
//  message arrives (which sets the textfield pointer), hence if the textfield pointer is found to be
//  null, we should try to get it from the currently focussed object. This happens with Chinese Traditional
//  IME DaYi 6.0 and Array
//  GASSERT(pImpl->pTextField.GetPtr());

    GASTextField* ptextFld = pImpl->pTextField;
    if (!ptextFld)
    {
        GPtr<GFxASCharacter> pfocusedCh = pImpl->pMovie->GetFocusedCharacter(0);
            if (pfocusedCh && pfocusedCh->GetObjectType() == GASObjectInterface::Object_TextField)
            {
                ptextFld = static_cast<GASTextField*>(pfocusedCh.GetPtr());
            }
        if(!ptextFld)
            return;
    }

    GFxFontResource* pfont = ptextFld->GetFontResource();
    if (!pfont)
        return;
    GFxValue v;
    if (!pImpl->pMovie->GetVariable(&v, "_global.gfx_ime_candidate_list_state"))
        v.SetNumber(0);
    GFxSprite* imeMovie = pImpl->pMovie->GetLevelMovie(GFX_CANDIDATELIST_LEVEL);
    if (!imeMovie || v.GetNumber() != 2)
        return;
    imeMovie->SetIMECandidateListFont(pfont);
}

void GFxIMEManager::HighlightText(UPInt pos, UPInt len, TextHighlightStyle style, bool clause)
{
    GUNUSED2(style, clause);
    if (pImpl->pTextField)
    {
        /*if (clause)
        {
            // clause flag is set in order to highlight whole composition string
            // by solid single underline (used for Japanese).
            pImpl->pTextField->HighlightCompositionString(GFxTextIMEStyle::SC_ConvertedSegment);
        }
        else
            pImpl->pTextField->HighlightCompositionString(GFxTextIMEStyle::SC_CompositionSegment);
            */
        GFxTextIMEStyle::Category cat;
        switch (style)
        {
        case GFxIMEManager::THS_CompositionSegment: cat = GFxTextIMEStyle::SC_CompositionSegment; break;
        case GFxIMEManager::THS_ClauseSegment:      cat = GFxTextIMEStyle::SC_ClauseSegment; break;
        case GFxIMEManager::THS_ConvertedSegment:   cat = GFxTextIMEStyle::SC_ConvertedSegment; break;
        case GFxIMEManager::THS_PhraseLengthAdj:    cat = GFxTextIMEStyle::SC_PhraseLengthAdj; break;
        case GFxIMEManager::THS_LowConfSegment:     cat = GFxTextIMEStyle::SC_LowConfSegment; break;
        default: GASSERT(0); cat = GFxTextIMEStyle::SC_CompositionSegment; // avoid warning
        }
        pImpl->pTextField->HighlightCompositionStringText(pos, len, cat);
    }
}

void GFxIMEManager::GetMetrics(GRectF* pviewRect, GRectF* pcursorRect, int cursorOffset)
{
    GUNUSED(pcursorRect);
    if (pImpl->pTextField)
    {
        GMatrix2D wm = pImpl->pTextField->GetWorldMatrix();
        GRectF vr = pImpl->pTextField->GetBounds(wm);
        if (pviewRect)
            *pviewRect = TwipsToPixels(vr);

        UPInt curspos = pImpl->pTextField->GetCompositionStringPosition();
        if (curspos == GFC_MAX_UPINT)
            curspos = pImpl->pTextField->GetCaretIndex();
        else
            curspos += pImpl->pTextField->GetCompositionStringLength();
        curspos += cursorOffset;
        if ((SPInt)curspos < 0)
            curspos = 0;

        GRectF cr = pImpl->pTextField->GetCursorBounds(curspos);
            
        cr = wm.EncloseTransform(cr);
        if (pcursorRect)
            *pcursorRect = TwipsToPixels(cr);
    }
}

bool GFxIMEManager::IsTextFieldFocused() const
{
    if (pImpl->pMovie)
    {
        GPtr<GFxASCharacter> pfocusedCh = pImpl->pMovie->GetFocusedCharacter(0);
        return IsTextFieldFocused(pfocusedCh);
    }
    return false;
}

bool GFxIMEManager::IsTextFieldFocused(GFxASCharacter* ptextfield) const
{
    if (pImpl->pMovie)
    {
        GPtr<GFxASCharacter> pfocusedCh = pImpl->pMovie->GetFocusedCharacter(0);
        return (pfocusedCh && pfocusedCh->GetObjectType() == GASObjectInterface::Object_TextField &&
                static_cast<GASTextField*>(pfocusedCh.GetPtr())->IsIMEEnabled() &&
                pfocusedCh == ptextfield);
    }
    return false;
}

void GFxIMEManager::SetIMEMoviePath(const char* pcandidateSwfPath)
{
    pImpl->CandidateSwfPath = pcandidateSwfPath;
}

bool GFxIMEManager::GetIMEMoviePath(GFxString& candidateSwfPath)
{
    candidateSwfPath = pImpl->CandidateSwfPath;
    return !candidateSwfPath.IsEmpty();
}

bool GFxIMEManager::SetCandidateListStyle(const GFxIMECandidateListStyle& st)
{
    if (pImpl->pMovie)
        pImpl->pMovie->SetIMECandidateListStyle(st);
    if (IsCandidateListLoaded())
    {
        OnCandidateListStyleChanged(st);
        return true;
    }
    return false;
}

bool GFxIMEManager::GetCandidateListStyle(GFxIMECandidateListStyle* pst) const
{
    if (IsCandidateListLoaded())
    {
        OnCandidateListStyleRequest(pst);
        return true;
    }
    else
    {
        if (pImpl->pMovie)
        {
            pImpl->pMovie->GetIMECandidateListStyle(pst);
            return true;
        }
    }
    return false;
}

namespace
{
    class CandidateListLoader : public GASMovieClipLoader
    {
        GFxIMEManager*      pIMEManager;
        GFxIMEManagerImpl*  pImpl;
    public:
        CandidateListLoader(GFxIMEManager* pimeManager, GFxIMEManagerImpl* pimImpl, GASEnvironment* penv) :
          GASMovieClipLoader(penv), pIMEManager(pimeManager), pImpl(pimImpl) {}

        void NotifyOnLoadInit(GASEnvironment* penv, GFxASCharacter* ptarget)
        {
            GUNUSED(penv);   
            if (pImpl->pMovie)
            {
                GFxValue v;
                v.SetNumber(2); // indicates - "loaded"
                pImpl->pMovie->SetVariable("_global.gfx_ime_candidate_list_state", v);
                v.SetString(pImpl->CandidateSwfPath.ToCStr());
                pImpl->pMovie->SetVariable("_global.gfx_ime_candidate_list_path", v);
            }

            if (ptarget)
            {
                ptarget->GetResourceMovieDef()->PinResource(); // so, do pin instead
                GFxString path;
                ptarget->GetAbsolutePath(&path);
                pIMEManager->OnCandidateListLoaded(path.ToCStr());
            }
            else
                pIMEManager->OnCandidateListLoaded(NULL);
        }
        void NotifyOnLoadError(GASEnvironment* penv, GFxASCharacter* ptarget, const char* errorCode, int status)
        {
            GUNUSED3(penv, ptarget, status);
            pImpl->CandidateSwfErrorMsg = "Error in loading candidate list from ";
            pImpl->CandidateSwfErrorMsg += pImpl->CandidateSwfPath;
            if (pImpl->pMovie)
            {
                GFxString level0Path;
                pImpl->pMovie->GetLevel0Path(&level0Path);
                pImpl->CandidateSwfErrorMsg += " at ";
                pImpl->CandidateSwfErrorMsg += level0Path;

                GFxValue v;
                v.SetNumber(-1); // means - "failed to load"
                pImpl->pMovie->SetVariable("_global.gfx_ime_candidate_list_state", v);
            }
            pImpl->CandidateSwfErrorMsg += ": ";
            pImpl->CandidateSwfErrorMsg += errorCode;
        }
    };
}

bool GFxIMEManager::IsCandidateListLoaded() const
{
    // check if we already have loaded candidate list into _level9999
    if (pImpl->pMovie)
    {
        GFxValue v;
        if (!pImpl->pMovie->GetVariable(&v, "_global.gfx_ime_candidate_list_state"))
            v.SetNumber(0);
        return pImpl->pMovie->GetLevelMovie(GFX_CANDIDATELIST_LEVEL) && v.GetNumber() == 2;
    }
    return false;
}

bool GFxIMEManager::AcquireCandidateList()
{
    if (pImpl->pMovie && pImpl->pMovie->GetLevelMovie(0))
    {
        // check if we already have loaded candidate list into _level9999
        // or if we are loading it at the time. We can't store bool inside the 
        // IME Manager since candidate lists are different for different movies,
        // but IME Manager could be the same.
        GFxValue v;
        if (!pImpl->pMovie->GetVariable(&v, "_global.gfx_ime_candidate_list_state"))
            v.SetNumber(0);
        if (v.GetNumber() < 0)
            return false;
        if (!pImpl->pMovie->GetLevelMovie(GFX_CANDIDATELIST_LEVEL) && v.GetNumber() != 1)
        {
            GFxValue v;
            // First check to see if the candidate list movie (ime.swf by default) exists in the context of the
            // current active movie- if not, it'll fail to load, so no point creating a load queue entry.
			if (CheckCandListExists())
			{
				GPtr<GFxFileOpenerBase> pfileOpener = pImpl->pMovie->GetFileOpener();
				GPtr<GFxURLBuilder> purlBuilder = pImpl->pMovie->GetURLBuilder();
	            
				if (pfileOpener)
				{
					GString parentPath;
					pImpl->pMovie->GetLevel0Path(&parentPath);
					GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Regular, pImpl->CandidateSwfPath, parentPath);
					GString path;
					if (purlBuilder)
						purlBuilder->BuildURL(&path, loc);
					else
						GFxURLBuilder::DefaultBuildURL(&path, loc);

					if (pfileOpener->GetFileModifyTime(path) == -1)
						return false;
				}
				else
				{
					return false;
				}
			}
            v.SetNumber(1); // means - "loading"
            pImpl->pMovie->SetVariable("_global.gfx_ime_candidate_list_state", v);
            
            GFxLoadQueueEntry* pentry = new GFxLoadQueueEntry
                (GFX_CANDIDATELIST_LEVEL, pImpl->CandidateSwfPath, GFxLoadQueueEntry::LM_None, false, true);
            GPtr<GASMovieClipLoader> clipLoader = *new CandidateListLoader
                (this, pImpl, pImpl->pMovie->GetLevelMovie(0)->GetASEnvironment());
            pentry->MovieClipLoaderHolder.SetAsObject(clipLoader);
            pImpl->pMovie->AddMovieLoadQueueEntry(pentry);
            return false;
        }
        return true;
    }
    return false;
}

bool GFxIMEManager::GetCandidateListErrorMsg(GFxString* pdest)
{
    if (pdest)
        *pdest = pImpl->CandidateSwfErrorMsg;
    return (pImpl->CandidateSwfErrorMsg.GetLength() > 0);
}

void GFxIMEManager::DoFinalize()
{
//    if (pImpl->pTextField)
    {
        OnFinalize();
        pImpl->pTextField = NULL;
    }
}

void GFxIMEManager::EnableIME(bool enable)
{
    if (pImpl->IMEDisabled == enable)
    {
        pImpl->IMEDisabled = !pImpl->IMEDisabled;
        OnEnableIME(enable);
    }
}

void GFxIMEManager::BroadcastIMEConversion(const wchar_t* pString)
{
    // Convert to UTF8
    GFxString gfxStr(pString);
    pImpl->BroadcastIMEConversion(gfxStr.ToCStr());
}

void GFxIMEManager::BroadcastSwitchLanguage(const char* pString)
{
    pImpl->BroadcastSwitchLanguage(pString);
}

void GFxIMEManager::BroadcastSetSupportedLanguages(const char* pString)
{
    pImpl->BroadcastSetSupportedLanguages(pString);
}

void GFxIMEManager::BroadcastSetSupportedIMEs(const char* pString)
{
    pImpl->BroadcastSetSupportedIMEs(pString);
}

void GFxIMEManager::BroadcastSetCurrentInputLanguage(const char* pString)
{
    pImpl->BroadcastSetCurrentInputLanguage(pString);
}

void GFxIMEManager::BroadcastSetIMEName(const char* pString)
{
    pImpl->BroadcastSetIMEName(pString);
}

void GFxIMEManager::BroadcastSetConversionStatus(const char* pString)
{
    pImpl->BroadcastSetConversionStatus(pString);
}

void GFxIMEManager::BroadcastRemoveStatusWindow()
{
    pImpl->BroadcastRemoveStatusWindow();
}

void GFxIMEManager::BroadcastDisplayStatusWindow()
{
    pImpl->BroadcastDisplayStatusWindow();
}

GFxASCharacter* GFxIMEManager::HandleFocus(GFxMovieView* pmovie, 
                                           GFxASCharacter* poldFocusedItem, 
                                           GFxASCharacter* pnewFocusingItem, 
                                           GFxASCharacter* ptopMostItem)
{
    
    if (IsMovieActive(pmovie))
    {
        // This case pertains to when the focus is transfered to a textfield for the first time before 
        // any ime messages are sent. This provides a safe place to set the candidate list font for the 
        // candidate list sprites.
        if (pnewFocusingItem && pnewFocusingItem->GetObjectType() == GASObjectInterface::Object_TextField)
        {
            GFxFontResource* pfont = ((GASTextField*)pnewFocusingItem)->GetFontResource();
            GFxValue v;
            if (pfont)
            {
                if (!pImpl->pMovie->GetVariable(&v, "_global.gfx_ime_candidate_list_state"))
                    v.SetNumber(0);
                GFxSprite* imeMovie = pImpl->pMovie->GetLevelMovie(GFX_CANDIDATELIST_LEVEL);
                if (imeMovie && v.GetNumber() == 2)
                    imeMovie->SetIMECandidateListFont(pfont);
            }
        }

        if (poldFocusedItem && 
            poldFocusedItem->GetObjectType() == GASObjectInterface::Object_TextField)
        {
            if (!pnewFocusingItem)
            {
                // empty, check for candidate list. Need to use ptopMostItem, since
                // pnewFocusingItem is NULL
                // check, if the newly focused item candidate list or not
                if (ptopMostItem)
                {
                    GFxString path;
                    ptopMostItem->GetAbsolutePath(&path);
                    if (IsCandidateList(path))
                    {
                        // prevent currently focused text field from losing focus.
                        return poldFocusedItem;
                    }

                    GFxASCharacter* currItem = ptopMostItem;
                    GASValue val;
                    while (currItem != NULL)
                    {
                        if (currItem->GetMemberRaw(currItem->GetASEnvironment()->GetSC(), 
                            currItem->GetASEnvironment()->GetSC()->CreateConstString("isLanguageBar"), &val)
                            ||
                            currItem->GetMemberRaw(currItem->GetASEnvironment()->GetSC(), 
                            currItem->GetASEnvironment()->GetSC()->CreateConstString("isStatusWindow"), &val))
                        {
                            return poldFocusedItem;
                        }
                        currItem = currItem->GetParent();
                    };

                    if (IsStatusWindow(path))
                    {
                        // prevent currently focused text field from losing focus.
                        return poldFocusedItem;
                    }

                    if (IsLangBar(path))
                    {
                        // prevent currently focused text field from losing focus.
                        return poldFocusedItem;
                    }
                }

                // finalize, something else was clicked
                 DoFinalize();
            }
            else 
            {
                if (pnewFocusingItem->GetObjectType() == GASObjectInterface::Object_TextField)
                {
                    // clicked on another text field
                }
                // finalize?
                DoFinalize();
            }
        }

     //   DoFinalize();

        EnableIME(pnewFocusingItem && 
            pnewFocusingItem->GetObjectType() == GASObjectInterface::Object_TextField &&
            static_cast<GASTextField*>(pnewFocusingItem)->IsIMEEnabled());

    }
    return pnewFocusingItem;
}

void GFxIMEManager::SetActiveMovie(GFxMovieView* pmovie)
{
    if (pmovie != pImpl->pMovie)
    {
        if (pImpl->pMovie)
        {
            // finalize (or, cancel?)
            DoFinalize();
        }
        pImpl->Reset();
        pImpl->SetActiveMovie(pmovie);
    }
}

void GFxIMEManager::ClearActiveMovie()
{
    if (pImpl->pMovie)
    {
        pImpl->Reset();
        // finalize (or, cancel?)
        OnShutdown();
    }
}

bool GFxIMEManager::IsMovieActive(GFxMovieView* pmovie) const
{
    return pImpl->pMovie == pmovie;
}

GFxMovieView* GFxIMEManager::GetActiveMovie() const
{
    return pImpl->pMovie;   
}

// callback, invoked when mouse button is down. buttonsState is a mask:
//   bit 0 - right button is pressed,
//   bit 1 - left
//   bit 2 - middle
void GFxIMEManager::OnMouseDown(GFxMovieView* pmovie, int buttonsState, GFxASCharacter* pitemUnderMousePtr)
{
    GUNUSED(buttonsState);
    if (IsMovieActive(pmovie) && pImpl->pTextField && pitemUnderMousePtr == pImpl->pTextField)
    {
        // if mouse clicked on the same active text field then finalize.
        // Otherwise let HandleFocus to handle the stuff.
        DoFinalize();
    }
}

void GFxIMEManager::OnMouseUp(GFxMovieView* pmovie, int buttonsState, GFxASCharacter* pitemUnderMousePtr)
{
    GUNUSED3(pmovie, buttonsState, pitemUnderMousePtr);
}

UInt GFxIMEManager::HandleIMEEvent(GFxMovieView* pmovie, const GFxIMEEvent& imeEvent)
{
    GUNUSED2(pmovie, imeEvent);
    return GFxMovieView::HE_NotHandled;
}

void GFxIMEManager::OnCandidateListLoaded(const char* pcandidateListPath) 
{ 
    GUNUSED(pcandidateListPath); 
}

void GFxIMEManager::OnCandidateListStyleRequest(GFxIMECandidateListStyle* pstyle) const
{
    if (pImpl->pMovie)
    {
        pImpl->pMovie->GetIMECandidateListStyle(pstyle);
    }
}
#else //#ifndef GFC_NO_IME_SUPPORT

// just to avoid warning "LNK4221: no public symbols found; archive member will be inaccessible" 
GFxIMEManager::GFxIMEManager() : GFxState(State_IMEManager) 
{ 
}

#endif //#ifndef GFC_NO_IME_SUPPORT

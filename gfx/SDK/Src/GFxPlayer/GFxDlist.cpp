/**********************************************************************

Filename    :   GFxDlist.cpp
Content     :   Implementation of Diplay List
Created     :   
Authors     :   Artem Bolgar, Michael Antonov, TU

Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Notes       :   Significantly modified for GFx 3.0

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxLog.h"
#include "GFxPlayer.h"

#include "GFxDlist.h"
#include "GFxDisplayContext.h"
#include "GFxAction.h"
#include "GFxFilterDesc.h"
#include "AMP/GFxAmpViewStats.h"


// ***** GFxDisplayList - Character entry access.

// Like the above, but looks for an exact match, and returns -1 if failed.
UPInt GFxDisplayList::GetDisplayIndex(int depth)
{
    UPInt index = FindDisplayIndex(depth);
    if (index >= DisplayObjectArray.GetSize()
        || GetDisplayObject(index).GetDepth() != depth)
    {
        // No object at that depth.
        return GFC_MAX_UPINT;
    }
    return index;
}


GFxCharacter*   GFxDisplayList::GetCharacterAtDepth(int depth, bool* pisMarkedForRemove)
{
    UPInt index = GetDisplayIndex(depth);
    if (index != GFC_MAX_UPINT)
    {
        GFxCharacter*   ch = DisplayObjectArray[index].GetCharacter();
        GASSERT(ch);
        if (ch->GetDepth() == depth)
        {
            if (pisMarkedForRemove)
                *pisMarkedForRemove = DisplayObjectArray[index].IsMarkedForRemove();
            return ch;
        }
    }

    return NULL;
}

GFxCharacter*   GFxDisplayList::GetCharacterAtDepthAndUnmark(int depth)
{
    UPInt index = GetDisplayIndex(depth);
    if (index != GFC_MAX_UPINT)
    {
        GFxCharacter*   ch = DisplayObjectArray[index].GetCharacter();
        GASSERT(ch);
        if (ch->GetDepth() == depth)
        {
            DisplayObjectArray[index].MarkForRemove(false);
            return ch;
        }
    }

    return NULL;
}


// Swaps two objects at depths.
bool    GFxDisplayList::SwapDepths(int depth1, int depth2, UInt frame)
{
    if (depth1 == depth2)
        return 1;

    UPInt index1 = GetDisplayIndex(depth1);
    if (index1 == GFC_MAX_UPINT)
        return 0;
    UPInt index2 = FindDisplayIndex(depth2);

    pCachedChar = NULL;
    if (index2 >= DisplayObjectArray.GetSize() ||
        GetDisplayObject(index2).GetDepth() != depth2)
    {
        // Index 2 must be used for insertion.
        // Move the object and set depth.
        DisplayEntry de(DisplayObjectArray[index1]);
        DisplayObjectArray.RemoveAt(index1);
        if (index1 < index2)
            index2--;
        DisplayObjectArray.InsertAt(index2, de);
    }
    else
    {
        // Otherwise, we are swapping depths with both existing entries.
        G_Swap(DisplayObjectArray[index1], DisplayObjectArray[index2]);
        GFxCharacter* ch1 = DisplayObjectArray[index1].GetCharacter();
        if (ch1)
        {
            ch1->SetDepth(depth1);
            // if depth is changed and char is a timeline object then this
            // char will be removed at the timeline's loop
            ch1->SetCreateFrame(frame + 1);
        }
    }

    GFxCharacter* ch2 = DisplayObjectArray[index2].GetCharacter();
    if (ch2)
    {
        ch2->SetDepth(depth2);
        // if depth is changed and char is a timeline object then this
        // char will be removed at the timeline's loop
        ch2->SetCreateFrame(frame + 1);
    }
    return 1;
}

// Returns maximum used depth.
int     GFxDisplayList::GetLargestDepthInUse() const
{
    // Display array is sorted by size, so get last value
    UPInt size = DisplayObjectArray.GetSize();
    return size ? DisplayObjectArray[size-1].GetDepth() : -1;
}


GFxASCharacter* GFxDisplayList::GetCharacterByName(GASStringContext* psc, const GASString& name)
{
    if (name.IsEmpty())
        return NULL;
    GFxCharacter*  pch = NULL;

    // See if we have a match on the display list.
    if (psc->IsCaseSensitive())
    {
        if (pCachedChar && pCachedChar->GetName() == name)
            return pCachedChar;
        
        UPInt          i, n = GetCharacterCount();
        for (i = 0; i < n; i++)
        {
            GFxCharacter* p = GetCharacter(i);
            GASSERT(p);
            if (p && p->IsCharASCharacter() && p->CharToASCharacter_Unsafe()->GetName() == name)
            {
                pch = p;
                break;
            }
        }
    }
    else
    {   // Case-insensitive version.
        name.ResolveLowercase();
        if (pCachedChar && name.Compare_CaseInsensitive_Resolved(pCachedChar->GetName()))
            return pCachedChar;

        UPInt          i, n = GetCharacterCount();
        for (i = 0; i < n; i++)
        {
            GFxCharacter* p = GetCharacter(i);
            GASSERT(p);
            if (p && p->IsCharASCharacter() && name.Compare_CaseInsensitive_Resolved(p->CharToASCharacter_Unsafe()->GetName()))
            {
                pch = p;
                break;
            }
        }
    }    
    // Only GFxASCharacter's have names.
    GASSERT((pch == 0) || pch->IsCharASCharacter());

    pCachedChar = (GFxASCharacter*)pch;
    return pCachedChar;
}



// ***** GFxDisplayList - Display list management


void    GFxDisplayList::AddDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, UInt32 addFlags)    
{
    GASSERT(ch);
    
    int depth   = pos.Depth;
    UPInt size  = DisplayObjectArray.GetSize();
    UPInt index = FindDisplayIndex(depth);
    pCachedChar = NULL;

    if (addFlags & Flags_ReplaceIfDepthIsOccupied)
    {
        // Eliminate an existing object if it's in the way.

        if (index < size)
        {
            DisplayEntry & dobj = DisplayObjectArray[index];

            if (dobj.GetDepth() == depth)
            {               
                // Unload the object at the occupied depth.
                UnloadEntryAtIndex(index);
                // correct size and index since they might be invalid
                // after UnloadEntryAtIndex.
                size    = DisplayObjectArray.GetSize();
                index   = FindDisplayIndex(depth);
            }
        }
    }
    else
    {
        // Basically, the display list should contain the only one character at one particular depth. 
        // But there are several exceptions from the depth’s uniqueness rule and for the short period 
        // of time the display list may contain characters at the same depth:
        //  1.  There could be several characters at the same depth < -32768. The ‘semi-dead’ characters 
        //      (being removed but having the unload handler) are being moved to the depth = (-32769 – curDepth). 
        //      If several characters with onUnload handler were created and removed at the same depth in the same 
        //      frame then we may get several ‘semi-dead’ characters at the same negative depth. These characters 
        //      will be cleaned up once their onUnload handlers are invoked.
        //  2.  It is possible to get characters at the same depth during gotoFrame, when the current character 
        //      is marked for remove and a new character is being added at the same depth. These characters will be 
        //      cleaned up once UnloadMarkedForRemove() function is called right after the gotoFrame is finished.
        //  3.  Another possibility of existence of several characters at the same depth is the gotoFrame backward. 
        //      For example if the character was created at frame 5 and we at the frame 10. Frame 4 contains another 
        //      character at the same depth. Now, we got gotoFrame(5). Since the CreateFrame of the existing character 
        //      is not greater than 5 this object will not be marked for remove. When gotoFrame(5) is executed we 
        //      create character for frame 4 at the same depth as the currently existing frame 5 instance has. 
        //      This extra object will be eliminated once ExecuteFrameTags(5) is executed (right after ExecuteSnapshot
        //      is finished), because the tags for frame 5 will contain RemoveObject for the frame 4 character and 
        //      then AddObject for frame 5. New character will not be re-created here because the existing one will 
        //      be reused. Thus, these two characters will have different CreateFrame. It is also important that 
        //      AddObject adds record for the same depth character IN FRONT OF existing one (so, lower bound algorithm 
        //      should be used). Otherwise, the RemoveObject tag might remove incorrect character (it should also use 
        //      the lower bound algorithm when searching for the character to remove).
        GASSERT(!(size > 0 && (index < size) && 
            DisplayObjectArray[index].GetDepth() == depth && !DisplayObjectArray[index].IsMarkedForRemove() &&
            (DisplayObjectArray[index].GetCharacter() && DisplayObjectArray[index].GetCharacter()->GetCreateFrame() == ch->GetCreateFrame())));
    }

    ch->SetDepth(depth);

    DisplayEntry di;    

    di.SetCharacter(ch);
    ch->SetCxform(pos.ColorTransform);
    ch->SetMatrix(pos.Matrix_1);
    ch->SetRatio(pos.Ratio);
    ch->SetClipDepth(pos.ClipDepth);
    ch->SetBlendMode((GFxCharacter::BlendType)pos.BlendMode);
    ch->SetFilters(pos.Filters);

    // Insert into the display list...
    GASSERT(index == FindDisplayIndex(depth));
    
    DisplayObjectArray.InsertAt(index, di);

    // Do the frame1 Actions (if applicable) and the "OnClipEvent (load)" event.
    ch->OnEventLoad();
}


// Updates the transform properties of the object at the specified depth.
void    GFxDisplayList::MoveDisplayObject(const GFxCharPosInfo &pos)
{
    int     depth   = pos.Depth;
    UPInt   size    = DisplayObjectArray.GetSize();
    UPInt   index   = FindDisplayIndex(depth);

    // Bounds check.
    if (index >= size)
    {       
        if (size == 0)
            GFC_DEBUG_WARNING(1, "MoveDisplayObject() - no objects on display list");
        else
            GFC_DEBUG_WARNING1(1, "MoveDisplayObject() - can't find object at depth %d", depth);      
        return;
    }
    
    DisplayEntry&   di = DisplayObjectArray[index];
    GFxCharacter*   ch = di.GetCharacter();
    GASSERT(ch);
    if (ch->GetDepth() != depth)
    {       
        GFC_DEBUG_WARNING1(1, "MoveDisplayObject() - no object at depth %d", depth);  
        return;
    }

    // Object re-added, unmark for remove.
    di.MarkForRemove(0);

    if (!ch->GetAcceptAnimMoves())
    {
        // This GFxCharacter is rejecting anim moves.  This happens after it
        // has been manipulated by ActionScript.
        if (ch->GetContinueAnimationFlag())
            ch->SetAcceptAnimMoves(true); // reset the flag
        else
        return;
    }

    if (pos.HasCxform())  
        ch->SetCxform(pos.ColorTransform);
    if (pos.HasMatrix())
        ch->SetMatrix(pos.Matrix_1);    
    if (pos.HasBlendMode())
        ch->SetBlendMode((GFxCharacter::BlendType)pos.BlendMode);
    if (pos.HasFilters())
        ch->SetFilters(pos.Filters);

    ch->SetRatio(pos.Ratio);

    // MoveDisplayObject apparently does not change clip depth!  Thanks to Alexeev Vitaly.
    // ch->SetClipDepth(ClipDepth);
}


// Puts a new GFxCharacter at the specified depth, replacing any
// existing GFxCharacter.  If HasCxform or HasMatrix are false,
// then keep those respective properties from the existing character.
void    GFxDisplayList::ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch)   
{   
    UPInt   size    = DisplayObjectArray.GetSize();
    int     depth   = pos.Depth;
    UPInt   index   = FindDisplayIndex(depth);
    
    // Bounds check.
    if (index >= size)
    {
        // Error, no existing object found at depth.
        // Fallback -- add the object.
        AddDisplayObject(pos, ch, Flags_ReplaceIfDepthIsOccupied);
        return;
    }
    
    DisplayEntry&   di = DisplayObjectArray[index];
    if (di.GetDepth() != depth)
    {   
        // This may occur if we are seeking *back* in a movie and find a ReplaceObject tag.
        // In that case, a previous object will not exist, since it would have been removed,
        // if it did.
        AddDisplayObject(pos, ch, Flags_ReplaceIfDepthIsOccupied);
        return;
    }
    
    GPtr<GFxCharacter>  poldCh = di.GetCharacter();
    GASSERT(poldCh);

    // Put the new GFxCharacter in its place.
    GASSERT(ch);
    ch->SetDepth(depth);
    ch->Restart();
    
    // Set the display properties.
    di.MarkForRemove(0);
    di.SetCharacter(ch);

    // Use old characters matrices if new ones not specified.
    ch->SetCxform(pos.HasCxform() ? pos.ColorTransform : poldCh->GetCxform());
    ch->SetMatrix(pos.HasMatrix() ? pos.Matrix_1 : poldCh->GetMatrix());
    ch->SetBlendMode((pos.HasBlendMode()) ? (GFxCharacter::BlendType)pos.BlendMode : poldCh->GetBlendMode());
    ch->SetRatio(pos.Ratio);
    ch->SetClipDepth(pos.ClipDepth);    
    ch->SetFilters(pos.Filters);

    poldCh->OnEventUnload();
    pCachedChar = NULL;
}

void    GFxDisplayList::ReplaceDisplayObjectAtIndex(UPInt index, GFxCharacter* ch)
{
    // 1st validity checks.
    if (index >= DisplayObjectArray.GetSize())
    {
        // Error -- no GFxCharacter at the given depth.
        GFC_DEBUG_WARNING1(1, "ReplaceDisplayObjectAtIndex - invalid index %d", (UInt)index);
        return;
    }

    pCachedChar = NULL;
    GFxDisplayList::DisplayEntry &e = GetDisplayObject(index);
    e.SetCharacter(ch);
}

// Removes the object at the specified depth.
void    GFxDisplayList::RemoveDisplayObject(int depth, GFxResourceId id)
{
    UPInt   size    = DisplayObjectArray.GetSize();    
    UPInt   index   = FindDisplayIndex(depth);
    
    // 1st validity checks.
    if (index >= size)
    {
        // Error -- no GFxCharacter at the given depth.
        GFC_DEBUG_WARNING1(1, "RemoveDisplayObject - no GFxCharacter at depth %d", depth);
        return;
    }

    GPtr<GFxCharacter> ch = GetCharacter(index);
    GASSERT(ch);

    // 2nd validity checks.
    if (ch->GetDepth() != depth)
    {
        // Error -- no GFxCharacter at the given depth.
        GFC_DEBUG_WARNING1(1, "RemoveDisplayObject - no GFxCharacter at depth %d", depth);
        return;
    }

    GASSERT(ch->GetDepth() == depth);
    pCachedChar = NULL;
    
    if (id != GFxResourceId::InvalidId)
    {
        // Caller specifies a specific id; scan forward til we find it.
        for (;;)
        {
            if (GetCharacter(index)->GetId() == id)
            {
                break;
            }
            if (index + 1 >= size || GetCharacter(index + 1)->GetDepth() != depth)
            {
                // Didn't find a match!
                GFC_DEBUG_WARNING2(1, "RemoveDisplayObject - no GFxCharacter at depth %d with id %d", depth, id.GetIdValue());
                return;
            }
            index++;
        }
        GASSERT(index < size);
        GASSERT(GetCharacter(index)->GetDepth() == depth);
        GASSERT(GetCharacter(index)->GetId() == id);
    }

    UnloadEntryAtIndex(index);
}

bool    GFxDisplayList::RemoveCharacter(GFxCharacter* ch)
{
    GASSERT(ch);
    int     depth   = ch->GetDepth();
    UPInt   size    = DisplayObjectArray.GetSize();    
    UPInt   index   = FindDisplayIndex(depth);

    // 1st validity checks.
    if (index >= size)
    {
        return false;
    }
    // we need to find a character to remove. The following loop is to iterate through
    // potential set of different characters at the same depth.
    GFxCharacter* pcurCh = GetCharacter(index);
    while (pcurCh && pcurCh != ch && depth == pcurCh->GetDepth() && index + 1 < size)
    {
        pcurCh = GetCharacter(++index);
    }
    if (index < size && pcurCh == ch)
    {
        // we've found a character to remove
        DisplayObjectArray.RemoveAt(index);
        pCachedChar = NULL;
        return true;
    }
    return false;
}

void    GFxDisplayList::MarkForRemoval(int depth, GFxResourceId id)
{
    UPInt   size    = DisplayObjectArray.GetSize();    
    UPInt   index   = FindDisplayIndex(depth);

    // Validity checks.
    if (index >= size || GetCharacter(index)->GetDepth() != depth)
    {
        // Error -- no GFxCharacter at the given depth.
        GFC_DEBUG_WARNING1(1, "RemoveDisplayObject - no GFxCharacter at depth %d", depth);
        return;
    }

    GASSERT(GetCharacter(index) && GetCharacter(index)->GetDepth() == depth);

    if (id != GFxResourceId::InvalidId)
    {
        // Caller specifies a specific id; scan forward til we find it.
        for (;;)
        {
            if (GetCharacter(index)->GetId() == id)
            {
                break;
            }
            if (index + 1 >= size || GetCharacter(index + 1)->GetDepth() != depth)
            {
                // Didn't find a match!
                GFC_DEBUG_WARNING2(1, "RemoveDisplayObject - no GFxCharacter at depth %d with id %d", depth, id.GetIdValue());
                return;
            }
            index++;
        }
        GASSERT(index < size);
        GASSERT(GetCharacter(index)->GetDepth() == depth);
        GASSERT(GetCharacter(index)->GetId() == id);
    }

    // Removing the GFxCharacter at GetDisplayObject(index).
    DisplayEntry&   di = DisplayObjectArray[index];
    di.MarkForRemove(true);
}

// Clear the display list.
void    GFxDisplayList::Clear() 
{
    UPInt i, n = DisplayObjectArray.GetSize();
    for (i = 0; i < n; i++)
    {
        DisplayEntry&   di = DisplayObjectArray[i];     
        GASSERT(di.GetCharacter());
        di.GetCharacter()->OnEventUnload();
        di.GetCharacter()->SetParent(NULL); //!AB: clear the parent for children in the display list 
                                        // being cleared, since their parent used to be removed, 
                                        // but children might stay alive for sometime.
    }   
    pCachedChar = NULL;
    DisplayObjectArray.Clear();
}

bool    GFxDisplayList::UnloadAll() 
{
    pCachedChar = NULL;
    bool mayRemove = true;
    // don't replace DisplayObjectArray.GetSize() by pre-saved variable here! 
    for (UPInt i = 0; i < DisplayObjectArray.GetSize(); )
    {
        // we need to correct index only if UnloadEntryAtIndex returns true;
        // this means, that the element at the index 'i' was removed and
        // we don't need to increment the index to get the next element.
        // If it returns false, then the element at index 'i' was moved
        // to the beginning of the array (since depth will be negative) and thus
        // we still need to increment the index.
        bool mayRemoveTheChild = UnloadEntryAtIndex(i);
        if (!mayRemoveTheChild)
        {
            ++i;
            mayRemove = false;
        }
    }   
    GASSERT(!pCachedChar);
    return mayRemove;
}


// Mark all entries for removal.
void    GFxDisplayList::MarkAllEntriesForRemoval(UInt targetFrame)  
{   
    UPInt i, n = DisplayObjectArray.GetSize();
    for (i = 0; i < n; i++) 
    {
        GPtr<GFxCharacter> ch = DisplayObjectArray[i].GetCharacter();

        GASSERT(ch);

        // do not mark characters created by ActionScript. But we still need to mark
        // characters, which were touched by AS, if they were created on frame other
        // than 0.
        int depth = ch->GetDepth();
        if (!ch || (depth >= 0 && depth < 16384 && ch->GetCreateFrame() > targetFrame))
        {
            //@DBG
            //if (ch && ch->IsASCharacter())
            //    printf(" dlist.markForRemoval: %s, depth = %d, createFrame = %d\n",
            //        ch->ToASCharacter()->GetCharacterHandle()->GetNamePath().ToCStr(),
            //        ch->GetDepth(), ch->GetCreateFrame());
            //else if (ch)
            //    printf(" dlist.markForRemoval: depth = %d, createFrame = %d\n", 
            //        ch->GetDepth(), ch->GetCreateFrame());
            //else 
            //    printf(" dlist.markForRemoval: <empty>\n");
            DisplayObjectArray[i].MarkForRemove(true);
        }
    }
}

// Removes all objects that are marked for removal.
void    GFxDisplayList::UnloadMarkedObjects()   
{   
    // Must be in first-to-last order.
    // don't replace DisplayObjectArray.GetSize() by pre-saved variable here! 
    for (UPInt i = 0; i < DisplayObjectArray.GetSize(); i++)
    {
        if (DisplayObjectArray[i].IsMarkedForRemove())
        {
            // remove "marked-for-removal" flag
            DisplayObjectArray[i].ClearMarkForRemove();
            
            // we need to correct index only if UnloadEntryAtIndex returns true;
            // this means, that the element at the index 'i' was removed and
            // we don't need to increment the index to get the next element.
            // If it returns false, then the element at index 'i' was moved
            // to the beginning of the array (since depth will be negative) and thus
            // we still need to increment the index.
            if (UnloadEntryAtIndex(i))
                i--;
        }
    }
    pCachedChar = NULL;
}

bool    GFxDisplayList::UnloadEntryAtIndex(UPInt index)
{
    const DisplayEntry & dobj = DisplayObjectArray[index];
    
    // If entry is already marked for remove, do nothing at the point,
    // let the UnloadMarkedObjects do its job later.
    if (dobj.IsMarkedForRemove())
        return false;

    GFxCharacter* pch = dobj.GetCharacter();
    GASSERT(pch);

    if (pch)    
    {
        // OnUnloading handler determines the possibility of instant unloading.
        // It also queues up the onUnload AS event, if it is defined.
        bool mayRemove = pch->OnUnloading();
        pch->SetUnloading(true);
        if (mayRemove)
        {
            // We can remove the object instantly, so, do it now.
            pch->OnEventUnload();
            DisplayObjectArray.RemoveAt(index);
        }
        else
        {
            //GASSERT(dobj.GetDepth() >= 0);
            // if the depth is already negative this means the clip is already being removed.
            // Such situation is possible when a child clip has onUnload handler and when
            // removeMovieClip is called for the child clip first, and then removeMovieClip for
            // the parent.
            if (dobj.GetDepth() >= 0)
            {
                // ok, we can't remove the object right now, because of unload handler.
                // In this case we need to move the object to the depth = -1 - curDepth.
                // Note, GFx shifts all depths by +16384, so the real new depth of the
                // character will be -32769 - curDepth.
                int newDepth = -1 - dobj.GetDepth();

                // Index 2 must be used for insertion.
                // Move the object and set depth.
                DisplayEntry de(dobj);
                DisplayObjectArray.RemoveAt(index);
                de.GetCharacter()->SetDepth(newDepth);
                UPInt index2 = FindDisplayIndex(newDepth);
                DisplayObjectArray.InsertAt(index2, de);
            }
        }
        pCachedChar = NULL; 
        return mayRemove;
    }
    else
    {
        DisplayObjectArray.RemoveAt(index);
        pCachedChar = NULL; 
    }
    GASSERT(!pCachedChar);
    return true;
}

// *** Frame processing.

#ifdef GFC_USE_OLD_ADVANCE
// Advance referenced characters.
void    GFxDisplayList::AdvanceFrame(bool nextFrame, Float framePos)
{
    // Objects further on display list get processed first in Flash.
    UPInt    n = DisplayObjectArray.GetSize();      
    
    for (SPInt i = (SPInt)(n - 1); i >= 0; i--)       
    {
        // @@@@ TODO FIX: If GArray changes size due to
        // GFxCharacter actions, the iteration may not be
        // correct!
        //
        // What's the correct thing to do here?  Options:
        //
        // * copy the display list at the beginning,
        // iterate through the copy
        //
        // * Use (or emulate) a linked list instead of
        // an GArray (still has problems; e.G. what
        // happens if the next or current object gets
        // removed from the dlist?)
        //
        // * iterate through current GArray in depth
        // order.  Always find the next object using a
        // search of current GArray (but optimize the
        // common case where nothing has changed).
        //
        // * ???
        //
        // Need to test to see what Flash does.
        if (n != DisplayObjectArray.GetSize())
        {
            GFC_DEBUG_WARNING(1, "GFxPlayer bug - dlist size changed due to GFxCharacter actions, bailing on update!");
            break;
        }

        DisplayEntry & dobj = DisplayObjectArray[i];
        GFxCharacter* pch = dobj.GetCharacter();
        GASSERT(pch);
        if (pch && pch->IsUnloaded())
        {
            pch->OnEventUnload();
            DisplayObjectArray.RemoveAt(i);
            pCachedChar = NULL; // should we check if we remove exactly the pCachedChar?
            n = DisplayObjectArray.GetSize();
            continue;
        }

        if (dobj.CanAdvanceChar())
        {
            pch->AdvanceFrame(nextFrame, framePos);          
        }
    }
}
#endif


// Display the referenced characters. Lower depths
// are obscured by higher depths.
void    GFxDisplayList::Display(GFxDisplayContext &context)
{
    UInt        maskCount          = 0;
    int         highestMaskedLayer = 0;
    GRenderer*  prenderer          = context.GetRenderer();
    
    for (UPInt i = 0, n = DisplayObjectArray.GetSize(); i < n; i++)
    {
        DisplayEntry&   dobj = DisplayObjectArray[i];
        GFxCharacter*   ch   = dobj.GetCharacter();
        GASSERT(ch);

        if (ch->IsUnloading())
            continue;

        // Don't display non-visible items, or items, used as a mask (MovieClip.setMask()).
        if (!ch->GetVisible() || ch->IsUsedAsMask() || ch->IsTopmostLevelFlagSet())
            continue;

        // If mask ended before this layer, remove it.
        if (maskCount && (ch->GetDepth() > highestMaskedLayer))
        {
            maskCount--;
            context.PopMask();
        }

        // Check to ensure that mask state is correct.
        if ((context.MaskStackIndexMax > context.MaskStackIndex) && !context.MaskRenderCount)
        {            
            // A different mask was rendered before this one; hence we must restore the mask state.
            // Draw mask with Decrement op.
            GRenderer::Matrix* poldmatrix = context.pParentMatrix;
            GRenderer::Cxform* poldcxform = context.pParentCxform;
            context.MaskRenderCount++;   
            while(context.MaskStackIndexMax > context.MaskStackIndex)
            {
                if (context.MaskStackIndexMax <= GFxDisplayContext::Mask_MaxStackSize)
                {
                    GFxCharacter* pparentmask = context.MaskStack[context.MaskStackIndexMax-1]->GetParent();
                    GRenderer::Matrix  matrix;

#ifndef GFC_NO_3D
                    if (pparentmask->Is3D() || context.Is3D)
                        context.pParentMatrix = &GMatrix2D::Identity;
                    else
#endif
                    {
                        pparentmask->GetWorldMatrix(&matrix);
                        context.pParentMatrix = &matrix;
                    }

                    prenderer->BeginSubmitMask(GRenderer::Mask_Decrement);
                    context.MaskStack[context.MaskStackIndexMax-1]->Display(context);
                    prenderer->EndSubmitMask();
                    context.MaskStack[context.MaskStackIndexMax-1] = 0;
                }                
                context.MaskStackIndexMax--;
            }
            context.MaskRenderCount--;
            context.pParentMatrix = poldmatrix;
            context.pParentCxform = poldcxform;
        }

        // Check whether this object should become mask; if so - apply.
        if ((ch->GetClipDepth() > 0) && !context.MaskRenderCount)
        {
            context.PushAndDrawMask(ch);
           
            highestMaskedLayer = ch->GetClipDepth();
            maskCount++;
        }
        else
        {
            // Rendering of regular shapes (not masks).
            prenderer->PushBlendMode(ch->GetBlendMode());
            ch->Display(context);
            prenderer->PopBlendMode();
        }
    }
    
    // If a mask masks the scene all the way up to the highest layer, it will not be
    // disabled at the end of drawing the display list, so disable it manually.        
    while (maskCount > 0)
    {        
        context.PopMask();
        maskCount--;
    }
}



// Calls all onClipEvent handlers due to a mouse event. 
void    GFxDisplayList::PropagateMouseEvent(const GFxEventId& id)
{       
    // Objects further on display list get mouse messages first.
    
    for (SPInt i = (SPInt)(DisplayObjectArray.GetSize() - 1); i >= 0; i--)
    {
        DisplayEntry&   dobj = DisplayObjectArray[i];
        GFxCharacter*   ch   = dobj.GetCharacter();
        GASSERT(ch);

        if (!ch->GetVisible())
            continue;

        ch->PropagateMouseEvent(id);

        // Can display list change during an event ? check for bounds.
        // This probably isn't the right behavior, since some of characters might miss
        // the mouse event.
        // Copy pointers first, and process them later ? @TODO
        if (i >= (SPInt)DisplayObjectArray.GetSize())
            i = (SPInt)DisplayObjectArray.GetSize(); //? reset to the end?
    }
}

// Calls all onClipEvent handlers due to a key event.   
void    GFxDisplayList::PropagateKeyEvent(const GFxEventId& id, int* pkeyMask)
{       
    // Objects at the beginning of display list get key messages first.
    
    for (UPInt i = 0, n = DisplayObjectArray.GetSize(); i < n; ++i)
    {
        DisplayEntry&   dobj = DisplayObjectArray[i];
        GFxCharacter*   ch   = dobj.GetCharacter();
        GASSERT(ch);

        if (!ch->GetVisible())  
            continue;

        ch->PropagateKeyEvent(id, pkeyMask);

        // Can display list change during an event ? (answer: yes!) check for recent bounds.
        // This probably isn't the right behavior, since some of characters might miss
        // the key event.
        // Copy pointers first, and process them later ? @TODO.
        if (i + 1 >= DisplayObjectArray.GetSize())
            break;
    }
}

// Visit all display list members that have a name.
void    GFxDisplayList::VisitMembers(GASObjectInterface::MemberVisitor *pvisitor, UInt visitFlags) const
{
    GUNUSED(visitFlags);

    // Enumeration Op lists children in-order.
    for (UPInt i = 0, n = DisplayObjectArray.GetSize(); i < n; ++i)
    {
        const DisplayEntry& dobj = DisplayObjectArray[i];
        GFxCharacter*       ch   = dobj.GetCharacter();
        GASSERT(ch);

        // This is used in enumeration op, which returns values by name only.
        if (ch->IsCharASCharacter())
        {
            const GASString& name = ch->CharToASCharacter_Unsafe()->GetName();
            if (!name.IsEmpty())
                pvisitor->Visit(name, GASValue(ch->CharToASCharacter_Unsafe()), 0);
        }
    }
}

#ifdef GFC_BUILD_DEBUG
// checks for consistency of the display list
void GFxDisplayList::CheckConsistency() const
{
    if (DisplayObjectArray.GetSize() > 0)
    {
        // check for multiple characters at the same depth. Note, sometimes
        // multiple characters at the same depth are allowed (see AddDisplayObject).

        GFxCharacter* curch = DisplayObjectArray[0].GetCharacter();
        GASSERT(curch);
        int curdepth = DisplayObjectArray[0].GetDepth();
        for (UInt i = 1; i < DisplayObjectArray.GetSize(); i++)
        {
            GFxCharacter* ich = DisplayObjectArray[i].GetCharacter();
            GASSERT(ich);
            if (DisplayObjectArray[i].GetDepth() >= 0)
                GASSERT(DisplayObjectArray[i].GetDepth() != curdepth);
            curdepth = DisplayObjectArray[i].GetDepth();
        }    
    }
}
#endif // GFC_BUILD_DEBUG


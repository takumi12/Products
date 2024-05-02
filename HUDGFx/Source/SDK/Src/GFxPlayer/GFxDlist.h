/**********************************************************************

Filename    :   GFxDlist.h
Content     :   Character display list for root and nested clips
Created     :   
Authors     :   Michael Antonov, TU

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXDLIST_H
#define INC_GFXDLIST_H

#include "GArray.h"
#include "GAlgorithm.h"
#include "GFxCharacter.h"
#include "GRenderer.h"

#include "GFxMovieDef.h"


// ***** Declared Classes
class GFxCharPosInfo;


// A list of active characters.
class GFxDisplayList : public GNewOverrideBase<GFxStatMV_MovieClip_Mem>
{
public:
    enum AddFlags
    {
        Flags_ReplaceIfDepthIsOccupied  = 0x1,
        Flags_DeadOnArrival             = 0x2,
        Flags_PlaceObject               = 0x4
    };
private:

    friend class GFxSprite;

    // A describing an entry in the display list. A depth-sorted array of these
    // structures is maintained.
    class DisplayEntry
    {
    private:
        enum  FrameFlags
        {           
            // An object is marked for removal. Objects that are marked for remove in
            // the last frame of the movie before wrap-around to first frame.
            // are not considered a part of display list, unless 
            Flag_MarkedForRemove    = 0x00000001
        };

        GFxCharacter* pCharacter;
    public:

        DisplayEntry()          
        {
            pCharacter = NULL;
        }

        DisplayEntry(const DisplayEntry& di)            
        {
            pCharacter = NULL;
            *this= di;
        }

        ~DisplayEntry()
        {
            GFxCharacter* prevch = GetCharacter();
            if (prevch)
                prevch->Release();
        }

        void    operator=(const DisplayEntry& di)
        {
            SetCharacter(di.GetCharacter());
        }

        void    SetCharacter(GFxCharacter* pch)
        {
            GFxCharacter* prevch = GetCharacter();
            if (prevch)
                prevch->Release();
            pCharacter = pch;
            if (pch)
                pch->AddRef();
        }
        GFxCharacter* GetCharacter() const
        {
            return pCharacter;
        }

        // Returns depth of a character, no null checking for speed.
        GINLINE int     GetDepth() const            { return GetCharacter()->GetDepth(); }

        GINLINE bool    IsMarkedForRemove() const   
        { 
            GASSERT(GetCharacter());
            return GetCharacter()->IsMarkedForRemove(); 
        }
        // Only advance characters if they are not just loaded and not marked for removal.
        GINLINE bool    CanAdvanceChar() const      
        { 
            return !IsMarkedForRemove(); 
        }
        GINLINE void    MarkForRemove(bool remove)
        {
            GASSERT(GetCharacter());
            GetCharacter()->MarkForRemove(remove); 
        }
        GINLINE void    ClearMarkForRemove() { MarkForRemove(false); }
    };

    
    // Display array, sorted by depth.
    GArrayLH<DisplayEntry>  DisplayObjectArray;

    // An instance of last character accessed by name (using GetCharacterByName). Needs to be
    // reset every time DisplayList is changed or name of any character
    // in display list is changed.
    GFxASCharacter*         pCachedChar;

private:
    static int DepthLess(const DisplayEntry& de, int depth)
    {
        return (de.GetDepth() < depth);
    }
public:
    GFxDisplayList() : pCachedChar(NULL) {}

    // *** Display list management.

    
    // Adds a new character to the display list.
    // addFlags - is a bit mask of GFxDisplayList::AddFlags bits.
    void        AddDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, UInt32 addFlags);
    // Moves an existing character in display list at given depth.
    void        MoveDisplayObject(const GFxCharPosInfo &pos);
    // Replaces a character at depth with a new one.
    void        ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch);
    // Replaces a character at index with a new one.
    void        ReplaceDisplayObjectAtIndex(UPInt index, GFxCharacter* ch);
    // Remove a character at depth.
    void        RemoveDisplayObject(int depth, GFxResourceId id);
    void        MarkForRemoval(int depth, GFxResourceId id);


    // Clear the display list, unloading all objects.
    void        Clear();
    // Unload is similar to Clear but it executes onUnload for all unloading chars
    bool        UnloadAll();

    // Mark all entries for removal, usually done when wrapping from
    // last frame to the next one. Objects that get re-created will
    // survive, other ones will die.
    void        MarkAllEntriesForRemoval(UInt targetFrame = 0);
    // Removes all objects that are marked for removal. Removed objects
    // receiveUnload event.
    void        UnloadMarkedObjects();

    bool        UnloadEntryAtIndex(UPInt index);

    // *** Frame processing.


#ifdef GFC_USE_OLD_ADVANCE
    // Advance referenced characters.
    void        AdvanceFrame(bool nextFrame, Float framePos);
#endif

    // Display the referenced characters.
    void        Display(GFxDisplayContext &context);

    // Calls all onClipEvent handlers due to a mouse event. 
    void        PropagateMouseEvent(const GFxEventId& id);

    // Calls all onClipEvent handlers due to a key event.   
    void        PropagateKeyEvent(const GFxEventId& id, int* pkeyMask);

    
    // *** Character entry access.

    // Finds display array index at depth, or next depth.
    UPInt       FindDisplayIndex(int depth)
    {
        return G_LowerBound(DisplayObjectArray, depth, DepthLess);
    }
    // Like the above, but looks for an exact match, and returns GFC_MAX_UPINT if failed.
    UPInt        GetDisplayIndex(int depth);

    // Swaps two objects at depths.
    // Depth1 must always have a valid object. Depth2 will be inserted, if not existent.
    bool        SwapDepths(int depth1, int depth2, UInt frame);
    // Returns maximum used depth, -1 if display list is empty.
    int         GetLargestDepthInUse() const;

    UPInt           GetCharacterCount() const        { return DisplayObjectArray.GetSize(); }
    GFxCharacter*   GetCharacter(UPInt index) const  { return DisplayObjectArray[index].GetCharacter(); }
    // Get the display object at the given position.
    DisplayEntry&   GetDisplayObject(UPInt idx)      { return DisplayObjectArray[idx]; }

    // May return NULL.
    GFxCharacter*   GetCharacterAtDepth(int depth, bool* pisMarkedForRemove = NULL);
    GFxCharacter*   GetCharacterAtDepthAndUnmark(int depth);

    // Note that only GFxASCharacters have names, so it's ok to return GFxASCharacter.
    // Case sensitivity depends on string context SWF version. May return NULL.
    // If there are multiples, returns the *first* match only!
    GFxASCharacter* GetCharacterByName(GASStringContext* psc, const GASString& name);    

    // Visit all display list members that have a name
    void            VisitMembers(GASObjectInterface::MemberVisitor *pvisitor, UInt visitFlags) const;

    // Removes character from display list. Note, this function just find and remove the object;
    // no events (unload) will be executed.
    // Returns true, if object is found and removed.
    bool            RemoveCharacter(GFxCharacter*);

    // Resets cached last character accessed by name.
    void            ResetCachedCharacter() { pCachedChar = NULL; }

#ifdef GFC_BUILD_DEBUG
    void            CheckConsistency() const;
#else
    inline void     CheckConsistency() const {}
#endif // GFC_BUILD_DEBUG
};


#endif // INC_GFXDLIST_H

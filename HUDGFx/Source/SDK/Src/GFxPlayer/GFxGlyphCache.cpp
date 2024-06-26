/*********************************************************************

Filename    :   GFxGlyphCache.cpp
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

----------------------------------------------------------------------
The code of stackBlur and recursiveBlur was taken from the Anti-Grain 
Geometry Project and modified for the use by Scaleform. 
Permission to use without restrictions is hereby granted to 
Scaleform Corporation by the author of Anti-Grain Geometry Project.
See http://antigtain.com for details.
**********************************************************************/

#ifndef GFC_NO_GLYPH_CACHE

#include "GFxGlyphCache.h"
#include "GFxDisplayContext.h"
#include "GFxShape.h"
#include "GFxFont.h"
#include "GFxAmpServer.h"


//------------------------------------------------------------------------
GFxGlyphSlotQueue::GFxGlyphSlotQueue() :
    MinSlotSpace(10),
    FirstTexture(0),
    NumTextures(0),
    TextureWidth(0),
    TextureHeight(0),
    MaxSlotHeight(0),
    NumBandsInTexture(0),
    SlotQueueSize(0),
    NumUsedBands(0),
    pEventHandler(0)
{}


//------------------------------------------------------------------------
void GFxGlyphSlotQueue::Clear()
{
    GlyphHTable.Clear();

    SlotQueue.Clear();
    ActiveSlots.Clear();

    for (UInt i = 0; i < NumUsedBands; ++i)
        Bands[i].Slots.Clear();

    Slots.ClearAndRelease();
    Glyphs.ClearAndRelease();

    SlotQueueSize = 0;
    NumUsedBands  = 0;
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::Init(UInt firstTexture, UInt numTextures, 
                             UInt textureWidth, UInt textureHeight, 
                             UInt maxSlotHeight)
{
    Clear();
    FirstTexture      = firstTexture;
    NumTextures       = numTextures;
    TextureWidth      = textureWidth;
    TextureHeight     = textureHeight;
    MaxSlotHeight     = maxSlotHeight;
    NumBandsInTexture = textureHeight / maxSlotHeight;
    Bands.Resize(NumBandsInTexture * NumTextures);
}



//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphSlotQueue::packGlyph(UInt w, UInt h, GFxGlyphNode* glyph)
{
    // A recursive procedure that performs Binary Space Partitioning.
    // It finds an appropriate suitable rectangle and subdivides the 
    // rest of the space. 
    // The method is described in details here: 
    // http://www.blackpawn.com/texts/lightmaps/default.html
    //
    // If glyph->pFont is zero it means an empty space. The children nodes
    // are pNext and pNex2. Name pNext is required by the GPodStructAllocator 
    // and reused in the tree.
    //--------------------------
    if (glyph->Param.pFont)
    {
        GFxGlyphNode* newGlyph = 0;
        if (glyph->pNext) 
            newGlyph = packGlyph(w, h, glyph->pNext);

        if (newGlyph)
            return newGlyph;

        if (glyph->pNex2) 
            newGlyph = packGlyph(w, h, glyph->pNex2);

        return newGlyph;
    }

    if (w <= glyph->Rect.w && h <= glyph->Rect.h)
    {
        UInt dw = glyph->Rect.w - w;
        UInt dh = glyph->Rect.h - h;

        // Subdivide space only if it's big enough. If it's not,
        // just use it as a whole.
        //----------------
        if (dw >= MinSlotSpace || dh >= MinSlotSpace)
        {
            if (dw > dh)
            {
                glyph->pNext = Glyphs.Alloc(*glyph);
                glyph->pNext->Rect.x = UInt16(glyph->Rect.x + w);
                glyph->pNext->Rect.w = UInt16(dw);
                if (dh >= MinSlotSpace)
                {
                    glyph->pNex2 = Glyphs.Alloc(*glyph);
                    glyph->pNex2->pNext  = 0;
                    glyph->pNex2->Rect.y = UInt16(glyph->Rect.y + h);
                    glyph->pNex2->Rect.h = UInt16(dh);
                    glyph->pNex2->Rect.w = UInt16(w);
                }
            }
            else
            {
                glyph->pNext = Glyphs.Alloc(*glyph);
                glyph->pNext->Rect.y = UInt16(glyph->Rect.y + h);
                glyph->pNext->Rect.h = UInt16(dh);
                if (dw >= MinSlotSpace)
                {
                    glyph->pNex2 = Glyphs.Alloc(*glyph);
                    glyph->pNex2->pNext  = 0;
                    glyph->pNex2->Rect.x = UInt16(glyph->Rect.x + w);
                    glyph->pNex2->Rect.w = UInt16(dw);
                    glyph->pNex2->Rect.h = UInt16(h);
                }
            }
        }
        glyph->Rect.w = UInt16(w);
        glyph->Rect.h = UInt16(h);
        return glyph;
    }
    return 0;
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::splitSlot(GFxGlyphDynaSlot* slot, UInt w)
{
    // Split the slot as "w" and "slot->w - w". The slot must be 
    // empty, but with the initialized BSP root. These conditions
    // are preserved by the callers.
    //-----------------------
    GFxGlyphDynaSlot* newSlot = initNewSlot(slot->pBand, slot->x + w, slot->w - w);
    slot->w             = UInt16(w);
    slot->pRoot->Rect.w = UInt16(w);
    SlotQueue.PushFront(newSlot);
    SlotQueueSize++;
    slot->pBand->Slots.InsertAfter(slot, newSlot);
    ActiveSlots.PushFront(newSlot);
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::splitGlyph(GFxGlyphDynaSlot* slot, bool left, UInt w)
{
    GFxGlyphNode* root = slot->pRoot;
    GFxGlyphDynaSlot* newSlot;
    UInt newW = root->Rect.w - w;
    if (left)
    {
        newSlot = initNewSlot(slot->pBand, root->Rect.x, newW);
        slot->pBand->Slots.InsertBefore(slot, newSlot);
        slot->x      = UInt16(slot->x + newW);
        root->Rect.x = slot->x;
    }
    else
    {
        newSlot = initNewSlot(slot->pBand, root->Rect.x + w, newW);
        slot->pBand->Slots.InsertAfter(slot, newSlot);
    }
    root->Rect.w = UInt16(w);
    slot->w      = UInt16(slot->w - newW);
    SlotQueue.PushFront(newSlot);
    SlotQueueSize++;
    ActiveSlots.PushFront(newSlot);
}

//------------------------------------------------------------------------
inline GFxGlyphNode* GFxGlyphSlotQueue::packGlyph(UInt w, UInt h, GFxGlyphDynaSlot* slot)
{
    // Check to see if the slot is empty (no glyph allocated and 
    // no BSP tree), and its width is twice as much than requested,
    // split it. This splits considerably reduce the number of 
    // BSP tree operations.
    //-------------------------
    GFxGlyphNode* glyph = slot->pRoot;
    if (glyph->Param.pFont == 0 && slot->w > 2*w)
    {
        if (glyph->pNext == 0 && glyph->pNex2 == 0)
        {
            splitSlot(slot, w);
        }
        else
        if (glyph->pNex2 == 0 && 
            glyph->Rect.h == slot->pBand->h &&
            glyph->Rect.w > 2*w)
        {
            // The slot may have a big empty glyph after merging, 
            // which is also good to split.
            //---------------------
            bool left  = glyph->Rect.x == slot->x;
            bool right = glyph->Rect.x + glyph->Rect.w == slot->x + slot->w;
            if (left != right)
                splitGlyph(slot, left, w);
        }
    }

    // An attempt to pack the glyph. First, the recursive packGlyph() 
    // is called, then, if it fails, the number of failures is increased.
    // If this number exceeds a certain value (MaxSlotFailures) the slot
    // is excluded from search. In case packing succeeds the number of failures
    // is decreased to keep the slot active.
    //-----------------------
    glyph = packGlyph(w, h, slot->pRoot);
    if (glyph == 0)
        slot->Failures++;

    if (slot->Failures > MaxSlotFailures)
    {
        ActiveSlots.Remove(slot);
        slot->TextureId |= GFxGlyphDynaSlot::FullFlag;
    }
    else
    if (slot->Failures && glyph)
        slot->Failures--;

    return glyph;
}

//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphSlotQueue::findSpaceInSlots(UInt w, UInt h)
{
    // Search for empty space in the active slots. 
    //-----------------------------

    //// Iterate trough the active slots and try to pack. If
    //// packing fails, but the slot still remains active, move
    //// it to the back of the active slot chain, as most probably
    //// the next glyph will fail too.
    ////---------------------------
    //GFxGlyphDynaSlot* slot = ActiveSlots.GetFirst();
    //GFxGlyphDynaSlot* last = ActiveSlots.GetLast();

    //while (!ActiveSlots.IsNull(slot) && slot != last)
    //{
    //    GFxGlyphDynaSlot* next  = ActiveSlots.GetNext(slot);
    //    GFxGlyphNode*     glyph = packGlyph(w, h, slot);

    //    if (glyph)
    //        return glyph;

    //    if ((slot->TextureId & GFxGlyphDynaSlot::FullFlag) == 0)
    //        ActiveSlots.SendToBack(slot);

    //    slot = next;
    //}
    //return ActiveSlots.IsNull(last) ? 0 : packGlyph(w, h, last);


    // Simple variant. Just iterate trough the active slots 
    // and try to pack, keeping the active slot chain as is.
    //----------------------------
    GFxGlyphDynaSlot* slot = ActiveSlots.GetFirst();
    while (!ActiveSlots.IsNull(slot))
    {
        GFxGlyphDynaSlot* next  = ActiveSlots.GetNext(slot);
        GFxGlyphNode*     glyph = packGlyph(w, h, slot);
        if (glyph)
            return glyph;
        slot = next;
    }

    return 0;
}

//------------------------------------------------------------------------
GFxGlyphDynaSlot* GFxGlyphSlotQueue::initNewSlot(GFxGlyphBand* band, UInt x, UInt w)
{
    // Allocate and init a new slot and the BSP tree root.
    //----------------------
    GFxGlyphDynaSlot* slot = Slots.Alloc();
    GFxGlyphNode*    glyph = Glyphs.Alloc();

    slot->pRoot       = glyph;
    slot->pBand       = band;
    slot->TextureId   = band->TextureId;
    slot->x           = UInt16(x);
    slot->w           = UInt16(w);
    slot->Failures    = 0;

    glyph->Param.Clear();
    glyph->pSlot      = slot;
    glyph->pNext      = 0;
    glyph->pNex2      = 0;
    glyph->Rect       = GFxGlyphRect(slot->x, band->y, slot->w, band->h);
    glyph->Origin.x   = 0;
    glyph->Origin.y   = 0;
    return slot;
}


//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphSlotQueue::allocateNewSlot(UInt w, UInt h)
{
    // Allocate a new slot if available. First, check for the necessity
    // to initialize a new band. 
    //----------------------
    if (NumUsedBands == 0 || Bands[NumUsedBands-1].RightSpace < w)
    {
        if (NumUsedBands < Bands.GetSize())
        {
            UInt bandInTexture = NumUsedBands % NumBandsInTexture;
            GFxGlyphBand& band = Bands[NumUsedBands];

            band.TextureId  =  UInt16(FirstTexture + NumUsedBands / NumBandsInTexture);
            band.y          =  UInt16(bandInTexture * MaxSlotHeight);
            band.h          =  UInt16((bandInTexture+1 == NumBandsInTexture) ? 
                                       TextureHeight - band.y : MaxSlotHeight);
            band.RightSpace =  UInt16(TextureWidth);
            band.Slots.Clear();
            ++NumUsedBands;
        }
    }

    // Check for the empty space in the right of the last band. 
    // If there is any and it fits the glyph, initialize the new slot. 
    // Also, check for the rest of the space. If it's less than 2*w
    // merge the rest with the slot.
    // -----------------------
    GFxGlyphBand& band = Bands[NumUsedBands-1];
    if (w <= band.RightSpace)
    {
        UInt dw = band.RightSpace - w;
        UInt ws = (dw < w) ? band.RightSpace : w;

        GFxGlyphDynaSlot* slot = initNewSlot(&band, TextureWidth - band.RightSpace, ws);

        band.RightSpace = UInt16(band.RightSpace - slot->w);
        SlotQueue.PushBack(slot); 
        SlotQueueSize++;
        band.Slots.PushBack(slot);
        ActiveSlots.PushFront(slot);
        return packGlyph(w, h, slot);
    }
    return 0;
}


//------------------------------------------------------------------------
void GFxGlyphSlotQueue::releaseGlyphTree(GFxGlyphNode* glyph)
{
    // Recursively release the glyph BSP tree and remove the 
    // glyphs from the hash table.
    //----------------------
    if (glyph)
    {
        releaseGlyphTree(glyph->pNext);
        releaseGlyphTree(glyph->pNex2);
        if (glyph->Param.pFont)
        {
            if (pEventHandler)
            {
                GRectF uvRect;
                uvRect.Left   = Float(glyph->Rect.x);
                uvRect.Top    = Float(glyph->Rect.y);
                uvRect.Right  = Float(glyph->Rect.x + glyph->Rect.w);
                uvRect.Bottom = Float(glyph->Rect.y + glyph->Rect.h);
                pEventHandler->OnEvictGlyph(glyph->Param, uvRect,
                                            glyph->pSlot->TextureId & ~GFxGlyphDynaSlot::Mask);
            }
            GlyphHTable.Remove(glyph->Param);
        }
        glyph->Param.pFont = 0;
        Glyphs.Free(glyph);
    }
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::releaseSlot(GFxGlyphDynaSlot* slot)
{
    // Release the slot. The whole BSP tree is being released, except
    // for the root. The root glyph is also released (if any), but 
    // the space is initialized as empty to be reused later.
    //-----------------------
    releaseGlyphTree(slot->pRoot->pNext);
    releaseGlyphTree(slot->pRoot->pNex2);

    if (slot->pRoot->Param.pFont)
        GlyphHTable.Remove(slot->pRoot->Param);

    GFxGlyphBand* band = slot->pBand;
    if (band->RightSpace && band->Slots.IsLast(slot))
    {
        // Merge the right space with the slot.
        slot->w = UInt16(slot->w + band->RightSpace);
        band->RightSpace = 0;
    }

    slot->pRoot->Param.pFont = 0;
    slot->pRoot->Rect  = GFxGlyphRect(slot->x, slot->pBand->y, slot->w, slot->pBand->h);
    slot->pRoot->pNext = 0;
    slot->pRoot->pNex2 = 0;
    slot->Failures     = 0;
    if (slot->TextureId & GFxGlyphDynaSlot::FullFlag)
    {
        slot->TextureId &= ~GFxGlyphDynaSlot::FullFlag;
        ActiveSlots.PushFront(slot);
    }
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::mergeSlots(GFxGlyphDynaSlot* from, GFxGlyphDynaSlot* to, UInt w)
{
    // Merge slots "from...to" in the band. All slots are being released 
    // and the ones on the right of "from" are removed.
    //---------------------
    GFxGlyphDynaSlot* slot = from;
    GFxGlyphBand*     band = slot->pBand;
    GUNUSED(band); // Suppress silly MSVC warning
    for (;;)
    {
        GFxGlyphDynaSlot* next = band->Slots.GetNext(slot);
        releaseSlot(slot);
        if (slot != from)
        {
            Glyphs.Free(slot->pRoot);
            SlotQueue.Remove(slot);
            SlotQueueSize--;
            if ((slot->TextureId & GFxGlyphDynaSlot::FullFlag) == 0)
            {
                ActiveSlots.Remove(slot);
            }
            band->Slots.Remove(slot);
            Slots.Free(slot);
        }
        if (slot == to)
            break;
        slot = next;
    }
    from->w             = UInt16(w);
    from->pRoot->Rect.w = UInt16(w);
    SlotQueue.BringToFront(from);
}

//------------------------------------------------------------------------
GFxGlyphDynaSlot* GFxGlyphSlotQueue::mergeSlotWithNeighbor(GFxGlyphDynaSlot* slot)
{
    // Merge the slot with its right or left neighbor in the band. 
    // This procedure prevents the old slots from "hanging" in the queue
    // for a long time.
    //-----------------------
    GFxGlyphBand*     band   = slot->pBand;
    GFxGlyphDynaSlot* target = band->Slots.GetNext(slot);
    bool toRight = true;
    if (band->Slots.IsNull(target))
    {
        target  = band->Slots.GetPrev(slot);
        toRight = false;
    }

    if (!band->Slots.IsNull(target))
    {
        GFxGlyphNode* slotGlyph   = slot->pRoot;
        GFxGlyphNode* targetGlyph = target->pRoot;
        releaseSlot(slot);
        UInt x = slot->x;
        UInt w = slot->w;
        SlotQueue.Remove(slot);
        SlotQueueSize--;
        if ((slot->TextureId & GFxGlyphDynaSlot::FullFlag) == 0)
        {
            ActiveSlots.Remove(slot);
        }
        band->Slots.Remove(slot);
        Slots.Free(slot);

        bool mergeRect = false;
        if (targetGlyph->Param.pFont == 0 && 
            targetGlyph->pNex2 == 0 &&
            targetGlyph->Rect.h == band->h)
        {
            mergeRect = toRight ?
                (x + w == UInt(targetGlyph->Rect.x)) :
                (UInt(targetGlyph->Rect.x + targetGlyph->Rect.w) == x);

        }
        if (mergeRect)
        {
            Glyphs.Free(slotGlyph);
            if (toRight)
                targetGlyph->Rect.x = UInt16(x);
            targetGlyph->Rect.w     = UInt16(targetGlyph->Rect.w + w);
        }
        else
        {
            slotGlyph->pNext  = targetGlyph;
            slotGlyph->pNex2  = 0;
            slotGlyph->pSlot  = target;
            slotGlyph->Rect   = GFxGlyphRect(x, band->y, w, band->h);
            target->pRoot     = slotGlyph;
        }

        if (toRight)
            target->x = UInt16(x);
        target->w     = UInt16(target->w + w);

        if (target->TextureId & GFxGlyphDynaSlot::FullFlag)
        {
            target->Failures = 0;
            target->TextureId &= ~GFxGlyphDynaSlot::FullFlag;
            ActiveSlots.PushFront(target);
        }
        return target;
    }
    return 0;
}

//------------------------------------------------------------------------
bool GFxGlyphSlotQueue::checkDistance(GFxGlyphDynaSlot* slot) const
{
    // Check the distance to the beginning. The function returns 
    // true if the slot is closer to the beginning of the queue. 
    //------------------
    GFxGlyphDynaSlot* slot1 = slot;
    GFxGlyphDynaSlot* slot2 = slot;
    for(;;)
    {
        if (SlotQueue.IsNull(slot1))
            return true;

        if (SlotQueue.IsNull(slot2))
            break;

        slot1 = SlotQueue.GetPrev(slot1);
        slot2 = SlotQueue.GetNext(slot2);
    }
    return false;
}

//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphSlotQueue::extrudeOldSlot(UInt w, UInt h)
{
    // Try to find an appropriate slot. First, iterate through the 
    // slot queue and find the slot of the required width. To keep
    // the LRU strategy in action, we iterate through only the first
    // half of the queue. Then, if the attempt fails, try to 
    // merge slots.
    //-------------------
    GFxGlyphDynaSlot* slot = SlotQueue.GetFirst();
    UInt i;
    UInt queueLimit = SlotQueueSize / 2;
    for (i = 0; i <= queueLimit && !SlotQueue.IsNull(slot); ++i)
    {
        if ((slot->TextureId & GFxGlyphDynaSlot::LockFlag) == 0)
        {
            if (UInt(slot->w) >= w)
            {
                releaseSlot(slot); // Slot is found, release it and pack.
                return packGlyph(w, h, slot);
            }
            else
            {
                // In case the slot is narrower than requested, 
                // merge it with its neighbor. It prevents small glyphs 
                // from "hanging" in the beginning of the queue for a 
                // long time. 
                //--------------------
                GFxGlyphDynaSlot* mergedWith = mergeSlotWithNeighbor(slot);
                if (mergedWith)
                {
                    if (mergedWith->pRoot->Rect.w >= w)
                    {
                        return packGlyph(w, h, mergedWith);
                    }
                    break;
                }
            }
        }
        slot = SlotQueue.GetNext(slot);
    }

    // An attempt to find the existing slot failed. Try to merge.
    // To do this we iterate the slots along the band until the 
    // required width is reached.
    //--------------------
    slot = SlotQueue.GetFirst();
    while (!SlotQueue.IsNull(slot))
    {
        if ((slot->TextureId & GFxGlyphDynaSlot::LockFlag) == 0)
        {
            UInt w2 = 0;
            GFxGlyphDynaSlot* to = slot;
            GFxGlyphBand* band = slot->pBand;
            while (!band->Slots.IsNull(to) && 
                  (to->TextureId & GFxGlyphDynaSlot::LockFlag) == 0)
            {
                if (to != slot && !checkDistance(to))
                    break;

                // Check to see if there's enough space in the slot.
                // If there is, reuse it without merging.
                // It prevents from obtaining too wide slots.
                //-----------------------
                if (to->w >= w)
                {
                    releaseSlot(to);
                    return packGlyph(w, h, to);
                }

                w2 += to->w;

                // Try to incorporate space on the right of the band
                // if there is any.
                //-----------------------
                if (band->RightSpace && 
                    band->Slots.IsLast(to) &&
                    w2 + band->RightSpace >= w)
                {
                    w2 += band->RightSpace;
                    band->RightSpace = 0;
                }

                // Merge slots slot...to in the band.
                if (w2 >= w)
                {
                    mergeSlots(slot, to, w2);
                    return packGlyph(w, h, slot);
                }
                to = band->Slots.GetNext(to);
            }
        }
        slot = SlotQueue.GetNext(slot);
    }

    return 0;
}


//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphSlotQueue::FindGlyph(const GFxGlyphParam& gp)
{
    GFxGlyphNode** f = GlyphHTable.Get(gp);
    return f ? *f : 0;
}

//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphSlotQueue::AllocateGlyph(const GFxGlyphParam& gp, UInt w, UInt h)
{
    if (h > MaxSlotHeight || w == 0 || h == 0)
    {
        return 0;           // Prevent from storing too tall and zero glyphs
    }

    if (h < MinSlotSpace) MinSlotSpace = h;
    if (w < MinSlotSpace) MinSlotSpace = w;

    GFxGlyphNode* glyph = findSpaceInSlots(w, h);
    if (glyph == 0)
    {
        glyph = allocateNewSlot(w, h);
        if (glyph == 0)
            glyph = extrudeOldSlot(w, h);
// DBG
//if(glyph) printf("X");
    }
    if (glyph)
    {
        glyph->Param      = gp;
        glyph->Origin.x   = 0;
        glyph->Origin.y   = 0;
        SendGlyphToBack(glyph);
        if(!GlyphHTable.Get(gp))
            GlyphHTable.Add(gp, glyph);
    }
    return glyph;
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::UnlockAllGlyphs()
{
    GFxGlyphDynaSlot* slot = SlotQueue.GetFirst();
    while(!SlotQueue.IsNull(slot))
    {
        slot->TextureId &= ~GFxGlyphDynaSlot::LockFlag;
        slot = SlotQueue.GetNext(slot);
    }
}


//------------------------------------------------------------------------
void GFxGlyphSlotQueue::computeGlyphArea(const GFxGlyphNode* glyph, UInt* used) const
{
    if (glyph)
    {
        if (glyph->Param.pFont)
            *used += glyph->Rect.w * glyph->Rect.h;

        computeGlyphArea(glyph->pNext, used);
        computeGlyphArea(glyph->pNex2, used);
    }
}

//------------------------------------------------------------------------
UInt GFxGlyphSlotQueue::ComputeUsedArea() const
{
    UInt totalUsed = 0;
    const GFxGlyphDynaSlot* slot = SlotQueue.GetFirst();
    while(!SlotQueue.IsNull(slot))
    {
        UInt used = 0;
        computeGlyphArea(slot->pRoot, &used);
        totalUsed += used;
        slot = SlotQueue.GetNext(slot);
    }
    return totalUsed;
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::CleanUpTexture(UInt textureId)
{
    GFxGlyphDynaSlot* slot = SlotQueue.GetFirst();
    while(!SlotQueue.IsNull(slot))
    {
        GFxGlyphDynaSlot* next = SlotQueue.GetNext(slot);
        if (UInt(slot->TextureId & ~GFxGlyphDynaSlot::Mask) == textureId)
        {
            releaseSlot(slot);
            SlotQueue.BringToFront(slot);
        }
        slot = next;
    }
}

//------------------------------------------------------------------------
const GFxGlyphNode* GFxGlyphSlotQueue::findFontInSlot(GFxGlyphNode* glyph, 
                                                      const GFxFontResource* font)
{
    if (glyph)
    {
        if (glyph->Param.pFont == font)
            return glyph;

        const GFxGlyphNode* g2 = findFontInSlot(glyph->pNext, font);
        if (g2)
            return g2;

        return findFontInSlot(glyph->pNex2, font);
    }
    return 0;
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::CleanUpFont(const GFxFontResource* font)
{
    GFxGlyphDynaSlot* slot = SlotQueue.GetFirst();
    while(!SlotQueue.IsNull(slot))
    {
        GFxGlyphDynaSlot* next = SlotQueue.GetNext(slot);
        if (findFontInSlot(slot->pRoot, font))
        {
            releaseSlot(slot);
            SlotQueue.BringToFront(slot);
        }
        slot = next;
    }
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::VisitGlyphs(GFxGlyphCacheVisitor* visitor) const
{
    CachedGlyphsType::ConstIterator it = GlyphHTable.Begin();
    const GFxGlyphNode* node;
    GRectF uvRect;

    for (; it != GlyphHTable.End(); ++it)
    {
        node = it->Second;
        uvRect.Left   = Float(node->Rect.x);
        uvRect.Top    = Float(node->Rect.y);
        uvRect.Right  = Float(node->Rect.x + node->Rect.w);
        uvRect.Bottom = Float(node->Rect.y + node->Rect.h);
        visitor->Visit(it->First, uvRect, node->pSlot->TextureId & ~GFxGlyphDynaSlot::Mask);
    }
}

//------------------------------------------------------------------------
void GFxGlyphSlotQueue::SetEventHandler(GFxGlyphCacheEventHandler* h)
{
    pEventHandler = h;
}



//------------------------------------------------------------------------
GFxGlyphRasterCache::GFxGlyphRasterCache():
    TextureWidth(0),
    TextureHeight(0),
    MaxNumTextures(0),
    MaxSlotHeight(0),
    SlotPadding(0),
    ScaleU(0),
    ScaleV(0),
    GlyphsToUpdate(),
    RectsToUpdate(),
    TexUpdBuffer(),
    TexUpdPacker(0, 0),
    KnockOutCopy(),
    Shape(),
    Rasterizer(),
    ScanlineFilter(4.5f/9.0f, 2.0f/9.0f, 1.0f/19.0f),
    Fitter(),
    SlotQueue(),
    pEventHandler(0)
{
    for(UInt i = 0; i < TexturePoolSize; i++)
    {
        Textures[i].pTexture    = 0;
        Textures[i].NumGlyphsToUpdate = 0;
    }
}


//------------------------------------------------------------------------
GFxGlyphRasterCache::~GFxGlyphRasterCache()
{
    Clear();
}


// Clear the cache
//------------------------------------------------------------------------
void GFxGlyphRasterCache::Clear()
{
    releaseAllTextures();
    GlyphsToUpdate.Clear();
    RectsToUpdate.Clear();
    TexUpdPacker.Clear();
    SlotQueue.Clear();
}



//
// Initialize or re-initialize the cache. The physical structure of the cache 
// slots is shown above. The function also clears the cache.
//
// texWidth, texHeight           - Texture size. Rounded to the nearest power of 2. 
//
// maxNumTextures                - The number of textures that is allowed to allocate.
//                                 The cache can be extended later with ExtendCache().
//
// maxSlotHeight                 - Slot size in pixels.
//
// slotPadding                   - Size decrement value to compute the glyph rectangles.
//
// texUpdWidth, texUpdHeight     - The buffer size to update textures. It allocates 
//                                 texUpdWidth*texUpdHeight bytes in system memory. The 
//                                 In Average 256x256 is capable to update about 30 big
//                                 glyphs at a time. Size 256x256 is good enough for 
//                                 all practical purposes.
//------------------------------------------------------------------------
void GFxGlyphRasterCache::Init(UInt texWidth, UInt texHeight, UInt maxNumTextures, 
                               UInt maxSlotHeight, UInt slotPadding,
                               UInt texUpdWidth, UInt texUpdHeight)
{
    Clear();

    UInt w  = (texWidth  < 64) ? 63 : texWidth  - 1;
    UInt h  = (texHeight < 64) ? 63 : texHeight - 1;
    UInt sw = 0;
    UInt sh = 0;

    while(w) { ++sw; w >>= 1; }
    while(h) { ++sh; h >>= 1; }

    if (maxNumTextures > TexturePoolSize)
        maxNumTextures = TexturePoolSize;

    TextureWidth         = 1 << sw;
    TextureHeight        = 1 << sh;
    MaxNumTextures       = maxNumTextures;
    MaxSlotHeight        = maxSlotHeight;
    SlotPadding          = slotPadding;
    ScaleU               = 1.0f / (Float)TextureWidth;
    ScaleV               = 1.0f / (Float)TextureHeight;

    TexUpdBuffer = *GHEAP_AUTO_NEW(this) 
        GImage(GImageBase::Image_A_8, (texUpdWidth + 3) & ~3, texUpdHeight);

    TexUpdPacker = GFxTextureUpdatePacker(texUpdWidth, texUpdHeight);
    SlotQueue.Init(0, maxNumTextures, texWidth, texHeight, maxSlotHeight);
}


//------------------------------------------------------------------------
void GFxGlyphRasterCache::releaseAllTextures()
{
    for(UInt i = 0; i < MaxNumTextures; ++i)
    {
        if (Textures[i].pTexture)
        {
            Textures[i].pTexture->RemoveChangeHandler(&Textures[i].Handler);
            Textures[i].pTexture->Release();
            Textures[i].pTexture = 0;
        }
        Textures[i].NumGlyphsToUpdate = 0;
    }
}

//------------------------------------------------------------------------
inline bool GFxGlyphRasterCache::initTextureIfRequired(GRenderer* ren, 
                                                       UInt textureId)
{
    TextureType* tex = &Textures[textureId];

    if (tex->pTexture == 0)
    {
        tex->pTexture = ren->CreateTexture();
        if (tex->pTexture == 0) 
            return false;
        tex->Handler.Bind(this, textureId);
        tex->pTexture->AddChangeHandler(&tex->Handler);
        if (!tex->pTexture->InitDynamicTexture(TextureWidth, 
                                            TextureHeight, 
                                            GImage::Image_A_8, 
                                            0, GTexture::Usage_Update))
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------
void GFxGlyphRasterCache::InitTextures(GRenderer* ren)
{
    for (UInt i = 0; i < MaxNumTextures; ++i)
        initTextureIfRequired(ren, i);
}

//------------------------------------------------------------------------
bool GFxGlyphRasterCache::RecreateTexture(UInt textureId)
{
    TextureType* tex = &Textures[textureId];
    return tex->pTexture->InitDynamicTexture(TextureWidth, TextureHeight, 
        GImage::Image_A_8, 0, GTexture::Usage_Update);
}

//------------------------------------------------------------------------
void GFxGlyphRasterCache::CleanUpTexture(UInt textureId, bool release)
{
    SlotQueue.CleanUpTexture(textureId);
    UInt i, j;
    for(i = j = 0; i < GlyphsToUpdate.GetSize(); i++)
    {
        if (GlyphsToUpdate[i].TextureId != textureId)
            GlyphsToUpdate[j++] = GlyphsToUpdate[i];
    }
    GlyphsToUpdate.CutAt(j);
    Textures[textureId].NumGlyphsToUpdate = 0;
    if (release)
    {
        Textures[textureId].pTexture->RemoveChangeHandler(&Textures[textureId].Handler);
        Textures[textureId].pTexture->Release();
        Textures[textureId].pTexture = 0;
    }
}

//------------------------------------------------------------------------
void GFxGlyphRasterCache::CleanUpFont(const GFxFontResource* font)
{
    SlotQueue.CleanUpFont(font);
}

//------------------------------------------------------------------------
void GFxGlyphRasterCache::filterScanline(const UByte* src, UByte* dst, UInt len)
{
    UInt i;
    for(i = 2; i+2 < len; ++i)
    {
        ScanlineFilter.Filter(src++, dst++);
    }
}



//------------------------------------------------------------------------
// The code of stackBlur and recursiveBlur was taken from the Anti-Grain 
// Geometry Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corporation by the author of Anti-Grain Geometry Project.
// See http://antigtain.com for details.
//------------------------------------------------------------------------


//------------------------------------------------------------------------
static UInt16 const GFx_StackBlurMul[255] = 
{
    512,512,456,512,328,456,335,512,405,328,271,456,388,335,292,512,
    454,405,364,328,298,271,496,456,420,388,360,335,312,292,273,512,
    482,454,428,405,383,364,345,328,312,298,284,271,259,496,475,456,
    437,420,404,388,374,360,347,335,323,312,302,292,282,273,265,512,
    497,482,468,454,441,428,417,405,394,383,373,364,354,345,337,328,
    320,312,305,298,291,284,278,271,265,259,507,496,485,475,465,456,
    446,437,428,420,412,404,396,388,381,374,367,360,354,347,341,335,
    329,323,318,312,307,302,297,292,287,282,278,273,269,265,261,512,
    505,497,489,482,475,468,461,454,447,441,435,428,422,417,411,405,
    399,394,389,383,378,373,368,364,359,354,350,345,341,337,332,328,
    324,320,316,312,309,305,301,298,294,291,287,284,281,278,274,271,
    268,265,262,259,257,507,501,496,491,485,480,475,470,465,460,456,
    451,446,442,437,433,428,424,420,416,412,408,404,400,396,392,388,
    385,381,377,374,370,367,363,360,357,354,350,347,344,341,338,335,
    332,329,326,323,320,318,315,312,310,307,304,302,299,297,294,292,
    289,287,285,282,280,278,275,273,271,269,267,265,263,261,259
};

//------------------------------------------------------------------------
static UInt8 const GFx_StackBlurShr[255] = 
{
    9,  11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 
    17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 
    23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
};


//------------------------------------------------------------------------
void GFxGlyphRasterCache::stackBlur(GImage& img, UInt sx, UInt sy, UInt w, UInt h, UInt rx, UInt ry)
{
    UInt x, y, xp, yp, i;
    UInt stackPtr;
    UInt stackStart;

    const UInt8* srcPixPtr;
          UInt8* dstPixPtr;

    UInt pix;
    UInt stackPix;
    UInt sum;
    UInt sumIn;
    UInt sumOut;

    UInt wm  = w - 1;
    UInt hm  = h - 1;

    UInt div;
    UInt mulSum;
    UInt shrSum;

    if(rx > 0)
    {
        if(rx > 254) rx = 254;
        div = rx * 2 + 1;
        mulSum = GFx_StackBlurMul[rx];
        shrSum = GFx_StackBlurShr[rx];
        BlurStack.Resize(div);

        for(y = 0; y < h; y++)
        {
            sum = sumIn = sumOut = 0;

            srcPixPtr = img.GetScanline(sy + y) + sx;
            pix = *srcPixPtr;
            for(i = 0; i <= rx; i++)
            {
                BlurStack[i] = UInt8(pix);
                sum     += pix * (i + 1);
                sumOut  += pix;
            }
            for(i = 1; i <= rx; i++)
            {
                if(i <= wm) ++srcPixPtr; 
                pix = *srcPixPtr; 
                BlurStack[i + rx] = UInt8(pix);
                sum   += pix * (rx + 1 - i);
                sumIn += pix;
            }

            stackPtr = rx;
            xp = rx;
            if(xp > wm) xp = wm;
            srcPixPtr = img.GetScanline(sy + y) + sx + xp;
            dstPixPtr = img.GetScanline(sy + y) + sx;
            for(x = 0; x < w; x++)
            {
                *dstPixPtr++ = UInt8((sum * mulSum) >> shrSum);

                sum -= sumOut;
    
                stackStart = stackPtr + div - rx;
                if(stackStart >= div) stackStart -= div;
                sumOut -= BlurStack[stackStart];

                if(xp < wm) 
                {
                    ++srcPixPtr;
                    pix = *srcPixPtr;
                    ++xp;
                }
    
                BlurStack[stackStart] = UInt8(pix);
    
                sumIn += pix;
                sum   += sumIn;
    
                ++stackPtr;
                if(stackPtr >= div) stackPtr = 0;
                stackPix = BlurStack[stackPtr];

                sumOut += stackPix;
                sumIn  -= stackPix;
            }
        }
    }
    if(ry > 0)
    {
        if(ry > 254) ry = 254;
        div = ry * 2 + 1;
        mulSum = GFx_StackBlurMul[ry];
        shrSum = GFx_StackBlurShr[ry];
        BlurStack.Resize(div);

        int stride = img.Pitch;
        for(x = 0; x < w; x++)
        {
            sum = sumIn = sumOut = 0;

            srcPixPtr = img.GetScanline(sy) + sx + x;
            pix = *srcPixPtr;
            for(i = 0; i <= ry; i++)
            {
                BlurStack[i] = UInt8(pix);
                sum     += pix * (i + 1);
                sumOut  += pix;
            }
            for(i = 1; i <= ry; i++)
            {
                if(i <= hm) srcPixPtr += stride; 
                pix = *srcPixPtr; 
                BlurStack[i + ry] = UInt8(pix);
                sum   += pix * (ry + 1 - i);
                sumIn += pix;
            }

            stackPtr = ry;
            yp = ry;
            if(yp > hm) yp = hm;
            srcPixPtr = img.GetScanline(sy + yp) + sx + x;
            dstPixPtr = img.GetScanline(sy)      + sx + x;
            for(y = 0; y < h; y++)
            {
                *dstPixPtr  = UInt8((sum * mulSum) >> shrSum);
                 dstPixPtr += stride;

                sum -= sumOut;
    
                stackStart = stackPtr + div - ry;
                if(stackStart >= div) stackStart -= div;
                sumOut -= BlurStack[stackStart];

                if(yp < hm) 
                {
                    srcPixPtr += stride;
                    pix = *srcPixPtr;
                    ++yp;
                }
    
                BlurStack[stackStart] = UInt8(pix);
    
                sumIn += pix;
                sum   += sumIn;
    
                ++stackPtr;
                if(stackPtr >= div) stackPtr = 0;
                stackPix = BlurStack[stackPtr];

                sumOut += stackPix;
                sumIn  -= stackPix;
            }
        }
    }
}



//------------------------------------------------------------------------
class GFxImgBlurWrapperX 
{
public:
    GFxImgBlurWrapperX(GImage& img, UInt x, UInt y, UInt w, UInt h) :
        Img(img), Sx(x), Sy(y), W(w), H(h) {}

    UInt  GetWidth()  const { return W; }
    UInt  GetHeight() const { return H; }
    UInt8 GetPixel(UInt x, UInt y) const { return Img.GetScanline(Sy+y)[Sx+x]; }

    void CopySpanTo(UInt x, UInt y, UInt len, const UInt8* buf)
    {
        memcpy(Img.GetScanline(Sy+y) + Sx+x, buf, len);
    }

private:
    GFxImgBlurWrapperX(const GFxImgBlurWrapperX&);
    const GFxImgBlurWrapperX& operator = (const GFxImgBlurWrapperX&);
    GImage& Img;
    UInt Sx, Sy, W, H;
};


//------------------------------------------------------------------------
class GFxImgBlurWrapperY
{
public:
    GFxImgBlurWrapperY(GImage& img, UInt x, UInt y, UInt w, UInt h) :
        Img(img), Sx(x), Sy(y), W(w), H(h) {}

    UInt  GetWidth()  const { return H; }
    UInt  GetHeight() const { return W; }
    UInt8 GetPixel(UInt x, UInt y) const { return Img.GetScanline(Sy+x)[Sx+y]; }

    void CopySpanTo(UInt x, UInt y, UInt len, const UInt8* buf)
    {
        UInt8* p = Img.GetScanline(Sy+x) + Sx+y;
        do
        {
            *p  = *buf++;
             p += Img.Pitch;
        }
        while(--len);
    }

private:
    GFxImgBlurWrapperY(const GFxImgBlurWrapperY&);
    const GFxImgBlurWrapperY& operator = (const GFxImgBlurWrapperY&);
    GImage& Img;
    UInt Sx, Sy, W, H;
};


//------------------------------------------------------------------------
template<class Img, class SumBuf, class ColorBuf>
void GFx_RecursiveBlur(Img& img, Float radius, SumBuf& sum, ColorBuf& buf)
{
    if (radius < 0.62f) 
        radius = 0.62f;

    int w   = img.GetWidth();
    int h   = img.GetHeight();
    int pad = int(ceilf(radius)) + 3;
    int x, y;

    float s = radius * 0.5f;
    float q = (s < 2.5f) ?
               3.97156f - 4.14554f * sqrtf(1 - 0.26891f * s) :
               0.98711f * s - 0.96330f;

    float q2 = q  * q;
    float q3 = q2 * q;

    float b0 =  1.0f / (1.578250f + 2.444130f * q + 1.428100f * q2 + 0.422205f * q3);
    float b2 =  2.44413f * q + 2.85619f * q2 + 1.26661f * q3;
    float b3 = -1.42810f * q2 + -1.26661f * q3;
    float b4 =  0.422205f * q3;

    float b1  = 1.0f - (b2 + b3 + b4) * b0;

    b2 *= b0;
    b3 *= b0;
    b4 *= b0;

    sum.Resize(w + 2*pad);
    buf.Resize(w + 2*pad);

    for(y = 0; y < h; y++)
    {
        register float a1=0, a2=0, a3=0, a4=0;

        for(x = 0; x < pad; ++x)
            sum[x] = 0;

        for(x = 0; x < w; ++x)
        {
            a1 = sum[x+pad] = b1*img.GetPixel(x, y) + b2*a2 + b3*a3 + b4*a4;
            a4 = a3;
            a3 = a2;
            a2 = a1;
        }

        for(x = 0; x < pad; ++x)
        {
            a1 = sum[x+w+pad] = b2*a2 + b3*a3 + b4*a4;
            a4 = a3;
            a3 = a2;
            a2 = a1;
        }

        a1 = a2 = a3 = a4 = 0;

        for(x = w+pad+pad-1; x >= pad; --x)
        {
            a1 = b1*sum[x] + b2*a2 + b3*a3 + b4*a4;
            a4 = a3;
            a3 = a2;
            a2 = a1;
            buf[x] = UInt8(a1 + 0.5f);
        }
        img.CopySpanTo(0, y, w, &buf[pad]);
    }
}

//------------------------------------------------------------------------
void GFxGlyphRasterCache::recursiveBlur(GImage& img, 
                                        UInt  sx, UInt  sy, 
                                        UInt  w,  UInt  h, 
                                        Float rx, Float ry)
{
    GFxImgBlurWrapperX imgWX(img, sx, sy, w, h); 
    GFx_RecursiveBlur( imgWX, rx, BlurSum, BlurStack);
    GFxImgBlurWrapperY imgWY(img, sx, sy, w, h); 
    GFx_RecursiveBlur( imgWY, ry, BlurSum, BlurStack);
}


//------------------------------------------------------------------------
void GFxGlyphRasterCache::strengthenImage(GImage& img, 
                                          UInt  sx, UInt  sy, 
                                          UInt  w,  UInt  h, 
                                          Float ratio,
                                          SInt  bias)
{
    if (ratio != 1.0f)
    {
        UInt x, y;
        for(y = 0; y < h; ++y)
        {
            UInt8* p = img.GetScanline(sy + y) + sx;
            for(x = 0; x < w; ++x)
            {
                SInt v = SInt((int(*p) - bias) * ratio + 0.5f) + bias;
                if (v <   0) v = 0;
                if (v > 255) v = 255;
                *p++ = UInt8(v);
            }
        }
    }
}


//------------------------------------------------------------------------
void GFxGlyphRasterCache::makeKnockOutCopy(const GImage& img, 
                                           UInt  sx, UInt  sy, 
                                           UInt  w,  UInt  h)
{
    if (KnockOutCopy.GetPtr() == 0 ||
        KnockOutCopy->Width < w ||
        KnockOutCopy->Height < h)
    {
        KnockOutCopy = *GHEAP_AUTO_NEW(this) 
            GImage(GImageBase::Image_A_8, w + 16, h + 16);
    }

    UInt y;
    for(y = 0; y < h; ++y)
    {
        const UInt8* src = img.GetScanline(sy + y) + sx;
              UInt8* dst = KnockOutCopy->GetScanline(y);
        memcpy(dst, src, w);
    }
}

//------------------------------------------------------------------------
void GFxGlyphRasterCache::knockOut(GImage& img, 
                                   UInt  sx, UInt  sy, 
                                   UInt  w,  UInt  h)
{
    if (KnockOutCopy.GetPtr() != 0)
    {
        UInt x, y;
        for(y = 0; y < h; ++y)
        {
            const UInt8* src = KnockOutCopy->GetScanline(y);
                  UInt8* dst = img.GetScanline(sy + y) + sx;
            for(x = 0; x < w; ++x)
            {
                UInt s = 255 - *src++;
                UInt d = *dst;
                *dst++ = UInt8((s * d + 255) >> 8);
            }
        }
    }
}


//------------------------------------------------------------------------
void GFxGlyphRasterCache::CalcGlyphParam(Float originalFontSize, 
                                         UInt  snappedFontSize, 
                                         Float heightRatio,
                                         Float glyphHeight,
                                         const GFxGlyphParam& textParam,
                                         GFxGlyphParam* glyphParam,
                                         UInt16* subpixelSize,
                                         UInt16* glyphScale) const
{
    glyphParam->SetFontSize(snappedFontSize);
    glyphParam->Flags = textParam.Flags;
    glyphParam->BlurStrength = textParam.BlurStrength;
    *subpixelSize = UInt16(snappedFontSize * SubpixelSizeScale);

    Float k = Float(snappedFontSize) / originalFontSize;

    if (k < 1.0f) k = 1.0f;

    Float blurX   = textParam.GetBlurX()   * heightRatio * k;
    Float blurY   = textParam.GetBlurY()   * heightRatio * k;

    k = snappedFontSize / 1024.0f;
    Float d = blurY;
    Float height    = glyphHeight * k + 2*d;
    Float maxHeight = (Float)(GetMaxGlyphHeight() - 2);

    *glyphScale = 256;

    if (height > maxHeight)
    {
        k = Float(maxHeight) / Float(height);
        *subpixelSize = UInt16(*subpixelSize * k);
        *glyphScale   = UInt16(k * 256.0f + 0.5f);
        blurX   *= k;
        blurY   *= k;
        glyphParam->SetAutoFit(false);
    }

    if (blurX > GFxGlyphParam::GetMaxBlur()) 
        blurX = GFxGlyphParam::GetMaxBlur();

    if (blurY > GFxGlyphParam::GetMaxBlur()) 
        blurY = GFxGlyphParam::GetMaxBlur();

    glyphParam->SetBlurX(blurX);
    glyphParam->SetBlurY(blurY);

    // Correction of scale when Stack Blur is in use. The Stack
    // Blur cannot have fractional radius, so that, we correct the 
    // glyph rectangles to make it look almost identical to 
    // fractional blurring radius.
    //---------------------------------
    if (!textParam.IsFineBlur() && !textParam.IsOptRead())
    {
        UInt intBlurY = UInt(glyphParam->GetBlurY() + 0.5f);
        if (intBlurY == 0 && glyphParam->BlurY) intBlurY = 1;
        if (Float(intBlurY) < blurY)
        {
            *subpixelSize = UInt16(*subpixelSize * intBlurY / blurY);
            *glyphScale = UInt16(*glyphScale * intBlurY / blurY);
        }
    }
}


/*
// The code of this class was taken from the Anti-Grain Geometry
// Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corporation by the author of Anti-Grain Geometry Project.
// See http://antigrain.com for details.
//------------------------------------------------------------------------
class GFxSubpixelBresenhamLine
{
public:
    enum SubpixelScale_e
    {
        SubpixelShift = 8,
        SubpixelScale = 1 << SubpixelShift,
        SubpixelMask  = SubpixelScale - 1,
    };

    //--------------------------------------------------------------------
    static int LineLowRes(int v) { return v >> SubpixelShift; }

    //--------------------------------------------------------------------
    GFxSubpixelBresenhamLine(int x1, int y1, int x2, int y2) :
        X1LowRes(LineLowRes(x1)),
        Y1LowRes(LineLowRes(y1)),
        X2LowRes(LineLowRes(x2)),
        Y2LowRes(LineLowRes(y2)),
        Ver(abs(X2LowRes - X1LowRes) < abs(Y2LowRes - Y1LowRes)),
        Len(Ver ? abs(Y2LowRes - Y1LowRes) : 
                  abs(X2LowRes - X1LowRes)),
        Inc(Ver ? ((y2 > y1) ? 1 : -1) : ((x2 > x1) ? 1 : -1)),
        Interpolator(Ver ? x1 : y1, Ver ? x2 : y2, Len ? Len : 1)
    {}

    //--------------------------------------------------------------------
    bool     IsVer()  const { return Ver; }
    unsigned GetLen() const { return Len; }
    int      GetInc() const { return Inc; }

    //--------------------------------------------------------------------
    void HorStep()
    {
        ++Interpolator;
        X1LowRes += Inc;
    }

    //--------------------------------------------------------------------
    void VerStep()
    {
        ++Interpolator;
        Y1LowRes += Inc;
    }

    //--------------------------------------------------------------------
    int GetX1() const { return X1LowRes; }
    int GetY1() const { return Y1LowRes; }
    int GetX2() const { return LineLowRes(Interpolator.y()); }
    int GetY2() const { return LineLowRes(Interpolator.y()); }
    int GetX2HiRes() const { return Interpolator.y(); }
    int GetY2HiRes() const { return Interpolator.y(); }

private:
    int                 X1LowRes;
    int                 Y1LowRes;
    int                 X2LowRes;
    int                 Y2LowRes;
    bool                Ver;
    unsigned            Len;
    int                 Inc;
    GLinearInterpolator Interpolator;
};

//------------------------------------------------------------------------
static void GFxDrawBresenhamLine(GImage* img, int originX, int originY, 
                                 int x1, int y1, int x2, int y2, UByte value)
{
    GFxSubpixelBresenhamLine line(x1, y1, x2, y2);
    unsigned len = line.GetLen();
    if (len)
    {
        if(line.IsVer())
        {
            do
            {
                img->GetScanline(originY + line.GetY1())[originX + line.GetX2()] = value;
                line.VerStep();
            }
            while(--len);
        }
        else
        {
            do
            {
                img->GetScanline(originY + line.GetY2())[originX + line.GetX1()] = value;
                line.HorStep();
            }
            while(--len);
        }
    }
}

//------------------------------------------------------------------------
static void GFxDrawGlyphOutline(GImage* img, int originX, int originY, 
                                const GCompoundShape& shape)
{
    unsigned i, j;
    int x1, y1, x2, y2;
    for(i = 0; i < shape.GetNumPaths(); i++)
    {
        const GCompoundShape::SPath& path = shape.GetPath(i);
        
        if(path.GetLeftStyle() != path.GetRightStyle())
        {
            x1 = int(path.GetVertex(0).x * 256);
            y1 = int(path.GetVertex(0).y * 256);
            for(j = 1; j < path.GetNumVertices(); j++)
            {
                x2 = int(path.GetVertex(j).x * 256);
                y2 = int(path.GetVertex(j).y * 256);
                GFxDrawBresenhamLine(img, originX, originY, x1, y1, x2, y2, 255);
                x1 = x2;
                y1 = y2;
            }
        }
    }
}

//------------------------------------------------------------------------
static const UInt8 GFx_HintedGamma[256] = 
{
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 23, 25, 27, 29,
     31, 32, 34, 36, 38, 39, 41, 43, 45, 46, 48, 50, 51, 53, 55, 56,
     58, 60, 61, 63, 64, 66, 68, 69, 71, 72, 74, 75, 77, 78, 80, 81,
     83, 84, 86, 87, 89, 90, 92, 93, 95, 96, 98, 99,101,102,103,105,
    106,108,109,111,112,113,115,116,117,119,120,122,123,124,126,127,
    128,130,131,132,134,135,136,137,139,140,141,143,144,145,146,148,
    149,150,151,153,154,155,156,158,159,160,161,163,164,165,166,167,
    169,170,171,172,173,175,176,177,178,179,181,182,183,184,185,186,
    188,189,190,191,192,193,194,196,197,198,199,200,201,202,203,204,
    206,207,208,209,210,211,212,213,214,215,217,218,219,220,221,222,
    223,224,225,226,227,228,229,230,231,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};
*/

//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphRasterCache::rasterizeAndPack(GRenderer* ren, 
                                                    const GFxGlyphParam& gp,
                                                    bool  canUseRaster,
                                                    const GFxShapeBase* shape,
                                                    UInt subpixelSize,
                                                    GFxLog* log)
{
    const GFxGlyphRaster* raster = 0;
    if (canUseRaster)
        raster = gp.pFont->GetGlyphRaster(gp.GlyphIndex, gp.FontSize);

    if (shape == 0 && raster == 0)
        return 0;

    UInt i,j;
    UInt stretch  = gp.GetStretch();
    UInt intBlurX = UInt(gp.GetBlurX() + 0.5f);
    UInt intBlurY = UInt(gp.GetBlurY() + 0.5f);

    if (intBlurX == 0 && gp.BlurX) intBlurX = 1;
    if (intBlurY == 0 && gp.BlurY) intBlurY = 1;

    Rasterizer.Clear();
    if (raster == 0)
    {
        if (shape->GetHintedGlyphSize() == 0)
        {
            int lowerCaseTop = 0;
            int upperCaseTop = 0;
            bool autoFit = false;

            if (gp.GetOutline() == 0 && gp.IsOptRead() && gp.IsAutoFit() && 
                subpixelSize > 6*SubpixelSizeScale && 
               (gp.pFont->HasLayout() || gp.pFont->IsDeviceFont()))
            {
                lowerCaseTop = gp.pFont->GetLowerCaseTop(log);
                upperCaseTop = gp.pFont->GetUpperCaseTop(log);
                if (lowerCaseTop && upperCaseTop)
                    autoFit = true;
            }

            shape->MakeCompoundShape(&Shape, GFxFontPackParams::GlyphBoundBox / MaxSlotHeight * 0.5f);
            if (autoFit)
            {
                Fitter.Clear();
                SInt nominalSize = subpixelSize * (64 / SubpixelSizeScale);
                if (nominalSize > 2048)
                    nominalSize = 2048;

                Fitter.SetNominalFontHeight(nominalSize);
                Float k = nominalSize / 1024.0f;

                for (i = 0; i < Shape.GetNumPaths(); ++i)
                {
                    const GCompoundShape::SPath& path = Shape.GetPath(i);
                    if (path.GetNumVertices() > 2)
                    {
                        Fitter.MoveTo( SInt16(path.GetVertex(0).x * k), 
                                      -SInt16(path.GetVertex(0).y * k));
                        for (j = 1; j < path.GetNumVertices(); ++j)
                        {
                            Fitter.LineTo( SInt16(path.GetVertex(j).x * k), 
                                          -SInt16(path.GetVertex(j).y * k));
                        }
                    }
                }

                Fitter.FitGlyph(subpixelSize / SubpixelSizeScale, 0, int(lowerCaseTop * k), int(upperCaseTop * k));
                k = 1.0f / Fitter.GetUnitsPerPixelY();

                for(i = 0; i < Fitter.GetNumContours(); ++i)
                {
                    const GFxGlyphFitter::ContourType& c = Fitter.GetContour(i);
                    if(c.NumVertices > 2)
                    {
                        GFxGlyphFitter::VertexType v = Fitter.GetVertex(c, 0);
                        Fitter.SnapVertex(v);

                        Rasterizer.MoveTo(v.x * k * stretch, -v.y * k);
                        for(j = 1; j < c.NumVertices; ++j)
                        {
                            v = Fitter.GetVertex(c, j);
                            Fitter.SnapVertex(v);
                            Rasterizer.LineTo(v.x * k * stretch, -v.y * k);
                        }
                        Rasterizer.ClosePolygon();
                    }
                }
            }
            else
            {
                Rasterizer.AddShapeScaled(Shape, 
                                          subpixelSize * stretch / (1024.0f * SubpixelSizeScale), 
                                          subpixelSize / (1024.0f * SubpixelSizeScale), 
                                          0, 0);
            }
        }
        else
        {
            shape->MakeCompoundShape(&Shape, 10);

            // k is the scaling ratio. Normally it should be just 1/20, but when 
            // rendering a shadow the gp.FontSize (and corresponding subpixelSize) 
            // may be smaller because the shadow does not fit the maximal height.
            // In this case the coefficient should consider it and scale the shape
            // accordingly, as gp.FontSize/shape->GetHintedGlyphSize(), or more 
            // accurate, using the subpixelSize value.
            Float k = Float(subpixelSize) / Float(shape->GetHintedGlyphSize() * SubpixelSizeScale * 20);
            Rasterizer.AddShapeScaled(Shape, k * stretch, k, 0, 0);
        }
    }
    GFxGlyphNode* glyph = 0;

    SInt padX = SlotPadding + intBlurX;
    SInt padY = SlotPadding + intBlurY;

    SInt imgX1 = -padX;
    SInt imgX2 =  padX;
    SInt imgY1 = -padY;
    SInt imgY2 =  padY;

    bool cellsToSweep = false;
    if (raster)
    {
        imgX1 = -raster->OriginX - padX;
        imgY1 = -raster->OriginY - padY;
        imgX2 =  raster->Width   - 1 - raster->OriginX + padX;
        imgY2 =  raster->Height  - 1 - raster->OriginY + padY;
    }
    else
    {
        if (Rasterizer.SortCells())
        {
            imgX1 = Rasterizer.GetMinX() - padX;
            imgX2 = Rasterizer.GetMaxX() + padX;
            imgY1 = Rasterizer.GetMinY() - padY;
            imgY2 = Rasterizer.GetMaxY() + padY;
            cellsToSweep = true;
        }
    }

    UInt imgW  = imgX2 - imgX1 + 1;
    UInt imgH  = imgY2 - imgY1 + 1;

    GImage& img = *TexUpdBuffer;

    if (imgH > GetMaxSlotHeight())
        imgH = GetMaxSlotHeight();

    if (imgW > img.Width)
        imgW = img.Width;

    glyph = SlotQueue.AllocateGlyph(gp, imgW, imgH);
    if (glyph == 0)
    {
        if (pEventHandler)
            pEventHandler->OnFailure(gp, imgW, imgH);
        return 0;
    }

    if (!initTextureIfRequired(ren, GetTextureId(glyph)))
        return 0;

    UInt imgX, imgY;
    if (!TexUpdPacker.Allocate(imgW, imgH, &imgX, &imgY))
    {
        UpdateTextures(ren);
        if (!TexUpdPacker.Allocate(imgW, imgH, &imgX, &imgY))
            return 0;
    }

    glyph->Origin.x = SInt16(SInt(glyph->Rect.x) - imgX1);
    glyph->Origin.y = SInt16(SInt(glyph->Rect.y) - imgY1);

    for (i = 0; i < imgH; ++i)
        memset(img.GetScanline(imgY + i) + imgX, 0, imgW);

// DBG
//for (i = 1; i < imgH-1; ++i)
//{
//    memset(img.GetScanline(imgY + i) + imgX + 1, 255, imgW-2);
//}
//if(imgH > 4 && imgW > 4)
//{
//    for (i = 2; i < imgH-2; ++i)
//    {
//        memset(img.GetScanline(imgY + i) + imgX + 2, 0, imgW-4);
//        img.GetScanline(imgY + i)[imgX + imgW / 2] = 255;
//    }
//    memset(img.GetScanline(imgY + imgH/2) + imgX + 2, 255, imgW-4);
//}

    // Sweep the raster writing the scan lines to the image.
    if (raster)
    {
        for(i = 0; i < raster->Height; ++i)
        {
            memcpy(img.GetScanline(imgY + padY + i) + imgX + padX,
                   &raster->Raster[i * raster->Width], 
                   raster->Width);
        }
    }
    else
    if (cellsToSweep)
    {
        Float gamma = 1.0f;
        if (gp.BlurX || gp.BlurY)
        {
            // Gamma correction improves the "strength" of glow and shadow
            // TO DO: Think of smarter control of it.
            gamma = 0.4f;
        }
        if (gamma != Rasterizer.GetGamma())
            Rasterizer.SetGamma(gamma);

        if (imgW < 5 || stretch < 3)
        {
            for(i = 0; i < Rasterizer.GetNumScanlines(); ++i)
            {
                Rasterizer.SweepScanline(i, img.GetScanline(imgY + padY + i) + imgX + padX);
            }
        }
        else
        {
            for(i = 0; i < Rasterizer.GetNumScanlines(); ++i)
            {
                memset(img.GetScanline(imgY) + imgX, 0, imgW);
                Rasterizer.SweepScanline(i, img.GetScanline(imgY) + imgX + padX);
                filterScanline(img.GetScanline(imgY) + imgX, 
                               img.GetScanline(imgY + padY + i) + imgX, imgW);
            }
            memset(img.GetScanline(imgY) + imgX, 0, imgW);
        }
        
        // Too Black!
        //GFxDrawGlyphOutline(&img, imgX + padX - Rasterizer.GetMinX(), 
        //                          imgY + padY - Rasterizer.GetMinY(), Shape);

        // In a practical average case the best Gamma is No Gamma!
        //if (shape->GetHintedGlyphSize())
        //{
        //    // Apply gamma correction to the hinted glyph
        //    for (i = 0; i < imgH; ++i)
        //    {
        //        UInt8* p = img.GetScanline(imgY + i) + imgX;
        //        for (j = 0; j < imgW; ++j)
        //            p[j] = GFx_HintedGamma[p[j]];
        //    }
        //}

        SInt bias = 0;
        if (gp.IsKnockOut())
            makeKnockOutCopy(img, imgX, imgY, imgW, imgH);

        if (gp.BlurX || gp.BlurY)
        {
            if (gp.IsFineBlur())
            {
                recursiveBlur(img, imgX, imgY, imgW, imgH, gp.GetBlurX(), gp.GetBlurY());
                bias = 8;
            }
            else
            {
                stackBlur(img, imgX, imgY, imgW, imgH, intBlurX, intBlurY);
                bias = 2;
            }
        }

        if (gp.BlurStrength != 16)
            strengthenImage(img, imgX, imgY, imgW, imgH, 
                            gp.GetBlurStrength(), 
                           (gp.GetBlurStrength() <= 1) ? 0 : bias);

        if (gp.IsKnockOut())
            knockOut(img, imgX, imgY, imgW, imgH);
    }

//static int gln = 1;
//char buf[20];
//sprintf(buf, "%03d.pgm", gln);
//FILE*fd = fopen(buf, "wb");
//fprintf(fd, "P5\n%d %d\n255\n", imgW, imgH);
//for(int i = 0; i < imgH; ++i)
//fwrite(img.GetScanline(imgY+i) + imgX, imgW, 1, fd);
//fclose(fd);
//gln++;

    GlyphUpdateType gu;
    gu.TextureId       = GetTextureId(glyph);
    gu.Rect.src.Left   = imgX;
    gu.Rect.src.Top    = imgY;
    gu.Rect.src.Right  = imgX + imgW;
    gu.Rect.src.Bottom = imgY + imgH;
    gu.Rect.dest.x     = glyph->Rect.x;
    gu.Rect.dest.y     = glyph->Rect.y;
    GlyphsToUpdate.PushBack(gu);
    Textures[gu.TextureId].NumGlyphsToUpdate++;

    if (pEventHandler)
    {
        GRectF uvRect;
        uvRect.Left   = Float(glyph->Rect.x);
        uvRect.Top    = Float(glyph->Rect.y);
        uvRect.Right  = Float(glyph->Rect.x + glyph->Rect.w);
        uvRect.Bottom = Float(glyph->Rect.y + glyph->Rect.h);
        pEventHandler->OnUpdateGlyph(glyph->Param, uvRect, GetTextureId(glyph),
                                     img.GetScanline(imgY) + imgX, imgW, imgH, img.Pitch);
    }


// DBG simulate 4bpp textures
//for(UInt y = 0; y < imgH; ++y)
//    for(UInt x = 0; x < imgW; ++x)
//        img.GetScanline(y + imgY)[x + imgX] &= 0xF0;

    return glyph;
}


//------------------------------------------------------------------------
void GFxGlyphRasterCache::UpdateTextures(GRenderer* ren)
{
    UInt i, j;
    for(i = 0; i < MaxNumTextures; ++i)
    {
        TextureType& tex = Textures[i];
        if (tex.NumGlyphsToUpdate)
        {
            if (initTextureIfRequired(ren, i))
            {
                RectsToUpdate.Resize(tex.NumGlyphsToUpdate, 32);
                UInt numRects = 0;
                for (j = 0; j < (UInt)GlyphsToUpdate.GetSize(); j++)
                {
                    if (GlyphsToUpdate[j].TextureId == i)
                    {
                        GASSERT(numRects < (UInt)RectsToUpdate.GetSize());
                        RectsToUpdate[numRects++] = GlyphsToUpdate[j].Rect;
                    }
                }
#ifdef GFX_AMP_SERVER
                GFxAmpServer::GetInstance().IncrementFontCacheTextureUpdate();
#endif
                Textures[i].pTexture->Update(0, numRects, &RectsToUpdate[0], TexUpdBuffer);
// DBG
//printf("\nUpd %d Packed %d\n", numRects, SlotQueue.GetNumPacked());
            }
            tex.NumGlyphsToUpdate = 0;
        }
    }
    GlyphsToUpdate.Clear();
    TexUpdPacker.Clear();
}


//------------------------------------------------------------------------
GFxGlyphNode* GFxGlyphRasterCache::GetGlyph(GRenderer* ren, 
                                            const GFxGlyphParam& gp,
                                            bool  canUseRaster,
                                            const GFxShapeBase* shape,
                                            UInt subpixelSize,
                                            GFxLog* log)
{   
    GFxGlyphNode* glyph = SlotQueue.FindGlyph(gp);
    if (glyph)
    {
// DBG
//printf("+");
//static int c=0;
//if((++c % 1000) == 0)
//{
//printf("\nPacked %d\n", SlotQueue.GetNumPacked());
//}
        SlotQueue.SendGlyphToBack(glyph);
        return glyph;
    }
    glyph = rasterizeAndPack(ren, gp, canUseRaster, shape, subpixelSize, log);
    return glyph;    
}

UInt GFxGlyphRasterCache::GetNumRasterizedGlyphs() const 
{ 
    return SlotQueue.GetNumPacked();
}

UInt GFxGlyphRasterCache::GetNumTextures() const
{
    UInt numTextures = 0;
    for (UPInt i = 0; i < MaxNumTextures; ++i)
    {
        if (Textures[i].pTexture != NULL)
        {
            ++numTextures;
        }
    }
    return numTextures;
}

//------------------------------------------------------------------------
void GFxGlyphRasterCache::VisitGlyphs(GFxGlyphCacheVisitor* visitor) const
{
    SlotQueue.VisitGlyphs(visitor);
}

void GFxGlyphRasterCache::SetEventHandler(class GFxGlyphCacheEventHandler* h)
{
    pEventHandler = h;
    SlotQueue.SetEventHandler(h);
}

UInt GFxGlyphRasterCache::ComputeUsedArea() const
{
    return SlotQueue.ComputeUsedArea();
}


#endif // GFC_NO_GLYPH_CACHE

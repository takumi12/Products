/**********************************************************************

Filename    :   GFxAmpMemoryImage.h
Content     :   Helper classes for memory fragmentation report
Created     :   March 2010
Authors     :   Maxim Shemenarev

Copyright   :   (c) 2005-2010 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFX_AMP_MEMORY_IMAGE_H
#define INC_GFX_AMP_MEMORY_IMAGE_H

template<class T> 
class GFxMemDataArray
{
    enum 
    { 
        MaxPages  = 32,
        PageShift = 12,
        PageSize  = 1 << PageShift, 
        PageMask  =      PageSize - 1
    };

public:
    typedef T ValueType;

    GFxMemDataArray() : NumPages(0), Size(0) {}
    ~GFxMemDataArray() { ClearAndRelease(); }

    void ClearAndRelease()
    {
        if(NumPages)
        {
            ValueType** page = Pages + NumPages - 1;
            UPInt freeCount = Size & PageMask;
            while(NumPages--)
            {
                GMemoryHeap::FreeSysDirect(*page, PageSize * sizeof(ValueType));
                freeCount = PageSize;
                --page;
            }
        }
        Size = NumPages = 0;
    }

    void Clear()
    {
        Size = NumPages = 0;
    }

    void PushBack(const ValueType& v)
    {
        *acquireDataPtr() = v;
        ++Size;
    }

    UPInt GetSize() const 
    { 
        return Size; 
    }

    const ValueType& operator [] (UPInt i) const
    {
        return Pages[i >> PageShift][i & PageMask];
    }

    ValueType& operator [] (UPInt i)
    {
        return Pages[i >> PageShift][i & PageMask];
    }

private:
    GINLINE ValueType* acquireDataPtr()
    {
        UPInt np = Size >> PageShift;
        if(np >= NumPages)
        {
#ifdef GFC_BUILD_DEBUG
            GASSERT(np < MaxPages);
#else
            if (np >= MaxPages)
            {
                --Size;
                return Pages[Size >> PageShift] + (Size & PageMask);
            }
#endif
            Pages[np] = (ValueType*)GMemory::pGlobalHeap->AllocSysDirect(PageSize * sizeof(ValueType));
            NumPages++;
        }
        return Pages[np] + (Size & PageMask);
    }

    ValueType*  Pages[MaxPages];
    UPInt       NumPages;
    UPInt       Size;
};

struct GFxMemSegment
{
    UPInt               Addr;
    UPInt               Size;
    const GMemoryHeap*  Heap;
    UByte               Type;

    bool operator<(const GFxMemSegment& rhs) const
    {
        if (Addr < rhs.Addr)
        {
            return true;
        }
        if (Addr == rhs.Addr)
        {
            if (Heap < rhs.Heap)
            {
                return true;
            }
            if (Heap == rhs.Heap)
            {
                return (Type < rhs.Type);
            }
        }
        return false;
    }
};

struct GFxMemSegVisitor : public GHeapSegVisitor
{
    virtual void Visit(UInt cat, const GMemoryHeap* heap, UPInt addr, UPInt size)
    {
        GFxMemSegment seg;
        seg.Addr = addr;
        seg.Size = size;
        seg.Heap = heap;
        seg.Type = static_cast<UByte>(cat);
        MemorySegments.PushBack(seg);
    }
    GFxMemDataArray<GFxMemSegment> MemorySegments;
};

struct GFxAmpHeapVisitor : public GMemoryHeap::HeapVisitor
{
    GFxAmpHeapVisitor(GFxMemSegVisitor* segVisitor) : SegVisitor(segVisitor) {}
    GArray<GMemoryHeap*> Heaps;
    virtual void Visit(GMemoryHeap*, GMemoryHeap *heap)
    { 
        heap->VisitHeapSegments(SegVisitor);
        heap->VisitChildHeaps(this);
    }
    GFxMemSegVisitor* SegVisitor;
};

struct GFxAmpImageVisitor : public GFxMovieDef::ResourceVisitor 
{ 
    GArray<GPtr<GFxImageResource> >     Images;

    virtual void Visit(GFxMovieDef*, GFxResource* resource, GFxResourceId, const char*) 
    { 
        if (resource->GetResourceType() == GFxResource::RT_Image)
        {
            Images.PushBack(static_cast<GFxImageResource*>(resource));
        }
    }
};

struct GFxAmpFontVisitor : public GFxMovieDef::ResourceVisitor
{        
    GArray<GString> Fonts;

    virtual void Visit(GFxMovieDef*, GFxResource* presource, GFxResourceId rid, const char*)
    {
        GFxFontResource* pfontResource = static_cast<GFxFontResource*>(presource);

        char buf[100];
        GString font;
        font = pfontResource->GetName();
        if (pfontResource->IsBold())
        {
            font += " - Bold";
        }
        else if (pfontResource->IsItalic())
        {
            font += " - Italic";
        }
        G_sprintf(buf, sizeof(buf), ", %d glyphs", pfontResource->GetGlyphShapeCount()); 
        font += buf;
        if (!pfontResource->HasLayout())
        {
            font += ", static only";
        }
        font += " (";
        rid.GenerateIdString(buf);
        font += buf;
        font += ")";

        Fonts.PushBack(font);
    }
};

#endif   // INC_GFX_AMP_MEMORY_IMAGE_H

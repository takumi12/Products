/**********************************************************************

Filename    :   GAutoPtr.h
Content     :   Siple auto pointers: GAutoPtr, GScopePtr
Created     :   2008
Authors     :   MaximShemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GAutoPtr_H
#define INC_GAutoPtr_H

#include "GTypes.h"

// ***** GAutoPtr
//
// GAutoPtr similar to std::auto_ptr. It keeps a boolean ownership 
// flag, so can be correctly assigned and copy-constructed, but 
// generates extra code.
// DO NOT use in performance critical code.
// DO NOT store in containers (GArray, GHash, etc).
//------------------------------------------------------------------------
template<class T> class GAutoPtr
{
public:
    GAutoPtr(T* p = 0) : pObject(p), Owner(true) {}
    GAutoPtr(T* p, bool owner) : pObject(p), Owner(owner) {}
    GAutoPtr(const GAutoPtr<T>& p) : pObject(0), Owner(p.Owner) 
    { 
        pObject = p.constRelease(); 
    }
    ~GAutoPtr(void) { Reset(); }
 
    GAutoPtr<T>& operator=(const GAutoPtr<T>& p)
    {
        if (this != &p) 
        {
            bool owner = p.Owner;
            Reset(p.constRelease());
            Owner = owner;
        }
        return *this;
    }
 
    GAutoPtr<T>& operator=(T* p)
    {
         Reset(p);
         return *this;
    }

    T& operator* ()     const { return *pObject; }
    T* operator->()     const { return  pObject; }
    T* GetPtr()         const { return  pObject; }
    operator bool(void) const { return  pObject != 0; }    

    T* Release()
    {
        Owner = false;
        return pObject;
    }

    void Reset(T* p = 0, bool owner = true)
    {
        if (pObject != p) 
        {
            if (pObject && Owner) 
                delete Release();
            pObject = p;
        }
        Owner = p != 0 && owner;
     }

private:
    T*   pObject; 
    bool Owner;

    // Release for const object.
    T* constRelease(void) const
    {
        return const_cast<GAutoPtr<T>*>(this)->Release();
    }
};



// ***** GScopePtr
//
// A simple wrapper used for automatic object deleting
// No ownership, no assignment operator, no copy-constructor,
// so that no extra code is generated compared with manual new/delete
//------------------------------------------------------------------------
template<class T> class GScopePtr
{
public:
    GScopePtr(T* p = 0) : pObject(p) {}
    ~GScopePtr(void) { delete pObject; }

    T& operator* () const { return *pObject; }
    T* operator->() const { return  pObject; }
    T* GetPtr()     const { return  pObject; }
    operator bool() const { return  pObject != 0; }

private:
    // Copying is prohibited
    GScopePtr(const GScopePtr<T>&);
    const GScopePtr<T>& operator=(const GScopePtr<T>&); 

    T* pObject;
};

#endif


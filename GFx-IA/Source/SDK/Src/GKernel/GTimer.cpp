/**********************************************************************

Filename    :   GTimer.cpp
Content     :   Provides static functions for precise timing
Created     :   June 28, 2005
Authors     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GTimer.h"

#if defined (GFC_OS_WIN32) || defined(GFC_OS_WINCE)
#include <windows.h>

#elif defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
#include <xtl.h>

#elif defined(GFC_OS_PS3)
#include <sys/sys_time.h>

#elif defined(GFC_OS_PS2)
#include <eekernel.h>

#elif defined(GFC_OS_PSP)
#include <time.h>
#include <rtcsvc.h>

#elif defined(GFC_OS_WII)
#include <revolution/os.h>

#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)
#include <dk_time.h>

#else
#include <sys/time.h>
#endif

UInt64  GTimer::GetProfileTicks()
{
    return (GetRawTicks() * GFX_MICROSECONDS_PER_SECOND) / GetRawFrequency();
}

Double  GTimer::ProfileTicksToSeconds(UInt64 ticks)
{
    return RawTicksToSeconds(ticks, GFX_MICROSECONDS_PER_SECOND);
}

Double GTimer::TicksToSeconds(UInt64 ticks)
{
    return RawTicksToSeconds(ticks, GFX_MICROSECONDS_PER_SECOND);
}

Double GTimer::RawTicksToSeconds(UInt64 rawTicks, UInt64 rawFrequency)
{
    // TBD: Is this precision good enough if Double == Float? (GFC_NO_DOUBLE).
    return static_cast<Double>(rawTicks) / rawFrequency;
}

// *** Win32,Xbox,Xbox360 Specific Timer
#if (defined (GFC_OS_WIN32) || defined (GFC_OS_XBOX) || defined (GFC_OS_XBOX360) || defined(GFC_OS_WINCE))

UInt64 GTimer::GetTicks()
{
#if defined (GFC_OS_XBOX360) 
    UInt64 ticks = GetTickCount();
#else
    UInt64 ticks = timeGetTime();
#endif
    return ticks * GFX_MILLISECONDS_PER_SECOND;
}

UInt64 GTimer::GetRawTicks()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

UInt64 GTimer::GetRawFrequency()
{
    static UInt64 perfFreq = 0;
    if (perfFreq == 0)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        perfFreq = freq.QuadPart;
    }
    return perfFreq;
}

#else   // !GFC_OS_WIN32


// *** Other OS Specific Timer     
     
// The profile ticks implementation is just fine for a normal timer.
UInt64 GTimer::GetTicks()
{
    return GetProfileTicks();
}

// System specific GetProfileTicks

#if defined(GFC_OS_PS3)


UInt64 GTimer::GetRawTicks()
{
    // read time
    UInt64 ticks;
#ifdef GFC_CC_SNC
    ticks = __mftb();
#else
    asm volatile ("mftb %0" : "=r"(ticks));
#endif
    return ticks;
}

UInt64 GTimer::GetRawFrequency()
{
    static bool bInitialized = false;
    static UInt64 iPerfFreq;
    if (!bInitialized)
    {
        iPerfFreq = sys_time_get_timebase_frequency();
		bInitialized = true;
    }
    return iPerfFreq;
}

#elif defined(GFC_OS_PS2)

UInt64 GTimer::GetRawTicks()
{
    u_int s, us;
    TimerBusClock2USec(GetTimerSystemTime(), &s, &us);

    return UInt64(us) + UInt64(s) * GFX_MICROSECONDS_PER_SECOND;
}

UInt64 GTimer::GetRawFrequency()
{
    return GFX_MICROSECONDS_PER_SECOND;
}

#elif defined(GFC_OS_PSP)

UInt64 GTimer::GetRawTicks()
{
    SceRtcTick ticks;
    sceRtcGetCurrentTick(&ticks);

    return static_cast<UInt64>(ticks.tick);
}

UInt64 GTimer::GetRawFrequency()
{
    return GFX_MICROSECONDS_PER_SECOND;
}

#elif defined(GFC_OS_WII)

UInt64 GTimer::GetRawTicks()
{
    return OSTicksToMicroseconds(OSGetTime());
}

UInt64 GTimer::GetRawFrequency()
{
    return GFX_MICROSECONDS_PER_SECOND;
}

#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)

UInt64  GTimer::GetRawTicks()
{
    return dk_tm_getCurrentTime();
}

UInt64 GTimer::GetRawFrequency()
{
    return GFX_MICROSECONDS_PER_SECOND;
}

// Other OS 
#else

UInt64  GTimer::GetRawTicks()
{
    // TODO: prefer rdtsc when available?

    // Return microseconds.
    struct timeval tv;
    UInt64 result;
    
    gettimeofday(&tv, 0);

    result = (UInt64)tv.tv_sec * 1000000;
    result += tv.tv_usec;

    return result;
}

UInt64 GTimer::GetRawFrequency()
{
    return GFX_MICROSECONDS_PER_SECOND;
}

#endif // GetProfileTicks


#endif  // !GFC_OS_WIN32


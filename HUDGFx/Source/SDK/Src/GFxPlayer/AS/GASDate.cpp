/**********************************************************************

Filename    :   GFxDate.cpp
Content     :   SWF (Shockwave Flash) player library
Created     :   October 24, 2006
Authors     :   Andrew Reisse

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxASString.h"
#include "GFxAction.h"
#include "GMsgFormat.h"
#include "AS/GASArrayObject.h"
#include "AS/GASNumberObject.h"
#include "AS/GASDate.h"

// GFC_NO_FXPLAYER_AS_DATE disables Date class.
#ifndef GFC_NO_FXPLAYER_AS_DATE

#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
#include <sys/timeb.h>
# if defined(GFC_OS_WIN32)
#include <windows.h>
# endif
#elif defined(GFC_OS_WINCE)
#include <windows.h>
#elif defined(GFC_OS_PS3)
#include <cell/rtc.h>
#elif defined(GFC_OS_PS2)
#include <sifdev.h>
#elif defined(GFC_OS_PSP)
#include <rtcsvc.h>
#elif defined(GFC_OS_WII)
#include <revolution/os.h>
#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)
#include <dk_time.h>
#else
#include <sys/time.h>
#endif

GASDate::GASDate(GASEnvironment* penv)
: GASObject(penv)
{
    commonInit(penv);
}

GASDate::GASDate(GASEnvironment* penv, SInt64 val)
: GASObject(penv)
{
    commonInit(penv);
    SetDate(val);
}

void GASDate::commonInit (GASEnvironment* penv)
{    
    Date        = 0; // ms +/- 1 Jan 1970
    Time        = 0; // ms within day
    Year        = 0;
    JDate       = 0; // days within year

    // local time
    LDate       = 0; // ms +/- 1 Jan 1970
    LTime       = 0; // ms within day
    LYear       = 0;
    LJDate      = 0; // days within year

    // time zone
    LocalOffset = 0;

    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Date));
}

const char* GASDate::GetTextValue(GASEnvironment*) const
{
    return "Date";
}

//////////////////////////////////////////
//

static inline bool IsLeapYear(SInt y)
{
    return 0 == (y % 4) && ((y % 100) || 0 == (y % 400));
}

static inline SInt StartOfYear (SInt y)
{
    y -= 1970;
    return 365 * y + ((y+1) / 4) - ((y+69) / 100) + ((y+369) / 400);
}

void GASDate::UpdateLocal()
{
    LTime = Time + LocalOffset;
    LDate = Date + LocalOffset;
    LJDate = JDate;
    LYear = Year;
    if (LTime >= 86400000 || LTime < 0)
    {
        SInt dd = ((LTime + 864000000) / 86400000) - 10;
        LJDate += dd;
        LTime -= dd * 86400000;
        if (LJDate >= (IsLeapYear(LYear) ? 366 : 365))
        {
            LJDate -= IsLeapYear(LYear) ? 366 : 365;
            LYear++;
        }
        else if (LJDate < 0)
        {
            LYear--;
            LJDate += IsLeapYear(LYear) ? 366 : 365;
        }
    }
}

void GASDate::UpdateGMT()
{
    Time = LTime - LocalOffset;
    Date = LDate - LocalOffset;
    JDate = LJDate;
    Year = LYear;
    if (Time >= 86400000 || Time < 0)
    {
        SInt dd = ((Time + 864000000) / 86400000) - 10;
        JDate += dd;
        Time -= dd * 86400000;
        if (JDate >= (IsLeapYear(Year) ? 366 : 365))
        {
            JDate -= IsLeapYear(Year) ? 366 : 365;
            Year++;
        }
        else if (JDate < 0)
        {
            Year--;
            JDate += IsLeapYear(Year);
        }
    }
}

void GASDate::SetDate(SInt64 val)
{
    SInt64 days = val / 86400000;
    Time = SInt32(val % 86400000);
    SInt64 qcents = days / (400 * 365 + 97);
    SInt64 qcdays = days % (400 * 365 + 97);
    Year = 1970 + SInt32(qcents * 400);
    if (val >= 0)
    {
        while (qcdays >= (IsLeapYear(Year) ? 366 : 365))
        {
            qcdays -= IsLeapYear(Year) ? 366 : 365;
            Year++;
        }
    }
    else
    {
        while (G_Abs(qcdays) >= (IsLeapYear(Year) ? 366 : 365))
        {
            Year--;
            qcdays += IsLeapYear(Year) ? 366 : 365;
        }
    }
    JDate = SInt(qcdays);
    Date = val;
    UpdateLocal();
}

void GASDateProto::DateGetTimezoneOffset(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(-pThis->LocalOffset / 60000) );
}

static const int months[2][12] = {{31,59,90,120,151,181,212,243,273,304,334,365},
                                  {31,60,91,121,152,182,213,244,274,305,335,366}};

void GASDateProto::DateGetMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    for (int i = 0; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            fn.Result->SetNumber( GASNumber(i) );
            return;
        }
    fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][0])
    {
        fn.Result->SetNumber( GASNumber(1 + pThis->LJDate) );
        return;
    }

    for (int i = 1; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            fn.Result->SetNumber( GASNumber(1 + pThis->LJDate - months[IsLeapYear(pThis->LYear)][i-1]) );
            return;
        }
    fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetDay(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber((4 + SInt(pThis->LDate / 86400000)) % 7) );
}

void GASDateProto::DateGetTime(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber((GASNumber)pThis->Date);
}

void GASDateProto::DateGetHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LTime / 3600000) );
}

void GASDateProto::DateGetMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber((pThis->LTime % 3600000) / 60000) );
}

void GASDateProto::DateGetSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber((pThis->LTime % 60000) / 1000) );
}

void GASDateProto::DateGetMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LTime % 1000) );
}

void GASDateProto::DateGetYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LYear - 1900) );
}

void GASDateProto::DateGetFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LYear) );
}

void GASDateProto::DateSetYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (newVal < 100 && newVal >= 0)
        newVal += 1900;
    if (pThis->LJDate >= 60)
        pThis->LJDate += IsLeapYear(newVal) - IsLeapYear(pThis->LYear);
    pThis->LDate = SInt64(pThis->LTime) + SInt64(StartOfYear(newVal) + pThis->LJDate) * SInt64(86400000);
    pThis->LYear = newVal;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (pThis->LJDate >= 60)
        pThis->LJDate += IsLeapYear(newVal) - IsLeapYear(pThis->LYear);
    pThis->LDate = SInt64(pThis->LTime) + SInt64(StartOfYear(newVal) + pThis->LJDate) * SInt64(86400000);
    pThis->LYear = newVal;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    DateGetDate(fn);
    int d = (int) fn.Result->ToNumber(fn.Env); //current date
    int oldLJDate =  pThis->LJDate;
    int monthStart = 0;
    if (newVal > 0)
    {
        monthStart = months[IsLeapYear(pThis->LYear)][newVal - 1];
    }
    int daysInMonth = months[IsLeapYear(pThis->LYear)][newVal] - monthStart;
    if (d > daysInMonth)
        d = daysInMonth;
    pThis->LJDate = monthStart + d - 1;
    pThis->LDate += SInt64(pThis->LJDate - oldLJDate) * SInt64(86400000);

    fn.Result->SetUndefined();
}

void GASDateProto::DateSetDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    for (int i = 0; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            int d = newVal - (pThis->LJDate - (i ? months[IsLeapYear(pThis->LYear)][i-1] : 0)) - 1;
            pThis->LJDate += d;
            pThis->LDate += SInt64(d) * SInt64(86400000);
            pThis->UpdateGMT();
            return;
        }
}

void GASDateProto::DateSetTime(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    pThis->SetDate((SInt64) fn.Arg(0).ToNumber(fn.Env));
}

void GASDateProto::DateSetHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - pThis->LTime / 3600000) * 3600000;
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->LTime % 3600000) / 60000)) * 60000;
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->LTime % 60000) / 1000)) * 1000;
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = newVal - (pThis->LTime % 1000);
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}


void GASDateProto::DateGetUTCMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    for (int i = 0; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            fn.Result->SetNumber(GASNumber(i));
            return;
        }
        fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetUTCDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (pThis->JDate < months[IsLeapYear(pThis->Year)][0])
    {
        fn.Result->SetNumber(GASNumber(1 + pThis->JDate));
        return;
    }

    for (int i = 1; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            fn.Result->SetNumber(GASNumber(1 + pThis->JDate - months[IsLeapYear(pThis->Year)][i-1]));
            return;
        }
        fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetUTCDay(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber((4 + SInt(pThis->Date / 86400000)) % 7));
}

void GASDateProto::DateGetUTCHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Time / 3600000));
}

void GASDateProto::DateGetUTCMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber((pThis->Time % 3600000) / 60000));
}

void GASDateProto::DateGetUTCSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber((pThis->Time % 60000) / 1000));
}

void GASDateProto::DateGetUTCMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Time % 1000));
}

void GASDateProto::DateGetUTCYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Year - 1900));
}

void GASDateProto::DateGetUTCFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Year));
}

void GASDateProto::DateSetUTCYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (newVal < 100 && newVal >= 0)
        newVal += 1900;
    if (pThis->JDate >= 60)
        pThis->JDate += IsLeapYear(newVal) - IsLeapYear(pThis->Year);
    pThis->Date = SInt64(pThis->Time) + SInt64(StartOfYear(newVal) + pThis->JDate) * SInt64(86400000);
    pThis->Year = newVal;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (pThis->JDate >= 60)
        pThis->JDate += IsLeapYear(newVal) - IsLeapYear(pThis->Year);
    pThis->Date = SInt64(pThis->Time) + SInt64(StartOfYear(newVal) + pThis->JDate) * SInt64(86400000);
    pThis->Year = newVal;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    for (int i = 0; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            int d = months[IsLeapYear(pThis->Year)][newVal] - months[IsLeapYear(pThis->Year)][i];
            pThis->JDate += d;
            pThis->Date += SInt64(d) * SInt64(86400000);
            pThis->UpdateLocal();
            return;
        }
}

void GASDateProto::DateSetUTCDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    for (int i = 0; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            int d = newVal - (pThis->JDate - (i ? months[IsLeapYear(pThis->Year)][i-1] : 0)) - 1;
            pThis->JDate += d;
            pThis->Date += SInt64(d) * SInt64(86400000);
            pThis->UpdateLocal();
            return;
        }
}
void GASDateProto::DateSetUTCHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - pThis->Time / 3600000) * 3600000;
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->Time % 3600000) / 60000)) * 60000;
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->Time % 60000) / 1000)) * 1000;
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = newVal - (pThis->Time % 1000);
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

static const char *daynames[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *monthnames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void GASDateProto::DateToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);
    SInt month=0, day=0;

    for (int i = 0; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            month = i;
            day = 1 + pThis->LJDate - (i ? months[IsLeapYear(pThis->LYear)][i-1] : 0);
            break;
        }

    char out[128];
    // 01/01/1970 is Thursday
    // We add offset of 4 since the start day is Thursday
    const char* dayname = (pThis->LDate >= 0) ? daynames[(4 + pThis->LDate / 86400000) % 7] :
                                                daynames[(7 + (3 - G_Abs(pThis->LDate) / 86400000) % 7) %7];
    //G_sprintf(out, sizeof(out), 
    //    "%s %s %2d %02d:%02d:%02d GMT%+03d%02d %d", dayname, monthnames[month], day, 
    //    pThis->LTime / 3600000, (pThis->LTime % 3600000) / 60000, (pThis->LTime % 60000) / 1000,
    //    pThis->LocalOffset / 3600000, (pThis->LocalOffset % 3600000) / 60000, pThis->LYear);

    G_Format(GStringDataPtr(out, sizeof(out)), 
        "{0} {1} {2:2} {3:02}:{4:02}:{5:02} GMT{6:+03}{7:02} {8}", 
        dayname, 
        monthnames[month], 
        day, 
        pThis->LTime / 3600000, 
        (pThis->LTime % 3600000) / 60000, 
        (pThis->LTime % 60000) / 1000,
        pThis->LocalOffset / 3600000, 
        (pThis->LocalOffset % 3600000) / 60000, 
        pThis->LYear
        );

    fn.Result->SetString(fn.Env->CreateString(out));
}

void GASDateProto::DateValueOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date);
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber((GASNumber)pThis->Date);
}

static const GASNameFunction GAS_DateFunctionTable[] = 
{
    { "getDate",            &GASDateProto::DateGetDate             },
    { "getDay",             &GASDateProto::DateGetDay              },
    { "getFullYear",        &GASDateProto::DateGetFullYear         },
    { "getHours",           &GASDateProto::DateGetHours            },
    { "getMilliseconds",    &GASDateProto::DateGetMilliseconds     },
    { "getMinutes",         &GASDateProto::DateGetMinutes          },
    { "getMonth",           &GASDateProto::DateGetMonth            },
    { "getSeconds",         &GASDateProto::DateGetSeconds          },
    { "getTime",            &GASDateProto::DateGetTime             },
    { "getTimezoneOffset",  &GASDateProto::DateGetTimezoneOffset   },
    { "getYear",            &GASDateProto::DateGetYear             },
    { "getUTCDate",         &GASDateProto::DateGetUTCDate          },
    { "getUTCDay",          &GASDateProto::DateGetUTCDay           },
    { "getUTCFullYear",     &GASDateProto::DateGetUTCFullYear      },
    { "getUTCHours",        &GASDateProto::DateGetUTCHours         },
    { "getUTCMilliseconds", &GASDateProto::DateGetUTCMilliseconds  },
    { "getUTCMinutes",      &GASDateProto::DateGetUTCMinutes       },
    { "getUTCMonth",        &GASDateProto::DateGetUTCMonth         },
    { "getUTCSeconds",      &GASDateProto::DateGetUTCSeconds       },
    { "getUTCYear",         &GASDateProto::DateGetUTCYear          },

    { "setDate",            &GASDateProto::DateSetDate             },
    { "setFullYear",        &GASDateProto::DateSetFullYear         },
    { "setHours",           &GASDateProto::DateSetHours            },
    { "setMilliseconds",    &GASDateProto::DateSetMilliseconds     },
    { "setMinutes",         &GASDateProto::DateSetMinutes          },
    { "setMonth",           &GASDateProto::DateSetMonth            },
    { "setSeconds",         &GASDateProto::DateSetSeconds          },
    { "setTime",            &GASDateProto::DateSetTime             },
    { "setYear",            &GASDateProto::DateSetYear             },
    { "setUTCDate",         &GASDateProto::DateSetUTCDate          },
    { "setUTCFullYear",     &GASDateProto::DateSetUTCFullYear      },
    { "setUTCHours",        &GASDateProto::DateSetUTCHours         },
    { "setUTCMilliseconds", &GASDateProto::DateSetUTCMilliseconds  },
    { "setUTCMinutes",      &GASDateProto::DateSetUTCMinutes       },
    { "setUTCSeconds",      &GASDateProto::DateSetUTCSeconds       },
    { "setUTCMonth",        &GASDateProto::DateSetUTCMonth         },
    { "setUTCYear",         &GASDateProto::DateSetUTCYear          },

    { "toString",     &GASDateProto::DateToString     },
    { "valueOf",      &GASDateProto::DateValueOf      },
    { 0, 0 }
};

GASDateProto::GASDateProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor)
    : GASPrototype<GASDate>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_DateFunctionTable);
}

//////////////////
const GASNameFunction GASDateCtorFunction::StaticFunctionTable[] = 
{
    { "UTC", &GASDateCtorFunction::DateUTC },
    { 0, 0 }
};

GASDateCtorFunction::GASDateCtorFunction(GASStringContext *psc) 
: GASCFunctionObject(psc, GlobalCtor)
{
    GASNameFunction::AddConstMembers(
        this, psc, StaticFunctionTable, 
        GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

void GASDateCtorFunction::DateUTC (const GASFnCall& fn)
{
    if (fn.NArgs >= 2)
    {
        SInt i = (SInt)fn.Arg(0).ToNumber(fn.Env);
        SInt y = i > 99 || i < 0 ? i : 1900 + i;
        GASNumber days = GASNumber(StartOfYear(y));
        GASNumber ms = 0;
        i = (SInt)fn.Arg(1).ToNumber(fn.Env);
        if (i)
            days += months[IsLeapYear(y)][i-1];
        if (fn.NArgs >= 3)
            days += (SInt)fn.Arg(2).ToNumber(fn.Env)-1;

        if (fn.NArgs >= 4)
            ms += fn.Arg(3).ToNumber(fn.Env) * 3600000;
        if (fn.NArgs >= 5)
            ms += fn.Arg(4).ToNumber(fn.Env) * 60000;
        if (fn.NArgs >= 6)
            ms += fn.Arg(5).ToNumber(fn.Env) * 1000;
        if (fn.NArgs >= 7)
            ms += fn.Arg(6).ToNumber(fn.Env);

        fn.Result->SetNumber(ms + days * 86400000);
    }
    else
        fn.Result->SetNumber(0);
}

void GASDateCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASDate> pdateObj;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == GASObject::Object_Date)
        pdateObj = static_cast<GASDate*>(fn.ThisPtr);
    else
        pdateObj = *GHEAP_NEW(fn.Env->GetHeap()) GASDate(fn.Env);

#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
    struct timeb t;
    ftime(&t);
    TIME_ZONE_INFORMATION tz;
    DWORD r = GetTimeZoneInformation(&tz);
    long bias = tz.Bias;
    if( r == TIME_ZONE_ID_STANDARD )
    { 
        bias += tz.StandardBias; 
    } 
    else if( r == TIME_ZONE_ID_DAYLIGHT)
    { 
        bias += tz.DaylightBias; 
    } 
    // else leave the bias alone
    pdateObj->LocalOffset = -60000 * bias;
    pdateObj->SetDate(SInt64(t.time) * 1000 + t.millitm);
#elif defined(GFC_OS_WINCE)
    SYSTEMTIME stime;
    GetSystemTime(&stime);
    FILETIME ftime;
    SystemTimeToFileTime(&stime, &ftime);

    //!AB: FILETIME is a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601.
    // Need to convert it to milliseconds since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC).
    // 11644473600000 is a number of milliseconds between Jan 1, 1601 and Jan 1, 1970.
    UInt64 mstime = ((UInt64(ftime.dwHighDateTime) << 32) | ftime.dwLowDateTime) / 10000 - 11644473600000i64;

    TIME_ZONE_INFORMATION tz;
    DWORD r = GetTimeZoneInformation(&tz);
    long bias = tz.Bias;
    if (r == TIME_ZONE_ID_STANDARD)
    { 
        bias += tz.StandardBias; 
    } 
    else if (r == TIME_ZONE_ID_DAYLIGHT)
    { 
        bias += tz.DaylightBias; 
    } 
    // else leave the bias alone
    pdateObj->LocalOffset = -60000 * bias;
    pdateObj->SetDate(mstime);
#elif defined(GFC_OS_PS3)
    CellRtcTick t, localt;
    cellRtcGetCurrentTick(&t);
    cellRtcConvertUtcToLocalTime(&t, &localt);
    pdateObj->LocalOffset = ((localt.tick/1000)-(t.tick/1000));
    pdateObj->SetDate(SInt64(t.tick/1000 - 62135596800000ull));

#elif defined(GFC_OS_PS2)
    unsigned char t[8];
    sceDevctl("cdrom0:", CDIOC_READCLOCK, NULL, 0, t, 8);

    SInt y = 2000 + (t[7]&0xf) + 10*((t[7]>>4)&0xf);
    SInt32 days = StartOfYear(y), ms = 0;
    SInt i = (t[6]&0xf) + 10*((t[6]>>4)&0xf);
    if (i > 1)
        days += months[IsLeapYear(y)][i-2];

    days += (t[5]&0xf) + 10*((t[5]>>4)&0xf) - 1;
    ms += ((t[3]&0xf) + 10*((t[3]>>4)&0xf)) * 3600000;
    ms += ((t[2]&0xf) + 10*((t[2]>>4)&0xf)) * 60000;
    ms += ((t[1]&0xf) + 10*((t[1]>>4)&0xf)) * 1000;

    pdateObj->LocalOffset = 0;
    pdateObj->LJDate = days - StartOfYear(y);
    pdateObj->LYear = y;
    pdateObj->LTime = ms;
    pdateObj->LDate = SInt64(days) * SInt64(86400000) + ms;
    pdateObj->UpdateGMT();
    pdateObj->SetDate(pdateObj->Date);

#elif defined(GFC_OS_PSP)
    SceRtcTick t, localt;
    sceRtcGetCurrentTick(&t);
    sceRtcConvertUtcToLocalTime(&t, &localt);
    pdateObj->LocalOffset = ((localt.tick/1000)-(t.tick/1000));
    pdateObj->SetDate(SInt64(t.tick/1000 - 62135596800000ull));

#elif defined(GFC_OS_WII)
    SInt64 t = OSTicksToMilliseconds(OSGetTime());
    pdateObj->LocalOffset = 0;
    pdateObj->SetDate(t + 946684800000LL);

#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)
    pdateObj->LocalOffset = 0;
    pdateObj->SetDate(1000 * SInt64(dk_tm_getCurrentTime()));
#else
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    long bias = tz.tz_minuteswest - (tz.tz_dsttime ? 60 : 0);
    pdateObj->LocalOffset = -60000 * bias;
    pdateObj->SetDate(SInt64(tv.tv_sec) * 1000 + SInt64(tv.tv_usec/1000));
#endif

    if (fn.NArgs == 1)
        pdateObj->SetDate((SInt64)fn.Arg(0).ToNumber(fn.Env));
    else if (fn.NArgs >= 2)
    {
        SInt i = (SInt)fn.Arg(0).ToNumber(fn.Env);
        SInt y = i > 99 || i < 0 ? i : 1900 + i;
        SInt32 days = StartOfYear(y), ms = 0;
        i = (SInt)fn.Arg(1).ToNumber(fn.Env);
        if (i)
            days += months[IsLeapYear(y)][i-1];
        if (fn.NArgs >= 3)
            days += (SInt)fn.Arg(2).ToNumber(fn.Env)-1;
        
        if (fn.NArgs >= 4)
            ms += (SInt)fn.Arg(3).ToNumber(fn.Env) * 3600000;
        if (fn.NArgs >= 5)
            ms += (SInt)fn.Arg(4).ToNumber(fn.Env) * 60000;
        if (fn.NArgs >= 6)
            ms += (SInt)fn.Arg(5).ToNumber(fn.Env) * 1000;
        if (fn.NArgs >= 7)
            ms += (SInt)fn.Arg(6).ToNumber(fn.Env);

        pdateObj->LJDate = days - StartOfYear(y);
        pdateObj->LYear = y;
        pdateObj->LTime = ms;
        pdateObj->LDate = SInt64(days) * SInt64(86400000) + ms;
        pdateObj->UpdateGMT();
        pdateObj->SetDate(pdateObj->Date);
    }
    fn.Result->SetAsObject(pdateObj);
    fn.Result->SetString(fn.Result->ToString(fn.Env));
}

GASObject* GASDateCtorFunction::CreateNewObject(GASEnvironment* penv) const 
{
    return GHEAP_NEW(penv->GetHeap()) GASDate(penv);
}

GASFunctionRef GASDateCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASDateCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASDateProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_Date, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_Date), GASValue(ctor));
    return ctor;
}

#else

void GASDate_DummyFunction() {}   // Exists to quelch compiler warning

#endif // GFC_NO_FXPLAYER_AS_DATE

/**********************************************************************

Filename    :   GFxASTextSnapshot.cpp
Content     :   TextSnapshot reference class for ActionScript 2.0
Created     :   12/10/2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GConfig.h"

#ifndef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

#include "AS/GASTextSnapshot.h"

#include "GFxSprite.h"

#include "AS/GASArrayObject.h"

GASTextSnapshotObject::GASTextSnapshotObject(GASEnvironment* penv)
: GASObject(penv)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_TextSnapshot));
}

GASTextSnapshotObject::~GASTextSnapshotObject()
{

}

void GASTextSnapshotObject::Process(GFxSprite* pmovieClip)
{
    pmovieClip->GetTextSnapshot(&SnapshotData);
}

// --------------------------------------------------------------------

const GASNameFunction GASTextSnapshotProto::FunctionTable[] = 
{
    { "findText", &GASTextSnapshotProto::FindText },
    { "getCount", &GASTextSnapshotProto::GetCount },
    { "getSelected", &GASTextSnapshotProto::GetSelected },
    { "getSelectedText", &GASTextSnapshotProto::GetSelectedText },
    { "getText", &GASTextSnapshotProto::GetText },
    { "getTextRunInfo", &GASTextSnapshotProto::GetTextRunInfo },
    { "hitTestTextNearPos", &GASTextSnapshotProto::HitTestTextNearPos },
    { "setSelectColor", &GASTextSnapshotProto::SetSelectColor },
    { "setSelected", &GASTextSnapshotProto::SetSelected },
    { 0, 0 }
};


GASTextSnapshotProto::GASTextSnapshotProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
GASPrototype<GASTextSnapshotObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, FunctionTable, GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete);
}


//
// findText (TextSnapshot.findText method)
//
// public findText(startIndex:Number, textToFind:String, caseSensitive:Boolean) : Number
//
// Searches the specified TextSnapshot object and returns the position 
// of the first occurrence of textToFind found at or after startIndex. 
// If textToFind is not found, the method returns -1.
//
void    GASTextSnapshotProto::FindText(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs < 3)
        return;

    UInt32      start = fn.Arg(0).ToUInt32(fn.Env);
    GASString   text = fn.Arg(1).ToString(fn.Env);
    bool        bcaseSensitive = fn.Arg(2).ToBool(fn.Env);

    GString     query(text.ToCStr(), text.GetSize());
    fn.Result->SetInt( pthis->GetData().FindText(start, query, bcaseSensitive) );
}

//
// getCount (TextSnapshot.getCount method)
//
// public getCount() : Number
// 
// Returns the number of characters in a TextSnapshot object.
//
void    GASTextSnapshotProto::GetCount(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    fn.Result->SetUInt((UInt)pthis->GetData().GetCharCount());
}


//
// getSelected (TextSnapshot.getSelected method)
//
// public getSelected(start:Number, [end:Number]) : Boolean
//
// Returns a Boolean value that specifies whether a TextSnapshot 
// object contains selected text in the specified range. To search 
// all characters, pass a value of 0 for start, and TextSnapshot.getCount() 
// (or any very large number) for end. To search a single character, pass 
// the end parameter a value that is one greater than the start parameter.
//
void    GASTextSnapshotProto::GetSelected(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs < 1)
        return;

    UInt32  start = fn.Arg(0).ToUInt32(fn.Env);
    UInt32  end = (fn.NArgs > 1) ? fn.Arg(1).ToUInt32(fn.Env) : 
        UInt32(pthis->GetData().GetCharCount());

    if (end <= start)
        end = start + 1;

    fn.Result->SetBool(pthis->GetData().IsSelected(start, end));
}


//
// getSelectedText (TextSnapshot.getSelectedText method)
//
// public getSelectedText([includeLineEndings:Boolean]) : String
// 
// Returns a string that contains all the characters specified by the 
// corresponding TextSnapshot.setSelected() method. If no characters 
// are specified (by the TextSnapshot.setSelected() method), an empty 
// string is returned. If you pass true for includeLineEndings, newline 
// characters are inserted in the return string, and the return string 
// might be longer than the input range. If includeLineEndings is false 
// or omitted, the method returns the selected text without adding any 
// characters.
//
void    GASTextSnapshotProto::GetSelectedText(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    bool    binclLineEndings = (fn.NArgs > 0) ? fn.Arg(0).ToBool(fn.Env) : false;

    GString   selectedText = pthis->GetData().GetSelectedText(binclLineEndings);
    fn.Result->SetString(fn.Env->CreateString(selectedText));
}


//
// getText (TextSnapshot.getText method)
//
// public getText(start:Number, end:Number, [includeLineEndings:Boolean]) : String
//
// Returns a string that contains all the characters specified by the 
// start and end parameters. If no characters are specified, the method 
// returns an empty string. 
// 
// To return all characters, pass a value of 0 for start, and TextSnapshot.getCount() 
// (or any very large number) for end. To return a single character, pass a value of 
// start +1 for end. 
// 
// If you pass true for includeLineEndings, newline characters are inserted in the 
// return string where deemed appropriate, and the return string might be longer than 
// the input range. If includeLineEndings is false or omitted, the method returns the 
// selected text without adding any characters.
// 
void    GASTextSnapshotProto::GetText(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs < 2)
        return;

    UInt32  start = fn.Arg(0).ToUInt32(fn.Env);
    UInt32  end = fn.Arg(1).ToUInt32(fn.Env);
    bool    bincLineEndings = (fn.NArgs < 3) ? false : fn.Arg(2).ToBool(fn.Env);

    if (end <= start)
        end = start + 1;

    GString   subStr = pthis->GetData().GetSubString(start, end, bincLineEndings);
    fn.Result->SetString(fn.Env->CreateString(subStr));
}



class GASTextSnapshotGlyphVisitor : public GFxStaticTextSnapshotData::GlyphVisitor
{
public:
    GASTextSnapshotGlyphVisitor(GASEnvironment* penv, GASArrayObject* parr)
        : pEnv(penv), pArrayObj(parr) {}

    void    OnVisit()
    {
        // Create new object
        GPtr<GASObject> pobj = *GHEAP_NEW(pEnv->GetHeap()) GASObject(pEnv);

        // Fill object
        GASValue val;
        val.SetUInt((UInt)GetRunIndex());
        pobj->SetMember(pEnv, pEnv->CreateConstString("indexInRun"), val);
        pobj->SetMember(pEnv, pEnv->CreateConstString("font"), pEnv->CreateString(GetFontName()));
        // Setting UInt below produces a negative number..??
        val.SetNumber((Float)GetColor().ToColor32());
        pobj->SetMember(pEnv, pEnv->CreateConstString("color"), val);
        val.SetNumber(GetHeight());
        pobj->SetMember(pEnv, pEnv->GetBuiltin(GASBuiltin_height), val);
        val.SetBool(IsSelected());
        pobj->SetMember(pEnv, pEnv->CreateConstString("selected"), val);

        const GMatrix2D& mat = GetMatrix();
        val.SetNumber(TwipsToPixels(mat.M_[0][0]));
        pobj->SetMember(pEnv, pEnv->CreateString("matrix_a"), val);
        val.SetNumber(TwipsToPixels(mat.M_[1][0]));
        pobj->SetMember(pEnv, pEnv->CreateString("matrix_b"), val);
        val.SetNumber(TwipsToPixels(mat.M_[0][1]));
        pobj->SetMember(pEnv, pEnv->CreateString("matrix_c"), val);
        val.SetNumber(TwipsToPixels(mat.M_[1][1]));
        pobj->SetMember(pEnv, pEnv->CreateString("matrix_d"), val);
        val.SetNumber(TwipsToPixels(mat.M_[0][2]));
        pobj->SetMember(pEnv, pEnv->CreateString("matrix_tx"), val);
        val.SetNumber(TwipsToPixels(mat.M_[1][2]));
        pobj->SetMember(pEnv, pEnv->CreateString("matrix_ty"), val);

        const GPointF& bl = GetCorners().BottomLeft();
        const GPointF& br = GetCorners().BottomRight();
        const GPointF& tl = GetCorners().TopLeft();
        const GPointF& tr = GetCorners().TopRight();
        val.SetNumber(TwipsToPixels(bl.x));
        pobj->SetMember(pEnv, pEnv->CreateString("corner0x"), val);
        val.SetNumber(TwipsToPixels(bl.y));
        pobj->SetMember(pEnv, pEnv->CreateString("corner0y"), val);
        val.SetNumber(TwipsToPixels(br.x));
        pobj->SetMember(pEnv, pEnv->CreateString("corner1x"), val);
        val.SetNumber(TwipsToPixels(br.y));
        pobj->SetMember(pEnv, pEnv->CreateString("corner1y"), val);
        val.SetNumber(TwipsToPixels(tr.x));
        pobj->SetMember(pEnv, pEnv->CreateString("corner2x"), val);
        val.SetNumber(TwipsToPixels(tr.y));
        pobj->SetMember(pEnv, pEnv->CreateString("corner2y"), val);
        val.SetNumber(TwipsToPixels(tl.x));
        pobj->SetMember(pEnv, pEnv->CreateString("corner3x"), val);
        val.SetNumber(TwipsToPixels(tl.y));
        pobj->SetMember(pEnv, pEnv->CreateString("corner3y"), val);

        // Add to array
        pArrayObj->PushBack(GASValue(pobj));
    }

private:
    GASEnvironment*     pEnv;
    GASArrayObject*     pArrayObj;
};

//
// getTextRunInfo (TextSnapshot.getTextRunInfo method)
//
// public getTextRunInfo(beginIndex:Number, endIndex:Number) : Array
//
// Returns an array of objects that contains information about a run 
// of text. Each object corresponds to one character in the range of 
// characters specified by the two method parameters. 
//
// Note: Using the getTextRunInfo() method for a large range of text 
// can return a large object. Macromedia recommends limiting the text 
// range defined by the beginIndex and endIndex parameters. 
//
void    GASTextSnapshotProto::GetTextRunInfo(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs < 2)
        return;

    UInt32  start = fn.Arg(0).ToUInt32(fn.Env);
    UInt32  end = fn.Arg(1).ToUInt32(fn.Env);

    GPtr<GASArrayObject> parr = *GHEAP_NEW(fn.Env->GetHeap()) GASArrayObject(fn.Env);

    //
    // Returns
    // Array - An array of objects in which each object contains 
    // information about a specific character in the specified range. 
    // Each object contains the following properties: 
    //
    // indexInRun - A zero-based integer index of the character 
    //              (relative to the entire string rather than the 
    //              selected run of text). 
    // selected - A Boolean value that indicates whether the character 
    //            is selected true; false otherwise. 
    // font     - The name of the character's font. 
    // color    - The combined alpha and color value of the character. 
    //            The first two hexidecimal digits represent the alpha 
    //            value, and the remaining digits represent the color 
    //            value. (The example includes a method for converting 
    //            decimal values to hexidecimal values.) 
    // height   - The height of the character, in pixels. 
    // matrix_a, matrix_b, matrix_c, matrix_d, matrix_tx, and matrix_ty - 
    //          The values of a matrix that define the geometric 
    //          transformation on the character. Normal, upright text 
    //          always has a matrix of the form [1 0 0 1 x y], where x 
    //          and y are the position of the character within the 
    //          parent movie clip, regardless of the height of the text. 
    //          The matrix is in the parent movie clip coordinate system, 
    //          and does not include any transformations that may be on 
    //          that movie clip itself (or its parent). 
    // corner0x, corner0y, corner1x, corner1y, corner2x, corner2y, 
    // corner3x, and corner3y - 
    //          The corners of the bounding box of the character, based 
    //          on the coordinate system of the parent movie clip. These 
    //          values are only available if the font used by the 
    //          character is embedded in the SWF file. 
    //
    GASTextSnapshotGlyphVisitor visitor(fn.Env, parr);
    pthis->GetData().Visit(&visitor, start, end);

    fn.Result->SetAsObject(parr);
}


//
// hitTestTextNearPos (TextSnapshot.hitTestTextNearPos method)
//
// public hitTestTextNearPos(x:Number, y:Number, [closeDist:Number]) : Number
//
// Lets you determine which character within a TextSnapshot object is 
// on or near the specified x, y coordinates of the movie clip containing 
// the text in the TextSnapshot object. 
// 
// If you omit or pass a value of 0 for closeDist, the location 
// specified by the x, y coordinates must lie inside the bounding box 
// of the TextSnapshot object. 
//
// This method works correctly only with fonts that include character 
// metric information; however, by default, Macromedia Flash does not 
// include this information for static text fields. Therefore, the 
// method might return -1 instead of an index value. To ensure that an 
// index value is returned, you can force the Flash authoring tool to 
// include the character metric information for a font. To do this, 
// add a dynamic text field that uses that font, select Character 
// Options for that dynamic text field, and then specify that font 
// outlines should be embedded for at least one character. (It doesn't 
// matter which characters you specify, nor whether they are the 
// characters used in the static text fields.)
//
void    GASTextSnapshotProto::HitTestTextNearPos(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs < 2)
        return;

    Float   x = (Float)fn.Arg(0).ToNumber(fn.Env);
    Float   y = (Float)fn.Arg(1).ToNumber(fn.Env);
    Float   closeDist = (fn.NArgs > 2) ? (Float)fn.Arg(2).ToNumber(fn.Env) : 0;

    // Returns the char Index in the snapshot, if cursor is over
    // static text char
    SInt idx = pthis->GetData().HitTestTextNearPos(Float(PixelsToTwips(x)), 
        Float(PixelsToTwips(y)), (Float)(PixelsToTwips(closeDist)));
    fn.Result->SetNumber(GASNumber(idx));
}


//
// setSelectColor (TextSnapshot.setSelectColor method)
//
// public setSelectColor(color:Number) : Void
//
// Specifies the color to use when highlighting characters that were 
// selected with the TextSnapshot.setSelected() method. The color is 
// always opaque; you can't specify a transparency value. 
//
// This method works correctly only with fonts that include character 
// metric information; however, by default, Macromedia Flash does not 
// include this information for static text fields. Therefore, the 
// method might return -1 instead of an index value. To ensure that an 
// index value is returned, you can force the Flash authoring tool to 
// include the character metric information for a font. To do this, 
// add a dynamic text field that uses that font, select Character 
// Options for that dynamic text field, and then specify that font 
// outlines should be embedded for at least one character. (It doesn't 
// matter which characters you specify, nor if they are the characters 
// used in the static text fields.)
//
void    GASTextSnapshotProto::SetSelectColor(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs < 1)
        return;

    UInt32  c32 = fn.Arg(0).ToUInt32(fn.Env);

    GColor color(c32);
    color.SetAlpha(255);
    pthis->SetSelectColor(color);
}


//
// setSelected (TextSnapshot.setSelected method)
//
// public setSelected(start:Number, end:Number, select:Boolean) : Void
//
// Specifies a range of characters in a TextSnapshot object to be 
// selected or not. Characters that are selected are drawn with a 
// colored rectangle behind them, matching the bounding box of the 
// character. The color of the bounding box is defined by 
// TextSnapshot.setSelectColor(). 
//
// To select or deselect all characters, pass a value of 0 for start 
// and TextSnapshot.getCount() (or any very large number) for end. To 
// specify a single character, pass a value of start + 1 for end. 
// 
// Because characters are individually marked as selected, you can 
// call this method multiple times to select multiple characters; that 
// is, using this method does not deselect other characters that have 
// been set by this method.
// 
// This method works correctly only with fonts that include character 
// metric information; by default, Flash does not include this 
// information for static text fields. Therefore, text that is selected 
// might not appear to be selected onscreen. To ensure that all selected 
// text appears to be selected, you can force the Flash authoring tool 
// to include the character metric information for a font. To do this, 
// in the library, include the font used by the static text field, and 
// in Linkage options for the font, select Export for ActionScript.
//
void    GASTextSnapshotProto::SetSelected(const GASFnCall &fn)
{
    CHECK_THIS_PTR(fn, TextSnapshot);
    GASTextSnapshotObject* pthis = (GASTextSnapshotObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs < 3)
        return;

    UInt32  start = fn.Arg(0).ToUInt32(fn.Env);
    UInt32  end = fn.Arg(1).ToUInt32(fn.Env);
    bool    bselect = fn.Arg(2).ToBool(fn.Env);

    if (end <= start)
        end = start + 1;

    pthis->GetData().SetSelected(start, end, bselect);
}


// --------------------------------------------------------------------


GASTextSnapshotCtorFunction::GASTextSnapshotCtorFunction(GASStringContext* psc)
: GASCFunctionObject(psc, NULL)
{
    GUNUSED(psc);
}


GASFunctionRef GASTextSnapshotCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef ctor(*GHEAP_NEW(pgc->GetHeap()) GASTextSnapshotCtorFunction(&sc));
    GPtr<GASTextSnapshotProto> proto = 
        *GHEAP_NEW(pgc->GetHeap()) GASTextSnapshotProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_TextSnapshot, proto);
    pgc->pGlobal->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_TextSnapshot), GASValue(ctor));
    return ctor;
}


#endif  // GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

/**********************************************************************

Filename    :   AS/GASStyleSheet.h
Content     :   StyleSheet (Textfield.StyleSheet) object functionality
Created     :   May 20, 2008
Authors     :   Prasad Silva
Notes       :   

Copyright   :   (c) 1998-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxText.h"
#include "GFile.h"
#include "GFxPlayerImpl.h"

#include "AS/GASStyleSheet.h"
#include "AS/GASArrayObject.h"
#include "AS/GASAsBroadcaster.h"
#include "AS/GASTextFormat.h"

#ifndef GFC_NO_CSS_SUPPORT
// ********************************************************************

//
// StyleSheet object's file loader implementations
// An instance of this object is sent to the GFx load queue for
// processing. It is thread safe, and therefore allows files
// to be loaded and processed asynchronously.
// This object performs both file loading and parsing
//
class GFxCSSFileLoaderAndParserImpl : public GFxASCSSFileLoader
{
    enum StreamType
    {
        STREAM_UTF8,
        STREAM_UTF16,
    };

    StreamType                  Type;
    UByte*                      pFileData;
    SInt                        FileSize;

public:
    GFxCSSFileLoaderAndParserImpl() : Type(STREAM_UTF8), pFileData(0), FileSize(0) {}
    virtual         ~GFxCSSFileLoaderAndParserImpl();

    //
    // File loader implementation
    //
    void            Load(const GString& filename, GFxFileOpener* pfo);
    void            Init(GASEnvironment* penv, GASObject* pTarget);
};


//
// Destructor
//
GFxCSSFileLoaderAndParserImpl::~GFxCSSFileLoaderAndParserImpl()
{
    if (pFileData != NULL)
        GFREE(pFileData);
}

//
// Loads and parses the file and returns a GFxXMLDocument object
//
void GFxCSSFileLoaderAndParserImpl::Load( const GString& filename, GFxFileOpener* pfo )
{
    // Could be on a separate thread here if thread support is enabled.

    GPtr<GFile> pFile = *pfo->OpenFile(filename);
    if (pFile && pFile->IsValid())
    {
        if ((FileSize = pFile->GetLength()) != 0)
        {
            // Load the file into memory
            pFileData = (UByte*) GALLOC(FileSize+2, GFxStatMV_Text_Mem);
            pFile->Read(pFileData, FileSize);
            pFileData[FileSize] = pFileData[FileSize+1] = 0;
        }
    }
}

//
// Parse the CSS1 stream depending on its encoding
//
void GFxCSSFileLoaderAndParserImpl::Init(GASEnvironment* penv, GASObject* pTarget)
{
    // Back on main thread at this point. Check if target is ok.
    GASStyleSheetObject* pASObj = static_cast<GASStyleSheetObject*>(pTarget);

    if ( !pFileData )
    {
        pASObj->NotifyOnLoad(penv, false);
    }
    else
    {
        wchar_t*            wcsptr = NULL;
        UByte*              ptextData = pFileData;
        SInt                textLength = FileSize;

        // the following converts byte stream to appropriate endianness
        // for UTF16/UCS2 (wide char format)
        UInt16* prefix16 = (UInt16*)pFileData;
        if (prefix16[0] == GByteUtil::BEToSystem((UInt16)0xFFFE)) // little endian
        {
            prefix16++;
            ptextData = (UByte*)prefix16;
            textLength = (FileSize / 2) - 1;
            Type = STREAM_UTF16;
            if (sizeof(wchar_t) == 2)
            {
                for (SInt i=0; i < textLength; i++)
                    prefix16[i] = (wchar_t)GByteUtil::LEToSystem(prefix16[i]);
            }
            else
            {
                // special case: create an aux buffer to hold the data
                wcsptr = (wchar_t*) GALLOC(textLength * sizeof(wchar_t), GFxStatMV_Text_Mem);
                for (SInt i=0; i < textLength; i++)
                    wcsptr[i] = (wchar_t)GByteUtil::LEToSystem(prefix16[i]);
                ptextData = (UByte*)wcsptr;
            }
        }
        else if (prefix16[0] == GByteUtil::BEToSystem((UInt16)0xFEFF)) // big endian
        {
            prefix16++;
            ptextData = (UByte*)prefix16;
            textLength = (FileSize / 2) - 1;
            Type = STREAM_UTF16;
            if (sizeof(wchar_t) == 2)
            {
                for (SInt i=0; i < textLength; i++)
                    prefix16[i] = GByteUtil::BEToSystem(prefix16[i]);
            }
            else
            {
                wcsptr = (wchar_t*) GALLOC(textLength * sizeof(wchar_t), GFxStatMV_Text_Mem);
                for (SInt i=0; i < textLength; i++)
                    wcsptr[i] = GByteUtil::BEToSystem(prefix16[i]);
                ptextData = (UByte*)wcsptr;
            }
        }
        else if (FileSize > 2 && pFileData[0] == 0xEF && pFileData[1] == 0xBB && pFileData[2] == 0xBF)
        {
            // UTF-8 BOM explicitly specified
            ptextData += 3;
            textLength -= 3;
        }

        // else we assume encoding is ASCII or UTF8

        bool ret = false;
        switch (Type)
        {
        case STREAM_UTF16:
            ret = pASObj->CSS.ParseCSS((wchar_t*)ptextData, textLength);
            break;
        default:
            ret = pASObj->CSS.ParseCSS((char*)ptextData, textLength);
        }              

        // Cleanup
        if (wcsptr)
            GFREE(wcsptr);
        GFREE(pFileData);
        pFileData = NULL;

        pASObj->NotifyOnLoad( penv, ret );
    }
}


// ********************************************************************


GASStyleSheetObject::GASStyleSheetObject(GASEnvironment* penv)
: GASObject(penv)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_StyleSheet));

    GASAsBroadcaster::Initialize(penv->GetSC(), this);
    GASAsBroadcaster::AddListener(penv, this, this);
}


// 
// Invoked when a XML.load() or XML.sendAndLoad() operation has ended
//
void GASStyleSheetObject::NotifyOnLoad(GASEnvironment* penv, bool success)
{
    // Flash Doc: Invoked when a load() operation has completed. If the 
    // StyleSheet loaded successfully, the success parameter is true. If 
    // the document was not received, or if an error occurred in receiving 
    // the response from the server, the success parameter is false.

    penv->Push(success);
    GASAsBroadcaster::BroadcastMessage(penv, this, 
        penv->CreateConstString("onLoad"), 1, penv->GetTopIndex());
    penv->Drop1();    
}


////////////////////////////////////////////////
//
static const GASNameFunction GAS_StyleSheetFunctionTable[] = 
{
    { "clear", GASStyleSheetProto::Clear },
    { "getStyle", GASStyleSheetProto::GetStyle },
    { "getStyleNames", GASStyleSheetProto::GetStyleNames },
    { "load", GASStyleSheetProto::Load },
    { "parseCSS", GASStyleSheetProto::ParseCSS },
    { "setStyle", GASStyleSheetProto::SetStyle },
    { "transform", GASStyleSheetProto::Transform },
    { 0, 0 }
};

GASStyleSheetProto::GASStyleSheetProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASStyleSheetObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_StyleSheetFunctionTable);
}


//
// StyleSheet.Clear() : Void
//
void    GASStyleSheetProto::Clear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, StyleSheet);
    GASStyleSheetObject* pthis = (GASStyleSheetObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;    

    // Flash Doc: Removes all styles from the specified StyleSheet object.

    pthis->CSS.ClearStyles();
}


//
// StyleSheet.getStyle(name:String) : Object
//
void    GASStyleSheetProto::GetStyle(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, StyleSheet);
    GASStyleSheetObject* pthis = (GASStyleSheetObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;    

    // Flash Doc: Returns a copy of the style object associated with the specified 
    // style (name). If there is no style object associated with name, the method 
    // returns null.

    if (fn.NArgs < 1)
    {
        fn.Result->SetNull();
        return;
    }
    
    GASString stylename = fn.Arg(0).ToString(fn.Env);
    const GFxTextStyle* pstyle = NULL;
    if (stylename.GetSize() > 0 && stylename[0] == '.')
        pstyle = pthis->CSS.GetStyle(GFxTextStyleKey::CSS_Class, stylename.ToCStr()+1, stylename.GetSize()-1);
    else
        pstyle = pthis->CSS.GetStyle(GFxTextStyleKey::CSS_Tag, stylename.ToCStr(), stylename.GetSize());
    if (!pstyle)
    {
        fn.Result->SetNull();
        return;
    }

    GPtr<GASObject> pobj = *GHEAP_NEW(fn.Env->GetHeap()) GASObject(fn.Env);
    GASStringContext* psc = fn.Env->GetSC();
    // fill in all set properties
    const GFxTextFormat& tf = pstyle->TextFormat;
    const GFxTextParagraphFormat& pf = pstyle->ParagraphFormat;

    // color
    if (tf.IsColorSet())
    {
        GString colorStr;
        // create html color string
        colorStr.AppendChar('#');
        GColor color = tf.GetColor();
        const char* refstr = "0123456789ABCDEF";
        UByte comp = color.GetRed();
        colorStr.AppendChar(refstr[comp >> 4]);
        colorStr.AppendChar(refstr[comp & 0x0F]);
        comp = color.GetGreen();
        colorStr.AppendChar(refstr[comp >> 4]);
        colorStr.AppendChar(refstr[comp & 0x0F]);
        comp = color.GetBlue();
        colorStr.AppendChar(refstr[comp >> 4]);
        colorStr.AppendChar(refstr[comp & 0x0F]);
        pobj->SetMember(fn.Env, psc->CreateString("color"), psc->CreateString(colorStr));
    }

    // display
    // TODO

    // fontFamily
    if (tf.IsFontListSet())
        pobj->SetMember(fn.Env, psc->CreateString("fontFamily"),
            psc->CreateString(tf.GetFontList()));

    // fontSize
    if (tf.IsFontSizeSet())
        pobj->SetMember(fn.Env, psc->CreateString("fontSize"),
        (GASNumber)tf.GetFontSize());

    // fontStyle
    if (tf.IsItalicSet())
        pobj->SetMember(fn.Env, psc->CreateString("fontStyle"), 
        tf.IsItalic() ? psc->CreateString("italic") : psc->CreateString("normal"));

    // fontWeight
    if (tf.IsBoldSet())
        pobj->SetMember(fn.Env, psc->CreateString("fontWeight"), 
        tf.IsBold() ? psc->CreateString("bold") : psc->CreateString("normal"));

    // kerning
    if (tf.IsKerningSet())
        pobj->SetMember(fn.Env, psc->CreateString("kerning"), 
        tf.IsKerning() ? psc->CreateString("true") : psc->CreateString("false"));

    // letterSpacing
    if (tf.IsLetterSpacingSet())
        pobj->SetMember(fn.Env, psc->CreateString("letterSpacing"),
        (GASNumber)tf.GetLetterSpacing());

    // marginLeft
    if (pf.IsLeftMarginSet())
        pobj->SetMember(fn.Env, psc->CreateString("marginLeft"),
        (GASNumber)pf.GetLeftMargin());

    // marginRight
    if (pf.IsRightMarginSet())
        pobj->SetMember(fn.Env, psc->CreateString("marginRight"),
        (GASNumber)pf.GetRightMargin());

    // textAlign
    if (pf.IsAlignmentSet())
        pobj->SetMember( fn.Env, psc->CreateString("textAlign"), 
            pf.IsLeftAlignment() ? psc->CreateString("left") : 
            pf.IsCenterAlignment() ? psc->CreateString("center") :
            pf.IsRightAlignment() ? psc->CreateString("right") : 
            psc->CreateString("justify") );

    // textDecoration
    if (tf.IsUnderlineSet())
        pobj->SetMember(fn.Env, psc->CreateString("textDecoration"), 
        tf.IsUnderline() ? psc->CreateString("underline") : psc->CreateString("none"));

    // textIndent
    if (pf.IsIndentSet())
        pobj->SetMember(fn.Env, psc->CreateString("textIndent"),
        (GASNumber)pf.GetIndent());

    fn.Result->SetAsObject(pobj);
}


//
// StyleSheet.getStyleNames() : Array
//
void    GASStyleSheetProto::GetStyleNames(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, StyleSheet);
    GASStyleSheetObject* pthis = (GASStyleSheetObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;    

    // Flash Doc: Returns an array that contains the names (as strings) of all 
    // of the styles registered in this style sheet.

    GPtr<GASArrayObject> parr = *static_cast<GASArrayObject*>(
        fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Array)));
    GASStringContext* psc = fn.Env->GetSC();
    GString temp;
    for (GFxTextStyleHash::ConstIterator iter = pthis->CSS.GetStyles().Begin(); 
            iter != pthis->CSS.GetStyles().End(); ++iter)
    {
        temp.Clear();
        if (iter->First.Type == GFxTextStyleKey::CSS_Class)
            temp.AppendChar('.');
        temp += iter->First.Value;
        parr->PushBack( psc->CreateString(temp) );
    }
    fn.Result->SetAsObject(parr);
}


//
// StyleSheet.load(url:String) : Boolean
//
void    GASStyleSheetProto::Load(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, StyleSheet);
    GASStyleSheetObject* pthis = (GASStyleSheetObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;    

    // Flash Doc: Starts loading the CSS file into the StyleSheet. The load 
    // operation is asynchronous; use the onLoad() callback handler to determine 
    // when the file has finished loading. The CSS file must reside in the same 
    // domain as the SWF file that is loading it.

    if (fn.NArgs == 0)
    {
        fn.Result->SetBool(false);
        return;
    }
    GASString urlStr(fn.Arg(0).ToString(fn.Env));

    // new ?! MA: Is this the right heap for loader?
    // PPS: Create the loader in the global heap
    GPtr<GFxASCSSFileLoader> pLoader = 
        *new GFxCSSFileLoaderAndParserImpl();
    fn.Env->GetMovieRoot()->AddCssLoadQueueEntry(pthis, pLoader, urlStr.ToCStr(), 
        GFxLoadQueueEntry::LM_None);
    fn.Result->SetBool(true);
}


//
// StyleSheet.parseCSS(cssText:String) : Boolean
//
void    GASStyleSheetProto::ParseCSS(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, StyleSheet);
    GASStyleSheetObject* pthis = (GASStyleSheetObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;    

    // Flash Doc: Parses the CSS in cssText and loads the StyleSheet with it. 
    // If a style in cssText is already in the StyleSheet, the StyleSheet 
    // retains its properties, and only the ones in cssText are added or changed. 

    if (fn.NArgs < 1)
    {
        fn.Result->SetBool(false);
        return;
    }

    GASValue csstxt = fn.Arg(0);
    GASString cssstr = csstxt.ToString(fn.Env);
    fn.Result->SetBool( pthis->CSS.ParseCSS( cssstr.ToCStr(), cssstr.GetSize() ) );
}


// 
// Member visitor to collate style properties
//
struct CSSStringBuilder : public GASObject::MemberVisitor
{
    GASEnvironment* pEnv;
    GString& Dest;
    CSSStringBuilder(GASEnvironment* penv, GString& dest) 
        : pEnv(penv), Dest(dest) {}
    CSSStringBuilder& operator=( const CSSStringBuilder& x )
    {
        pEnv = x.pEnv;
        Dest = x.Dest;
        return *this;
    }
    void Visit(const GASString& name, const GASValue& val, UByte flags)
    {
        GUNUSED(flags);
        // translate from Flash prop to CSS1 prop
        if (name == "fontFamily")               Dest += "font-family";
        else if (name == "fontSize")            Dest += "font-size";
        else if (name == "fontStyle")           Dest += "font-style";
        else if (name == "fontWeight")          Dest += "font-weight";
        else if (name == "letterSpacing")       Dest += "letter-spacing";
        else if (name == "marginLeft")          Dest += "margin-left";
        else if (name == "marginRight")         Dest += "margin-right";
        else if (name == "textAlign")           Dest += "text-align";
        else if (name == "textDecoration")      Dest += "text-decoration";
        else if (name == "textIndent")          Dest += "text-indent";
        else                                    Dest += name.ToCStr();
        Dest += ":";
        Dest += val.ToString(pEnv).ToCStr();
        Dest += ";";        
    }
};

//
// StyleSheet.setStyle(name:String, style:Object) : Void
//
void    GASStyleSheetProto::SetStyle(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, StyleSheet);
    GASStyleSheetObject* pthis = (GASStyleSheetObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;    

    // Flash Doc: Adds a new style with the specified name to the StyleSheet 
    // object. If the named style does not already exist in the StyleSheet, 
    // it is added. If the named style already exists in the StyleSheet, it 
    // is replaced. If the style parameter is null, the named style is removed. 

    if (fn.NArgs < 1)
        return;

    if (fn.NArgs < 2 || fn.Arg(1).IsNull())
    {
        // remove style
        GASString stylename = fn.Arg(0).ToString(fn.Env);
        if (stylename.GetSize() > 0)
        {
            if (stylename[0] == '.')
                pthis->CSS.ClearStyle(GFxTextStyleKey::CSS_Class, stylename.ToCStr());
            else
                pthis->CSS.ClearStyle(GFxTextStyleKey::CSS_Tag, stylename.ToCStr());
        }
        return;
    }
    else
    {
        // replace style
        GASString stylename = fn.Arg(0).ToString(fn.Env);
        GASObject* pobj = fn.Arg(1).ToObject(fn.Env);
        if (!pobj)
        {
            GFC_DEBUG_WARNING(1, "Style is not an Object");
            return;
        }
        // build a css string with given info
        GString cssstr;
        cssstr.AppendString(stylename.ToCStr());
        cssstr.AppendChar('{');
        CSSStringBuilder propvis(fn.Env, cssstr);
        pobj->VisitMembers(fn.Env->GetSC(), &propvis, 0);
        cssstr.AppendChar('}');
        // parse it using the textstylemanager
        pthis->CSS.ParseCSS(cssstr.ToCStr(), cssstr.GetSize());
    }
}


// 
// Member visitor to collate style properties
//
struct CSSTextFormatLoader : public GASObject::MemberVisitor
{
    GASEnvironment* pEnv;
    GASTextFormatObject* pTFO;
    CSSTextFormatLoader(GASEnvironment* penv, GASTextFormatObject* dest) 
        : pEnv(penv), pTFO(dest) {}
    CSSTextFormatLoader& operator=( const CSSTextFormatLoader& x )
    {
        pEnv = x.pEnv;
        pTFO = x.pTFO;
        return *this;
    }
    void Visit(const GASString& name, const GASValue& val, UByte flags)
    {
        GUNUSED(flags);
        // switch on valid props
        GASString valstr = val.ToString(pEnv);
        const char* pstr = valstr.ToCStr();
        char* temp = NULL;
        UInt sz = valstr.GetSize();
        if (name == "color")
        {
            UInt32 colval = G_strtol(pstr+1, &temp, 16);
            pTFO->TextFormat.SetColor32(colval);
        }
        else if (name == "display")
        {
            // TODO: Not supported currently
        }
        else if (name == "fontFamily")
        {
            pTFO->TextFormat.SetFontList(pstr, sz);
        }
        else if (name == "fontSize")            
        {
            Float num = (Float)G_strtod(pstr, &temp);
            pTFO->TextFormat.SetFontSize(num);
        }
        else if (name == "fontStyle")           
        {
            if ( !G_strncmp("normal", pstr, (sz < 4) ? sz : 4) ) 
                pTFO->TextFormat.SetItalic(false);        
            else if ( !G_strncmp("italic", pstr, (sz < 9) ? sz : 9) )
                pTFO->TextFormat.SetItalic(true);        
        }
        else if (name == "fontWeight")          
        {
            if ( !G_strncmp("normal", pstr, (sz < 6) ? sz : 6) ) 
                pTFO->TextFormat.SetBold(false);
            else if ( !G_strncmp("bold", pstr, (sz < 4) ? sz : 4) )
                pTFO->TextFormat.SetBold(true);
        }
        else if (name == "kerning")
        {
            if ( !G_strncmp("true", pstr, (sz < 4) ? sz : 4) ) 
                pTFO->TextFormat.SetKerning(false);        
            else if ( !G_strncmp("false", pstr, (sz < 5) ? sz : 5) )
                pTFO->TextFormat.SetKerning(true);        
        }
        else if (name == "letterSpacing")       
        {
            Float num = (Float)G_strtod(pstr, &temp);
            pTFO->TextFormat.SetLetterSpacing(num);
        }
        else if (name == "marginLeft")          
        {
            Float num = (Float)G_strtod(pstr, &temp);
            pTFO->ParagraphFormat.SetLeftMargin((UInt)num);
        }
        else if (name == "marginRight")         
        {
            Float num = (Float)G_strtod(pstr, &temp);
            pTFO->ParagraphFormat.SetRightMargin((UInt)num);
        }
        else if (name == "textAlign")           
        {
            if ( !G_strncmp("left", pstr, (sz < 4) ? sz : 4) ) 
                pTFO->ParagraphFormat.SetAlignment( GFxTextParagraphFormat::Align_Left );
            else if ( !G_strncmp("center", pstr, (sz < 6) ? sz : 6) )
                pTFO->ParagraphFormat.SetAlignment( GFxTextParagraphFormat::Align_Center );
            else if ( !G_strncmp("right", pstr, (sz < 5) ? sz : 5) ) 
                pTFO->ParagraphFormat.SetAlignment( GFxTextParagraphFormat::Align_Right );
            else if ( !G_strncmp("justify", pstr, (sz < 7) ? sz : 7) )
                pTFO->ParagraphFormat.SetAlignment( GFxTextParagraphFormat::Align_Justify );
        }
        else if (name == "textDecoration")      
        {
            if ( !G_strncmp("none", pstr, (sz < 4) ? sz : 4) ) 
                pTFO->TextFormat.SetUnderline(false);        
            else if ( !G_strncmp("underline", pstr, (sz < 9) ? sz : 9) )
                pTFO->TextFormat.SetUnderline(true);        
        }
        else if (name == "textIndent")          
        {
            Float num = (Float)G_strtod(pstr, &temp);
            pTFO->ParagraphFormat.SetIndent((SInt)num);
        }
    }
};

//
// StyleSheet.transform(style:Object) : TextFormat
//
void    GASStyleSheetProto::Transform(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, StyleSheet);
    GASStyleSheetObject* pthis = (GASStyleSheetObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;    

    // Flash Doc: Extends the CSS parsing capability. Advanced developers can 
    // override this method by extending the StyleSheet class.
    // (NOTE: By default this method returns a TextFormat object with
    // the style properties)

    if (fn.NArgs < 1)
        return;

    GASObject* pobj = fn.Arg(0).ToObject(fn.Env);
    if (!pobj)
        return;

    GPtr<GASTextFormatObject> ptfo = *static_cast<GASTextFormatObject*>(
        fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_TextFormat)));
    
    CSSTextFormatLoader tfl(fn.Env, ptfo);
    pobj->VisitMembers(fn.Env->GetSC(), &tfl, 0);

    fn.Result->SetAsObject(ptfo);
}


////////////////////////////////////////////////
//
GASStyleSheetCtorFunction::GASStyleSheetCtorFunction(GASStringContext *psc) :
    GASCFunctionObject(psc, GlobalCtor)
{
    GUNUSED(psc);
}

void GASStyleSheetCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASStyleSheetObject> ab;
    if (fn.ThisPtr && fn.ThisPtr->GetObjectType() == Object_StyleSheet && !fn.ThisPtr->IsBuiltinPrototype())
        ab = static_cast<GASStyleSheetObject*>(fn.ThisPtr);
    else
        ab = *GHEAP_NEW(fn.Env->GetHeap()) GASStyleSheetObject(fn.Env);
    
    fn.Result->SetAsObject(ab.GetPtr());
}

GASObject* GASStyleSheetCtorFunction::CreateNewObject(GASEnvironment* penv) const
{
    return GHEAP_NEW(penv->GetHeap()) GASStyleSheetObject(penv);
}

GASFunctionRef GASStyleSheetCtorFunction::Register(GASGlobalContext* pgc)
{
    GASStringContext sc(pgc, 8);
    GASFunctionRef  ctor(*GHEAP_NEW(pgc->GetHeap()) GASStyleSheetCtorFunction(&sc));
    GPtr<GASObject> proto = *GHEAP_NEW(pgc->GetHeap()) GASStyleSheetProto(&sc, pgc->GetPrototype(GASBuiltin_Object), ctor);
    pgc->SetPrototype(GASBuiltin_StyleSheet, proto);
    
    GASFunctionRef textFieldCtor = static_cast<GASTextFieldProto*>(pgc->GetPrototype(GASBuiltin_TextField))->GetConstructor();
    textFieldCtor->SetMemberRaw(&sc, pgc->GetBuiltin(GASBuiltin_StyleSheet), GASValue(ctor)); 

    return ctor;
}

#else

void GASStyleSheet_DummyFunction() {}   // Exists to quelch compiler warning

#endif // GFC_NO_CSS_SUPPORT


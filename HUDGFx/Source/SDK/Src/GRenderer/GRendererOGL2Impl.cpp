/**********************************************************************

Filename    :   GRendererOGL2Impl.cpp
Content     :   OpenGL 2 / ES 2.0 Sample renderer implementation
Created     :   
Authors     :   Andrew Reisse, Michael Antonov, TU

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Does not support any older GL versions

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GImage.h"
#include "GAtomic.h"
#include "GArray.h"
#include "GHash.h"
#include "GString.h"
#include "GMsgFormat.h"

#if defined(GFC_OS_WIN32)
#include <windows.h>
#endif

#include "GRendererOGL.h"
#include "GRendererCommonImpl.h"

#include <string.h> // for memset()

#if defined(GFC_OS_WIN32)
#include <gl/gl.h>
#include "glext.h"
#define GFC_GL_RUNTIME_LINK(x) wglGetProcAddress(x)

#elif defined(GFC_OS_WINCE)
#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#define GL_BGRA GL_BGRA_EXT
#define GL_TEXTURE_MAX_LEVEL GL_TEXTURE_MAX_LEVEL_SGIS

#elif defined(GFC_OS_MAC)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

#elif defined(GFC_OS_ANDROID)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#elif defined(GFC_OS_IPHONE)
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#else

// Use GLX to link gl extensions at runtime; comment out if not using GLX
#define GFC_GLX_RUNTIME_LINK

#ifdef GFC_GLX_RUNTIME_LINK
#include <GL/glx.h>
#include <GL/glxext.h>
#define GFC_GL_RUNTIME_LINK(x) glXGetProcAddressARB((const GLubyte*) (x))
#else
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
#include <GL/glext.h>
#endif

#if defined(GL_ES_VERSION_2_0)

// Unsupported features
#define GL_MIN GL_FUNC_ADD
#define GL_MAX GL_FUNC_ADD

// standard names
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
#define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE
#define GL_RENDERBUFFER_EXT GL_RENDERBUFFER
#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0
#define GL_DEPTH_ATTACHMENT_EXT GL_DEPTH_ATTACHMENT
#define GL_STENCIL_ATTACHMENT_EXT GL_STENCIL_ATTACHMENT

typedef char GLchar;

#endif

#ifdef GFC_GL_RUNTIME_LINK
#define pRendererEXT pRenderer->
#else
#define pRendererEXT 
#endif


// ***** Classes implemented
class GRendererOGL2Impl;
class GTextureOGL2Impl;


//#define GFC_GL_BLEND_DISABLE

enum VShaderType
{
    VS_None,
    VS_Pos,
    VS_PosTC,
    VS_Glyph,
    VS_XY16iC32,
    VS_XY16iCF32,
    VS_XY16iCF32_T2,

    VS_CountSrc,

#ifndef GFC_NO_3D
    VS_3D_None = VS_CountSrc,
    VS_3D_Pos,
    VS_3D_PosTC,
    VS_3D_Glyph,
    VS_3D_XY16iC32,
    VS_3D_XY16iCF32,
    VS_3D_XY16iCF32_T2,
#endif

    VS_Count
};

// Pixel shaders
enum PixelShaderType
{
    PS_None                 = 0,
    PS_Solid,
    PS_TextTexture,
    PS_TextTextureColor,
    PS_TextTextureColorMultiply,

    PS_TextTextureYUV,
    PS_TextTextureYUVMultiply,
    PS_TextTextureYUVA,
    PS_TextTextureYUVAMultiply,

    PS_CxformTexture,
    PS_CxformTextureMultiply,

    PS_CxformGouraud,
    PS_CxformGouraudNoAddAlpha,
    PS_CxformGouraudTexture,
    PS_Cxform2Texture,

    // Multiplies - must come in same order as other gourauds
    PS_CxformGouraudMultiply,
    PS_CxformGouraudMultiplyNoAddAlpha,
    PS_CxformGouraudMultiplyTexture,
    PS_CxformMultiply2Texture,

    PS_CmatrixTexture,
    PS_CmatrixTextureMultiply,

    PS_Count,
};

static const VShaderType VShaderForPShader[PS_Count] = 
{
    VS_None,
    VS_Pos,
    VS_Glyph,
    VS_Glyph,
    VS_Glyph,

    VS_Glyph,
    VS_Glyph,
    VS_Glyph,
    VS_Glyph,

    VS_PosTC,
    VS_PosTC,

    VS_XY16iCF32,
    VS_XY16iC32,
    VS_XY16iCF32,
    VS_XY16iCF32_T2,

    VS_XY16iCF32,
    VS_XY16iC32,
    VS_XY16iCF32,
    VS_XY16iCF32_T2,

    VS_Glyph,
    VS_Glyph,
};

// ***** GTextureOGL2Impl implementation


// GTextureOGL2Impl declaration
class GTextureOGL2Impl : public GTextureOGL
{
public:

    // Renderer
    GRendererOGL2Impl *  pRenderer;
    // GL texture id
    GLuint              TextureId;
    GLenum              TextureFmt, TextureData;
    bool                DeleteTexture;
#ifdef GFC_GL_NO_ALPHA_TEXTURES
    bool                IsAlpha;
#endif
    SInt                TexWidth, TexHeight;
    GLubyte **          TexData;
    SInt                Mipmaps;

    GTextureOGL2Impl(GRendererOGL2Impl *prenderer);
    ~GTextureOGL2Impl();

    // Obtains the renderer that create TextureInfo 
    virtual GRenderer*  GetRenderer() const;        
    virtual bool        IsDataValid() const;

    // Creates a texture based on parameters    
    bool                InitTextureAlpha(GImageBase* pim);

    virtual bool        InitTexture(UInt texID, bool deleteTexture);
    virtual bool        InitTexture(GImageBase* pim, UInt usage = Usage_Wrap);
    virtual bool        InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage);
    virtual void        Update(int level, int n, const UpdateRect *rects, const GImageBase *pim);

    virtual int         Map(int level, int n, MapRect* maps, int flags);
    virtual bool        Unmap(int level, int n, MapRect* maps, int flags);

    // Helper function
    // Creates a texture id ans sets filter parameters
    void                InitTextureId(GLint minFilter);
    // Releases texture and clears vals
    virtual void        ReleaseTextureId();

    // Remove texture from renderer, notifies of renderer destruction
    void                RemoveFromRenderer();

    virtual UInt        GetNativeTexture() const { return TextureId; }

    virtual int         IsYUVTexture() { return 0; }
    virtual void        Bind(int stage, GRenderer::BitmapWrapMode WrapMode, GRenderer::BitmapSampleMode SampleMode, bool useMipmaps);
};

class GTextureOGL2ImplYUV : public GTextureOGL2Impl
{
public:
    GLuint              TextureUVA[3];

    GTextureOGL2ImplYUV(GRendererOGL2Impl *prenderer) : GTextureOGL2Impl(prenderer)
    { TextureUVA[0] = TextureUVA[1] = TextureUVA[2] = 0; }
    ~GTextureOGL2ImplYUV();

    virtual bool        InitTexture(UInt, bool) { return 0; }
    virtual bool        InitTexture(GImageBase*, UInt) { return 0; }
    virtual bool        InitTextureFromFile(const char*, int, int) { return 0; }
    virtual void        Update(int, int, const UpdateRect *, const GImageBase *) { }

    virtual bool        InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage);
    virtual int         Map(int level, int n, MapRect* maps, int flags);
    virtual bool        Unmap(int level, int n, MapRect* maps, int flags);

    virtual void        ReleaseTextureId();

    virtual int         IsYUVTexture() { return TextureUVA[2] ? 2 : 1; }
    virtual void        Bind(int stage, GRenderer::BitmapWrapMode WrapMode, GRenderer::BitmapSampleMode SampleMode, bool useMipmaps);
};

class GDepthBufferOGL2Impl : public GTextureOGL2Impl
{
public:
    GLuint RBuffer;

    GDepthBufferOGL2Impl(GRendererOGL2Impl *prenderer) : GTextureOGL2Impl(prenderer)
    { RBuffer = 0; }
    ~GDepthBufferOGL2Impl();

    virtual bool        IsDataValid() const { return RBuffer != 0; }
    virtual bool        InitTexture(UInt, bool = 0) { return 0; }
    virtual bool        InitTexture(GImageBase*, UInt = Usage_Wrap) { return 0; }
    virtual void        Update(int, int, const UpdateRect *, const GImageBase *) { }

    virtual bool        InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage);
    virtual int         Map(int, int, MapRect*, int) { return 0; }
    virtual bool        Unmap(int, int, MapRect*, int) { return 0; }

    virtual void        ReleaseTexture();
};

class GRenderTargetOGL2Impl : public GRenderTargetOGL
{
public:
    GPtr<GTextureOGL2Impl>     pTexture;
    GPtr<GDepthBufferOGL2Impl> pDepth, pStencil;
    GRendererOGL2Impl*         pRenderer;
    GLuint                     FboId;
    UInt                       TargetWidth, TargetHeight;
    bool                       IsTemp;

    GRenderTargetOGL2Impl(GRendererOGL2Impl* prenderer);
    ~GRenderTargetOGL2Impl();

    void  ReleaseResources();
    void  RemoveFromRenderer();

    virtual GRenderer*  GetRenderer() const;

    virtual bool       InitRenderTarget(GTexture *ptarget, GTexture* pdepth = 0, GTexture* pstencil = 0);
    virtual bool       InitRenderTarget(UInt FboId);
};

#if (GFC_BYTE_ORDER == GFC_BIG_ENDIAN) && !defined(GFC_OS_PS3)
#define SWAPCOLOR(color) ((color >> 24) | ((color >> 8) & 0xff00) | ((color & 0xff00) << 8) | ((color & 0xff) << 24))
#else
#define SWAPCOLOR(color) color
#endif

// Vertex buffer structure used for glyphs.
struct GGlyphVertex
{
    float x,y,z,w;
    float u,v;
    GColor color;

    void SetVertex2D(float xx, float yy, float uu, float vv, GColor c)
    {
        x = xx; y = yy; u = uu; v = vv;
        color.Raw = SWAPCOLOR(c.Raw);
    }
};

class GBufferNode : public GRendererNode, public GNewOverrideBase<GStatRender_Mem>
{
public:
    GLuint                      buffer;
    GRenderer::CachedData       *pcache;

    GBufferNode() : GRendererNode() { buffer = 0; pcache = 0; }
    GBufferNode(GBufferNode *p, GRenderer::CachedData *d) : GRendererNode(p) { buffer = 0; pcache = d; }
};

// ***** Renderer Implementation


class GRendererOGL2Impl : public GRendererOGL
{
public:
    // Some renderer state.

    bool                Initialized;
    SInt                RenderMode;

    GLint               MaxTextureOps, MaxTexCoords;
    GLint               MaxAttributes;

    GRenderer::Matrix   ViewportMatrix;
    GViewport           ViewRect;   
    GRenderer::Matrix   UserMatrix;
    GRenderer::Matrix   CurrentMatrix;
    GRenderer::Cxform   CurrentCxform;

#ifndef GFC_NO_3D
    bool                UVPMatricesChanged;
    GMatrix3D           ViewMatrix, ProjMatrix;
    GMatrix3D           UVPMatrix;
    const GMatrix3D*    pWorldMatrix;
#endif

    // Link list of all allocated textures
    GRendererNode       Textures;
    GRendererNode       RenderTargets;
    GLock               TexturesLock;

    // Statistics
    Stats               RenderStats;
    // Video memory
    GMemoryStat         TextureVMem;
    GMemoryStat         BufferVMem;
    // Counts
    GCounterStat        TextureUploadCnt;
    GCounterStat        TextureUpdateCnt;
    GCounterStat        DPLineCnt;
    GCounterStat        DPTriangleCnt;
    GCounterStat        LineCnt;
    GCounterStat        TriangleCnt;
    GCounterStat        MaskCnt;
    GCounterStat        FilterCnt;

    // Vertex data pointer
    GBufferNode         BufferObjects;
    const void*         pVertexData;
    const void*         pIndexData;
    GLuint              VertexArray, IndexArray;
    VertexFormat        VertexFmt;
    GLenum              IndexFmt;

    // Stencil counter - for increment/decrement compares.
    SInt                StencilCounter;
    bool                DrawingMask;
    bool                StencilEnabled;

    // Current sample mode
    BitmapSampleMode        SampleMode;

    typedef GArrayConstPolicy<0, 4, true> NeverShrinkPolicy;
    typedef GArrayLH<BlendType, GStat_Default_Mem, NeverShrinkPolicy> BlendModeStackType;

    // Current blend mode
    BlendType           BlendMode;
    BlendModeStackType  BlendModeStack;

    // render target
    struct RTState
    {
        GRenderTargetOGL2Impl*  pRT;
        GMatrix2D               ViewMatrix;
        GMatrix3D               ViewMatrix3D;
        GMatrix3D               PerspMatrix3D;
        const GMatrix3D*        pWorldMatrix3D;
        GViewport               ViewRect;
        SInt                    RenderMode;
        SInt                    StencilCounter;
        bool                    StencilEnabled;

        RTState() { pRT = 0; pWorldMatrix3D = 0; }
        RTState(GRenderTargetOGL2Impl* prt, const GMatrix2D& vm,
            const GMatrix3D& vm3d, const GMatrix3D &vp3d, const GMatrix3D* pw3d, const GViewport& vp, SInt rm, SInt sc, bool se)
            : pRT(prt), ViewMatrix(vm), ViewMatrix3D(vm3d), PerspMatrix3D(vp3d), pWorldMatrix3D(pw3d), ViewRect(vp),
            RenderMode(rm), StencilCounter(sc), StencilEnabled(se) { }
    };
    typedef GArrayLH<RTState, GStat_Default_Mem, NeverShrinkPolicy> RTStackType;

    GRenderTargetOGL2Impl*  pCurRenderTarget;
    RTStackType             RenderTargetStack;
    GArrayLH<GPtr<GRenderTargetOGL2Impl> >  TempRenderTargets;
    GArrayLH<GPtr<GDepthBufferOGL2Impl> >   TempStencilBuffers;
    GPtr<GRenderTargetOGL2Impl> pScreenRenderTarget;

    // Buffer used to pass along glyph vertices
    enum { GlyphVertexBufferSize = 6 * 48 };
    GGlyphVertex            GlyphVertexBuffer[GlyphVertexBufferSize];

    // Linked list used for buffer cache testing, otherwise holds no data.
    CacheNode               CacheList;

    struct Shader : public GNewOverrideBase<GStat_Default_Mem>
    {
        GLuint      prog;
        GLint       mvp, texgen;
        GLint       cxadd, cxmul, tex[4];

        GLint       pos, color, factor, tc[16];
        GLint       offset;

	    // Blur filters
	    UInt  BoxTCs;
	    UInt  BaseTCs;
	    UInt  MaxTCs;
	    UInt  Samples;
    };
    Shader *                Shaders;
    SInt                    CurrentShader;
    GArray<Shader>          StaticFilterShaders;

    bool                    UseBuffers;
    bool                    UseFbo, PackedDepthStencil;
    bool                    UseLowEndFilters;
#ifdef GL_ES_VERSION_2_0
    SInt                    TexNonPower2;
    bool                    UseBgra;
#endif

    typedef GHash<BlurFilterParams,Shader*> FilterShadersType;
    FilterShadersType FilterShaders;

#ifdef GFC_GL_RUNTIME_LINK
    PFNGLBLENDEQUATIONPROC                       p_glBlendEquation;
    PFNGLDRAWRANGEELEMENTSPROC                   p_glDrawRangeElements;
    PFNGLBLENDFUNCSEPARATEPROC                   p_glBlendFuncSeparate;
    PFNGLMULTITEXCOORD2FPROC                     p_glVertexAttrib2f;
    PFNGLCLIENTACTIVETEXTUREPROC                 p_glClientActiveTexture;
    PFNGLACTIVETEXTUREPROC                       p_glActiveTexture;
    PFNGLENABLEVERTEXATTRIBARRAYPROC             p_glEnableVertexAttribArray;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC            p_glDisableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC                 p_glVertexAttribPointer;
    PFNGLVERTEXATTRIB4FPROC                      p_glVertexAttrib4f;
    PFNGLDELETESHADERPROC                        p_glDeleteShader;
    PFNGLDELETEPROGRAMPROC                       p_glDeleteProgram;
    PFNGLCREATESHADERPROC                        p_glCreateShader;
    PFNGLSHADERSOURCEPROC                        p_glShaderSource;
    PFNGLCOMPILESHADERPROC                       p_glCompileShader;
    PFNGLCREATEPROGRAMPROC                       p_glCreateProgram;
    PFNGLATTACHSHADERPROC                        p_glAttachShader;
    PFNGLLINKPROGRAMPROC                         p_glLinkProgram;
    PFNGLUSEPROGRAMPROC                          p_glUseProgram;
    PFNGLGETSHADERIVPROC                         p_glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC                    p_glGetShaderInfoLog;
    PFNGLGETPROGRAMIVPROC                        p_glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC                   p_glGetProgramInfoLog;
    PFNGLGETATTRIBLOCATIONPROC                   p_glGetAttribLocation;
    PFNGLGETUNIFORMLOCATIONPROC                  p_glGetUniformLocation;
    PFNGLBINDATTRIBLOCATIONPROC                  p_glBindAttribLocation;
    PFNGLUNIFORM1IARBPROC                        p_glUniform1i;
    PFNGLUNIFORM2FARBPROC                        p_glUniform2f;
    PFNGLUNIFORM4FARBPROC                        p_glUniform4f;
    PFNGLUNIFORM4FVARBPROC                       p_glUniform4fv;
    PFNGLUNIFORMMATRIX4FVARBPROC                 p_glUniformMatrix4fv;
    PFNGLCOMPRESSEDTEXIMAGE2DPROC                p_glCompressedTexImage2D;
#if defined(GL_ARB_vertex_buffer_object)
    PFNGLGENBUFFERSARBPROC                       p_glGenBuffers;
    PFNGLDELETEBUFFERSARBPROC                    p_glDeleteBuffers;
    PFNGLBINDBUFFERARBPROC                       p_glBindBuffer;
    PFNGLBUFFERDATAARBPROC                       p_glBufferData;
#endif
    PFNGLBINDRENDERBUFFEREXTPROC                        p_glBindRenderbufferEXT;
    PFNGLDELETERENDERBUFFERSEXTPROC                     p_glDeleteRenderBuffersEXT;
    PFNGLGENRENDERBUFFERSEXTPROC                        p_glGenRenderbuffersEXT;
    PFNGLBINDFRAMEBUFFEREXTPROC                         p_glBindFramebufferEXT;
    PFNGLDELETEFRAMEBUFFERSEXTPROC                      p_glDeleteFramebuffersEXT;
    PFNGLGENFRAMEBUFFERSEXTPROC                         p_glGenFramebuffersEXT;
    PFNGLRENDERBUFFERSTORAGEEXTPROC                     p_glRenderbufferStorageEXT;
    PFNGLFRAMEBUFFERTEXTURE2DEXTPROC                    p_glFramebufferTexture2DEXT;
    PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC                 p_glFramebufferRenderbufferEXT;
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC     p_glFramebufferAttachmentParameterivEXT;
    PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC                  p_glCheckFramebufferStatusEXT;
    PFNGLGENERATEMIPMAPEXTPROC                          p_glGenerateMipmapEXT;

    void glBlendEquation (GLenum e) const { if (p_glBlendEquation) p_glBlendEquation (e); }
    void glBlendFuncSeparate (GLenum sc, GLenum dc, GLenum sa, GLenum da) const
    {
        if (p_glBlendFuncSeparate)
            p_glBlendFuncSeparate (sc,dc,sa,da);
        else
            ::glBlendFunc (sc, dc);
    }
    void glDrawRangeElements (GLenum p, GLuint s, GLuint e, GLsizei c, GLenum t, const void *a) const
    {
        if (p_glDrawRangeElements) p_glDrawRangeElements(p,s,e,c,t,a);
        else glDrawElements(p,c,t,a);
    }
    void glVertexAttrib2f(GLenum t, GLfloat x, GLfloat y) const { p_glVertexAttrib2f(t,x,y); }
    void glActiveTexture(GLenum e) const { p_glActiveTexture(e); }
    void glClientActiveTexture(GLenum e) const { p_glClientActiveTexture(e); }

    void glEnableVertexAttribArray(GLuint i) const { if (p_glEnableVertexAttribArray) p_glEnableVertexAttribArray(i); }
    void glDisableVertexAttribArray(GLuint i) const { if (p_glDisableVertexAttribArray) p_glDisableVertexAttribArray(i); }
    void glVertexAttribPointer(GLuint i, GLint c, GLenum t, GLboolean n, GLsizei s, const GLvoid *a) const
    {
        p_glVertexAttribPointer(i,c,t,n,s,a);
    }

    void   glDeleteShader (GLuint o) const { p_glDeleteShader(o); }
    GLuint glCreateShader (GLenum o) const { return p_glCreateShader(o); }
    void   glDeleteProgram (GLuint o) const { p_glDeleteProgram(o); }
    GLuint glCreateProgram () const { return p_glCreateProgram(); }

    void   glShaderSource (GLuint s, GLsizei n, const GLchar* *t, const GLint *l) const { p_glShaderSource(s,n,t,l); }
    void   glCompileShader (GLuint s) const { p_glCompileShader(s); }
    void   glAttachShader (GLuint o, GLuint s) const { p_glAttachShader(o,s); }
    void   glLinkProgram (GLuint o) const { p_glLinkProgram(o); }
    void   glUseProgram (GLuint o) const { p_glUseProgram(o); }
    void   glGetShaderiv (GLuint o, GLenum e, GLint *v) const { p_glGetShaderiv(o,e,v); }
    void   glGetProgramiv (GLuint o, GLenum e, GLint *v) const { p_glGetProgramiv(o,e,v); }
    void   glGetShaderInfoLog (GLuint o, GLsizei m, GLsizei *l, GLchar *t) const { p_glGetShaderInfoLog (o,m,l,t); }
    void   glGetProgramInfoLog (GLuint o, GLsizei m, GLsizei *l, GLchar *t) const { p_glGetProgramInfoLog (o,m,l,t); }

    GLint  glGetAttribLocation (GLuint o, const GLchar *n) const { return p_glGetAttribLocation(o,n); }
    GLint  glGetUniformLocation (GLuint o, const GLchar *n) const { return p_glGetUniformLocation(o,n); }
    void   glBindAttribLocation (GLuint o, GLuint l, GLchar *n) const { p_glBindAttribLocation(o,l,n); }
    void   glUniform2f (GLint i, GLfloat a, GLfloat b) const { p_glUniform2f(i,a,b); }
    void   glUniform4f (GLint i, GLfloat a, GLfloat b, GLfloat c, GLfloat d) const { p_glUniform4f(i,a,b,c,d); }
    void   glUniform4fv (GLint i, GLuint s, const GLfloat* p) const { p_glUniform4fv(i,s,p); }
    void   glUniformMatrix4fv (GLint i, GLsizei s, GLboolean t, const GLfloat* p) const { p_glUniformMatrix4fv(i,s,t,p); }
    void   glUniform1i (GLint i, GLint s) const { p_glUniform1i(i,s); }

#if defined(GL_ARB_vertex_buffer_object)
    void  glGenBuffers(GLsizei n, GLuint *o) { p_glGenBuffers(n,o); }
    void  glDeleteBuffers(GLsizei n, GLuint *o) { p_glDeleteBuffers(n,o); }
    void  glBindBuffer(GLenum t, GLuint b) { p_glBindBuffer(t,b); }
    void  glBufferData(GLenum t, GLsizeiptr s, const GLvoid *p, GLenum u) { p_glBufferData(t,s,p,u);}
#endif

    void  glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
    {
        if (p_glCompressedTexImage2D)
            p_glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
        else
        {
            GFC_DEBUG_WARNING(1, "GRendererOGL texture creation failed - glCompressedTexImage2D ext not available");
        }
    }

    void  glBindRenderbuffer (GLenum t, GLuint rb) const
    {
        p_glBindRenderbufferEXT(t,rb);
    }

    void  glDeleteRenderbuffers (GLsizei n, const GLuint *b) const
    {
        p_glDeleteRenderBuffersEXT(n,b);
    }

    void  glGenRenderbuffers (GLsizei n, GLuint *b) const
    {
        p_glGenRenderbuffersEXT(n,b);
    }

    void  glRenderbufferStorage (GLenum t, GLenum f, GLsizei w, GLsizei h)
    {
        p_glRenderbufferStorageEXT(t,f,w,h);
    }

    void  glBindFramebuffer (GLenum t, GLuint b) const
    {
        p_glBindFramebufferEXT(t,b);
    }

    void  glDeleteFramebuffers (GLsizei n, const GLuint *b) const
    {
        p_glDeleteFramebuffersEXT(n,b);
    }

    void  glGenFramebuffers (GLsizei n, GLuint *b) const
    {
        p_glGenFramebuffersEXT(n,b);
    }

    GLenum  glCheckFramebufferStatus (GLenum p)
    {
        return p_glCheckFramebufferStatusEXT(p);
    }
    void  glFramebufferTexture1D (GLenum, GLenum, GLenum, GLuint, GLint);

    void  glFramebufferTexture2D (GLenum t, GLenum a, GLenum tt, GLuint o, GLint l) const
    {
        p_glFramebufferTexture2DEXT(t,a,tt,o,l);
    }

    void  glFramebufferRenderbuffer (GLenum t, GLenum a, GLenum rt, GLuint r) const
    {
        p_glFramebufferRenderbufferEXT(t,a,rt,r);
    }

    void  glGetFramebufferAttachmentParameteriv (GLenum t, GLenum a, GLenum q, GLint *v) const
    {
        p_glFramebufferAttachmentParameterivEXT(t,a,q,v);
    }

    void glGenerateMipmap (GLenum t) const
    {
        p_glGenerateMipmapEXT(t);
    }

#else
    void glBlendEquation (GLenum e) const
    {
#ifdef GL_VERSION_ES_CM_1_0
#ifdef GL_OES_blend_subtract
        ::glBlendEquationOES (e);
#endif
#else
        ::glBlendEquation (e);
#endif
    }
    void glActiveTexture(GLenum e) const { ::glActiveTexture(e); }

    void glDrawRangeElements (GLenum p, GLuint s, GLuint e, GLsizei c, GLenum t, const void *a) const
    {
        glDrawElements(p,c,t,a);
    }

    void glBlendFuncSeparate (GLenum sc, GLenum dc, GLenum sa, GLenum da) const
    {
#ifdef GL_ARB_blend_func_separate
        ::glBlendFuncSeparate(sc,dc,sa,da);
#else
        glBlendFunc(sc,dc);
#endif
    }

    void glUniform1i(GLint l, GLint x) const
    {
        ::glUniform1i(l,x);
    }
#endif

    // all OGL calls should be done on the main thread, but textures can be released from 
    // different threads so we collect system resources and release them on the main thread
    struct ReleaseResource
    {
	enum Type
	{
	    Texture,
	    Fbo,
	    Buffer
	} type;
	GLuint object;
    };
    GArray<ReleaseResource>  ResourceReleasingQueue;
    GLock                    ResourceReleasingQueueLock;

    // this method is called from GTextureXXX destructor to put a system resource into releasing queue
    void AddResourceForReleasing(GLuint textureId, ReleaseResource::Type type = ReleaseResource::Texture)
    {
        if (textureId == 0) return;
        GLock::Locker guard(&ResourceReleasingQueueLock);
	ReleaseResource res;
	res.object = textureId;
	res.type = type;
        ResourceReleasingQueue.PushBack(res);
    }

    // this method is called from GetTexture, EndDisplay and destructor to actually release
    // collected system resources
    void ReleaseQueuedResources()
    {
        GLock::Locker guard(&ResourceReleasingQueueLock);
        for (UInt i = 0; i < ResourceReleasingQueue.GetSize(); ++i)
	    switch (ResourceReleasingQueue[i].type)
	    {
	    case ReleaseResource::Texture:
		glDeleteTextures(1, &ResourceReleasingQueue[i].object);
		break;
	    case ReleaseResource::Fbo:
		glDeleteFramebuffers(1, &ResourceReleasingQueue[i].object);
		break;
	    case ReleaseResource::Buffer:
		glDeleteRenderbuffers(1, &ResourceReleasingQueue[i].object);
		break;
	    }
        ResourceReleasingQueue.Clear();
    }

    // ****

    GRendererOGL2Impl()
    {       
        Initialized = 0;

        SampleMode  = Sample_Linear;
        RenderMode  = 0;
        pVertexData = 0;
        pIndexData  = 0;
        VertexFmt   = Vertex_None;
        IndexFmt    = GL_UNSIGNED_SHORT;
#ifndef GFC_NO_3D
        Shaders     = (Shader*) GALLOC(sizeof(Shader) * (PS_Count + 15) * 2, GStat_Default_Mem);
#else
        Shaders     = (Shader*) GALLOC(sizeof(Shader) * PS_Count + 15, GStat_Default_Mem);
#endif

        StencilCounter = 0;
#ifndef GFC_NO_3D
	    pWorldMatrix   = 0;
#endif

        pCurRenderTarget = 0;

        // Glyph buffer z, w components are always 0 and 1.
        UInt i;
        for(i = 0; i < GlyphVertexBufferSize; i++)
        {
            GlyphVertexBuffer[i].z = 0.0f;
            GlyphVertexBuffer[i].w = 1.0f;
        }
    }

    ~GRendererOGL2Impl()
    {
        Clear();
        GFREE(Shaders);
    }

    void Clear()
    {
        // Remove/notify all textures
        {
            GLock::Locker guard(&TexturesLock);
            while (Textures.pFirst != &Textures)
                ((GTextureOGL2Impl*)Textures.pFirst)->RemoveFromRenderer();

            while (RenderTargets.pFirst != &RenderTargets)
                ((GRenderTargetOGL2Impl*)RenderTargets.pFirst)->RemoveFromRenderer();
        }

	    TempRenderTargets.Clear();
        TempStencilBuffers.Clear();
        CacheList.ReleaseList();

        for (GRendererNode *p = BufferObjects.pFirst; p; )
        {
            GBufferNode *pn = (GBufferNode*)p;
#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
            if (pn->buffer)
                glDeleteBuffers(1, &pn->buffer);
#endif
            if (pn->pcache)
                pn->pcache->ReleaseDataByRenderer();
            p = p->pNext;
            if (p == BufferObjects.pLast)
                p->pNext = 0;
            if (pn != &BufferObjects)
                delete pn;
        }
        BufferObjects.pFirst = NULL;
        ReleaseQueuedResources();

        ReleaseShaders();
    }

    void ReleaseResources()
    {
        Clear();
    }


    // Utility.  Mutates *width, *height and *data to create the
    // next mip level.
    static void MakeNextMiplevel(int* width, int* height, UByte* data, GLenum format = GL_ALPHA)
    {
        GASSERT(width);
        GASSERT(height);
        GASSERT(data);
        GASSERT_ON_RENDERER_MIPMAP_GEN;

        int newW = *width >> 1;
        int newH = *height >> 1;
        if (newW < 1) newW = 1;
        if (newH < 1) newH = 1;

        if (newW * 2 != *width || newH * 2 != *height)
        {
            // Image can not be shrunk Along (at least) one
            // of its dimensions, so no need to do
            // resampling.  Technically we should, but
            // it's pretty useless at this point.  Just
            // change the image dimensions and leave the
            // existing pixels.
        }
        else if (format == GL_ALPHA || format == GL_LUMINANCE)
        {
            // Resample.  Simple average 2x2 --> 1, in-place.
            for (int j = 0; j < newH; j++) 
            {
                UByte*  out = ((UByte*) data) + j * newW;
                UByte*  in  = ((UByte*) data) + (j << 1) * *width;
                for (int i = 0; i < newW; i++) 
                {
                    int a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
                    *(out) = UByte(a >> 2);
                    out++;
                    in += 2;
                }
            }
        }
        else
        {
            // Resample.  Simple average 2x2 --> 1, in-place.
            for (int j = 0; j < newH; j++) 
            {
                UByte*  out = ((UByte*) data) + j * (4 * newW);
                UByte*  in = ((UByte*) data) + (j << 1) * (4 * *width);
                for (int i = 0; i < newW; i++) 
                {
                    int r,g,b,a;
                    r = ( *(in + 0) + *(in + 4) + *(in + 0 + (*width * 4)) + *(in + 4 + (*width * 4)) );
                    g = ( *(in + 1) + *(in + 5) + *(in + 1 + (*width * 4)) + *(in + 5 + (*width * 4)) );
                    b = ( *(in + 2) + *(in + 6) + *(in + 2 + (*width * 4)) + *(in + 6 + (*width * 4)) );
                    a = ( *(in + 3) + *(in + 7) + *(in + 3 + (*width * 4)) + *(in + 7 + (*width * 4)) );
                    out[0] = UByte(r >> 2);
                    out[1] = UByte(g >> 2);
                    out[2] = UByte(b >> 2);
                    out[3] = UByte(a >> 2);
                    out += 4;
                    in += 8;
                }
            }
        }

        // Set parameters to reflect the new size
        *width = newW;
        *height = newH;
    }

    virtual void SetPixelShader (PixelShaderType ps);
    virtual void ApplyPShaderCxform(const Cxform &cxform);
    virtual GLuint InitShader(GLenum type, const char *psrc);
    virtual GLuint InitShader(const char *pVText, const char *pFText);
    virtual bool InitShaders();
    virtual void ReleaseShaders();

    const Shader* GetBlurShader(const BlurFilterParams& params, bool LowEnd);

    virtual UInt CheckFilterSupport(const BlurFilterParams& params);
    virtual void DrawBlurRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& indestrect, const BlurFilterParams& params, bool islast);

    void DrawQuad (const Shader* pShader, const GRectF& srcrect, const GRectF& destrect)
    {
        Float strip[16] =
        {
            destrect.Left, destrect.Top,    srcrect.Left, srcrect.Top,
            destrect.Right, destrect.Top,   srcrect.Right, srcrect.Top,
            destrect.Left, destrect.Bottom, srcrect.Left, srcrect.Bottom,
            destrect.Right, destrect.Bottom, srcrect.Right, srcrect.Bottom,
        };

 	glEnableVertexAttribArray(pShader->pos);
 	glEnableVertexAttribArray(pShader->tc[0]);
	glVertexAttribPointer(pShader->pos, 2, GL_FLOAT, 0, 16, strip);
	glVertexAttribPointer(pShader->tc[0], 2, GL_FLOAT, 0, 16, strip+2);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 
	glDisableVertexAttribArray(pShader->pos);
	glDisableVertexAttribArray(pShader->tc[0]);
    }

    void DrawColorMatrixRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& destrect, const Float *matrix, bool islast)
    {
		GUNUSED(islast);
        GTextureOGL2Impl* psrc = (GTextureOGL2Impl*) psrcin;

        if (BlendMode == Blend_Darken || BlendMode == Blend_Multiply)
            SetPixelShader(PS_CmatrixTextureMultiply);
        else
            SetPixelShader(PS_CmatrixTexture);

        GRectF srcrect = GRectF(insrcrect.Left * 1.0f/psrc->TexWidth, insrcrect.Top * 1.0f/psrc->TexHeight,
            insrcrect.Right * 1.0f/psrc->TexWidth, insrcrect.Bottom * 1.0f/psrc->TexHeight);

        const Shader *pShader = &Shaders[CurrentShader];

        ApplyMatrix(CurrentMatrix);

	    ApplyBlendMode(BlendMode, true, true);
        glEnable(GL_BLEND);

        glUniformMatrix4fv(pShader->cxmul, 1, 0, matrix);
        glUniform4fv(pShader->cxadd, 1, matrix+16);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, psrc->TextureId);
        glUniform1i(pShader->tex[0], 0);

	DrawQuad(pShader, srcrect, destrect);

        ApplyBlendMode(BlendMode);
        FilterCnt.AddCount(1);
        RenderStats.Filters += 1;
    }


    // Fill helper function:
    // Applies fill texture by setting it to the specified stage, initializing samplers and vertex constants
    virtual void    ApplyFillTexture(const FillTexture &fill, UInt stageIndex)
    {
        GASSERT (fill.pTexture != 0);
        if (fill.pTexture == 0) return; // avoid crash in release build

        GTextureOGL2Impl* ptexture = ((GTextureOGL2Impl*)fill.pTexture);

        ptexture->Bind(stageIndex, fill.WrapMode, fill.SampleMode, 0);

        const GRenderer::Matrix&    m = fill.TextureMatrix;
        Float   p[8];

        p[0] = m.M_[0][0];
        p[1] = m.M_[0][1];
        p[2] = 0;
        p[3] = m.M_[0][2];
        p[4] = m.M_[1][0];
        p[5] = m.M_[1][1];
        p[6] = 0;
        p[7] = m.M_[1][2];
        glUniform4fv(Shaders[CurrentShader].texgen+2*stageIndex, 2, p);

        glActiveTexture(GL_TEXTURE0);
    }

    class GFxFillStyle
    {
    public:
        enum FillMode
        {
            FM_None,
            FM_Color,
            FM_Bitmap,
            FM_Gouraud
        };

        FillMode                Mode;
        GouraudFillType         GouraudType;
        GColor                  Color;
        FillTexture             Fill, Fill2;

        GRenderer::Cxform       BitmapColorTransform;
        bool                    HasNonzeroAdditiveCxform;

        GFxFillStyle()
        {
            Mode            = FM_None;
            GouraudType     = GFill_Color;
            Fill.pTexture   = 0;
            Fill2.pTexture  = 0;
            HasNonzeroAdditiveCxform = false;
        }

        // Push our style into OpenGL.
        void    Apply(GRendererOGL2Impl *prenderer) const
        {
            GASSERT(Mode != FM_None);

            if (Mode == FM_Color || prenderer->DrawingMask)
            {
#ifdef GFC_GL_BLEND_DISABLE
                if ((Color.GetAlpha() == 0xFF) && (prenderer->BlendMode <= Blend_Normal))
                    glDisable(GL_BLEND);
                else
                    glEnable(GL_BLEND);
#endif

                prenderer->SetPixelShader(PS_Solid);
                prenderer->ApplyColor(Color, 0);
            }
            else if (Mode == FM_Bitmap)
            {
                if (Fill.pTexture == NULL)
                {
#ifdef GFC_GL_BLEND_DISABLE
                    if ((Color.GetAlpha() == 0xFF) && (prenderer->BlendMode <= Blend_Normal))
                        glDisable(GL_BLEND);
                    else
                        glEnable(GL_BLEND);
#endif

                    prenderer->SetPixelShader(PS_Solid);
                    prenderer->ApplyColor(Color, 0);
                }
                else
                {
#ifdef GFC_GL_BLEND_DISABLE
                    glEnable(GL_BLEND);
#endif

                    if (prenderer->BlendMode == Blend_Multiply || prenderer->BlendMode == Blend_Darken)
                        prenderer->SetPixelShader(PS_CxformTextureMultiply);
                    else
                        prenderer->SetPixelShader(PS_CxformTexture);

                    prenderer->ApplyFillTexture(Fill, 0);
                    prenderer->ApplyPShaderCxform(BitmapColorTransform);
                }
            }
            else if (Mode == FM_Gouraud)
            {
                PixelShaderType shader = PS_None;

#ifdef GFC_GL_BLEND_DISABLE
                glEnable(GL_BLEND);
#endif

                // No texture: generate color-shaded triangles.
                if (Fill.pTexture == NULL)
                {
                    if (prenderer->VertexFmt == Vertex_XY16iC32)
                    {
                        // Cxform Alpha Add can not be non-zero is this state because 
                        // cxform blend equations can not work correctly.
                        // If we hit this assert, it means Vertex_XY16iCF32 should have been used.
                        //GASSERT (BitmapColorTransform.M_[3][1] < 1.0f);

                        shader = PS_CxformGouraudNoAddAlpha;                        
                    }
                    else
                    {
                        shader = PS_CxformGouraud;
                    }
                }
                // We have a textured or multi-textured gouraud case.
                else
                {   
                    if ((GouraudType == GFill_1TextureColor) ||
                        (GouraudType == GFill_1Texture))
                        shader = PS_CxformGouraudTexture;
                    else
                        shader = PS_Cxform2Texture;
                }

                if ((prenderer->BlendMode == Blend_Multiply) ||
                    (prenderer->BlendMode == Blend_Darken) )
                { 
                    shader = (PixelShaderType)(shader + (PS_CxformGouraudMultiply - PS_CxformGouraud));

                    // For indexing to work, these should hold:
                    GCOMPILER_ASSERT( (PS_Cxform2Texture - PS_CxformGouraud) ==
                        (PS_CxformMultiply2Texture - PS_CxformGouraudMultiply));
                    GCOMPILER_ASSERT( (PS_CxformGouraudMultiply - PS_CxformGouraud) ==
                        (PS_Cxform2Texture - PS_CxformGouraud + 1) );
                }
                prenderer->SetPixelShader(shader);
                prenderer->ApplyPShaderCxform(BitmapColorTransform);

                if (Fill.pTexture)
                {
                    prenderer->ApplyFillTexture(Fill, 0);

                    if (Fill2.pTexture &&
                        (GouraudType == GFill_2TextureColor ||
                         GouraudType == GFill_2Texture))
                        prenderer->ApplyFillTexture(Fill2, 1);
                }
            }
        }

        void    Disable()               { Mode = FM_None; Fill.pTexture = 0; }

        void    SetColor(GColor color)  { Mode = FM_Color; Color = color; }

        void    SetCxform(const Cxform& colorTransform)
        {
            BitmapColorTransform = colorTransform;

            if ( BitmapColorTransform.M_[0][1] > 1.0f ||
                BitmapColorTransform.M_[1][1] > 1.0f ||
                BitmapColorTransform.M_[2][1] > 1.0f ||
                BitmapColorTransform.M_[3][1] > 1.0f )         
                HasNonzeroAdditiveCxform = true;            
            else            
                HasNonzeroAdditiveCxform = false;
        }

        void    SetBitmap(const FillTexture* pft, const Cxform& colorTransform)
        {
            Mode            = FM_Bitmap;
            Fill            = *pft;
            Color           = GColor(0xFFFFFFFF);

            SetCxform(colorTransform);
        }

        // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
        // The specified textures are applied to vertices {0, 1, 2} of each triangle based
        // on factors of Complex vertex. Any or all subsequent pointers can be NULL, in which case
        // texture is not applied and vertex colors used instead.
        void    SetGouraudFill(GouraudFillType gfill,
            const FillTexture *ptexture0,
            const FillTexture *ptexture1,
            const FillTexture *ptexture2, const Cxform& colorTransform)
        {

            // Texture2 is not yet used.
            if (ptexture0 || ptexture1 || ptexture2)
            {
                const FillTexture *p = ptexture0;
                if (!p) p = ptexture1;              

                SetBitmap(p, colorTransform);
                // Used in 2 texture mode
                if (ptexture1)
                    Fill2 = *ptexture1;             
            }
            else
            {
                SetCxform(colorTransform);              
                Fill.pTexture = 0;  
            }

            Mode        = GFxFillStyle::FM_Gouraud;
            GouraudType = gfill;
        }

        bool    IsValid() const { return Mode != FM_None; }
        bool    UsesTextures() const { return Fill.pTexture != 0; }
    };


    // Style state.
    enum StyleIndex
    {
        FILL_STYLE = 0,     
        LINE_STYLE,
        STYLE_COUNT
    };
    GFxFillStyle    CurrentStyles[STYLE_COUNT];


    // Given an image, returns a Pointer to a GTexture struct
    // that can later be passed to FillStyleX_bitmap(), to set a
    // bitmap fill style.
    GTextureOGL2Impl*   CreateTexture()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GTextureOGL2Impl(this);
    }

    GTextureOGL*   CreateTextureYUV()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GTextureOGL2ImplYUV(this);
    }

    GDepthBufferOGL2Impl*   CreateDepthStencilBuffer()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GDepthBufferOGL2Impl(this);
    }
    GRenderTargetOGL2Impl*      CreateRenderTarget()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GRenderTargetOGL2Impl(this);
    }

    // Helper function to query renderer capabilities.
    bool        GetRenderCaps(RenderCaps *pcaps)
    {
        if (!Initialized)
        {
            GFC_DEBUG_WARNING(1, "GRendererOGL::GetRenderCaps fails - video mode not set");
            return 0;
        }

        pcaps->CapBits      = Cap_Index16 | Cap_Index32 | Cap_NestedMasks;
        pcaps->BlendModes   = (1<<Blend_None) | (1<<Blend_Normal) |
            (1<<Blend_Multiply) | (1<<Blend_Lighten) | (1<<Blend_Darken) |
            (1<<Blend_Add) | (1<<Blend_Subtract);
        pcaps->VertexFormats= (1<<Vertex_None) | (1<<Vertex_XY16i) | (1<<Vertex_XY16iC32) | (1<<Vertex_XY16iCF32);
        pcaps->CapBits |= Cap_CxformAdd | Cap_FillGouraud | Cap_FillGouraudTex;

        GLint maxTextureSize = 64;
        glGetIntegerv (GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        pcaps->MaxTextureSize = maxTextureSize;

        if (UseFbo)
        {
            pcaps->CapBits |= Cap_RenderTargets | Cap_RenderTargetMip;
            pcaps->CapBits |= Cap_Filter_Blurs | Cap_Filter_ColorMatrix;
        }

#ifdef GL_ES_VERSION_2_0
        if (TexNonPower2 >= 2)
            pcaps->CapBits |= Cap_TexNonPower2 | Cap_TexNonPower2Wrap | Cap_TexNonPower2Mip;
        else
            pcaps->CapBits |= Cap_TexNonPower2;
#else
        pcaps->CapBits |= Cap_TexNonPower2 | Cap_TexNonPower2Wrap | Cap_TexNonPower2Mip | Cap_RenderTargetNonPow2;

#endif

        return 1;
    }

    void SetDisplayRenderTarget(GRenderTarget* prt, bool setstate = 1)
    {
        GASSERT(RenderTargetStack.GetSize() == 0);
        pScreenRenderTarget = 0;
        pCurRenderTarget = (GRenderTargetOGL2Impl*)prt;
        if (setstate)
            glBindFramebuffer(GL_FRAMEBUFFER_EXT, pCurRenderTarget->FboId);
    }

    void        PushRenderTarget(const GRectF& FrameRect, GRenderTarget* prt)
    {
        RenderTargetStack.PushBack(RTState(pCurRenderTarget,ViewportMatrix, 
#ifndef GFC_NO_3D
            ViewMatrix, ProjMatrix, pWorldMatrix,
#else
            GMatrix3D(), GMatrix3D(), 0,
#endif
            ViewRect,RenderMode, StencilCounter, StencilEnabled));
        pCurRenderTarget = (GRenderTargetOGL2Impl*)prt;

        float   dx = FrameRect.Width(); 
        float   dy = FrameRect.Height();
        if (dx < 1) { dx = 1; }
        if (dy < 1) { dy = 1; }

        ViewportMatrix.SetIdentity();
        ViewportMatrix.M_[0][0] = 2.0f / dx;
        ViewportMatrix.M_[1][1] = 2.0f / dy;
        ViewportMatrix.M_[0][2] = -1.0f - ViewportMatrix.M_[0][0] * (FrameRect.Left); 
        ViewportMatrix.M_[1][2] = -1.0f - ViewportMatrix.M_[1][1] * (FrameRect.Top);

#ifndef GFC_NO_3D
        // set 3D view
        GMatrix3D matView, matPersp;
        MakeViewAndPersp3D(FrameRect, matView, matPersp, DEFAULT_FLASH_FOV, true /*invert Y */);
        SetView3D(matView);
        SetPerspective3D(matPersp);
        SetWorld3D(0);
#endif
        if (pCurRenderTarget->pTexture)
            ViewRect = GViewport(pCurRenderTarget->pTexture->TexWidth, pCurRenderTarget->pTexture->TexHeight,
                0,0,pCurRenderTarget->TargetWidth, pCurRenderTarget->TargetHeight, GViewport::View_RenderTextureAlpha);
        else // Must be external render target, assuming viewport will be set by BeginDisplay.
            ViewRect = GViewport(64,64,0,0,64,64, GViewport::View_RenderTextureAlpha);

        RenderMode = GViewport::View_AlphaComposite;

        glBindFramebuffer(GL_FRAMEBUFFER_EXT, pCurRenderTarget->FboId);
        glViewport(ViewRect.Left, ViewRect.Top, ViewRect.Width, ViewRect.Height);
        glDisable(GL_SCISSOR_TEST);

        GASSERT(GL_FRAMEBUFFER_COMPLETE_EXT == glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT));

        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        StencilEnabled = 0;
#ifndef GFC_ZBUFFER_MASKING
        glDisable(GL_STENCIL_TEST);
#else
        glDisable(GL_DEPTH_TEST);
#endif

        ApplyBlendMode(BlendMode);
    }

    void        PopRenderTarget()
    {
        if (pCurRenderTarget->IsTemp)
        {
            pCurRenderTarget->pStencil = 0;
            pCurRenderTarget->pDepth = 0;
        }

        if (pCurRenderTarget->pTexture && pCurRenderTarget->pTexture->Mipmaps > 1)
        {
            glBindTexture(GL_TEXTURE_2D, pCurRenderTarget->pTexture->TextureId);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        RTState rts = RenderTargetStack.Back();
        RenderTargetStack.PopBack();
        pCurRenderTarget = rts.pRT;
        ViewportMatrix = rts.ViewMatrix;
        ViewRect = rts.ViewRect;
        RenderMode = rts.RenderMode;
        StencilEnabled = rts.StencilEnabled;
        StencilCounter = rts.StencilCounter;

#ifndef GFC_NO_3D
        // restore 3D view matrix
        SetView3D(rts.ViewMatrix3D);
        SetPerspective3D(rts.PerspMatrix3D);
        SetWorld3D(rts.pWorldMatrix3D);
#endif
        glBindFramebuffer(GL_FRAMEBUFFER_EXT, pCurRenderTarget->FboId);
        glViewport(ViewRect.Left, ViewRect.Top, ViewRect.Width, ViewRect.Height);
         // xxx restore scissor

        GASSERT(GL_FRAMEBUFFER_COMPLETE_EXT == glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT));

        ApplyBlendMode(BlendMode);
        if (StencilEnabled)
        {
#ifndef GFC_ZBUFFER_MASKING
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_EQUAL, StencilCounter, 0xFF);      
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);             
#else
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_EQUAL);
#endif 
        }
        else
        {
#ifndef GFC_ZBUFFER_MASKING
            glDisable(GL_STENCIL_TEST);
#else
            glDisable(GL_DEPTH_TEST);
#endif
        }
    }

    GTextureOGL2Impl*   PushTempRenderTarget(const GRectF& FrameRect, UInt inw, UInt inh, bool wantStencil = 0)
    {
        GASSERT(DrawingMask == 0);

        GRenderTargetOGL2Impl* pRT = 0;
        SInt w = 1, h = 1;
        bool needsinit = false;

        for (UInt i = 0; i < TempRenderTargets.GetSize(); i++)
            if (TempRenderTargets[i]->pTexture->GetRefCount() <= 1 &&
                TempRenderTargets[i]->pTexture->TexWidth >= SInt(inw) &&
                TempRenderTargets[i]->pTexture->TexHeight >= SInt(inh))
            {
                pRT = TempRenderTargets[i];
                goto dostencil;
            }

        while (w < SInt(inw)) w <<= 1;
        while (h < SInt(inh)) h <<= 1;

        for (UInt i = 0; i < TempRenderTargets.GetSize(); i++)
            if (TempRenderTargets[i]->pTexture->GetRefCount() <= 1)
            {
                pRT = TempRenderTargets[i];
                needsinit = true;
                goto dostencil;
            }

        {
            GTexture* prtt = CreateTexture();
            pRT = (GRenderTargetOGL2Impl *)CreateRenderTarget();
            pRT->IsTemp = true;
            pRT->pTexture = (GTextureOGL2Impl*) prtt;  // will be properly initialized later
            TempRenderTargets.PushBack(*pRT);
            prtt->Release();
            needsinit = true;
        }
dostencil:
        GDepthBufferOGL2Impl* pDepth = 0;
        GDepthBufferOGL2Impl* pStencil = 0;

        if (w < pRT->pTexture->TexWidth)
            w = pRT->pTexture->TexWidth;
        if (h < pRT->pTexture->TexHeight)
            h = pRT->pTexture->TexHeight;

        GASSERT(!pRT->pStencil);
        if (wantStencil && (pRT->pStencil.GetPtr() == 0 || pRT->pStencil->GetRefCount() > 1))
        {
            for (UInt i = 0; i < TempStencilBuffers.GetSize(); i++)
            {
                if (TempStencilBuffers[i]->GetRefCount() <= 1 &&
                    TempStencilBuffers[i]->TexWidth == w &&
                    TempStencilBuffers[i]->TexHeight == h)
                {
                    pStencil = TempStencilBuffers[i];
                    if (PackedDepthStencil)
                        pDepth = pStencil;
                    break;
                }
            }
            if (!pStencil)
            {
                for (UInt i = 0; i < TempStencilBuffers.GetSize(); i++)
                    if (TempStencilBuffers[i]->GetRefCount() <= 1)
                    {
                        pStencil = TempStencilBuffers[i];
                        if (PackedDepthStencil)
                        {
                            pStencil->InitDynamicTexture(w, h, GImage::Image_DepthStencil, 0, GTexture::Usage_RenderTarget);
                            pDepth = pStencil;
                        }
                        else
                            pStencil->InitDynamicTexture(w, h, GImage::Image_Stencil, 0, GTexture::Usage_RenderTarget);
                        break;
                    }

                if (!pStencil)
                {
                    pStencil = (GDepthBufferOGL2Impl*) CreateDepthStencilBuffer();
                    if (PackedDepthStencil)
                    {
                        pStencil->InitDynamicTexture(w, h, GImage::Image_DepthStencil, 0, GTexture::Usage_RenderTarget);
                        pDepth = pStencil;
                    }
                    else
                        pStencil->InitDynamicTexture(w, h, GImage::Image_Stencil, 0, GTexture::Usage_RenderTarget);
                    TempStencilBuffers.PushBack(*pStencil);
                }
            }

            needsinit = true;
        }

        if (needsinit)
        {
            if (w != pRT->pTexture->TexWidth || h != pRT->pTexture->TexHeight)
                pRT->pTexture->InitDynamicTexture(w, h, GImage::Image_ARGB_8888, 0, GTexture::Usage_RenderTarget);

            // should use handlers instead
            pRT->InitRenderTarget(pRT->pTexture, pDepth, pStencil);
        }

        pRT->TargetWidth = inw;
        pRT->TargetHeight = inh;
        PushRenderTarget(FrameRect, pRT);
        return pRT->pTexture;		
    }

    void ReleaseTempRenderTargets(UInt keepArea)
    {
    	GArray<GPtr<GRenderTargetOGL2Impl> >  NewTempRenderTargets;
        GArray<GPtr<GDepthBufferOGL2Impl> >   NewTempStencilBuffers;
        UInt stencilArea = keepArea;

        for (UInt i = 0; i < TempRenderTargets.GetSize(); i++)
            if (TempRenderTargets[i]->pTexture->GetRefCount() > 1 ||
		        UInt(TempRenderTargets[i]->pTexture->TexWidth * TempRenderTargets[i]->pTexture->TexHeight) <= keepArea)
	        {
		        NewTempRenderTargets.PushBack(TempRenderTargets[i]);
		        keepArea -= TempRenderTargets[i]->pTexture->TexWidth * TempRenderTargets[i]->pTexture->TexHeight;
      	    }

	    TempRenderTargets.Clear();
	    TempRenderTargets.Append(NewTempRenderTargets.GetDataPtr(), NewTempRenderTargets.GetSize());

        for (UInt i = 0; i < TempStencilBuffers.GetSize(); i++)
            if (TempStencilBuffers[i]->GetRefCount() > 1 ||
                UInt(TempStencilBuffers[i]->TexWidth * TempStencilBuffers[i]->TexHeight) <= stencilArea)
            {
                NewTempStencilBuffers.PushBack(TempStencilBuffers[i]);
                stencilArea -= TempStencilBuffers[i]->TexWidth * TempStencilBuffers[i]->TexHeight;
            }

        TempStencilBuffers.Clear();
        TempStencilBuffers.Append(NewTempStencilBuffers.GetDataPtr(), NewTempStencilBuffers.GetSize());
    }


    // Set up to render a full frame from a movie and fills the
    // background.  Sets up necessary transforms, to scale the
    // movie to fit within the given dimensions.  Call
    // EndDisplay() when you're done.
    //
    // The Rectangle (viewportX0, viewportY0, 
    // viewportX0 + viewportWidth, viewportY0 + viewportHeight) 
    // defines the window coordinates taken up by the movie.
    //
    // The Rectangle (x0, y0, x1, y1) defines the pixel
    // coordinates of the movie that correspond to the viewport bounds.
    void    BeginDisplay(
        GColor backgroundColor, const GViewport &viewport,
        Float x0, Float x1, Float y0, Float y1)

    {
        RenderMode = (viewport.Flags & GViewport::View_AlphaComposite);

        BlendModeStack.Clear();
        BlendModeStack.Reserve(16);
        BlendMode = Blend_None;
        ApplyBlendMode(BlendMode); 

        float   dx = x1 - x0;
        float   dy = y1 - y0;
        if (dx < 1) { dx = 1; }
        if (dy < 1) { dy = 1; }

        ViewportMatrix.SetIdentity();
        if (viewport.Flags & GViewport::View_IsRenderTexture)
        {
            ViewportMatrix.M_[0][0] = 2.0f  / dx;
            ViewportMatrix.M_[1][1] = 2.0f / dy;
            ViewportMatrix.M_[0][2] = -1.0f - ViewportMatrix.M_[0][0] * (x0); 
            ViewportMatrix.M_[1][2] = -1.0f  - ViewportMatrix.M_[1][1] * (y0);
        }
        else
        {
            ViewportMatrix.M_[0][0] = 2.0f  / dx;
            ViewportMatrix.M_[1][1] = -2.0f / dy;
            ViewportMatrix.M_[0][2] = -1.0f - ViewportMatrix.M_[0][0] * (x0); 
            ViewportMatrix.M_[1][2] = 1.0f  - ViewportMatrix.M_[1][1] * (y0);
        }

        ViewportMatrix.Prepend(UserMatrix);

#ifndef GFC_NO_3D
        pWorldMatrix   = 0;
#endif

        ViewRect = viewport;
        ViewRect.Top = viewport.BufferHeight-viewport.Top-viewport.Height;

        if (0 == (viewport.Flags & GViewport::View_NoSetState))
        {
            glViewport(viewport.Left, viewport.BufferHeight-viewport.Top-viewport.Height, 
                       viewport.Width, viewport.Height);

            if (viewport.Flags & GViewport::View_UseScissorRect)
            {
                glEnable(GL_SCISSOR_TEST);
                glScissor(viewport.ScissorLeft, viewport.BufferHeight-viewport.ScissorTop-viewport.ScissorHeight, 
                          viewport.ScissorWidth, viewport.ScissorHeight);
            }
            else
            {
                glDisable(GL_SCISSOR_TEST);
            }
        }

        glDisable(GL_CULL_FACE);

        // No stencil or depth testing.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);

#ifndef GL_ES_VERSION_2_0
        glDisable(GL_ALPHA_TEST);
#endif
        glStencilMask(0xffffffff);

        glEnable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Sample Mode
        SampleMode = Sample_Linear;

        BlendMode = Blend_Normal;

        // Stencil counter starts out at 0.
        StencilCounter = 0;
        DrawingMask = 0;
        StencilEnabled = 0;

        // Clear the background, if background color has alpha > 0.
        if (backgroundColor.GetAlpha() > 0)
        {
#ifdef GFC_GL_BLEND_DISABLE
            if (backgroundColor.GetAlpha() == 0xFF)
                glDisable(GL_BLEND);
            else
                glEnable(GL_BLEND);
#endif

            SetPixelShader(PS_Solid);
            ApplyColor(backgroundColor, 0);
            GMatrix2D m;
            m.SetIdentity();
            ApplyMatrix(m);

            // Draw a big quad.
            GLfloat bgVertBuffer[8];
            bgVertBuffer[0] = x0; bgVertBuffer[1] = y0;
            bgVertBuffer[2] = x1; bgVertBuffer[3] = y0;
            bgVertBuffer[4] = x0; bgVertBuffer[5] = y1;
            bgVertBuffer[6] = x1; bgVertBuffer[7] = y1;

            glEnableVertexAttribArray(Shaders[CurrentShader].pos);
            glVertexAttribPointer(Shaders[CurrentShader].pos, 2, GL_FLOAT, 0, 0, bgVertBuffer);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 
            glDisableVertexAttribArray(Shaders[CurrentShader].pos);
        }
    }


    // Clean up after rendering a frame.  Client program is still
    // responsible for calling glSwapBuffers() or whatever.
    void    EndDisplay()
    {
        ReleaseQueuedResources();
    }


    // Set the current transform for mesh & line-strip rendering.
    void    SetMatrix(const GRenderer::Matrix& m)
    {
        CurrentMatrix = m;
    }

    void    SetUserMatrix(const GRenderer::Matrix& m)
    {
        UserMatrix = m;
#ifndef GFC_NO_3D
        UVPMatricesChanged = 1;
#endif
    }

    // Set the current color transform for mesh & line-strip rendering.
    void    SetCxform(const GRenderer::Cxform& cx)
    {
        CurrentCxform = cx;
    }

    struct BlendModeDesc
    {
        GLenum op, src, dest;
    };

    struct BlendModeDescAlpha
    {
        GLenum op, srcc, srca, destc, desta;
    };

  void    ApplyBlendMode(BlendType mode, bool forceAc = 0, bool sourceAc = 0)
    {
        static BlendModeDesc modes[15] =
        {
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // None
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Normal
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Layer

            { GL_FUNC_ADD,              GL_DST_COLOR,           GL_ZERO                 }, // Multiply
            // (For multiply, should src be pre-multiplied by its inverse alpha?)

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Screen *??

            { GL_MAX,                   GL_SRC_ALPHA,           GL_ONE                  }, // Lighten
            { GL_MIN,                   GL_SRC_ALPHA,           GL_ONE                  }, // Darken

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Difference *??

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE                  }, // Add
            { GL_FUNC_REVERSE_SUBTRACT, GL_SRC_ALPHA,           GL_ONE                  }, // Subtract

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Invert *??

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Alpha *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Erase *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Overlay *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // HardLight *??
        };

        // Blending into alpha textures with premultiplied colors
        static BlendModeDescAlpha acmodes[15] =
        {
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // None
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Normal
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Layer

            { GL_FUNC_ADD,              GL_DST_COLOR,  GL_DST_ALPHA,  GL_ZERO,                GL_ZERO                 }, // Multiply

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Screen *??

            { GL_MAX,                   GL_SRC_ALPHA,  GL_SRC_ALPHA,  GL_ONE,                 GL_ONE                  }, // Lighten *??
            { GL_MIN,                   GL_SRC_ALPHA,  GL_SRC_ALPHA,  GL_ONE,                 GL_ONE                  }, // Darken *??

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Difference

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Add
            { GL_FUNC_REVERSE_SUBTRACT, GL_SRC_ALPHA,  GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Subtract

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Invert *??

            { GL_FUNC_ADD,              GL_ZERO,       GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Alpha *??
            { GL_FUNC_ADD,              GL_ZERO,       GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Erase *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Overlay *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // HardLight *??
        };

        // For debug build
        GASSERT(((UInt) mode) < 15);
        // For release
        if (((UInt) mode) >= 15)
            mode = Blend_None;

        if (RenderMode & GViewport::View_AlphaComposite || forceAc)
        {
	        BlendModeDescAlpha ms = acmodes[mode];
	        if (sourceAc && ms.srcc == GL_SRC_ALPHA)
	            ms.srcc = GL_ONE;
            glBlendFuncSeparate(ms.srcc, ms.destc, ms.srca, ms.desta);
            glBlendEquation(ms.op);
        }
        else
        {
	        BlendModeDesc ms = modes[mode];
	        if (sourceAc && ms.src == GL_SRC_ALPHA)
	            ms.src = GL_ONE;
            glBlendFunc(ms.src, ms.dest);
            glBlendEquation(ms.op);
        }
    }

    // Pushes a Blend mode onto renderer.
    virtual void    PushBlendMode(BlendType mode)
    {
        // Blend modes need to be applied cumulatively, ideally through an extra RT texture.
        // If the nested clip has a "Multiply" effect, and its parent has an "Add", the result
        // of Multiply should be written to a buffer, and then used in Add.

        // For now we only simulate it on one level -> we apply the last effect on top of stack.
        // (Note that the current "top" element is BlendMode, it is not actually stored on stack).
        // Although incorrect, this will at least ensure that parent's specified effect
        // will be applied to children.

        BlendModeStack.PushBack(BlendMode);

        if ((mode > Blend_Layer) && (BlendMode != mode))
        {
            BlendMode = mode;
            ApplyBlendMode(BlendMode);
        }
    }

    // Pops a blend mode, restoring it to the previous one. 
    virtual void    PopBlendMode()
    {
        if (BlendModeStack.GetSize() != 0)
        {                       
            // Find the next top interesting mode.
            BlendType   newBlendMode = Blend_None;

            for (SInt i = (SInt)BlendModeStack.GetSize()-1; i>=0; i--)
                if (BlendModeStack[i] > Blend_Layer)
                {
                    newBlendMode = BlendModeStack[i];
                    break;
                }

                BlendModeStack.PopBack();

                if (newBlendMode != BlendMode)
                {
                    BlendMode = newBlendMode;
                    ApplyBlendMode(BlendMode);
                }

                return;
        }

        // Stack was empty..
        GFC_DEBUG_WARNING(1, "GRendererGL::PopBlendMode - blend mode stack is empty");
    }

    void ApplySampleMode(BitmapWrapMode wrapMode, BitmapSampleMode mode, bool useMipmaps)
    {
        GLint filter = (mode == Sample_Point) ? GL_NEAREST : (useMipmaps ? GL_LINEAR_MIPMAP_LINEAR  : GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (filter == GL_NEAREST) ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);              

        if (wrapMode == Wrap_Clamp)
        {   
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else
        {
            GASSERT(wrapMode == Wrap_Repeat);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
    }

    // Set the given color.
    void ApplyColor(const GColor c, int alpha = 0)
    {
        if (alpha || 
            (BlendMode == Blend_Multiply) ||
            (BlendMode == Blend_Darken))
        {
            const float alpha = c.GetAlpha() * (1.0f / 65025.0f);
            glUniform4f(Shaders[CurrentShader].cxmul, c.GetRed() * alpha, c.GetGreen() * alpha, c.GetBlue() * alpha, 
                c.GetAlpha() * (1.0f / 255.0f));
        }
        else
        {
            const float m = (1.0f / 255.0f);
            glUniform4f(Shaders[CurrentShader].cxmul, c.GetRed() * m, c.GetGreen() * m, c.GetBlue() * m, c.GetAlpha() * m);
        }
    }

#ifndef GFC_NO_3D
    virtual void  SetPerspective3D(const GMatrix3D &projMatIn)
    {
        ProjMatrix = projMatIn;
        UVPMatricesChanged = 1;
    }

    virtual void  SetView3D(const GMatrix3D &viewMatIn) 
    {
        ViewMatrix = viewMatIn;
        UVPMatricesChanged = 1;
    }

    virtual void  SetWorld3D(const GMatrix3D *pWorldMatIn) 
    {
        pWorldMatrix = pWorldMatIn;
    }
#endif

    // multiply current GRenderer::Matrix with GL GRenderer::Matrix
    void    ApplyMatrix(const GRenderer::Matrix& m1)
    {
#ifndef GFC_NO_3D
        if (pWorldMatrix)
	    {
	        GASSERT(CurrentShader > PS_Count);

            if (UVPMatricesChanged)
            {
                UVPMatricesChanged = 0;
                UVPMatrix = UserMatrix;
                UVPMatrix.Append(ViewMatrix);
                UVPMatrix.Append(ProjMatrix);

                if (RenderTargetStack.GetSize())
                    Adjust3DMatrixForRT(UVPMatrix, ProjMatrix, RenderTargetStack[0].ViewMatrix, ViewportMatrix);
            }

	        GMatrix3D Final(m1);
            Final.Append(*pWorldMatrix);
	        Final.Append(UVPMatrix);
#if defined(GL_ES_VERSION_2_0)
            Final.Transpose();
	        glUniformMatrix4fv(Shaders[CurrentShader].mvp, 1, 0, Final);
#else
            glUniformMatrix4fv(Shaders[CurrentShader].mvp, 1, 1, Final);
#endif
	    }
	    else
#endif
	    {
	        GRenderer::Matrix m(ViewportMatrix);
	        m.Prepend(m1);

	        Float mvp[8];
	        mvp[0] = m.M_[0][0];
	        mvp[1] = m.M_[0][1];
	        mvp[2] = 0;
	        mvp[3] = m.M_[0][2];

	        mvp[4] = m.M_[1][0];
	        mvp[5] = m.M_[1][1];
	        mvp[6] = 0;
	        mvp[7] = m.M_[1][2];

	        glUniform4fv(Shaders[CurrentShader].mvp, 2, mvp);
	    }
    }

    void    ApplyMatrixForQuad(const GRenderer::Matrix& m1, SInt mvpi)
    {
#ifndef GFC_NO_3D
        if (pWorldMatrix)
        {
            if (UVPMatricesChanged)
            {
                UVPMatricesChanged = 0;
                UVPMatrix = UserMatrix;
                UVPMatrix.Append(ViewMatrix);
                UVPMatrix.Append(ProjMatrix);
            }

            GMatrix3D Final(m1);
            Final.Append(*pWorldMatrix);
            Final.Append(UVPMatrix);
#if defined(GL_ES_VERSION_2_0)
            Final.Transpose();
            glUniformMatrix4fv(mvpi, 1, 0, Final);
#else
            glUniformMatrix4fv(mvpi, 1, 1, Final);
#endif
        }
        else
#endif
        {
            GRenderer::Matrix m(ViewportMatrix);
            m.Prepend(m1);

            Float mvp[4][4];
            mvp[0][0] = m.M_[0][0];
            mvp[0][1] = m.M_[0][1];
            mvp[0][2] = 0;
            mvp[0][3] = m.M_[0][2];

            mvp[1][0] = m.M_[1][0];
            mvp[1][1] = m.M_[1][1];
            mvp[1][2] = 0;
            mvp[1][3] = m.M_[1][2];

            mvp[2][0] = 0;
            mvp[2][1] = 0;
            mvp[2][2] = 1.0f;
            mvp[2][3] = 0;

            mvp[3][0] = 0;
            mvp[3][1] = 0;
            mvp[3][2] = 0;
            mvp[3][3] = 1.0f;

            glUniformMatrix4fv(mvpi, 1, 0, &mvp[0][0]);
        }
    }

    // Don't fill on the {0 == left, 1 == right} side of a path.
    void    FillStyleDisable()
    {
        CurrentStyles[FILL_STYLE].Disable();
    }
    // Don't draw a line on this path.
    void    LineStyleDisable()
    {
        CurrentStyles[LINE_STYLE].Disable();
    }

    // Set fill style for the left interior of the shape.  If
    // enable is false, turn off fill for the left interior.
    void    FillStyleColor(GColor color)
    {
        CurrentStyles[FILL_STYLE].SetColor(CurrentCxform.Transform(color));
    }

    // Set the line style of the shape.  If enable is false, turn
    // off lines for following curve segments.
    void    LineStyleColor(GColor color)
    {
        CurrentStyles[LINE_STYLE].SetColor(CurrentCxform.Transform(color));
    }

    void    FillStyleBitmap(const FillTexture *pfill)
    {       
        CurrentStyles[FILL_STYLE].SetBitmap(pfill, CurrentCxform);
    }

    // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
    void    FillStyleGouraud(GouraudFillType gfill,
        const FillTexture *ptexture0,
        const FillTexture *ptexture1,
        const FillTexture *ptexture2)
    {
        CurrentStyles[FILL_STYLE].SetGouraudFill(gfill, ptexture0, ptexture1, ptexture2, CurrentCxform);
    }

    void    SetVertexData(const void* pvertices, int numVertices, VertexFormat vf, CacheProvider *pcache)
    {
        pVertexData = pvertices;
        VertexFmt   = vf;
        VertexArray = 0;

#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
        if (pcache && UseBuffers)
        {
            CachedData *pcd = pcache->GetCachedData(this);
            if (pcd)
            {
                VertexArray = ((GBufferNode*)pcd->GetRendererData())->buffer;
                return;
            }

            GLsizeiptr size;
            switch (vf)
            {
            case Vertex_XY16i:      size = 2 * sizeof(GLshort); break;
            case Vertex_XY32f:      size = 2 * sizeof(GLfloat); break;
            case Vertex_XY16iC32:   size = 2 * sizeof(GLshort) + 4; break;
            case Vertex_XY16iCF32:  size = 2 * sizeof(GLshort) + 8; break;
            default: return;
            }

            GBufferNode *pva;
            pcd = pcache->CreateCachedData(Cached_Vertex,this,0);

            if (!pcd->GetRendererData())
            {
                GLsizeiptr vasize = (size * numVertices + 15) & ~15;
                pva = new GBufferNode(&BufferObjects, pcd);
                pcd->SetRendererData(pva);
                glGenBuffers(1, &pva->buffer);

                glBindBuffer(GL_ARRAY_BUFFER, pva->buffer);
                glBufferData(GL_ARRAY_BUFFER, vasize, pvertices, GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                VertexArray = pva->buffer;
            }
        }
#endif
        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Vertex, (pvertices!=0),
            numVertices, (((pvertices!=0)&&numVertices) ? *((SInt16*)pvertices) : 0) );
    }

    void    SetIndexData(const void* pindices, int numIndices, IndexFormat idxf, CacheProvider *pcache)
    {
        pIndexData  = pindices;
        IndexArray  = 0;
        switch(idxf)
        {
        case Index_None:    IndexFmt = 0; break;
        case Index_16:      IndexFmt = GL_UNSIGNED_SHORT; break;
        case Index_32:
#ifdef GL_UNSIGNED_INT
            IndexFmt = GL_UNSIGNED_INT;
#else
            GFC_DEBUG_WARNING(1, "GL_UNSIGNED_INT not supported, no 32-bit indices.");
#endif
            break;
        }

#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
        if (pcache && UseBuffers)
        {
            CachedData *pcd = pcache->GetCachedData(this);
            if (pcd)
            {
                IndexArray = ((GBufferNode*)pcd->GetRendererData())->buffer;
                return;
            }

            GLsizeiptr size;
            switch (idxf)
            {
            case Index_16:      size = sizeof(GLshort); break;
            case Index_32:      size = sizeof(GLint); break;
            default: return;
            }

            GBufferNode *pva;
            pcd = pcache->CreateCachedData(Cached_Index,this,0);

            if (!pcd->GetRendererData())
            {
                pva = new GBufferNode(&BufferObjects, pcd);
                pcd->SetRendererData(pva);
                glGenBuffers(1, &pva->buffer);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pva->buffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * numIndices, pindices, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                IndexArray = pva->buffer;
            }
        }
#endif

        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Index, (pindices!=0),
            numIndices, (((pindices!=0)&&numIndices) ? *((SInt16*)pindices) : 0) );
    }

    void    ReleaseCachedData(CachedData *pdata, CachedDataType type)
    {
        // Releases cached data that was allocated from the cache providers.
        CacheList.ReleaseCachedData(pdata, type);

#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
        if (pdata->GetRendererData())
        {
            GBufferNode *pn = (GBufferNode*)pdata->GetRendererData();
            glDeleteBuffers(1, &pn->buffer);
            pn->RemoveNode();
            delete pn;
            pdata->SetRendererData(0);
        }
#endif
    }


    void    DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices,
        int startIndex, int triangleCount)
    {       
        GUNUSED2(minVertexIndex, numVertices);

        if (!pVertexData || !pIndexData || !IndexFmt) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(!pVertexData, "GRendererOGL::DrawIndexedTriList failed, vertex data not specified");
            GFC_DEBUG_WARNING(!pIndexData, "GRendererOGL::DrawIndexedTriList failed, index data not specified");
            GFC_DEBUG_WARNING(!IndexFmt, "GRendererOGL::DrawIndexedTriList failed, index buffer format not specified");
            return;
        }

        // Set up current style.
        CurrentStyles[FILL_STYLE].Apply(this);
        ApplyMatrix(CurrentMatrix);

        const void* pindices = (UByte*)(IndexArray ? 0 : pIndexData) + startIndex *
            ((IndexFmt == GL_UNSIGNED_SHORT) ? sizeof(UInt16) : sizeof(UInt32));
        const void *pVertexBase = pVertexData;

#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
        if (VertexArray)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VertexArray);
            pVertexBase = 0;
        }
        if (IndexArray)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexArray);
#endif

        // Send the tris to OpenGL
        glEnableVertexAttribArray(Shaders[CurrentShader].pos);

        // Gouraud colors
        if (VertexFmt == Vertex_XY16iC32)
        {
            glVertexAttribPointer(Shaders[CurrentShader].pos, 2, GL_SHORT, 0, sizeof(VertexXY16iC32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iC32));

            glEnableVertexAttribArray(Shaders[CurrentShader].color);
            glVertexAttribPointer(Shaders[CurrentShader].color, 4, GL_UNSIGNED_BYTE, 1, sizeof (VertexXY16iC32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iC32) + 2 * sizeof(UInt16));
        }
        else if (VertexFmt == Vertex_XY16iCF32)
        {
            glVertexAttribPointer(Shaders[CurrentShader].pos, 2, GL_SHORT, 0, sizeof(VertexXY16iCF32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iCF32));

            glEnableVertexAttribArray(Shaders[CurrentShader].color);
            glVertexAttribPointer(Shaders[CurrentShader].color, 4, GL_UNSIGNED_BYTE, 1, sizeof (VertexXY16iCF32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iCF32) + 2 * sizeof(UInt16));

            glEnableVertexAttribArray(Shaders[CurrentShader].factor);
            glVertexAttribPointer(Shaders[CurrentShader].factor, 4, GL_UNSIGNED_BYTE, 1, sizeof (VertexXY16iCF32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iCF32) + 2 * sizeof(UInt16) + 4);
        }
        else
        {
            glVertexAttribPointer(Shaders[CurrentShader].pos, 2, GL_SHORT, 0, sizeof(SInt16) * 2,
                ((UByte*)pVertexBase) + baseVertexIndex * 2 * sizeof(SInt16));
        }

        glDrawRangeElements(GL_TRIANGLES, minVertexIndex, 
            numVertices-1, 3*triangleCount, IndexFmt, pindices);

        RenderStats.Triangles += triangleCount;
        RenderStats.Primitives++;

        TriangleCnt.AddCount(triangleCount);
        DPTriangleCnt.AddCount(1);

        switch (VertexFmt)
        {
        case Vertex_XY16iCF32:
            glDisableVertexAttribArray(Shaders[CurrentShader].factor);
        case Vertex_XY16iC32:
            glDisableVertexAttribArray(Shaders[CurrentShader].color);
        default:
            glDisableVertexAttribArray(Shaders[CurrentShader].pos);
        }

#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
        if (VertexArray) glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (IndexArray) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
    }



    // Draw the line strip formed by the sequence of Points.
    void    DrawLineStrip(int baseVertexIndex, int lineCount)
    {       
        if (!pVertexData) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(!pVertexData, "GRendererOGL::DrawLineStrip failed, vertex data not specified");           
            return;
        }

        // Set up current style.
        CurrentStyles[LINE_STYLE].Apply(this);
        ApplyMatrix(CurrentMatrix);

        const void *pVertexBase = pVertexData;

#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
        if (VertexArray)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VertexArray);
            pVertexBase = 0;
        }
#endif

        // Send the line-strip to OpenGL
        glEnableVertexAttribArray(Shaders[CurrentShader].pos);
        glVertexAttribPointer(Shaders[CurrentShader].pos, 2, GL_SHORT, 0, sizeof(SInt16) * 2,
            ((UByte*)pVertexBase) + baseVertexIndex * 2 * sizeof(SInt16));
        glDrawArrays(GL_LINE_STRIP, 0, lineCount + 1);
        glDisableVertexAttribArray(Shaders[CurrentShader].pos);

        RenderStats.Lines += lineCount;
        RenderStats.Primitives++;

        LineCnt.AddCount(lineCount);
        DPLineCnt.AddCount(1);

#if defined(GL_ARB_vertex_buffer_object) || defined(GL_ES_VERSION_2_0)
        if (VertexArray) glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
    }


    // Draw a set of rectangles textured with the given bitmap, with the given color.
    // Apply given transform; ignore any currently set transforms.  
    //
    // Intended for textured glyph rendering.
    void    DrawBitmaps(BitmapDesc* pbitmapList, int listSize, int startIndex, int count,
        const GTexture* pti, const Matrix& m,
        CacheProvider *pcache)
    {
        if (!pbitmapList || !pti)
            return;

        // Test cache buffer management support.
        // Optimized implementation could use this spot to set vertex buffer and/or initialize offset in it.
        // Note that since bitmap lists are usually short, it would be a good idea to combine many of them
        // into one buffer, instead of creating individual buffers for each pbitmapList data instance.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_BitmapList, (pbitmapList!=0),
            listSize, (((pbitmapList!=0)&&listSize) ? ((SInt)pbitmapList->Coords.Left) : 0) );

#ifdef GFC_GL_BLEND_DISABLE
        glEnable(GL_BLEND);
#endif
        bool IsAlpha =
#ifdef GFC_GL_NO_ALPHA_TEXTURES
            ((GTextureOGL2Impl*)pti)->IsAlpha;
#else
            /*((GTextureOGL2Impl*)pti)->TextureFmt == GL_ALPHA ||*/ ((GTextureOGL2Impl*)pti)->TextureFmt == GL_LUMINANCE;
#endif

        if (((GTextureOGL2Impl*)pti)->IsYUVTexture() == 1)
        {
            IsAlpha = 0;
            if (BlendMode == Blend_Multiply || BlendMode == Blend_Darken)
                SetPixelShader(PS_TextTextureYUVMultiply);
            else
                SetPixelShader(PS_TextTextureYUV);
        }
        else if (((GTextureOGL2Impl*)pti)->IsYUVTexture() == 2)
        {
            IsAlpha = 0;
            if (BlendMode == Blend_Multiply || BlendMode == Blend_Darken)
                SetPixelShader(PS_TextTextureYUVAMultiply);
            else
                SetPixelShader(PS_TextTextureYUVA);
        }
        else if (IsAlpha)
            SetPixelShader(PS_TextTexture);
        else if (BlendMode == Blend_Multiply || BlendMode == Blend_Darken)
            SetPixelShader(PS_TextTextureColorMultiply);
        else
            SetPixelShader(PS_TextTextureColor);

        ((GTextureOGL2Impl*)pti)->Bind(0, Wrap_Clamp, Sample_Linear, IsAlpha);

        // Custom matrix per call.
        ApplyMatrix(m);
        ApplyPShaderCxform(CurrentCxform);

        SInt ibitmap = 0, ivertex = 0;
        GCOMPILER_ASSERT((GlyphVertexBufferSize%6) == 0);

        glEnableVertexAttribArray(Shaders[CurrentShader].pos);
        glEnableVertexAttribArray(Shaders[CurrentShader].tc[0]);

        glVertexAttribPointer(Shaders[CurrentShader].pos, 4, GL_FLOAT, 0, sizeof(GGlyphVertex), GlyphVertexBuffer);
        glVertexAttribPointer(Shaders[CurrentShader].tc[0], 2, GL_FLOAT, 0, sizeof(GGlyphVertex), ((UByte*)GlyphVertexBuffer) + sizeof(Float)*4);

        if (IsAlpha)
        {
            glEnableVertexAttribArray(Shaders[CurrentShader].color);
            glVertexAttribPointer(Shaders[CurrentShader].color, 4, GL_UNSIGNED_BYTE, 1, sizeof(GGlyphVertex), &GlyphVertexBuffer[0].color);
        }

        while (ibitmap < count)
        {
            for(ivertex = 0; (ivertex < GlyphVertexBufferSize) && (ibitmap<count); ibitmap++, ivertex+= 6)
            {
                BitmapDesc &  bd = pbitmapList[ibitmap + startIndex];
                GGlyphVertex* pv = GlyphVertexBuffer + ivertex;

                // Triangle 1.
                pv[0].SetVertex2D(bd.Coords.Left, bd.Coords.Top,    bd.TextureCoords.Left, bd.TextureCoords.Top, bd.Color);
                pv[1].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[2].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                // Triangle 2.
                pv[3].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                pv[4].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[5].SetVertex2D(bd.Coords.Right, bd.Coords.Bottom,bd.TextureCoords.Right, bd.TextureCoords.Bottom, bd.Color);
            }

            // Draw the generated triangles.
            if (ivertex)
            {
                glDrawArrays(GL_TRIANGLES, 0, ivertex);
                RenderStats.Primitives++;
                DPTriangleCnt.AddCount(1);
            }
        }

        glDisableVertexAttribArray(Shaders[CurrentShader].pos);
        glDisableVertexAttribArray(Shaders[CurrentShader].tc[0]);
        if (IsAlpha)
            glDisableVertexAttribArray(Shaders[CurrentShader].color);
        RenderStats.Triangles += count * 2;
        TriangleCnt.AddCount(count * 2);
    }

    void BeginSubmitMask(SubmitMaskMode maskMode)
    {
        DrawingMask = 1;
        glColorMask(0,0,0,0);                       // disable framebuffer writes
        SetPixelShader(PS_Solid);

#ifndef GFC_ZBUFFER_MASKING
        glEnable(GL_STENCIL_TEST);

        switch(maskMode)
        {
        case Mask_Clear:
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilFunc(GL_ALWAYS, 1, 1);             // Always pass, 1 bit plane, 1 as mask
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  // 
            StencilCounter = 1;
            break;

        case Mask_Increment:
            glStencilFunc(GL_EQUAL, StencilCounter, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
            StencilCounter++;
            break;

        case Mask_Decrement:
            glStencilFunc(GL_EQUAL, StencilCounter, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
            StencilCounter--;
            break;
        }

#else
        glEnable(GL_DEPTH_TEST);                    // enable the depth test and the depth write        
        glDepthMask(GL_TRUE);

        if (maskMode == Mask_Clear)
        {
#if defined(GL_VERSION_ES_CM_1_0)
            glClearDepthf(1.0);                      // clear the depth buffer to the farthest value
#else
            glClearDepth(1.0);                      // clear the depth buffer to the farthest value
#endif
            glClear(GL_DEPTH_BUFFER_BIT);
            glDepthFunc(GL_ALWAYS);                 // always write on the depth buffer
        }
        else
        {
            // Don't modify buffer for increment/decrement.
            glDepthFunc(GL_NEVER);
        }

#endif
        RenderStats.Masks++;
        MaskCnt.AddCount(1);
    }

    void EndSubmitMask()
    {        
        DrawingMask = 0;
        glColorMask(1,1,1,1);                       // Enable frame-buffer writes
        StencilEnabled = 1;

#ifndef GFC_ZBUFFER_MASKING
        // We draw only where the (stencil == StencilCounter); i.e. where the mask was drawn.
        // Don't change the stencil buffer    
        glStencilFunc(GL_EQUAL, StencilCounter, 0xFF);      
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);             
#else
        glEnable(GL_DEPTH_TEST);                    // Enable the depth test and the depth write
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_EQUAL);                      // We draw only where the mask was drawn with whatever depth is assumed by your driver.
        // According to the spec, this should be 0.0. If it is 1.0, change the clear depth above.
#endif 
    }

    void DisableMask()
    {   
        StencilEnabled = 0;

#ifndef GFC_ZBUFFER_MASKING
        glDisable(GL_STENCIL_TEST);
        StencilCounter = 0;
#else
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
#endif
    }

    virtual void        GetRenderStats(Stats *pstats, bool resetStats)
    {
        if (pstats)
            memcpy(pstats, &RenderStats, sizeof(Stats));
        if (resetStats)
            RenderStats.Clear();
    }

    virtual void        GetStats(GStatBag* pbag, bool reset)
    {
        if (pbag)
        {
            pbag->Add(GStatRender_Texture_VMem, &TextureVMem);
            pbag->Add(GStatRender_Buffer_VMem, &BufferVMem);
            
            pbag->Add(GStatRender_TextureUpload_Cnt, &TextureUploadCnt);
            pbag->Add(GStatRender_TextureUpdate_Cnt, &TextureUpdateCnt);

            pbag->Add(GStatRender_DP_Line_Cnt, &DPLineCnt);
            pbag->Add(GStatRender_DP_Triangle_Cnt, &DPTriangleCnt);
            pbag->Add(GStatRender_Line_Cnt, &LineCnt);
            pbag->Add(GStatRender_Triangle_Cnt, &TriangleCnt);
            pbag->Add(GStatRender_Mask_Cnt, &MaskCnt);
            pbag->Add(GStatRender_Filter_Cnt, &FilterCnt);
        }

        if (reset)
        {
            TextureUploadCnt.Reset();
            TextureUpdateCnt.Reset();

            DPLineCnt.Reset();
            DPTriangleCnt.Reset();
            LineCnt.Reset();
            TriangleCnt.Reset();
            MaskCnt.Reset();
            FilterCnt.Reset();
        }
    }

    static bool CheckExtension (const char *exts, const char *name)
    {
        const char *p = strstr(exts, name);
        return (p && (p[strlen(name)] == 0 || p[strlen(name)] == ' '));
    }

    // Specify pixel format (?)
    // (we can also get it from the window)
    // Specify OpenGL context ?
    virtual bool                SetDependentVideoMode()
    {
        if (Initialized)
            return 1;

#ifdef GFC_GL_RUNTIME_LINK
        p_glBlendEquation           = (PFNGLBLENDEQUATIONPROC) GFC_GL_RUNTIME_LINK ("glBlendEquation");
        p_glBlendFuncSeparate       = (PFNGLBLENDFUNCSEPARATEPROC) GFC_GL_RUNTIME_LINK ("glBlendFuncSeparate");
        p_glDrawRangeElements       = (PFNGLDRAWRANGEELEMENTSPROC) GFC_GL_RUNTIME_LINK("glDrawRangeElements");
        p_glVertexAttrib2f         = (PFNGLMULTITEXCOORD2FPROC) GFC_GL_RUNTIME_LINK("glVertexAttrib2f");
        p_glActiveTexture           = (PFNGLACTIVETEXTUREPROC) GFC_GL_RUNTIME_LINK("glActiveTexture");
        p_glClientActiveTexture     = (PFNGLCLIENTACTIVETEXTUREPROC) GFC_GL_RUNTIME_LINK("glClientActiveTexture");
        p_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) GFC_GL_RUNTIME_LINK("glEnableVertexAttribArray");
        p_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) GFC_GL_RUNTIME_LINK("glDisableVertexAttribArray");
        p_glVertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC) GFC_GL_RUNTIME_LINK("glVertexAttribPointer");
        p_glVertexAttrib4f          = (PFNGLVERTEXATTRIB4FPROC) GFC_GL_RUNTIME_LINK("glVertexAttrib4f");
        p_glDeleteShader            = (PFNGLDELETESHADERPROC) GFC_GL_RUNTIME_LINK ("glDeleteShader");
        p_glDeleteProgram           = (PFNGLDELETEPROGRAMPROC) GFC_GL_RUNTIME_LINK ("glDeleteProgram");
        p_glCreateShader            = (PFNGLCREATESHADERPROC) GFC_GL_RUNTIME_LINK ("glCreateShader");
        p_glShaderSource            = (PFNGLSHADERSOURCEARBPROC) GFC_GL_RUNTIME_LINK ("glShaderSource");
        p_glCompileShader           = (PFNGLCOMPILESHADERARBPROC) GFC_GL_RUNTIME_LINK ("glCompileShader");
        p_glCreateProgram           = (PFNGLCREATEPROGRAMPROC) GFC_GL_RUNTIME_LINK ("glCreateProgram");
        p_glAttachShader            = (PFNGLATTACHSHADERPROC) GFC_GL_RUNTIME_LINK ("glAttachShader");
        p_glLinkProgram             = (PFNGLLINKPROGRAMPROC) GFC_GL_RUNTIME_LINK ("glLinkProgram");
        p_glUseProgram              = (PFNGLUSEPROGRAMPROC) GFC_GL_RUNTIME_LINK ("glUseProgram");
        p_glGetShaderInfoLog        = (PFNGLGETSHADERINFOLOGPROC) GFC_GL_RUNTIME_LINK ("glGetShaderInfoLog");
        p_glGetProgramInfoLog       = (PFNGLGETPROGRAMINFOLOGPROC) GFC_GL_RUNTIME_LINK ("glGetProgramInfoLog");
        p_glGetAttribLocation       = (PFNGLGETATTRIBLOCATIONPROC) GFC_GL_RUNTIME_LINK ("glGetAttribLocation");
        p_glGetUniformLocation      = (PFNGLGETUNIFORMLOCATIONPROC) GFC_GL_RUNTIME_LINK ("glGetUniformLocation");
        p_glBindAttribLocation      = (PFNGLBINDATTRIBLOCATIONPROC) GFC_GL_RUNTIME_LINK ("glBindAttribLocation");
        p_glUniform1i               = (PFNGLUNIFORM1IPROC) GFC_GL_RUNTIME_LINK ("glUniform1i");
        p_glUniform2f               = (PFNGLUNIFORM2FPROC) GFC_GL_RUNTIME_LINK ("glUniform2f");
        p_glUniform4f               = (PFNGLUNIFORM4FPROC) GFC_GL_RUNTIME_LINK ("glUniform4f");
        p_glUniform4fv              = (PFNGLUNIFORM4FVPROC) GFC_GL_RUNTIME_LINK ("glUniform4fv");
        p_glUniformMatrix4fv        = (PFNGLUNIFORMMATRIX4FVPROC) GFC_GL_RUNTIME_LINK ("glUniformMatrix4fv");
        p_glGetShaderiv             = (PFNGLGETSHADERIVPROC) GFC_GL_RUNTIME_LINK("glGetShaderiv");
        p_glGetProgramiv            = (PFNGLGETPROGRAMIVPROC) GFC_GL_RUNTIME_LINK("glGetProgramiv");
        p_glGenBuffers              = (PFNGLGENBUFFERSARBPROC) GFC_GL_RUNTIME_LINK("glGenBuffersARB");
        p_glDeleteBuffers           = (PFNGLDELETEBUFFERSARBPROC) GFC_GL_RUNTIME_LINK("glDeleteBuffersARB");
        p_glBindBuffer              = (PFNGLBINDBUFFERARBPROC) GFC_GL_RUNTIME_LINK("glBindBufferARB");
        p_glBufferData              = (PFNGLBUFFERDATAARBPROC) GFC_GL_RUNTIME_LINK("glBufferDataARB");
        p_glCompressedTexImage2D    = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) GFC_GL_RUNTIME_LINK("glCompressedTexImage2D");
        p_glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) GFC_GL_RUNTIME_LINK ("glBindRenderbufferEXT");
        p_glDeleteRenderBuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) GFC_GL_RUNTIME_LINK ("glDeleteRenderbuffersEXT");
        p_glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) GFC_GL_RUNTIME_LINK ("glGenRenderbuffersEXT");
        p_glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) GFC_GL_RUNTIME_LINK ("glBindFramebufferEXT");
        p_glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) GFC_GL_RUNTIME_LINK ("glDeleteFramebuffersEXT");
        p_glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) GFC_GL_RUNTIME_LINK ("glGenFramebuffersEXT");
        p_glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) GFC_GL_RUNTIME_LINK ("glFramebufferTexture2DEXT");
        p_glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) GFC_GL_RUNTIME_LINK ("glFramebufferRenderbufferEXT");
        p_glFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) GFC_GL_RUNTIME_LINK ("glFramebufferAttachmentParameterivEXT");
        p_glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) GFC_GL_RUNTIME_LINK("glRenderbufferStorageEXT");
        p_glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) GFC_GL_RUNTIME_LINK("glCheckFramebufferStatusEXT");
        p_glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC) GFC_GL_RUNTIME_LINK("glGenerateMipmapEXT");
#endif

        UseBuffers = 0;
        UseLowEndFilters = 0;

        const char *glexts = (const char *) glGetString(GL_EXTENSIONS);

#if defined(GL_ES_VERSION_2_0)
        UseBuffers = 1;
        TexNonPower2 = CheckExtension(glexts, "GL_OES_texture_npot") ? 2 : 1;
	    UseFbo = 1;
	    if (CheckExtension(glexts, "GL_OES_required_internalformat") &&
	        CheckExtension(glexts, "GL_IMG_texture_format_BGRA8888"))
	        UseBgra = 1;
	    else
	        UseBgra = 0;
#else
        UseFbo = CheckExtension(glexts, "EXT_framebuffer_object");
#endif

    	//GFC_DEBUG_MESSAGE2(1, "GL Renderer: %s\nGL Extensions: %s", (const char*) glGetString(GL_RENDERER), glexts);

#if defined(GL_ES_VERSION_2_0)
        PackedDepthStencil = CheckExtension(glexts, "OES_packed_depth_stencil");
#else
        PackedDepthStencil = CheckExtension(glexts, "EXT_packed_depth_stencil");
#endif

	    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &MaxAttributes);

#if defined(GL_ES_VERSION_2_0)
	    glGetIntegerv(GL_MAX_VARYING_VECTORS, &MaxTexCoords);
	    MaxTexCoords -= 2;
#else
	    glGetIntegerv(GL_MAX_TEXTURE_COORDS, &MaxTexCoords);
#endif

	    // more conservative than needed, but "low end" mode becomes faster at large sizes
	    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MaxTextureOps);

        pScreenRenderTarget = *(GRenderTargetOGL2Impl*)CreateRenderTarget();
        pCurRenderTarget = pScreenRenderTarget;
        pCurRenderTarget->FboId = 0;

        if (InitShaders())
        {
            Initialized = 1;
            return 1;
        }
        else
            return 0;
    }

    // Returns back to original mode (cleanup)                                                      
    virtual bool                ResetVideoMode()
    {
        ReleaseQueuedResources();
        return 1;
    }

    virtual DisplayStatus       CheckDisplayStatus() const
    {
        return Initialized ? DisplayStatus_Ok : DisplayStatus_NoModeSet;
    }

    inline void SetCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
    {
        glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);   
    }

};  // end class GRendererOGL2Impl




// ***** GTextureOGL implementation

GTextureOGL2Impl::GTextureOGL2Impl(GRendererOGL2Impl *prenderer)
: GTextureOGL(&prenderer->Textures)
{
    pRenderer   = prenderer;
    TextureId   = 0;
    TexData     = 0;
    Mipmaps     = 0;
}

GTextureOGL2Impl::~GTextureOGL2Impl()
{
    ReleaseTextureId(); 
    if (!pRenderer)
        return;
    GLock::Locker guard(&pRenderer->TexturesLock);
    if (pFirst)
        RemoveNode();
}

// Obtains the renderer that create TextureInfo 
GRenderer*  GTextureOGL2Impl::GetRenderer() const
{ return pRenderer; }
bool        GTextureOGL2Impl::IsDataValid() const
{ return (TextureId > 0);   }

// Remove texture from renderer, notifies renderer destruction
void    GTextureOGL2Impl::RemoveFromRenderer()
{
    pRenderer = 0;
    if (AddRef_NotZero())
    {
        ReleaseTextureId();
        CallHandlers(ChangeHandler::Event_RendererReleased);
        if (pNext) // We may have been released by user
            RemoveNode();
        Release();
    } else {
        if (pNext) // We may have been released by user
            RemoveNode();
    }
}

// Creates a texture id ans sets filter parameters, initializes Width/Height
void    GTextureOGL2Impl::InitTextureId(GLint minFilter)
{
    // Create the texture.
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, (GLuint*)&TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);   

    DeleteTexture = 1;
}

// Releases texture and clears values
void    GTextureOGL2Impl::ReleaseTextureId()
{
    if (TextureId > 0)
    {
        if (DeleteTexture)
        {
            if (pRenderer)
                pRenderer->AddResourceForReleasing(TextureId);
            else
                glDeleteTextures(1, &TextureId);
        }
        TextureId = 0;
    }

    if (TexData)
    {
        for (int i = 0; i < Mipmaps; i++)
        {
            if (TexData[i])
                GFREE(TexData[i]);
        }
        GFREE(TexData);
        TexData = 0;
    }
}


// Forward declarations for sampling/mipmaps
static void    SoftwareResample(int bytesPerPixel, bool useBgra, int srcWidth, int srcHeight, int srcPitch, UByte* psrcData, int dstWidth, int dstHeight);


bool GTextureOGL2Impl::InitTexture(UInt texID, bool deleteTexture)
{
    ReleaseTextureId();

    if (texID)
    {
        TextureId = texID;
        DeleteTexture = deleteTexture;
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

bool GTextureOGL2Impl::InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage)
{
    ReleaseTextureId();

    GLint   internalFormat=0, texFormat=0;
    GLint   datatype = GL_UNSIGNED_BYTE;
    GLuint  bpp = 0;
    //bool    resample = 0;

#ifdef GFC_GL_NO_ALPHA_TEXTURES
    IsAlpha = 0;
#endif

    if (format == GImage::Image_ARGB_8888)
    {
        bpp             = 4;
#if defined(GL_ES_VERSION_2_0)
    if (pRenderer->UseBgra)
	{
	    texFormat = internalFormat = GL_BGRA;
	}
	else
#endif
	{
	    texFormat       = GL_RGBA;
#if defined(GFC_OS_WIN32) && defined(GL_BGRA)
	    internalFormat  = (usage & Usage_Map) ? GL_BGRA : GL_RGBA;
#else
	    internalFormat  = GL_RGBA;
#endif
	}
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
    }
    else if (format == GImage::Image_RGB_888)
    {
        bpp             = 3;
        texFormat       = GL_RGB;
#if defined(GFC_OS_WIN32) && defined(GL_BGR)
        internalFormat  = (usage & Usage_Map) ? GL_BGR : GL_RGB;
#else
        internalFormat  = GL_RGB;
#endif
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
    }
    else if (format == GImage::Image_A_8)
    {
#ifdef GFC_GL_NO_ALPHA_TEXTURES
        if (usage & Usage_Update)
        {
            IsAlpha = 1;
            texFormat       = GL_RGBA;
            internalFormat  = GL_RGBA;
#ifdef GFC_OS_PS3
            datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
        }
        else
#endif
        {
            internalFormat  = GL_LUMINANCE;
            texFormat       = GL_LUMINANCE;
            bpp             = 1;
        }
    }
    else
        GASSERT(0);

    InitTextureId(GL_LINEAR);
    TextureFmt = internalFormat;
    TextureData = datatype;
    TexWidth = width;
    TexHeight = height;
    Mipmaps = mipmaps+1;

    if (usage & Usage_Map)
    {
        TexData = (GLubyte**)GALLOC(sizeof(GLubyte*) * Mipmaps, GStat_Default_Mem);
        TexData[0] = (GLubyte*)GALLOC(bpp * TexWidth * TexHeight, GStat_Default_Mem);
        memset(TexData[0], 255, bpp * TexWidth * TexHeight);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, texFormat, width, height, 0, internalFormat, datatype, 0);

#if defined(GL_TEXTURE_MAX_LEVEL)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmaps);
#endif

    for (int i = 0; i < mipmaps; i++)
    {
        width >>= 1;
        height >>= 1;
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;
        glTexImage2D(GL_TEXTURE_2D, i+1, texFormat, width, height, 0, internalFormat, datatype, 0);

        if (usage & Usage_Map)
        {
            TexData[i+1] = (GLubyte*)GALLOC(bpp * width * height, GStat_Default_Mem);
            memset(TexData[i+1], 255, bpp * width * height);
        }
    }

#ifdef GL_ES_VERSION_2_0
    GFC_DEBUG_WARNING(mipmaps && (width > 1 || height > 1),
        "GRendererOGL: updatable texture created with incomplete mipmaps");
#endif

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

// NOTE: This function destroys pim's data in the process of making mipmaps.
bool GTextureOGL2Impl::InitTexture(GImageBase* pim, UInt usage)
{   
    GUNUSED(usage);

    // Delete old data
    ReleaseTextureId();
    if (!pim)
    {
        // Kill texture     
        CallHandlers(ChangeHandler::Event_DataChange);
        return 1;
    }

    // Determine format
    UInt    bytesPerPixel=0;
    GLint   internalFormat=0;
    GLint   datatype = GL_UNSIGNED_BYTE;
    bool    resample = 0;

    if (pim->Format == GImage::Image_ARGB_8888)
    {
        bytesPerPixel   = 4;
        internalFormat  = GL_RGBA;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
    }
    else if (pim->Format == GImage::Image_RGB_888)
    {
        bytesPerPixel   = 3;
        internalFormat  = GL_RGB;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
        resample        = 1;
#endif
    }
    else if (pim->Format == GImage::Image_A_8)
    {
        return InitTextureAlpha(pim);
    }
#ifdef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
    else if (pim->Format == GImage::Image_DXT1)
    {
        bytesPerPixel   = 1;
        internalFormat  = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    }
    else if (pim->Format == GImage::Image_DXT3)
    {
        bytesPerPixel   = 1;
        internalFormat  = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    }
    else if (pim->Format == GImage::Image_DXT5)
    {
        bytesPerPixel   = 1;
        internalFormat  = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    }
#endif
    else
    { // Unsupported format
        GASSERT(0);
    }

#ifdef GFC_GL_NO_ALPHA_TEXTURES
    IsAlpha = 0;
#endif

    UInt    w = pim->Width;
    UInt    h = pim->Height;

#if defined(GL_ES_VERSION_2_0)
    if (pRenderer->TexNonPower2 < 2)
    {
        w = 1; while (w < pim->Width) { w <<= 1; }
        h = 1; while (h < pim->Height) { h <<= 1; }
    }

    GPtr<GImage> pswapImage;

    if (pRenderer->UseBgra && (internalFormat == GL_RGBA || internalFormat == GL_RGB))
    {
	pswapImage = *new GImage(GImage::Image_ARGB_8888, pim->Width, pim->Height);

	if (internalFormat == GL_RGB)
	    for (UInt y = 0; y < pim->Height; y++)
	    {
		UByte*  pdest    = (UByte*) pswapImage->GetScanline(y);
		UByte*   psrc    = (UByte*) pim->GetScanline(y);
		for (UInt x = 0; x < pim->Width; x++)
		{
		    pdest[x * 4 + 2] = psrc[x * 3 + 0]; // red
		    pdest[x * 4 + 1] = psrc[x * 3 + 1]; // green
		    pdest[x * 4 + 0] = psrc[x * 3 + 2]; // blue
		    pdest[x * 4 + 3] = 255; // alpha
		}
	    }
	else
	    for (UInt y = 0; y < pim->Height; y++)
	    {
		UInt32*  pdest    = (UInt32*) pswapImage->GetScanline(y);
		UInt32*  psrc    = (UInt32*) pim->GetScanline(y);
		for (UInt x = 0; x < pim->Width; x++)
		    pdest[x] = (psrc[x] & 0xff00ff00) | ((psrc[x] & 0xff) << 16) | ((psrc[x] >> 16) & 0xff);
	    }

	internalFormat = GL_BGRA;
	pim = pswapImage;
	bytesPerPixel = 4;
    }
#endif

    // Create the texture.
    InitTextureId(GL_LINEAR);
    TextureFmt = internalFormat;
    TextureData = datatype;

    TexWidth = w;
    TexHeight = h;

    if (!pim->IsDataCompressed() && (w != pim->Width || h != pim->Height || resample))
    {
        GASSERT_ON_RENDERER_RESAMPLING;
#if defined(GL_TEXTURE_MAX_LEVEL)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif
	Mipmaps = 1;

        // Faster/simpler software bilinear rescale.
#if defined(GL_ES_VERSION_2_0)
        SoftwareResample(bytesPerPixel, pRenderer->UseBgra, pim->Width, pim->Height, pim->Pitch, pim->pData, w, h);
#else
        SoftwareResample(bytesPerPixel, 0, pim->Width, pim->Height, pim->Pitch, pim->pData, w, h);
#endif
    }
    else
    {
        // Use original image directly.
        UInt level = 0;
        UInt mipW, mipH;
        do 
        {
            const UByte* pdata = pim->GetMipMapLevelData(level, &mipW, &mipH);
            if (pdata == 0) //????
            {
                GFC_DEBUG_WARNING(1, "GRendererOGL: can't find mipmap level in texture");
                break;
            }
            if (pim->IsDataCompressed())
            {
                pRenderer->SetCompressedTexImage2D(GL_TEXTURE_2D, level, internalFormat, mipW, mipH, 0, 
                    GImage::GetMipMapLevelSize(pim->Format, mipW, mipH), pdata);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, level, internalFormat, mipW, mipH, 0, internalFormat, datatype,
                    pdata);
            }
        } while(++level < pim->MipMapCount);

#if !defined(GL_TEXTURE_MAX_LEVEL)
        GFC_DEBUG_WARNING(level > 1 && (mipW > 1 || mipH > 1),
            "GRendererOGL: texture with incomplete mipmaps");
#else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level-1);
#endif
	Mipmaps = level;
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

bool    GTextureOGL2Impl::InitTextureAlpha(GImageBase* pim)
{
    GASSERT(pim);
    int width  = pim->Width;
    int height = pim->Height;
    UByte* data = pim->pData;

    bool    retVal = 1;
    // Delete old data
    ReleaseTextureId(); 
    if (!data)
    {
        // Kill texture
empty_texture:      
        CallHandlers(ChangeHandler::Event_DataChange);
        return retVal;
    }

    GASSERT(width > 0);
    GASSERT(height > 0);

    InitTextureId(GL_LINEAR_MIPMAP_LINEAR);
    // Copy data to avoid destruction
#ifdef GFC_GL_NO_ALPHA_TEXTURES
    IsAlpha = 1;
    UByte *pnewData = (UByte*)GALLOC(width * height * 4, GStat_Default_Mem);
    if (!pnewData)
    {
        retVal = 0;
        ReleaseTextureId();
        goto empty_texture;
    }       

    int len = width * height * 4;
    for( int i = 0, j = 0; i < len; i += 4, j++ )
    {
        pnewData[i] = 255;
        pnewData[i+1] = 255;
        pnewData[i+2] = 255;
        pnewData[i+3] = data[j];
    }
#else
    UByte *pnewData = 0;
    if (pim->MipMapCount <= 1)
    {
        pnewData = (UByte*)GALLOC(width * height, GStat_Default_Mem);
        if (!pnewData)
        {
            retVal = 0;
            ReleaseTextureId();
            goto empty_texture;
        }       
        memcpy(pnewData, data, width*height);
    }
    else
        pnewData = data;
#endif

#if defined(GFC_BUILD_DEBUG) && defined(GL_ES_VERSION_2_0)
    if (pRenderer->TexNonPower2)
    {
        // must use power-of-two dimensions
        int w = 1; while (w < width) { w <<= 1; }
        int h = 1; while (h < height) { h <<= 1; }
        GASSERT(w == width);
        GASSERT(h == height);
    }
#endif

    TexWidth = width;
    TexHeight = height;

#ifdef GFC_GL_NO_ALPHA_TEXTURES
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pnewData);

    // Build mips.
    int level = 1;
    while (width > 1 || height > 1)
    {
        GRendererOGL2Impl::MakeNextMiplevel(&width, &height, pnewData, GL_RGBA);
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pnewData);
        level++;
    }
    TextureFmt = GL_RGBA;
    TextureData = GL_UNSIGNED_INT_8_8_8_8;
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pnewData);

    int level = 1;
    if (pim->MipMapCount <= 1)
    {
        // Build mips.
        while (width > 1 || height > 1)
        {
            GRendererOGL2Impl::MakeNextMiplevel(&width, &height, pnewData);
            glTexImage2D(GL_TEXTURE_2D, level, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pnewData);
            level++;
        }
    }
    else
    {
        while(level < int(pim->MipMapCount)) 
        {
            UInt mipW, mipH;
            const UByte* pmipdata = pim->GetMipMapLevelData(level, &mipW, &mipH);
            if (pmipdata == 0) //????
            {
                GFC_DEBUG_WARNING(1, "GRendererOGL: can't find mipmap level in texture");
                break;
            }
            glTexImage2D(GL_TEXTURE_2D, level, GL_LUMINANCE, mipW, mipH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 
                pmipdata);
            ++level;
        }
    }
    TextureFmt = GL_LUMINANCE;
    TextureData = GL_UNSIGNED_BYTE;
#endif
#if !defined(GL_TEXTURE_MAX_LEVEL)
    GFC_DEBUG_WARNING(level > 1 && (width > 1 || height > 1),
        "GRendererOGL: texture with incomplete mipmaps");
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level-1);
#endif

    if (pnewData != data)
        GFREE(pnewData);    
    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

void GTextureOGL2Impl::Update(int level, int n, const UpdateRect *rects, const GImageBase *pim)
{
    GLenum datatype = GL_UNSIGNED_BYTE;
    GLenum internalFormat = GL_RGBA; // avoid warning.
    bool   convert = 0;

    if (pim->Format == GImage::Image_ARGB_8888)
    {
        internalFormat  = GL_RGBA;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
        //        if (TextureFmt != internalFormat || TextureData != datatype)
#endif
    }
    else if (pim->Format == GImage::Image_RGB_888)
        internalFormat  = GL_RGB;
    else if (pim->Format == GImage::Image_A_8)
    {
#ifdef GFC_GL_NO_ALPHA_TEXTURES
        convert         = 1;
        internalFormat  = GL_RGBA;
# ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
# endif
#else
        internalFormat = GL_LUMINANCE;
#endif
    }

    glBindTexture(GL_TEXTURE_2D, TextureId);

#if (defined(GL_UNPACK_ROW_LENGTH) && defined(GL_UNPACK_ALIGNMENT))
    if (!convert && pim->Pitch == pim->Width * pim->GetBytesPerPixel())
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pim->Width);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (int i = 0; i < n; i++)
            glTexSubImage2D(GL_TEXTURE_2D, level,
            rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
            internalFormat, datatype, pim->pData + pim->Pitch * rects[i].src.Top + pim->GetBytesPerPixel() * rects[i].src.Left);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    else if (!convert && pim->Pitch == ((3 + pim->Width * pim->GetBytesPerPixel()) & ~3))
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pim->Width);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        for (int i = 0; i < n; i++)
            glTexSubImage2D(GL_TEXTURE_2D, level,
            rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
            internalFormat, datatype, pim->pData + pim->Pitch * rects[i].src.Top + pim->GetBytesPerPixel() * rects[i].src.Left);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    else
#endif
        if (convert) // convert alpha to rgba
        {
            int s = rects[0].src.Width() * rects[0].src.Height();
            for (int i = 0; i < n; i++)
                if (rects[i].src.Width() * rects[i].src.Height() > s)
                    s = rects[i].src.Width() * rects[i].src.Height();
            UByte *pdata = (UByte*)GALLOC(s * 4, GStat_Default_Mem);

            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < rects[i].src.Height(); j++)
                    for (int k = 0; k < rects[i].src.Width(); k++)
                    {
                        pdata[k*4+j*rects[i].src.Width()*4+0] = 255;
                        pdata[k*4+j*rects[i].src.Width()*4+1] = 255;
                        pdata[k*4+j*rects[i].src.Width()*4+2] = 255;
                        pdata[k*4+j*rects[i].src.Width()*4+3] =
                            pim->pData[pim->Pitch * (j + rects[i].src.Top) +
                            pim->GetBytesPerPixel() * (k + rects[i].src.Left)];
                    }

                    glTexSubImage2D(GL_TEXTURE_2D, level,
                        rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
                        internalFormat, datatype, pdata);
            }

            GFREE(pdata);
        }
        else
        {
            int s = rects[0].src.Width() * rects[0].src.Height();
            for (int i = 0; i < n; i++)
                if (rects[i].src.Width() * rects[i].src.Height() > s)
                    s = rects[i].src.Width() * rects[i].src.Height();
            UByte *pdata = (UByte*)GALLOC(s * pim->GetBytesPerPixel(), GStat_Default_Mem);

#if defined(GL_UNPACK_ALIGNMENT)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#endif

            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < rects[i].src.Height(); j++)
                    memcpy(pdata + j * rects[i].src.Width() * pim->GetBytesPerPixel(),
                    pim->pData + pim->Pitch * (j + rects[i].src.Top) + pim->GetBytesPerPixel() * rects[i].src.Left,
                    rects[i].src.Width() * pim->GetBytesPerPixel());

                glTexSubImage2D(GL_TEXTURE_2D, level,
                    rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
                    internalFormat, datatype, pdata);
            }

            GFREE(pdata);
        }

        CallHandlers(ChangeHandler::Event_DataChange);
}

int     GTextureOGL2Impl::Map(int level, int n, MapRect* maps, int flags)
{
    GUNUSED2(flags,n);
    GASSERT(level < Mipmaps && n > 0 && maps && TexData[level]);

    UInt bpp=0;
    switch (TextureFmt)
    {
    case GL_BGRA:
    case GL_RGBA:
        bpp = 4;
        break;
    case GL_RGB:
        bpp = 3;
        break;
    case GL_ALPHA:
    case GL_LUMINANCE:
        bpp = 1;
        break;
    }
    UInt mipw = TexWidth, miph = TexHeight;
    for (int i = 0; i < level; i++)
    {
        mipw >>= 1;
        miph >>= 1;
        if (mipw < 1)
            mipw = 1;
        if (miph < 1)
            miph = 1;
    }

    maps[0].width = mipw;
    maps[0].height = miph;
    maps[0].pitch = mipw * bpp;
    maps[0].pData = TexData[level];
    return 1;
}

bool    GTextureOGL2Impl::Unmap(int level, int n, MapRect* maps, int flags)
{
    GUNUSED2(flags,n);

    GLuint texFormat = TextureFmt;
#if defined(GFC_OS_WIN32) && defined(GL_BGRA)
    if (texFormat == GL_BGRA)
        texFormat = GL_RGBA;
    else if (texFormat == GL_BGR)
        texFormat = GL_RGB;
#endif
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexImage2D(GL_TEXTURE_2D, level, texFormat, maps[0].width, maps[0].height, 0, TextureFmt, TextureData, maps[0].pData);
    return 0;
}

void GTextureOGL2Impl::Bind(int stageIndex, GRenderer::BitmapWrapMode WrapMode, GRenderer::BitmapSampleMode SampleMode, bool useMipmaps)
{
    pRenderer->glActiveTexture(GL_TEXTURE0 + stageIndex);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    pRenderer->glUniform1i(pRenderer->Shaders[pRenderer->CurrentShader].tex[stageIndex], stageIndex);
    pRenderer->ApplySampleMode(WrapMode, SampleMode, useMipmaps && Mipmaps > 1);
}

GTextureOGL2ImplYUV::~GTextureOGL2ImplYUV()
{
    ReleaseTextureId();
}

void    GTextureOGL2ImplYUV::ReleaseTextureId()
{
    for (int i = 0; i < 3; i++)
    {
        if (TextureUVA[i])
        {
            if (DeleteTexture)
            {
                if (pRenderer)
                    pRenderer->AddResourceForReleasing(TextureId);
                else
                    glDeleteTextures(1, TextureUVA+i);
            }
            for (int j = 0; j < Mipmaps; j++)
                if (TexData[Mipmaps+i*Mipmaps+j])
                    GFREE(TexData[Mipmaps+i*Mipmaps+j]);
        }
        TextureUVA[i] = 0;
    }

    if (TextureId)
    {
        if (DeleteTexture)
        {
            if (pRenderer)
                pRenderer->AddResourceForReleasing(TextureId);
            else
                glDeleteTextures(1, &TextureId);
        }

        for (int j = 0; j < Mipmaps; j++)
            if (TexData[j])
                GFREE(TexData[j]);
        TextureId = 0;
    }

    GFREE(TexData);
    TexData = 0;
}

bool GTextureOGL2ImplYUV::InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage)
{
    if (!pRenderer)
        return 0;

    GUNUSED(usage);
    GASSERT(usage & Usage_Map);
    ReleaseTextureId();

    TexWidth = width;
    TexHeight = height;
    DeleteTexture = 1;

    UInt ntex = (format == GImage::Image_ARGB_8888 ? 4 : 3);
    Mipmaps = mipmaps+1;
    TextureFmt = GL_LUMINANCE;
    TextureData = GL_UNSIGNED_BYTE;
    TexData = (GLubyte**)GALLOC(sizeof(GLubyte*) * Mipmaps * ntex, GStat_Default_Mem);
    UInt mipw = TexWidth, miph = TexHeight;
    for (int i = 0; i < Mipmaps; i++)
    {
        TexData[i] = (GLubyte*)GALLOC(mipw * miph, GStat_Default_Mem);
        TexData[i+Mipmaps] = (GLubyte*)GALLOC((mipw >> 1) * (miph >> 1), GStat_Default_Mem);
        TexData[i+Mipmaps*2] = (GLubyte*)GALLOC((mipw >> 1) * (miph >> 1), GStat_Default_Mem);
        if (ntex == 4)
            TexData[i+Mipmaps*3] = (GLubyte*)GALLOC( mipw * miph, GStat_Default_Mem);

        mipw >>= 1;
        miph >>= 1;
        if (mipw < 1)
            mipw = 1;
        if (miph < 1)
            miph = 1;
    }

    glGenTextures(1, (GLuint*)&TextureId);
    glGenTextures(ntex-1, TextureUVA);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, TextureFmt, TexWidth, TexHeight, 0, TextureFmt, TextureData, 0);
    glBindTexture(GL_TEXTURE_2D, TextureUVA[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, TextureFmt, TexWidth>>1, TexHeight>>1, 0, TextureFmt, TextureData, 0);
    glBindTexture(GL_TEXTURE_2D, TextureUVA[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, TextureFmt, TexWidth>>1, TexHeight>>1, 0, TextureFmt, TextureData, 0);
    if (ntex == 4)
    {
        glBindTexture(GL_TEXTURE_2D, TextureUVA[2]);
        glTexImage2D(GL_TEXTURE_2D, 0, TextureFmt, TexWidth, TexHeight, 0, TextureFmt, TextureData, 0);
    }
    return 1;
}

int     GTextureOGL2ImplYUV::Map(int level, int n, MapRect* maps, int flags)
{
    GUNUSED(flags);
    GASSERT(n >= (TextureUVA[2] ? 4 : 3) && maps);

    int h = TexHeight;
    int w = TexWidth;

    for (int i = 0; i < level; i++)
    {
        h >>= 1;
        if (h < 1)
            h = 1;
        w >>= 1;
        if (w < 1)
            w = 1;
    }

    n = TextureUVA[2] ? 4 : 3;
    maps[0].width = w;
    maps[0].height = h;
    maps[0].pitch = w;
    maps[0].pData = TexData[level];
    maps[1].width = w>>1;
    maps[1].height = h>>1;
    maps[1].pitch = w>>1;
    maps[1].pData = TexData[level+Mipmaps];
    maps[2].width = w>>1;
    maps[2].height = h>>1;
    maps[2].pitch = w>>1;
    maps[2].pData = TexData[level+Mipmaps*2];
    if (TextureUVA[2])
    {
        maps[3].width = w;
        maps[3].height = h;
        maps[3].pitch = w;
        maps[3].pData = TexData[level+Mipmaps*3];
    }
    return n;
}

bool    GTextureOGL2ImplYUV::Unmap(int level, int n, MapRect* maps, int flags)
{
    GUNUSED2(n,flags);
    GASSERT(n == (TextureUVA[2] ? 4 : 3));

    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexImage2D(GL_TEXTURE_2D, level, TextureFmt, maps[0].width, maps[0].height, 0, TextureFmt, TextureData, maps[0].pData);
    glBindTexture(GL_TEXTURE_2D, TextureUVA[0]);
    glTexImage2D(GL_TEXTURE_2D, level, TextureFmt, maps[1].width, maps[1].height, 0, TextureFmt, TextureData, maps[1].pData);
    glBindTexture(GL_TEXTURE_2D, TextureUVA[1]);
    glTexImage2D(GL_TEXTURE_2D, level, TextureFmt, maps[2].width, maps[2].height, 0, TextureFmt, TextureData, maps[2].pData);
    if (TextureUVA[2])
    {
        glBindTexture(GL_TEXTURE_2D, TextureUVA[2]);
        glTexImage2D(GL_TEXTURE_2D, level, TextureFmt, maps[3].width, maps[3].height, 0, TextureFmt, TextureData, maps[3].pData);
    }
    return 1;
}

void GTextureOGL2ImplYUV::Bind(int stageIndex, GRenderer::BitmapWrapMode WrapMode, GRenderer::BitmapSampleMode SampleMode, bool useMipmaps)
{
    GUNUSED(stageIndex);
    GASSERT(stageIndex == 0);

    for (int i = 0; i < (TextureUVA[2] ? 3 : 2); i++)
    {
        pRenderer->glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_2D, TextureUVA[i]);
        pRenderer->glUniform1i(pRenderer->Shaders[pRenderer->CurrentShader].tex[i+1], i+1);
        pRenderer->ApplySampleMode(WrapMode, SampleMode, useMipmaps && Mipmaps > 1);
    }

    pRenderer->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    pRenderer->glUniform1i(pRenderer->Shaders[pRenderer->CurrentShader].tex[0], 0);
    pRenderer->ApplySampleMode(WrapMode, SampleMode, useMipmaps && Mipmaps > 1);
}


// GTextureOGL2Impl implementation

// Creates an OpenGL texture of the specified dst dimensions, from a
// resampled version of the given src image.  Does a bilinear
// resampling to create the dst image.
static void    SoftwareResample(
                         int bytesPerPixel,
			             bool useBgra,
                         int srcWidth,
                         int srcHeight,
                         int srcPitch,
                         UByte* psrcData,
                         int dstWidth,
                         int dstHeight)
{
    UByte* rescaled = 0;
    GUNUSED(useBgra);

    unsigned   internalFormat = 0;
    unsigned   inputFormat    = 0;
    GLint      dataType       = GL_UNSIGNED_BYTE;

    switch(bytesPerPixel)
    {
    case 3:
#ifdef GFC_OS_PS3
        dataType       = GL_UNSIGNED_INT_8_8_8_8;
        inputFormat    = GL_RGBA;
        internalFormat = GL_RGBA;

        rescaled = (UByte*)GALLOC(dstWidth * dstHeight * 4);

        GRenderer::ResizeImage(&rescaled[0], dstWidth, dstHeight, dstWidth * 4,
            psrcData,    srcWidth, srcHeight, srcPitch,
            GRenderer::ResizeRgbToRgba);
#else
        inputFormat    = GL_RGB;
        internalFormat = GL_RGB;
        rescaled = (UByte*)GALLOC(dstWidth * dstHeight * bytesPerPixel, GStat_Default_Mem);

        GRenderer::ResizeImage(&rescaled[0], dstWidth, dstHeight, dstWidth * 3,
            psrcData,    srcWidth, srcHeight, srcPitch,
            GRenderer::ResizeRgbToRgb);
#endif
        break;

    case 4:
        rescaled = (UByte*)GALLOC(dstWidth * dstHeight * bytesPerPixel, GStat_Default_Mem);

#if defined(GL_ES_VERSION_2_0)
	if (useBgra)
	{
	    inputFormat    = GL_BGRA;
	    internalFormat = GL_BGRA;
	}
	else
#endif
	{
	    inputFormat    = GL_RGBA;
	    internalFormat = GL_RGBA;
	}
#ifdef GFC_OS_PS3
        dataType       = GL_UNSIGNED_INT_8_8_8_8;
#endif
        GRenderer::ResizeImage(&rescaled[0], dstWidth, dstHeight, dstWidth * 4,
            psrcData,    srcWidth, srcHeight, srcPitch,
            GRenderer::ResizeRgbaToRgba);
        break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, dstWidth, dstHeight, 
        0, inputFormat, dataType, &rescaled[0]);

    GFREE(rescaled);
}

GDepthBufferOGL2Impl::~GDepthBufferOGL2Impl()
{
    if (RBuffer)
    {
        if (pRenderer)
            pRenderer->AddResourceForReleasing(RBuffer, GRendererOGL2Impl::ReleaseResource::Buffer);
    }
}

void    GDepthBufferOGL2Impl::ReleaseTexture()
{
    if (RBuffer)
        pRendererEXT glDeleteRenderbuffers(1, &RBuffer);
    RBuffer = 0;
}

bool GDepthBufferOGL2Impl::InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage)
{
    if (!pRenderer)
        return 0;

    GUNUSED(usage);
    GUNUSED(mipmaps);
    GASSERT(usage & Usage_RenderTarget);
    ReleaseTexture();
    TexWidth = width;
    TexHeight = height;

    pRendererEXT glGenRenderbuffers(1, &RBuffer);
    pRendererEXT glBindRenderbuffer(GL_RENDERBUFFER_EXT, RBuffer);

    if (format == GImage::Image_Depth)
        pRendererEXT glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, TexWidth, TexHeight);
    else if (format == GImage::Image_Stencil)
        pRendererEXT glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX, TexWidth, TexHeight);
    else if (format == GImage::Image_DepthStencil)
#if defined(GL_ES_VERSION_2_0)
        pRendererEXT glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_OES, TexWidth, TexHeight);
#else
        pRendererEXT glRenderbufferStorage(GL_RENDERBUFFER_EXT, 0x84f9, TexWidth, TexHeight);
#endif
    else
        GASSERT(0);

    pRendererEXT glBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);
    return 1;
}

void GRendererOGL2Impl::SetPixelShader(PixelShaderType ps)
{
    CurrentShader = ps;
#ifndef GFC_NO_3D
    if (pWorldMatrix)
      CurrentShader += PS_Count;
#endif
    glUseProgram(Shaders[CurrentShader].prog);
}

void GRendererOGL2Impl::ApplyPShaderCxform(const Cxform &cxform)
{
    const float mult = 1.0f / 255.0f;

    float   cxformData[4 * 2] =
    { 
        cxform.M_[0][0], cxform.M_[1][0],
        cxform.M_[2][0], cxform.M_[3][0],
        // Cxform color is already pre-multiplied
        cxform.M_[0][1] * mult, cxform.M_[1][1] * mult,
        cxform.M_[2][1] * mult, cxform.M_[3][1] * mult
    };

    glUniform4fv(Shaders[CurrentShader].cxmul, 1, cxformData);
    glUniform4fv(Shaders[CurrentShader].cxadd, 1, cxformData+4);
}

GRenderTargetOGL2Impl::GRenderTargetOGL2Impl(GRendererOGL2Impl* prenderer) :
    GRenderTargetOGL(&prenderer->RenderTargets), pRenderer(prenderer)
{
    FboId = 0; 
    IsTemp = 0;
}

GRenderTargetOGL2Impl::~GRenderTargetOGL2Impl()
{
    if (!pRenderer)
        return;

    ReleaseResources();
    GLock::Locker guard(&pRenderer->TexturesLock);
    if (pFirst)
        RemoveNode();
}

GRenderer*  GRenderTargetOGL2Impl::GetRenderer() const
{
    return pRenderer;
}
void GRenderTargetOGL2Impl::RemoveFromRenderer()
{
    if (AddRef_NotZero())
    {
        ReleaseResources();
    	pRenderer = 0;
        CallHandlers(ChangeHandler::Event_RendererReleased);
        if (pNext) // We may have been released by user
            RemoveNode();
        Release();
    } else {
        if (pNext) // We may have been released by user
            RemoveNode();
	    pRenderer = 0;
    }
}

void GRenderTargetOGL2Impl::ReleaseResources()
{
    if (pRenderer && FboId)
    {
        pRenderer->AddResourceForReleasing(FboId, GRendererOGL2Impl::ReleaseResource::Fbo);
        FboId = 0;
    }
}

bool GRenderTargetOGL2Impl::InitRenderTarget(UInt fbo)
{
    ReleaseResources();
    FboId = fbo;

    CallHandlers(ChangeHandler::Event_DataChange);
    return true;
}

bool GRenderTargetOGL2Impl::InitRenderTarget(GTexture *ptarget, GTexture* pdepth, GTexture* pstencil)
{
    if (FboId && !pTexture)
        ReleaseResources();
    if (!FboId)
        pRendererEXT glGenFramebuffers(1, &FboId);

    pTexture = (GTextureOGL2Impl*)ptarget;
    TargetWidth = pTexture->TexWidth;
    TargetHeight = pTexture->TexHeight;

    pRendererEXT glBindFramebuffer(GL_FRAMEBUFFER_EXT, FboId);

    glBindTexture(GL_TEXTURE_2D, pTexture->TextureId);
    if (pTexture->Mipmaps > 1)
        pRendererEXT glGenerateMipmap(GL_TEXTURE_2D);
    pRendererEXT glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, pTexture->TextureId, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (pdepth)
    {
        GDepthBufferOGL2Impl* pbuf = (GDepthBufferOGL2Impl*)pdepth;
        pRendererEXT glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, pbuf->RBuffer);
        pDepth = pbuf;
    }
    if (pstencil)
    {
        GDepthBufferOGL2Impl* pbuf = (GDepthBufferOGL2Impl*)pstencil;
        pRendererEXT glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, pbuf->RBuffer);
        pStencil = pbuf;
    }
    GLenum status = 0;

    status = pRendererEXT glCheckFramebufferStatus (GL_FRAMEBUFFER_EXT);

    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        // Try with no depth buffer
        pRendererEXT glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
        pDepth = 0;
        status = pRendererEXT glCheckFramebufferStatus (GL_FRAMEBUFFER_EXT);
    }
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        // Try with no stencil buffer
        pRendererEXT glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
        pStencil = 0;
        status = pRendererEXT glCheckFramebufferStatus (GL_FRAMEBUFFER_EXT);
    }
    GASSERT(status == GL_FRAMEBUFFER_COMPLETE_EXT);

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    status = pRendererEXT glCheckFramebufferStatus (GL_FRAMEBUFFER_EXT);

    pRendererEXT glBindFramebuffer(GL_FRAMEBUFFER_EXT, pRenderer->pCurRenderTarget->FboId);

    CallHandlers(ChangeHandler::Event_DataChange);

    GFC_DEBUG_ERROR1(status != GL_FRAMEBUFFER_COMPLETE_EXT, "GRendererOGL2: render target fbo incomplete: %d", status);
    return (status == GL_FRAMEBUFFER_COMPLETE_EXT);
}


#ifdef GFC_GL_SHADER_IMPL
#include GFC_GL_SHADER_IMPL
#else

#include "ShadersGL/glsl.cpp"
#if defined(GL_ES_VERSION_2_0)
#include "GL/GLESShaders.cpp"
#else
#include "GL/GLShaders.cpp"
#endif

using namespace GL;

static const char* PShaderSources[PS_Count] =
{   
    0,
    p1PS_SolidColor,
    p1PS_TextTextureAlpha,
    p1PS_TextTextureColor,
    p1PS_TextTextureColorMultiply,
    p1PS_TextTextureYUV,
    p1PS_TextTextureYUVMultiply,
    p1PS_TextTextureYUVA,
    p1PS_TextTextureYUVAMultiply,
    p1PS_CxformTexture,
    p1PS_CxformTextureMultiply,

    p1PS_CxformGauraud, 
    p1PS_CxformGauraudNoAddAlpha,
    p1PS_CxformGauraudTexture, 
    p1PS_Cxform2Texture, 
    p1PS_CxformGauraudMultiply,       
    p1PS_CxformGauraudMultiplyNoAddAlpha, 
    p1PS_CxformGauraudMultiplyTexture,
    p1PS_CxformMultiply2Texture,

    p1PS_CmatrixTexture,
    p1PS_CmatrixTextureMultiply,
};

GLuint GRendererOGL2Impl::InitShader(GLenum type, const char *psrc)
{
    GLuint vs = glCreateShader(type);
    glShaderSource(vs, 1, &psrc, 0);
    glCompileShader(vs);
    GLint result;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        GLchar msg[512];
        glGetShaderInfoLog(vs, sizeof(msg), 0, msg);
        GFC_DEBUG_ERROR2(1, "%s: %s\n", psrc, msg);
        return 0;
    }
    return vs;

}

GLuint GRendererOGL2Impl::InitShader(const char *pVText, const char *pFText)
{
    GLint result;
    GLuint vp = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vp, 1, &pVText, 0);
    glCompileShader(vp);
    glGetShaderiv(vp, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        GLchar msg[512];
        glGetShaderInfoLog(vp, sizeof(msg), 0, msg);
        GFC_DEBUG_ERROR2(1, "%s: %s\n", pVText, msg);
        glDeleteShader(vp);
        return 0;
    }
    GLuint out = glCreateProgram();
    glAttachShader(out, vp);
    GLuint fp = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fp, 1, &pFText, 0);
    glCompileShader(fp);
    glGetShaderiv(fp, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        GLchar msg[1512];
        glGetShaderInfoLog(fp, sizeof(msg), 0, msg);
        GFC_DEBUG_ERROR2(1, "%s: %s\n", pFText, msg);
        glDeleteShader(fp);
        glDeleteProgram(out);
        return 0;
    }
    glAttachShader(out, fp);
    glBindAttribLocation(out, 0, "pos");
    glBindAttribLocation(out, 8, "intc");
    glLinkProgram(out);
    glDeleteShader(vp);
    glDeleteShader(fp);
    glGetProgramiv(out, GL_LINK_STATUS, &result);
    if (!result)
    {
        GLchar msg[512];
        glGetProgramInfoLog(out, sizeof(msg), 0, msg);
        GFC_DEBUG_ERROR1(1, "link: %s\n", msg);
        result = 0;
        glDeleteProgram(out);
    }
    return (result ? out : 0);
}

bool GRendererOGL2Impl::InitShaders()
{
    GLenum vshaders[VS_Count] = { 0,
        InitShader(GL_VERTEX_SHADER, p1VS_Pos),
        InitShader(GL_VERTEX_SHADER, p1VS_PosTC),
        InitShader(GL_VERTEX_SHADER, p1VS_Glyph),
        InitShader(GL_VERTEX_SHADER, p1VS_PosCTC),
        InitShader(GL_VERTEX_SHADER, p1VS_PosCFTC),
        InitShader(GL_VERTEX_SHADER, p1VS_PosCFTC2)

#ifndef GFC_NO_3D
	,0,
        InitShader(GL_VERTEX_SHADER, p1VS_3D_Pos),
        InitShader(GL_VERTEX_SHADER, p1VS_3D_PosTC),
        InitShader(GL_VERTEX_SHADER, p1VS_3D_Glyph),
        InitShader(GL_VERTEX_SHADER, p1VS_3D_PosCTC),
        InitShader(GL_VERTEX_SHADER, p1VS_3D_PosCFTC),
        InitShader(GL_VERTEX_SHADER, p1VS_3D_PosCFTC2)
#endif
    };

    bool result = 1;
    int j = 0;
#ifndef GFC_NO_3D
    for (j = 0; j < 2; j++)
#endif
    for (int i = 1; i < PS_Count; i++)
    {
        int psi = i+j*PS_Count;
        Shaders[psi].prog = glCreateProgram();
        GLuint fs = InitShader(GL_FRAGMENT_SHADER, PShaderSources[i]);
        if (!fs)
        {
            result = 0;
            break;
        }
        glAttachShader(Shaders[psi].prog, fs);
        glAttachShader(Shaders[psi].prog, vshaders[VShaderForPShader[i] + j * VS_CountSrc]);
        glBindAttribLocation(Shaders[psi].prog, 0, "pos");
    	glBindAttribLocation(Shaders[psi].prog, 8, "tc0");
        glLinkProgram(Shaders[psi].prog);

        GLint result;
        glGetProgramiv(Shaders[psi].prog, GL_LINK_STATUS, &result);
        if (!result)
        {
            GLchar msg[512];
            glGetProgramInfoLog(Shaders[psi].prog, sizeof(msg), 0, msg);
            GFC_DEBUG_ERROR2(1, "%s\nlink: %s\n", PShaderSources[i], msg);
            result = 0;
            glDeleteShader(fs);
            break;
        }

        Shaders[psi].mvp = glGetUniformLocation(Shaders[psi].prog, "mvp");
        Shaders[psi].texgen = glGetUniformLocation(Shaders[psi].prog, "texgen");
        Shaders[psi].cxmul = glGetUniformLocation(Shaders[psi].prog, "cxmul");
        Shaders[psi].cxadd = glGetUniformLocation(Shaders[psi].prog, "cxadd");
        Shaders[psi].tex[0] = glGetUniformLocation(Shaders[psi].prog, "tex0");
        if (Shaders[psi].tex[0] < 0)
        {
            Shaders[psi].tex[0] = glGetUniformLocation(Shaders[psi].prog, "tex_y");
            if (Shaders[psi].tex[0] >= 0)
            {
                Shaders[psi].tex[1] = glGetUniformLocation(Shaders[psi].prog, "tex_u");
                Shaders[psi].tex[2] = glGetUniformLocation(Shaders[psi].prog, "tex_v");
                Shaders[psi].tex[3] = glGetUniformLocation(Shaders[psi].prog, "tex_a");
            }
        }
        else
            Shaders[psi].tex[1] = glGetUniformLocation(Shaders[psi].prog, "tex1");

        Shaders[psi].pos = glGetAttribLocation(Shaders[psi].prog, "pos");
        Shaders[psi].tc[0] = glGetAttribLocation(Shaders[psi].prog, "tc0");
        Shaders[psi].color = glGetAttribLocation(Shaders[psi].prog, "color");
        Shaders[psi].factor = glGetAttribLocation(Shaders[psi].prog, "factor");

        glDeleteShader(fs);
    }

    for (int i = 1; i < VS_Count; i++)
        glDeleteShader(vshaders[i]);

    if (!result)
        return result;

    // Filters
    StaticFilterShaders.Resize(FS2_Count);
    memset(&StaticFilterShaders[0], 0, sizeof(Shader) * FS2_Count);
    GLenum vstc = InitShader(GL_VERTEX_SHADER, pSource_VVatc);

    for (int i = 0; i < FS2_FCMatrix; i++)
    {
        if (!FShaderSources2[i])
	    continue;

        StaticFilterShaders[i].prog = glCreateProgram();
        GLuint fs = InitShader(GL_FRAGMENT_SHADER, FShaderSources2[i]);
        if (!fs)
        {
            result = 0;
            break;
        }
        glAttachShader(StaticFilterShaders[i].prog, fs);
        glAttachShader(StaticFilterShaders[i].prog, vstc);
        glBindAttribLocation(StaticFilterShaders[i].prog, 0, "pos");
	    glBindAttribLocation(StaticFilterShaders[i].prog, 8, "atc");
        glLinkProgram(StaticFilterShaders[i].prog);

        GLint result;
        glGetProgramiv(StaticFilterShaders[i].prog, GL_LINK_STATUS, &result);
        if (!result)
        {
            GLchar msg[512];
            glGetProgramInfoLog(StaticFilterShaders[i].prog, sizeof(msg), 0, msg);
            GFC_DEBUG_ERROR2(1, "%s\nlink: %s\n", FShaderSources2[i], msg);
            glDeleteShader(fs);
            break;
        }

#if !defined(GL_ES_VERSION_2_0)
        glGetProgramiv(StaticFilterShaders[i].prog, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &result);
        if (!result)
    	{
            glDeleteShader(fs);
            break;
	}
#endif

    	Shader* pfs = &StaticFilterShaders[i];
        pfs->mvp = glGetUniformLocation(pfs->prog, "mvp");
        pfs->cxmul = glGetUniformLocation(pfs->prog, "cxmul");
        pfs->cxadd = glGetUniformLocation(pfs->prog, "cxadd");
        pfs->tex[0] = glGetUniformLocation(pfs->prog, "tex");
        pfs->tex[1] = glGetUniformLocation(pfs->prog, "srctex");
	    pfs->tc[0] = glGetAttribLocation(pfs->prog, "atc");
	    pfs->tc[1] = glGetUniformLocation(pfs->prog, "texscale");
	    pfs->tc[2] = glGetUniformLocation(pfs->prog, "fsize");
        pfs->texgen = glGetUniformLocation(pfs->prog, "offset");
        pfs->tex[3] = glGetUniformLocation(pfs->prog, "srctexscale");
        pfs->color = glGetUniformLocation(pfs->prog, "scolor");
        pfs->factor = glGetUniformLocation(pfs->prog, "scolor2");
        pfs->pos = glGetAttribLocation(pfs->prog, "pos");
    }
    glDeleteShader(vstc);

    if (!result)
    {
        // successful init, but use dynamic filter shaders
        for (int j = 0; j < FS2_Count; j++)
            glDeleteShader(StaticFilterShaders[j].prog);

        StaticFilterShaders.Clear();
	    if (MaxTexCoords > 16)
	        MaxTexCoords = 16;
    }

    return 1;
}

void GRendererOGL2Impl::ReleaseShaders()
{
    glUseProgram(0);
    int j = 0;
#ifndef GFC_NO_3D
    for (j = 0; j < 2; j++)
#endif
    for (int i = 1; i < PS_Count; i++)
        glDeleteProgram(Shaders[i+j*PS_Count].prog);

    if (StaticFilterShaders.GetSize() > 0)
    {
        for (int j = 0; j < FS2_Count; j++)
            glDeleteShader(StaticFilterShaders[j].prog);

        StaticFilterShaders.Clear();
    }

    for (FilterShadersType::ConstIterator i = FilterShaders.Begin(); i != FilterShaders.End(); ++i)
    {
        Shader* pfs = i->Second;
        glDeleteProgram(pfs->prog);
        delete pfs;
    }
}

const GRendererOGL2Impl::Shader* GRendererOGL2Impl::GetBlurShader(const BlurFilterParams& params, bool LowEnd)
{
    GRendererOGL2Impl::Shader** pfs = FilterShaders.Get(params);
    if (pfs)
        return *pfs;

    GStringBuffer vsrc, fsrc;
    if (params.Mode & (Filter_Shadow|Filter_Blur))
    {
        Float SizeX = UInt(params.BlurX-1) * 0.5f;
        Float SizeY = UInt(params.BlurY-1) * 0.5f;
        UInt  np = UInt((SizeX*2+1)*(SizeY*2+1));

	    UInt  MaxTCs = G_Min(MaxAttributes-1, 16);
        UInt  BaseTCs = 0;
        if (params.Mode & Filter_Shadow)
            BaseTCs++;
        UInt BoxTCs = MaxTCs - BaseTCs;
        if ((params.Mode & (Filter_Highlight|Filter_Shadow)) == (Filter_Highlight|Filter_Shadow))
        {
            BoxTCs >>= 1;
            MaxTCs = BoxTCs*2 + BaseTCs;
        }
        if (BoxTCs > np)
        {
            BoxTCs = np;
            MaxTCs = BoxTCs * (((params.Mode & (Filter_Highlight|Filter_Shadow)) == (Filter_Highlight|Filter_Shadow)) ? 2 : 1) + BaseTCs;
        }

        static const char *pVText =
            "uniform   vec4 mvp[2];\n"
            "attribute vec2 intc;\n"
            "attribute vec4 pos;\n"
            "varying   vec2 tc;\n"
            "void main(void)\n"
            "{ tc = intc;\n"
            "  vec4 opos = pos;\n"
            "  opos.x = dot(pos, mvp[0]);\n"
            "  opos.y = dot(pos, mvp[1]);\n"
            "  gl_Position = opos; }\n";

	    fsrc.AppendString(
#if defined(GL_ES_VERSION_2_0)
                          "precision mediump float;\n"
#endif
			              "uniform sampler2D tex;\n"
			              "uniform vec2 texscale;\n"
			              "uniform vec4 cxmul;\n"
			              "uniform vec4 cxadd;\n");

	    if (LowEnd)
	    {
	        vsrc.AppendString("uniform vec4 mvp[2];\n");
	        for (UInt i = 0; i < MaxTCs; i++)
	        {
	            G_SPrintF(vsrc, "attribute vec2 intc%d;\n", i);
		        G_SPrintF(vsrc, "varying   vec2 tc%d;\n", i);
		        G_SPrintF(fsrc, "varying   vec2 tc%d;\n", i);
	        }
	        vsrc.AppendString("attribute vec4 pos;\n"
			                  "void main(void)\n{\n"
			                  "  vec4 opos = pos;\n"
			                  "  opos.x = dot(pos, mvp[0]);\n"
			                  "  opos.y = dot(pos, mvp[1]);\n"
			                  "  gl_Position = opos;\n");
	        for (UInt i = 0; i < MaxTCs; i++)
    		    G_SPrintF(vsrc, "  tc%d = intc%d;\n", i, i);

	        vsrc.AppendString("\n}");
	    }
	    else
    	    fsrc.AppendString("varying vec2 tc;\n");

	    if (params.Mode & Filter_Shadow)
        {
            fsrc.AppendString("uniform vec2 offset;\n"
			                  "uniform vec4 scolor;\n"
			                  "uniform vec2 srctexscale;\n"
			                  "uniform sampler2D srctex;\n");

	        if (params.Mode & Filter_Highlight)
	        {
    		    fsrc.AppendString("uniform vec4 scolor2;\n");
	        }
        }

	    if (LowEnd)
	    {
	        fsrc.AppendString("void main(void)\n {"
			                  "  vec4 color = vec4(0);\n");
	    }
	    else
	    {
	        G_SPrintF(fsrc,   "const vec2 fsize = vec2(%f,%f);\n", SizeX, SizeY);
	        fsrc.AppendString("void main(void)\n {"
                              "  vec4 color = vec4(0);\n"
			                  "  vec2 i = vec2(0);\n"
			                  "  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
			                  "  for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n");
	    }

        if (params.Mode & Filter_Shadow)
        {
	        const char *color;
	        if (params.Mode & Filter_Highlight)
	        {
		        if (LowEnd)
		        {
                    for (UInt i = 0; i < BoxTCs; i++)
                    {
                        G_SPrintF(fsrc, "  color.a += texture2D(tex, tc%d).a;\n", i*2);
                        G_SPrintF(fsrc, "  color.r += texture2D(tex, tc%d).a;\n", i*2+1);
                    }
                }
		        else
		            fsrc.AppendString("  {\n"
				              "    color.a += texture2D(tex, tc + (offset + i) * texscale).a;\n"
				              "    color.r += texture2D(tex, tc - (offset + i) * texscale).a;\n  }\n");
    		    color = "(scolor * color.a + scolor2 * color.r)";
	        }
	        else
	        {
		        if (LowEnd)
		        {
		            for (UInt i = 0; i < BoxTCs; i++)
			        G_SPrintF(fsrc, "  color += texture2D(tex, tc%d);\n", i);
		        }
		        else
		            fsrc.AppendString("    color += texture2D(tex, tc + (offset + i) * texscale);\n");
                color = "(scolor * color.a)";
	        }

	        G_SPrintF(fsrc,   "  color *= %f;\n", 1.0f/((SizeX*2+1)*(SizeY*2+1)));

	        if (params.Mode & Filter_HideObject)
    		    G_SPrintF(fsrc, "  gl_FragColor = %s;}\n", color);
	        else
	        {
		        if (LowEnd)
		            G_SPrintF(fsrc,   "  vec4 base = texture2D(srctex, tc%d);\n", MaxTCs-1);
		        else
		            fsrc.AppendString("  vec4 base = texture2D(srctex, tc * srctexscale);\n");

		        if (params.Mode & Filter_Inner)
		        {
		            if (params.Mode & Filter_Highlight)
		            {
			            fsrc.AppendString("  color.ar = clamp((1.0 - color.ar) - (1.0 - color.ra) * 0.5f, 0.0,1.0);\n");
			            fsrc.AppendString("  color = (scolor * (color.a) + scolor2 * (color.r)\n"
					                      "           + base * (1.0 - color.a - color.r)) * base.a;\n");
		            }
                    else if (params.Mode & Filter_Knockout)
                    {
                        fsrc.AppendString("  color = scolor * (1-color.a) * base.a;\n");
                    }
		            else
    			        fsrc.AppendString("  color = mix(scolor, base, color.a) * base.a;\n");

		            fsrc.AppendString("  gl_FragColor = color * cxmul + cxadd * color.a;\n}");
		        }
		        else
		        {
		            G_SPrintF  (fsrc, "  color = %s * (1.0-base.a) + base;\n", color);

		            if (params.Mode & Filter_Knockout)
		            {
			            fsrc.AppendString("  color *= (1.0 - base.a);\n"
					                      "  gl_FragColor = color * cxmul + cxadd * color.a;\n}");
		            }
		            else
	    		        fsrc.AppendString("  gl_FragColor = color * cxmul + cxadd * color.a;\n}");
    	        }
	        }
        }
        else
        {
	        if (LowEnd)
	        {
	            for (UInt i = 0; i < BoxTCs; i++)
	                G_SPrintF(fsrc, "  color += texture2D(tex, tc%d);\n", i);
	        }
	        else
		        fsrc.AppendString("    color += texture2D(tex, tc + i * texscale);\n");
                    G_SPrintF(fsrc,       "  color = color * %f;\n", 1.0f/((SizeX*2+1)*(SizeY*2+1)));
            fsrc.AppendString(    "  gl_FragColor = color * cxmul + cxadd * color.a;\n}\n");
        }

        GLuint prog = InitShader(LowEnd ? vsrc.ToCStr() : pVText, fsrc.ToCStr());
        if (!prog)
        {
            UseLowEndFilters = true;
            return 0;
        }

        GRendererOGL2Impl::Shader* pfs = new Shader;
        pfs->prog = prog;
        pfs->mvp = glGetUniformLocation(pfs->prog, "mvp");
        pfs->cxmul = glGetUniformLocation(pfs->prog, "cxmul");
        pfs->cxadd = glGetUniformLocation(pfs->prog, "cxadd");
        pfs->tex[0] = glGetUniformLocation(pfs->prog, "tex");
        pfs->tex[1] = glGetUniformLocation(pfs->prog, "srctex");

	    if (LowEnd)
	    {
	        for (UInt i = 0; i < MaxTCs; i++)
	        {
		        char tc[16];
		        G_SPrintF(tc, "intc%d", i);
		        pfs->tc[i] = glGetAttribLocation(pfs->prog, tc);
	        }
	    }
	    else
	    {
	        pfs->tc[0] = glGetAttribLocation(pfs->prog, "intc");
	        pfs->tc[1] = glGetUniformLocation(pfs->prog, "texscale");
	    }
        pfs->texgen = glGetUniformLocation(pfs->prog, "offset");
        pfs->tex[3] = glGetUniformLocation(pfs->prog, "srctexscale");
        pfs->color = glGetUniformLocation(pfs->prog, "scolor");
        pfs->factor = glGetUniformLocation(pfs->prog, "scolor2");
        pfs->pos = glGetAttribLocation(pfs->prog, "pos");

	    pfs->Samples = np;
	    pfs->BoxTCs = BoxTCs;
	    pfs->BaseTCs = BaseTCs;
	    pfs->MaxTCs = LowEnd ? MaxTCs : 0;

        FilterShaders.Add(params, pfs);
        return pfs;
    }

    return 0;
}

UInt GRendererOGL2Impl::CheckFilterSupport(const BlurFilterParams& params)
{
    UInt flags = FilterSupport_Ok;
    if (params.Passes > 1 || (params.BlurX * params.BlurY > MaxTextureOps))
        flags |= FilterSupport_Multipass;

    return flags;
}

void GRendererOGL2Impl::DrawBlurRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& indestrect, const BlurFilterParams& params, bool islast)
{
	GUNUSED(islast);
    GPtr<GTextureOGL2Impl> psrcinRef = (GTextureOGL2Impl*)psrcin;
    GPtr<GTextureOGL2Impl> psrc = (GTextureOGL2Impl*)psrcin;
    GRectF srcrect, destrect(-1,-1,1,1);

    bool LowEnd = (StaticFilterShaders.GetSize() == 0 && (UseLowEndFilters || 2 + params.BlurX > MaxTextureOps || 2 + params.BlurY > MaxTextureOps));
retry_shaders:
    UInt n = params.Passes;
    bool   Prims = 0;

    BlurFilterParams pass[3];
    UInt             passis[3];
    const Shader *   passes[3];

    pass[0] = params;
    pass[1] = params;
    pass[2] = params;

    if (StaticFilterShaders.GetSize() > 0)
    {
        bool mul = (BlendMode == Blend_Multiply || BlendMode == Blend_Darken);
        passis[0] = passis[1] = FS2_FBox2Blur;
        passis[2] = mul ? FS2_FBox2BlurMul : FS2_FBox2Blur;

        if (params.Mode & Filter_Shadow)
        {
            if (params.Mode & Filter_HideObject)
	        {
                passis[2] = FS2_FBox2Shadowonly;

	            if (params.Mode & Filter_Highlight)
	                passis[2] = (FragShaderType2) (passis[2] + FS2_shadows_Highlight);
            }
            else
	        {
		        if (params.Mode & Filter_Inner)
		            passis[2] = FS2_FBox2InnerShadow;
		        else
		            passis[2] = FS2_FBox2Shadow;

		        if (params.Mode & Filter_Knockout)
                    passis[2] = (FragShaderType2) (passis[2] + FS2_shadows_Knockout);

		        if (params.Mode & Filter_Highlight)
		            passis[2] = (FragShaderType2) (passis[2] + FS2_shadows_Highlight);
	        }

            if (mul)
                passis[2] = (FragShaderType2) (passis[2] + FS2_shadows_Mul);
        }

        if (params.BlurX * params.BlurY > MaxTextureOps)
        {
            n *= 2;
            pass[0].BlurY = 1;
            pass[1].BlurX = 1;
            pass[2].BlurX = 1;

            passis[0] = passis[1] = FS2_FBox1Blur;
            if (passis[2] == FS2_FBox2Blur)
                passis[2] = FS2_FBox1Blur;
            else if (passis[2] == FS2_FBox2BlurMul)
                passis[2] = FS2_FBox1BlurMul;
        }

        for (UInt i = 0; i < 3; i++)
            passes[i] = &StaticFilterShaders[passis[i]];

        if (!passes[0] || !passes[1] || !passes[2])
            return;
    }
    else
    {
	    if (params.BlurX * params.BlurY > MaxTextureOps)
	    {
	        n *= 2;
	        pass[0].Mode &= ~Filter_Shadow;
	        pass[0].Mode |= Filter_Blur;
	        pass[1].Mode &= ~Filter_Shadow;
	        pass[1].Mode |= Filter_Blur;
	        pass[0].BlurY = 1;
	        pass[1].BlurX = 1;
	        pass[2].BlurX = 1;
	        passes[0] = GetBlurShader(pass[0], LowEnd);
	        passes[1] = GetBlurShader(pass[1], LowEnd);
	        passes[2] = GetBlurShader(pass[2], LowEnd);
	    }
	    else if (params.Mode & Filter_Shadow)
	    {
	        pass[0].Mode &= ~Filter_Shadow;
	        pass[0].Mode |= Filter_Blur;
	        pass[1].Mode &= ~Filter_Shadow;
	        pass[1].Mode |= Filter_Blur;

	        passes[0] = passes[1] = GetBlurShader(pass[0], LowEnd);
	        passes[2] = GetBlurShader(params, LowEnd);
	    }
	    else
	        passes[0] = passes[1] = passes[2] = GetBlurShader(params, LowEnd);

	    if (!passes[0] || !passes[1] || !passes[2])
        {
            if (!LowEnd)
            {
                LowEnd = 1;
                goto retry_shaders;
            }
            GFC_DEBUG_MESSAGE(1, "GRendererOGL2: Failed to build filter shaders, not drawing");
	        return;
        }

	    LowEnd = (passes[0]->MaxTCs > 0);

	    if (LowEnd && (passes[2]->BoxTCs < pass[2].BlurX*pass[2].BlurY || BlendMode > Blend_Normal))
	    {
	        n++;
	        Prims = 1;
	        pass[2].BlurX = pass[2].BlurY = 1;
	        passes[2] = GetBlurShader(pass[2], LowEnd);
	    }
	    if (LowEnd)
	        pass[0].Offset.x = pass[0].Offset.y = pass[1].Offset.x = pass[1].Offset.y = 0;
    }

    UInt   bufWidth = (UInt)ceilf(insrcrect.Width());
    UInt   bufHeight = (UInt)ceilf(insrcrect.Height());
    UInt   last = n-1;

    // need base image for LastPassOnly

    for (UInt i = (params.Mode & Filter_LastPassOnly) ? last : 0; i < n; i++)
    {
        UInt passi = (i == last) ? 2 : (i&1);
        const BlurFilterParams& pparams = pass[passi];
        const Shader *pShader = passes[passi];

        glUseProgram(pShader->prog);

        GTextureOGL2Impl* pnextsrc;
        if (i != n - 1)
        {
            pnextsrc = PushTempRenderTarget(GRectF(-1,-1,1,1), bufWidth, bufHeight);
            ApplyMatrixForQuad(GMatrix2D::Identity, pShader->mvp);
            destrect = GRectF(-1,-1,1,1);
        }
        else
        {
            pnextsrc = 0;
            ApplyMatrixForQuad(CurrentMatrix, pShader->mvp);
            destrect = indestrect;
        }

        srcrect = GRectF(insrcrect.Left * 1.0f/psrc->TexWidth, insrcrect.Top * 1.0f/psrc->TexHeight,
            insrcrect.Right * 1.0f/psrc->TexWidth, insrcrect.Bottom * 1.0f/psrc->TexHeight);

	    if (Prims && i < last)
	    {
    	    glBlendFunc(GL_ONE, GL_ONE);
	        glBlendEquation(GL_FUNC_ADD);
	    }
	    else if (i < last)
	    {
    	    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	        glBlendEquation(GL_FUNC_ADD);
	    }
	    else
	        ApplyBlendMode(BlendMode, true, true);

        glEnable(GL_BLEND);

	    const float mult = 1.0f / 255.0f;

        if (pShader->cxadd >= 0)
	{
            if (i == n - 1)
            {
                float   cxformData[4 * 2] =
                { 
                    params.cxform.M_[0][0] * params.cxform.M_[3][0],
                    params.cxform.M_[1][0] * params.cxform.M_[3][0],
                    params.cxform.M_[2][0] * params.cxform.M_[3][0],
                    params.cxform.M_[3][0],

                    params.cxform.M_[0][1] * mult * params.cxform.M_[3][0],
                    params.cxform.M_[1][1] * mult * params.cxform.M_[3][0],
                    params.cxform.M_[2][1] * mult * params.cxform.M_[3][0],
                    params.cxform.M_[3][1] * mult
                };

                glUniform4fv(pShader->cxmul, 1, cxformData);
                glUniform4fv(pShader->cxadd, 1, cxformData+4);
            }
            else
            {
                const float add[] = {0,0,0,0};
                const float mul[] = {1,1,1,1};
                glUniform4fv(pShader->cxmul, 1, mul);
                glUniform4fv(pShader->cxadd, 1, add);
            }
	}

	    if (StaticFilterShaders.GetSize() > 0)
	    {
	        Float SizeX = UInt(pparams.BlurX-1) * 0.5f;
	        Float SizeY = UInt(pparams.BlurY-1) * 0.5f;

            if (passis[passi] == FS2_FBox1Blur || passis[passi] == FS2_FBox1BlurMul)
	        {
		        const float fsize[] = {pparams.BlurX > 1 ? SizeX : SizeY, 0, 0, 1.0f/((SizeX*2+1)*(SizeY*2+1))};
		        glUniform4fv(pShader->tc[2], 1, fsize);

		        glUniform2f(pShader->tc[1], pparams.BlurX > 1 ? 1.0f/psrc->TexWidth : 0, pparams.BlurY > 1 ? 1.0f/psrc->TexHeight : 0);
	        }
	        else
	        {
		        const float fsize[] = {SizeX, SizeY, 0, 1.0f/((SizeX*2+1)*(SizeY*2+1))};
		        glUniform4fv(pShader->tc[2], 1, fsize);
		        glUniform2f(pShader->tc[1], 1.0f/psrc->TexWidth, 1.0f/psrc->TexHeight);
	        }
	    }
	    else if (!LowEnd)
	        glUniform2f(pShader->tc[1], 1.0f/psrc->TexWidth, 1.0f/psrc->TexHeight);

        if (pShader->texgen >= 0)
    	{
            glUniform2f(pShader->texgen, -params.Offset.x, -params.Offset.y);
	        glUniform2f(pShader->tex[3], psrc->TexWidth/Float(psrcinRef->TexWidth), psrc->TexHeight/Float(psrcinRef->TexHeight));
	    }

	    if (pShader->color >= 0)
	    {
            glUniform4f(pShader->color,
                params.Color.GetRed() * mult, params.Color.GetGreen() * mult, params.Color.GetBlue() * mult, params.Color.GetAlpha() * mult);

	        if (pShader->factor >= 0)
		        glUniform4f(pShader->factor,
			        params.Color2.GetRed() * mult, params.Color2.GetGreen() * mult, params.Color2.GetBlue() * mult, params.Color2.GetAlpha() * mult);
	    }

        if (pShader->tex[1] >= 0)
	    {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ((GTextureOGL2Impl*)psrcin)->TextureId);
            glUniform1i(pShader->tex[1], 1);
    	}

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, psrc->TextureId);
        glUniform1i(pShader->tex[0], 0);

        if (LowEnd)
        {
            Float SizeX = UInt(pparams.BlurX-1) * 0.5f;
            Float SizeY = UInt(pparams.BlurY-1) * 0.5f;
            UInt  np = UInt((SizeX*2+1)*(SizeY*2+1));
	        UInt  count = (np + pShader->BoxTCs - 1) / pShader->BoxTCs;

#if !defined(GL_ES_VERSION_2_0)
	        const Float vertices[] = {destrect.Left, destrect.Top,     srcrect.Left, srcrect.Top,    
				          destrect.Right, destrect.Top,    srcrect.Right, srcrect.Top,   
					  destrect.Right, destrect.Bottom, srcrect.Right, srcrect.Bottom,
					  destrect.Left, destrect.Bottom,  srcrect.Left, srcrect.Bottom};

		glBegin(GL_QUADS);

            Float x = -SizeX, y = -SizeY;
            for (UInt p = 0; p < count; p++)
            {
                Float xv[16], yv[16];
                for (UInt tc = 0; tc < pShader->BoxTCs; tc++)
                    if (y <= SizeY)
                    {
                        xv[tc] = (x - pparams.Offset.x) * 1.0f/psrc->TexWidth;
			            yv[tc] = (y - pparams.Offset.y) * 1.0f/psrc->TexHeight;
                        x++;
                        if (x > SizeX)
                        {
                            x = -SizeX;
                            y++;
                        }
                    }
                    else
                        xv[tc] = yv[tc] = 1e10;

                for (UInt j = 0; j < 4; j++)
                {
                    if ((pparams.Mode & (Filter_Highlight|Filter_Shadow)) == (Filter_Highlight|Filter_Shadow))
                    {
                        for (UInt tc = 0; tc < pShader->BoxTCs; tc++)
                            if (xv[tc] < 1e9)
			    {
                                glVertexAttrib2f(pShader->tc[tc*2], vertices[j*4+2] + xv[tc], vertices[j*4+3] + yv[tc]);
                                glVertexAttrib2f(pShader->tc[tc*2+1], vertices[j*4+2] - xv[tc], vertices[j*4+3] - yv[tc]);
			    }
                            else
                            {
                                glVertexAttrib2f(pShader->tc[tc*2], 0, 0);
                                glVertexAttrib2f(pShader->tc[tc*2+1], 0, 0);
                            }
                    }
                    else
                        for (UInt tc = 0; tc < pShader->BoxTCs; tc++)
                            if (xv[tc] < 1e9)
			        glVertexAttrib2f(pShader->tc[tc], vertices[j*4+2] + xv[tc], vertices[j*4+3] + yv[tc]);
                            else
                                glVertexAttrib2f(pShader->tc[tc], 0, 0);


                    if (pparams.Mode & Filter_Shadow)
                        glVertexAttrib2f(pShader->tc[pShader->MaxTCs-1],
		                    vertices[j*4+2] * psrc->TexWidth/Float(psrcinRef->TexWidth),
		                    vertices[j*4+3] * psrc->TexHeight/Float(psrcinRef->TexHeight));

                    glVertex2f(vertices[j*4], vertices[j*4+1]);
                }
            }

	    glEnd();
#else

                const Float vertices[] =   {destrect.Left, destrect.Top,     srcrect.Left, srcrect.Top,    
                                            destrect.Right, destrect.Top,    srcrect.Right, srcrect.Top,   
                                            destrect.Left, destrect.Bottom,  srcrect.Left, srcrect.Bottom,
                                            destrect.Right, destrect.Bottom, srcrect.Right, srcrect.Bottom};

		UInt   vbstride = (pShader->MaxTCs + 1) * 8;
		Float* pbuffer = (Float*)alloca(count * 4 * vbstride);
                UInt   vi = 0;

                Float x = -SizeX, y = -SizeY;
                for (UInt p = 0; p < count; p++)
                {
                    Float xv[16], yv[16];
                    for (UInt tc = 0; tc < pShader->BoxTCs; tc++)
                        if (y <= SizeY)
                        {
                            xv[tc] = (x - pparams.Offset.x) * 1.0f/psrc->TexWidth;
                            yv[tc] = (y - pparams.Offset.y) * 1.0f/psrc->TexHeight;
                            x++;
                            if (x > SizeX)
                            {
                                x = -SizeX;
                                y++;
                            }
                        }
                        else
                            xv[tc] = yv[tc] = 1e10;

                    for (UInt j = 0; j < 4; j++)
                    {
                        pbuffer[vi++] = vertices[j*4];
                        pbuffer[vi++] = vertices[j*4+1];
                        for (UInt tc = 0; tc < pShader->BoxTCs; tc++)
                        {
                            pbuffer[vi++] = xv[tc] < 1e9 ? (vertices[j*4+2] + xv[tc]) : 0;
                            pbuffer[vi++] = yv[tc] < 1e9 ? (vertices[j*4+3] + yv[tc]) : 0;
                            if ((pparams.Mode & (Filter_Highlight|Filter_Shadow)) == (Filter_Highlight|Filter_Shadow))
                            {
                                pbuffer[vi++] = xv[tc] < 1e9 ? (vertices[j*4+2] - xv[tc]) : 0;
                                pbuffer[vi++] = yv[tc] < 1e9 ? (vertices[j*4+3] - yv[tc]) : 0;
                            }
                        }
                        if (pparams.Mode & Filter_Shadow)
                        {
                            pbuffer[vi++] = vertices[j*4+2] * psrc->TexWidth/Float(psrcinRef->TexWidth);
                            pbuffer[vi++] = vertices[j*4+3] * psrc->TexHeight/Float(psrcinRef->TexHeight);
                        }
                    }
                }
		GASSERT(vi == (pShader->MaxTCs * 2 + 2) * count * 4);

	     	glEnableVertexAttribArray(pShader->pos);
		glVertexAttribPointer(pShader->pos, 2, GL_FLOAT, 0, vbstride, pbuffer);
		for (UInt i = 0; i < pShader->MaxTCs; i++)
		{
		    glEnableVertexAttribArray(pShader->tc[i]);
		    glVertexAttribPointer(pShader->tc[i], 2, GL_FLOAT, 0, vbstride, pbuffer+(2+i*2));
		}
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(pShader->pos);
		for (UInt i = 0; i < pShader->MaxTCs; i++)
		    glDisableVertexAttribArray(pShader->tc[i]);
#endif
	    }
	    else
	    {
	        DrawQuad(pShader, srcrect, destrect);
	    }

        if (i != n - 1)
        {
            PopRenderTarget();
            psrc = pnextsrc;
        }
    }

    ApplyBlendMode(BlendMode);
    FilterCnt.AddCount(n);
    RenderStats.Filters += n;
}


// Factory.
GRendererOGL*   GRendererOGL::CreateRenderer()
{
    return new GRendererOGL2Impl;
}

#endif

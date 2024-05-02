/**********************************************************************

Filename    :   glsl.cpp
Content     :   GLSL shaders
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#if defined(GL_ES_VERSION_2_0)
#define PRECISION "precision mediump float;\n"
#else
#define PRECISION ""
#endif

static const char* p1VS_Pos =
"uniform   vec4 mvp[2];\n"
"attribute vec4 pos;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_PosTC =
"uniform   vec4 mvp[2];\n"
"uniform   vec4 texgen[2];\n"
"attribute vec4 pos;\n"
"varying   vec2 vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_Glyph =
"uniform   vec4 mvp[2];\n"
"attribute vec4 pos;\n"
"attribute vec2 tc0;\n"
"attribute vec4 color;\n"
"varying   vec2 vtc0;\n"
"varying   vec4 vcolor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  vcolor = color;\n"
"  vtc0 = tc0;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_PosCTC =
"uniform   vec4 mvp[2];\n"
"uniform   vec4 texgen[2];\n"
"attribute vec4 pos;\n"
"attribute vec4 color;\n"
"varying   vec2 vtc0;\n"
"varying   vec4 vcolor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  vcolor = color;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_PosCFTC =
"uniform   vec4 mvp[2];\n"
"uniform   vec4 texgen[2];\n"
"attribute vec4 pos;\n"
"attribute vec4 color;\n"
"attribute vec4 factor;\n"
"varying   vec2 vtc0;\n"
"varying   vec4 vcolor;\n"
"varying   vec4 vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  vcolor = color;\n"
"  vfactor = factor;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_PosCFTC2 =
"uniform   vec4 mvp[2];\n"
"uniform   vec4 texgen[4];\n"
"attribute vec4 pos;\n"
"attribute vec4 color;\n"
"attribute vec4 factor;\n"
"varying   vec2 vtc0;\n"
"varying   vec2 vtc1;\n"
"varying   vec4 vcolor;\n"
"varying   vec4 vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  vtc1.x = dot(pos, texgen[2]);\n"
"  vtc1.y = dot(pos, texgen[3]);\n"
"  vcolor = color;\n"
"  vfactor = factor;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_3D_Pos =
"uniform   mat4 mvp;\n"
"attribute vec4 pos;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos * mvp;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_3D_PosTC =
"uniform   mat4 mvp;\n"
"uniform   vec4 texgen[2];\n"
"attribute vec4 pos;\n"
"varying   vec2 vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos * mvp;\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_3D_Glyph =
"uniform   mat4 mvp;\n"
"attribute vec4 pos;\n"
"attribute vec2 tc0;\n"
"attribute vec4 color;\n"
"varying   vec2 vtc0;\n"
"varying   vec4 vcolor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos * mvp;\n"
"  vcolor = color;\n"
"  vtc0 = tc0;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_3D_PosCTC =
"uniform   mat4 mvp;\n"
"uniform   vec4 texgen[2];\n"
"attribute vec4 pos;\n"
"attribute vec4 color;\n"
"varying   vec2 vtc0;\n"
"varying   vec4 vcolor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos * mvp;\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  vcolor = color;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_3D_PosCFTC =
"uniform   mat4 mvp;\n"
"uniform   vec4 texgen[2];\n"
"attribute vec4 pos;\n"
"attribute vec4 color;\n"
"attribute vec4 factor;\n"
"varying   vec2 vtc0;\n"
"varying   vec4 vcolor;\n"
"varying   vec4 vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos * mvp;\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  vcolor = color;\n"
"  vfactor = factor;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_3D_PosCFTC2 =
"uniform   mat4 mvp;\n"
"uniform   vec4 texgen[4];\n"
"attribute vec4 pos;\n"
"attribute vec4 color;\n"
"attribute vec4 factor;\n"
"varying   vec2 vtc0;\n"
"varying   vec2 vtc1;\n"
"varying   vec4 vcolor;\n"
"varying   vec4 vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 opos = pos * mvp;\n"
"  vtc0.x = dot(pos, texgen[0]);\n"
"  vtc0.y = dot(pos, texgen[1]);\n"
"  vtc1.x = dot(pos, texgen[2]);\n"
"  vtc1.y = dot(pos, texgen[3]);\n"
"  vcolor = color;\n"
"  vfactor = factor;\n"
"  gl_Position = opos;\n"
"}\n"
;

static const char* p1VS_3d = 
"uniform   mat4 mvp;\n"
"attribute vec4 pos;\n"
"attribute vec2 tc0;\n"
"attribute vec4 color;\n"
"varying   vec2 vtc0;\n"
"varying   vec4 vcolor;\n"
"void main(void)\n"
"{\n"
"  gl_Position = pos * mvp;\n"
"  vtc0 = tc0;\n"
"  vcolor = color;\n"
"}\n"
;

static const char* p1PS_SolidColor = PRECISION
"uniform vec4 cxmul;\n"
"void main(void)\n"
"{ gl_FragColor = cxmul;\n}\n"
;

static const char* p1PS_CxformTexture = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  gl_FragColor = texture2D(tex0, vtc0.st) * cxmul + cxadd;\n"
"}\n"
;

static const char* p1PS_CxformTextureMultiply = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 c = texture2D(tex0, vtc0.st) * cxmul + cxadd;\n"
"  gl_FragColor = mix(vec4(1), c, vec4(c.a));\n"
"}\n"
;

static const char* p1PS_TextTextureAlpha = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"varying vec4      vcolor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vcolor.bgra * cxmul + cxadd;\n"
"  c.a = c.a * texture2D(tex0, vtc0).r;\n"
"  gl_FragColor = c;\n"
"}\n"
;

static const char* p1PS_TextTextureColor = p1PS_CxformTexture;
static const char* p1PS_TextTextureColorMultiply = p1PS_CxformTextureMultiply;

static const char* p1PS_TextTextureYUV = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex_y, tex_u, tex_v;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vec4((texture2D(tex_y, vtc0).r - 16./255.) * 1.164);\n"
"  vec4 U = vec4(texture2D(tex_u, vtc0).r - 128./255.);\n"
"  vec4 V = vec4(texture2D(tex_v, vtc0).r - 128./255.);\n"
"  c += V * vec4(1.596, -0.813, 0, 0);\n"
"  c += U * vec4(0, -0.392, 2.017, 0);\n"
"  c.a = 1.0;\n"
"  gl_FragColor = c * cxmul + cxadd;\n"
"}\n"
;

static const char* p1PS_TextTextureYUVA = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex_y, tex_u, tex_v, tex_a;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vec4((texture2D(tex_y, vtc0).r - 16./255.) * 1.164);\n"
"  vec4 U = vec4(texture2D(tex_u, vtc0).r - 128./255.);\n"
"  vec4 V = vec4(texture2D(tex_v, vtc0).r - 128./255.);\n"
"  c.a = texture2D(tex_a, vtc0).r;\n"
"  c += V * vec4(1.596, -0.813, 0, 0);\n"
"  c += U * vec4(0, -0.392, 2.017, 0);\n"
"  gl_FragColor = c * cxmul + cxadd;\n"
"}\n"
;

static const char* p1PS_TextTextureYUVMultiply = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex_y, tex_u, tex_v;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vec4((texture2D(tex_y, vtc0).r - 16./255.) * 1.164);\n"
"  vec4 U = vec4(texture2D(tex_u, vtc0).r - 128./255.);\n"
"  vec4 V = vec4(texture2D(tex_v, vtc0).r - 128./255.);\n"
"  c += V * vec4(1.596, -0.813, 0, 0);\n"
"  c += U * vec4(0, -0.392, 2.017, 0);\n"
"  c.a = 1.0;\n"
"  c = c * cxmul + cxadd;\n"
"  gl_FragColor = mix(vec4(1), c, vec4(c.a));\n"
"}\n"
;

static const char* p1PS_TextTextureYUVAMultiply = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex_y, tex_u, tex_v, tex_a;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vec4((texture2D(tex_y, vtc0).r - 16./255.) * 1.164);\n"
"  vec4 U = vec4(texture2D(tex_u, vtc0).r - 128./255.);\n"
"  vec4 V = vec4(texture2D(tex_v, vtc0).r - 128./255.);\n"
"  c.a = texture2D(tex_a, vtc0).r;\n"
"  c += V * vec4(1.596, -0.813, 0, 0);\n"
"  c += U * vec4(0, -0.392, 2.017, 0);\n"
"  c = c * cxmul + cxadd;\n"
"  gl_FragColor = mix(vec4(1), c, vec4(c.a));\n"
"}\n"
;

static const char* p1PS_CxformGauraud = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"varying vec4      vcolor;\n"
"varying vec4      vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vcolor * cxmul + cxadd;\n"
"  c.a = c.a * vfactor.a;\n"
"  gl_FragColor = c;\n"
"}\n"
;

// Same, for Multiply blend version.
static const char* p1PS_CxformGauraudMultiply = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"varying vec4      vcolor;\n"
"varying vec4      vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vcolor * cxmul + cxadd;\n"
"  c.a = c.a * vfactor.a;\n"
"  gl_FragColor = mix(vec4(1), c, vec4(c.a));\n"
"}\n"
;

// The difference from above is that we don't have separate EdgeAA alpha channel;
// it is instead pre-multiplied into the color alpha (VertexXY16iC32). So we
// don't do an EdgeAA multiply in the end.
static const char* p1PS_CxformGauraudNoAddAlpha = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"varying vec4      vcolor;\n"
"void main(void)\n"
"{\n"
"  gl_FragColor = vcolor * cxmul + cxadd;\n"
"}\n"
;

static const char* p1PS_CxformGauraudMultiplyNoAddAlpha = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"varying vec4      vcolor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = vcolor * cxmul + cxadd;\n"
"  gl_FragColor = mix(vec4(1), c, vec4(c.a));\n"
"}\n"
;

static const char* p1PS_CxformGauraudTexture = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"varying vec4      vcolor;\n"
"varying vec4      vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = mix(vcolor, texture2D(tex0, vtc0), vec4(vfactor.b));\n"
"  c = c * cxmul + cxadd;\n"
"  c.a = c.a * vfactor.a;\n"
"  gl_FragColor = c;\n"
"}\n"
;

static const char* p1PS_CxformGauraudMultiplyTexture = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"varying vec4      vcolor;\n"
"varying vec4      vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = mix(vcolor, texture2D(tex0, vtc0), vec4(vfactor.b));\n"
"  c = c * cxmul + cxadd;\n"
"  c.a = c.a * vfactor.a;\n"
"  gl_FragColor = mix(vec4(1), c, c.a);\n"
"}\n"
;

static const char* p1PS_Cxform2Texture = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"uniform sampler2D tex1;\n"
"varying vec2      vtc0;\n"
"varying vec2      vtc1;\n"
"varying vec4      vcolor;\n"
"varying vec4      vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = mix(texture2D(tex1, vtc1), texture2D(tex0, vtc0), vec4(vfactor.b));\n"
"  gl_FragColor = c * cxmul + cxadd;\n"
"}\n"
;

static const char* p1PS_CxformMultiply2Texture = PRECISION
"uniform vec4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"uniform sampler2D tex1;\n"
"varying vec2      vtc0;\n"
"varying vec2      vtc1;\n"
"varying vec4      vcolor;\n"
"varying vec4      vfactor;\n"
"void main(void)\n"
"{\n"
"  vec4 c = mix(texture2D(tex1, vtc1), texture2D(tex0, vtc0), vec4(vfactor.b));\n"
"  c = c * cxmul + cxadd;\n"
"  gl_FragColor = mix(vec4(1), c, vec4(c.a));\n"
"}\n"
;


static const char* p1PS_CmatrixTexture = PRECISION
"uniform mat4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 c = texture2D(tex0, vtc0);\n"
"  gl_FragColor = c * cxmul + cxadd * (c.a + cxadd.a);\n"
"}\n"
;

static const char* p1PS_CmatrixTextureMultiply = PRECISION
"uniform mat4      cxmul;\n"
"uniform vec4      cxadd;\n"
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  vec4 c = texture2D(tex0, vtc0);\n"
"  c = c * cxmul + cxadd * (c.a + cxadd.a);\n"
"  gl_FragColor = mix(vec4(1), c, vec4(c.a));\n"
"}\n"
;

static const char* p1PS_Tex2d = PRECISION
"uniform sampler2D tex0;\n"
"varying vec2      vtc0;\n"
"void main(void)\n"
"{\n"
"  gl_FragColor = texture2D(tex0, vtc0);\n"
"}\n"
;

static const char* p1PS_Solid = PRECISION
"varying vec4      vcolor;\n"
"void main(void)\n"
"{\n"
"  gl_FragColor = vcolor;\n"
"}\n"
;

static inline void GFx_GL2_UnusedShaders()
{
    GUNUSED3(p1VS_3d, p1PS_Tex2d, p1PS_Solid);

    GUNUSED3(p1VS_3D_Pos, p1VS_3D_PosTC, p1VS_3D_Glyph);
    GUNUSED3(p1VS_3D_PosCTC, p1VS_3D_PosCFTC, p1VS_3D_PosCFTC2);
}

#undef PRECISION

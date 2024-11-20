namespace GL { 

const char* pSource_FBox2InnerShadow = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"        gl_FragColor = mix(scolor, base, gl_FragColor.a) * base.a;\n"
"	    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2InnerShadowHighlight = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"      gl_FragColor.ar = saturate((1.0 - gl_FragColor.ar) - (1.0 - gl_FragColor.ra) * 0.5);\n"
"    gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r\n"
"              + base * (1.0 - gl_FragColor.a - gl_FragColor.r)) * base.a;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2InnerShadowHighlightKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"      gl_FragColor.ar = saturate((1.0 - gl_FragColor.ar) - (1.0 - gl_FragColor.ra) * 0.5);\n"
"    gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r\n"
"              + base * (1.0 - gl_FragColor.a - gl_FragColor.r)) * base.a;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2InnerShadowKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"        gl_FragColor = scolor * (1.0 - gl_FragColor.a) * base.a;\n"
"	    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2InnerShadowMul = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"        gl_FragColor = mix(scolor, base, gl_FragColor.a) * base.a;\n"
"	    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2InnerShadowMulHighlight = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"      gl_FragColor.ar = saturate((1.0 - gl_FragColor.ar) - (1.0 - gl_FragColor.ra) * 0.5);\n"
"    gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r\n"
"              + base * (1.0 - gl_FragColor.a - gl_FragColor.r)) * base.a;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2InnerShadowMulHighlightKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"      gl_FragColor.ar = saturate((1.0 - gl_FragColor.ar) - (1.0 - gl_FragColor.ra) * 0.5);\n"
"    gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r\n"
"              + base * (1.0 - gl_FragColor.a - gl_FragColor.r)) * base.a;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2InnerShadowMulKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2DLod(srctex, tc * srctexscale, 0.0);\n"
"        gl_FragColor = scolor * (1.0 - gl_FragColor.a) * base.a;\n"
"	    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2Shadow = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = scolor * gl_FragColor.a * (1.0-base.a) + base;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2ShadowHighlight = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r) * (1.0-base.a) + base;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2ShadowHighlightKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r) * (1.0-base.a) + base;\n"
"      gl_FragColor *= (1.0 - base.a);\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2ShadowKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = scolor * gl_FragColor.a * (1.0-base.a) + base;\n"
"      gl_FragColor *= (1.0 - base.a);\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2ShadowMul = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = scolor * gl_FragColor.a * (1.0-base.a) + base;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2ShadowMulHighlight = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r) * (1.0-base.a) + base;\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2ShadowMulHighlightKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = (scolor * gl_FragColor.a + scolor2 * gl_FragColor.r) * (1.0-base.a) + base;\n"
"      gl_FragColor *= (1.0 - base.a);\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2ShadowMulKnockout = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform vec4 scolor2;\n"
"uniform sampler2D srctex;\n"
"uniform vec2 srctexscale;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  vec4 base = texture2D(srctex, tc * srctexscale);\n"
"\n"
"      gl_FragColor = scolor * gl_FragColor.a * (1.0-base.a) + base;\n"
"      gl_FragColor *= (1.0 - base.a);\n"
"    gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2Shadowonly = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = scolor * gl_FragColor.a;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2ShadowonlyHighlight = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = scolor * gl_FragColor.a;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2ShadowonlyMul = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color += texture2DLod(tex, tc + (offset + i) * texscale, 0.0);\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = scolor * gl_FragColor.a;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2ShadowonlyMulHighlight = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform vec2 offset;\n"
"uniform vec4 scolor;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"  {           color.a += texture2DLod(tex, tc + (offset + i) * texscale, 0.0).a;\n"
"       color.r += texture2DLod(tex, tc - (offset + i) * texscale, 0.0).a;\n"
"      }  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = scolor * gl_FragColor.a;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_VVatc = 
"uniform mat4 mvp;\n"
"attribute vec2 atc;\n"
"attribute vec4 pos;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  tc = atc;\n"
"\n"
"  gl_Position = pos * mvp;\n"
"\n"
"}";

const char* pSource_FBox1Blur = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  float  i = 0.0;\n"
"  for (i = -fsize.x; i <= fsize.x; i++)\n"
"    color += texture2DLod(tex, tc + i * texscale, 0.0);\n"
"  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox1BlurMul = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  float  i = 0.0;\n"
"  for (i = -fsize.x; i <= fsize.x; i++)\n"
"    color += texture2DLod(tex, tc + i * texscale, 0.0);\n"
"  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FBox2Blur = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"    color += texture2DLod(tex, tc + i * texscale, 0.0);\n"
"  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"\n"
"}";

const char* pSource_FBox2BlurMul = 
"uniform vec4 cxadd;\n"
"uniform vec4 cxmul;\n"
"uniform vec4 fsize;\n"
"uniform sampler2D tex;\n"
"uniform vec2 texscale;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 color = vec4(0.0);\n"
"  vec2 i = vec2(0.0);\n"
"  for (i.x = -fsize.x; i.x <= fsize.x; i.x++)\n"
"    for (i.y = -fsize.y; i.y <= fsize.y; i.y++)\n"
"    color += texture2DLod(tex, tc + i * texscale, 0.0);\n"
"  gl_FragColor = color * fsize.w;\n"
"  gl_FragColor = gl_FragColor * cxmul + cxadd * gl_FragColor.a;\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

const char* pSource_FCMatrix = 
"uniform vec4 cxadd;\n"
"uniform mat4 cxmul;\n"
"uniform sampler2D tex;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 c = texture2D(tex, tc);\n"
"  gl_FragColor = mul(c,cxmul) + cxadd * (c.a + cxadd.a);\n"
"\n"
"}";

const char* pSource_FCMatrixMul = 
"uniform vec4 cxadd;\n"
"uniform mat4 cxmul;\n"
"uniform sampler2D tex;\n"
"varying vec2 tc;\n"
"void main() {\n"
"  vec4 c = texture2D(tex, tc);\n"
"  gl_FragColor = mul(c,cxmul) + cxadd * (c.a + cxadd.a);\n"
"  gl_FragColor = mix(vec4(1), gl_FragColor, vec4(gl_FragColor.a));\n"
"\n"
"}";

enum VertexShaderType2
{
    VS2_None = 0,
    VS2_start_shadows,
    VS2_VVatc = 1,
    VS2_end_shadows = 1,
    VS2_start_blurs,
    VS2_end_blurs = 1,
    VS2_start_cmatrix,
    VS2_end_cmatrix = 1,
    VS2_Count,

};

static const char* VShaderSources2[VS2_Count] = {
    NULL,
    pSource_VVatc,
};

enum VertexShader2Uniform
{
    VSU_mvp,
    VSU_Count
};

enum FragShaderType2
{
    FS2_None = 0,
    FS2_start_shadows,
    FS2_FBox2InnerShadow = 1,
    FS2_FBox2InnerShadowHighlight,
    FS2_FBox2InnerShadowMul,
    FS2_FBox2InnerShadowMulHighlight,
    FS2_FBox2InnerShadowKnockout,
    FS2_FBox2InnerShadowHighlightKnockout,
    FS2_FBox2InnerShadowMulKnockout,
    FS2_FBox2InnerShadowMulHighlightKnockout,
    FS2_FBox2Shadow,
    FS2_FBox2ShadowHighlight,
    FS2_FBox2ShadowMul,
    FS2_FBox2ShadowMulHighlight,
    FS2_FBox2ShadowKnockout,
    FS2_FBox2ShadowHighlightKnockout,
    FS2_FBox2ShadowMulKnockout,
    FS2_FBox2ShadowMulHighlightKnockout,
    FS2_FBox2Shadowonly,
    FS2_FBox2ShadowonlyHighlight,
    FS2_FBox2ShadowonlyMul,
    FS2_FBox2ShadowonlyMulHighlight,
    FS2_end_shadows = 20,
    FS2_start_blurs,
    FS2_FBox1Blur = 21,
    FS2_FBox2Blur,
    FS2_FBox1BlurMul,
    FS2_FBox2BlurMul,
    FS2_end_blurs = 24,
    FS2_start_cmatrix,
    FS2_FCMatrix = 25,
    FS2_FCMatrixMul,
    FS2_end_cmatrix = 26,
    FS2_Count,

    FS2_shadows_Highlight            = 0x00000001,
    FS2_shadows_Mul                  = 0x00000002,
    FS2_shadows_Knockout             = 0x00000004,
    FS2_blurs_Box2                 = 0x00000001,
    FS2_blurs_Mul                  = 0x00000002,
    FS2_cmatrix_Mul                  = 0x00000001,
};

static const char* FShaderSources2[FS2_Count] = {
    NULL,
    pSource_FBox2InnerShadow,
    pSource_FBox2InnerShadowHighlight,
    pSource_FBox2InnerShadowMul,
    pSource_FBox2InnerShadowMulHighlight,
    pSource_FBox2InnerShadowKnockout,
    pSource_FBox2InnerShadowHighlightKnockout,
    pSource_FBox2InnerShadowMulKnockout,
    pSource_FBox2InnerShadowMulHighlightKnockout,
    pSource_FBox2Shadow,
    pSource_FBox2ShadowHighlight,
    pSource_FBox2ShadowMul,
    pSource_FBox2ShadowMulHighlight,
    pSource_FBox2ShadowKnockout,
    pSource_FBox2ShadowHighlightKnockout,
    pSource_FBox2ShadowMulKnockout,
    pSource_FBox2ShadowMulHighlightKnockout,
    pSource_FBox2Shadowonly,
    pSource_FBox2ShadowonlyHighlight,
    pSource_FBox2ShadowonlyMul,
    pSource_FBox2ShadowonlyMulHighlight,
    pSource_FBox1Blur,
    pSource_FBox2Blur,
    pSource_FBox1BlurMul,
    pSource_FBox2BlurMul,
    pSource_FCMatrix,
    pSource_FCMatrixMul,
};

enum FragShader2Uniform
{
    FSU_cxadd,
    FSU_cxmul,
    FSU_fsize,
    FSU_offset,
    FSU_scolor,
    FSU_scolor2,
    FSU_srctex,
    FSU_srctexscale,
    FSU_tex,
    FSU_texscale,
    FSU_Count
};

static VertexShaderType2 VShaderForFShader[FS2_Count] = {
    VS2_None,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
    VS2_VVatc,
};

} // namespace GL

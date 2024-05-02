#pragma once
#include "GTimer.h"
#include "GFile.h"
#include "GFxLoader.h"
#include "GFxEvent.h"
#include "GFxPlayer.h"
#include "GFxFontLib.h"
#include "GrendererOGL.h"
#include "CGFxManager.h"

class CFrameEIHandler;

class CFSCommandHandler : public GFxFSCommandHandler
{
private:
	CInternalmethod* pCommand;
	CFrameEIHandler* inheritance;
public:
	CFSCommandHandler(CInternalmethod* c, CFrameEIHandler* mt):pCommand(c),inheritance(mt) {
	};
	virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg);
};

class CExternalEIHandler : public GFxExternalInterface
{
private:
	CExternalmethod* method;
	CFrameEIHandler* inheritance;
public:
	CExternalEIHandler(CExternalmethod* _m, CFrameEIHandler* mt):method(_m),inheritance(mt) {
	};
public:
	virtual void Callback(GFxMovieView* pmovieView, const char* methodName, const GFxValue* args, UInt argCount);
};

class CFrameEIHandler : public CNewUIObj
{
public:
	CFrameEIHandler();
	virtual ~CFrameEIHandler();
	static  bool Make(int _UIType, CGFxManager*_GfxRepo, const char* _pfilename, CInternalmethod* pcommand, CExternalmethod* method, UPInt _memoryArena = 0, GFxMovieView::AlignType AlignType = GFxMovieView::Align_TopLeft, FontConfig* _pconfig = NULL,const char* _languag = NULL);

	bool Render();
	bool Update();
	bool UpdateMouseEvent(int key);
	bool UpdateKeyEvent();
	float GetLayerDepth();
	float GetKeyEventOrder();
	void* GetMovie();
	void InvokeObj(const char* pmethodName, CEICommandContainer* Obj);
	void InvokeArray(const char* pmethodName, CEICommandContainer* Obj, int size);
	void Invoke(const char* pmethodName, const char* pargFmt, va_list args);
	bool OnCreateDevice(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags = 0);

	void OnLostDevice();
	void OnResetDevice();
	void OnDestroyDevice();
	void ReceiveStrUID(const char* name);
private:
	bool OpenGLWin32App();
	bool InitGFx(const char* pfilename, CInternalmethod* pcommand, CExternalmethod* method, UPInt memoryArena, GFxMovieView::AlignType AlignType, FontConfig* pconfig, const char* language);
private:
	GFxLoader				gfxLoader;
	GPtr<GFxMovieDef>		pContainerMovieDef;
	GPtr<GFxMovieView>		pContainerMovie;
	GPtr<GRendererOGL>		pRenderer;
	GPtr<GFxRenderConfig>	pRenderConfig;
	GPtr<GFxRenderStats>	pRenderStats;

	int						m_ResolutionX;
	int						m_ResolutionY;
	bool					m_bWireFrame;
	DWORD					MovieLastTime;

	char					_strUID[120];
};

#define gFxLoader		 (&gfxLoader)
extern void GetHashMembers(void* pClass, const char* member, void* val);
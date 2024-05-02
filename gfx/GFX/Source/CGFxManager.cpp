#include "stdafx.h"
#include "Util.h"
#include "CFrameEIHandler.h"
#include "CGFxManager.h"

GFileStat m_FontConfigFileStats;

CGFxManager* CGFxManager::Instance()
{
	static CGFxManager _pGFx;
	return &_pGFx;
}

CGFxManager::CGFxManager()
{
	m_ResolutionX = 1024;
	m_ResolutionY = 768;
	m_iUISelection = 1;
	m_bNoFontConfig = FALSE;
	m_FontConfigIndex = -1;
	m_isCompletedLoad = FALSE;
	m_iNowSceneFlag = NON_SCENE;
}

CGFxManager::~CGFxManager()
{
}

bool CGFxManager::Initialize()
{
	if(m_isCompletedLoad != TRUE)
	{
		m_AbsolutePath = GetModulePath() + "Data\\Interface\\HUDX\\";

		m_FontConfigs = new FontConfigSet;

		if (!LoadDefaultFontConfigFromPath(m_AbsolutePath.c_str()) )
		{
			CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR","[CGFxManager::Initialize][File: %s][Line: %d]", __FILE__, __LINE__);
		}

		if(m_FontConfigs->GetSize() > 0)
			m_FontConfigIndex %= (SInt)m_FontConfigs->GetSize();

		m_isCompletedLoad = TRUE;

		return !m_isCompletedLoad;
	}

	return m_isCompletedLoad;
}

bool CGFxManager::AddObj(DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, GFxMovieView::AlignType AlignType)
{
	if(m_isCompletedLoad != TRUE)
		return false;

	type_map_uibase::iterator mi = m_mapUI.find(index);

	if(mi == m_mapUI.end())
	{
		FontConfig* pconfig = NULL;

		if (m_FontConfigIndex != -1) 
			pconfig = GetCurrentFontConfig();

		const char* language = (m_FontConfigIndex == -1) ? "Default": (*m_FontConfigs)[m_FontConfigIndex]->ConfigName.ToCStr();

		std::string filename = m_AbsolutePath;
		filename += FileName;
		return CFrameEIHandler::Make(index, this, filename.c_str(), pcommand, method, 0 , AlignType, pconfig, language);
	}
	return true;
}

void CGFxManager::AddObj(DWORD index, CNewUIObjPtr pUIObj)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);
	if(mi == m_mapUI.end())
	{
		m_vecUI.push_back(pUIObj);
		m_mapUI.insert( type_map_uibase::value_type(index, pUIObj) );
	}
}

void CGFxManager::RemoveUIObj(DWORD dwKey)
{
	type_map_uibase::iterator mi = m_mapUI.find(dwKey);
	if(mi != m_mapUI.end())
	{
		type_vector_uibase::iterator vi = std::find(m_vecUI.begin(), m_vecUI.end(), (*mi).second);
		if(vi != m_vecUI.end())
		{
			m_vecUI.erase(vi);
		}
		m_mapUI.erase(mi);
	}
}

void CGFxManager::RemoveAllUIObjs()
{
	m_vecUI.clear();
	m_mapUI.clear();
	SAFE_DELETE(m_FontConfigs);
}

CNewUIObjPtr CGFxManager::FindUIObj(DWORD key)
{
	type_map_uibase::iterator mi = m_mapUI.find(key);

	if( mi != m_mapUI.end() )
	{
		return (*mi).second;
	}
	return NULL;
}

bool CGFxManager::Update()
{
	if(!m_isCompletedLoad)
		return false;

	type_vector_uibase::iterator vi = m_vecUI.begin();
	for(; vi != m_vecUI.end(); vi++)
	{
		if((*vi)->IsEnabled()) 
		{
			if(false == (*vi)->Update())
			{
				return false;
			}
		}
	}
	return true;
}

bool CGFxManager::Render()
{
	if(!m_isCompletedLoad)
		return false;

	type_vector_uibase::iterator vi = m_vecUI.begin();

	for(; vi != m_vecUI.end(); vi++)
	{
		if((*vi)->IsVisible()) 
		{
			if(m_BackResolutionX != m_ResolutionX || m_BackResolutionY != m_ResolutionY)
			{
				m_BackResolutionX = m_ResolutionX;
				m_BackResolutionY = m_ResolutionY;
				(*vi)->OnCreateDevice(m_ResolutionX, m_ResolutionY, 0, 0, m_ResolutionX, m_ResolutionY);
			}
			(*vi)->Render();
		}
	}

	return true;
}

bool CGFxManager::Update(DWORD dwKey)
{
	if(!m_isCompletedLoad)
		return false;

	type_map_uibase::iterator mi = m_mapUI.find(dwKey);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Update();
	}
	return true;
}

bool CGFxManager::Render(DWORD dwKey)
{
	if(!m_isCompletedLoad)
		return false;

	type_map_uibase::iterator mi = m_mapUI.find(dwKey);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Render();
	}
	return true;
}

bool CGFxManager::UpdateMouseEvent(DWORD dwKey, int Event)
{
	if(!m_isCompletedLoad)
		return false;

	type_map_uibase::iterator mi = m_mapUI.find(dwKey);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->UpdateMouseEvent(Event);
	}
	return true;
}

void CGFxManager::HideAll()
{
}

void CGFxManager::Show(DWORD index)
{
	if(!m_isCompletedLoad)
		return;

	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Show(true);
	}
}

void CGFxManager::Hide(DWORD index)
{
	if(!m_isCompletedLoad)
		return;

	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Show(false);
	}
}

void CGFxManager::Toggle(DWORD index)
{
	if(!m_isCompletedLoad)
		return;

	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		if((*mi).second->IsVisible())
		{
			(*mi).second->Show(false);
		}
		else
		{
			(*mi).second->Show(true);
		}
	}
}

bool CGFxManager::IsVisible(DWORD index)
{
	if(m_isCompletedLoad)
	{
		type_map_uibase::iterator mi = m_mapUI.find(index);

		if( mi != m_mapUI.end() )
		{
			return (*mi).second->IsVisible();
		}
	}
	return false;
}

bool CGFxManager::Invoke(DWORD index, const char* pmethodName, const char* pargFmt, va_list args)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Invoke(pmethodName, pargFmt, args);
	}
	return true;
}

bool CGFxManager::InvokeArg(DWORD index, const char* pmethodName, CEICommandContainer& args)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->InvokeObj(pmethodName, &args);
	}
	return true;
}

bool CGFxManager::InvokeArray(DWORD index, const char* pmethodName, CEICommandContainer* Array, int Size)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->InvokeArray(pmethodName, Array, Size);
	}
	return true;
}

void CGFxManager::OnCreateDevice(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags)
{
	m_ResolutionX = w;
	m_ResolutionY = h;

	m_BackResolutionX = m_ResolutionX;
	m_BackResolutionY = m_ResolutionY;
}

void CGFxManager::OnResetDevice()
{
	if(!m_isCompletedLoad)
		return;
}

void CGFxManager::OnLostDevice()
{
	if(!m_isCompletedLoad)
		return;
}

void CGFxManager::OnDestroyDevice()
{
	if(!m_isCompletedLoad)
		return;
}

FontConfig* CGFxManager::GetCurrentFontConfig()
{
	if (m_FontConfigIndex == -1)
		return NULL;

	FontConfig* fc = NULL;
	SInt sIdx = m_FontConfigIndex;
	bool ok = false;

	while (!ok)
	{
		ok = true;
		fc = (*m_FontConfigs)[m_FontConfigIndex];
		// check if all fontlib files exist
		for (UInt i=0; i < fc->FontLibFiles.GetSize(); i++)
		{
			// check if file exists
			GSysFile file(fc->FontLibFiles[i]);
			if (!file.IsValid())
			{
				ok = false;
				CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR","Fontlib file '%s' cannot be found. Skipping config '%s'..\\n", fc->FontLibFiles[i].ToCStr(), fc->ConfigName.ToCStr());
				break;
			}
		}

		if (!ok)
		{
			m_FontConfigIndex++;
			m_FontConfigIndex %= (SInt)m_FontConfigs->GetSize();
			if (m_FontConfigIndex == sIdx)
				return NULL;
		}
	}

	return (*m_FontConfigs)[m_FontConfigIndex];
}

void CGFxManager::LoadFontConfigs(ConfigParser* conf)
{
	m_FontConfigs->Parse(conf);

	if (m_FontConfigs->GetSize() > 0)
		m_FontConfigIndex = 0;
	else
		m_FontConfigIndex = -1;
}

int CGFxManager::GetFontConfigIndexByName(const char* pname)
{
	for (UInt i = 0; i < m_FontConfigs->GetSize(); i++)
	{
		if (G_stricmp(pname, (*m_FontConfigs)[i]->ConfigName) == 0) return i;
	}
	return -1;
}

bool CGFxManager::LoadDefaultFontConfigFromPath( const GString& path )
{
	if (!m_bNoFontConfig)
	{
		GString fontConfigFilePath;

		fontConfigFilePath.AppendString(path);

		if ( !GFxURLBuilder::ExtractFilePath(&fontConfigFilePath) )
		{
			fontConfigFilePath = "";
		}

		fontConfigFilePath += "fontconfig.txt";
		bool maintainIndex = false;

		if (m_FontConfig.size() == 0)
		{
			GFileStat fileStats;
			if ( GSysFile::GetFileStat(&fileStats, fontConfigFilePath.ToCStr()) )
			{
				m_FontConfig = fontConfigFilePath;
				m_FontConfigFileStats = fileStats;
			}
		}
		else
		{
			if (fontConfigFilePath == m_FontConfig.c_str())
			{
				GFileStat fileStats;
				if ( GSysFile::GetFileStat(&fileStats, fontConfigFilePath.ToCStr()) )
				{
					if ( !(fileStats == m_FontConfigFileStats) )
					{
						m_FontConfigFileStats = fileStats;
						maintainIndex = true;
					}
				}
			}
		}

		ConfigParser parser(fontConfigFilePath.ToCStr());
		SInt oldIdx = m_FontConfigIndex;
		LoadFontConfigs(&parser);

		if (maintainIndex && (m_FontConfigIndex == 0) && (oldIdx != -1))
		{
			m_FontConfigIndex = oldIdx;
			m_FontConfigIndex %= (SInt)m_FontConfigs->GetSize();
		}
		return true;
	}
	return false;
}

bool CGFxManager::CallBack(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *pbNoFurtherProcessing)
{
	uMouse = uMsg;
	//g_pNewKeyInput->ScanAsyncKeyState();

	switch (uMsg)
	{
		case WM_ACTIVATE:
			if(LOWORD(wParam) == WA_INACTIVE)
			{
				MouseLButton = false;
				MouseLButtonPop = false;
				MouseLButtonPush = false;
				MouseRButton = false;
				MouseRButtonPop = false;
				MouseRButtonPush = false;
				MouseMButton = false;
				MouseMButtonPop = false;
				MouseMButtonPush = false;
				MouseWheel = 0;
			}
		break;
	}

	if (MouseLButtonPop == true && (g_iMousePopPosition_x != MouseX || g_iMousePopPosition_y != MouseY)) 
		MouseLButtonPop = false;

	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		{
			MouseX = LOWORD(lParam);
			MouseY = HIWORD(lParam);
		}
		break;
	case WM_LBUTTONDOWN:
		MouseLButtonPop = false;
		if(!MouseLButton) 
			MouseLButtonPush = true;
		MouseLButton = true;
		SetCapture(hWnd);
		break;
	case WM_LBUTTONUP:
		MouseLButtonPush = false;
		MouseLButtonPop = true;
		MouseLButton = false;
		g_iMousePopPosition_x = MouseX;
		g_iMousePopPosition_y = MouseY;
		ReleaseCapture();
		break;
	case WM_RBUTTONDOWN:
		MouseRButtonPop = false;
		if(!MouseRButton) MouseRButtonPush = true;
		MouseRButton = true;
		SetCapture(hWnd);
		break;
	case WM_RBUTTONUP:
		MouseRButtonPush = false;
		if(MouseRButton)
			MouseRButtonPop = true;
		MouseRButton = false;
		ReleaseCapture();
		break;
	case WM_MBUTTONDOWN:
		MouseMButtonPop = false;
		if(!MouseMButton)
			MouseMButtonPush = true;
		MouseMButton = true;
		SetCapture(hWnd);
		break;
	case WM_MBUTTONUP:
		MouseMButtonPush = false;
		if(MouseMButton)
			MouseMButtonPop = true;
		MouseRButton = false;
		ReleaseCapture();
		break;
	case WM_MOUSEWHEEL:
		{
			MouseWheel = (short)GET_WHEEL_DELTA_WPARAM( wParam)/WHEEL_DELTA;
		}
		break;
	}

	if(m_isCompletedLoad)
	{
		int mx = LOWORD(lParam), my = HIWORD(lParam);

		type_vector_uibase::iterator vi = m_vecUI.begin();

		for(; vi != m_vecUI.end(); vi++)
		{
			if((*vi)->IsVisible()) 
			{
				GFxMovieView* pMovie = (GFxMovieView*)(*vi)->GetMovie();

				bool processedMouseEvent = false;

				if(uMsg == WM_MOUSEMOVE)
				{
					GFxMouseEvent mevent(GFxEvent::MouseMove, 0, (Float)mx, (Float)my);
					pMovie->HandleEvent(mevent);
					processedMouseEvent = true;
				}
				else if(uMsg == WM_LBUTTONDOWN)
				{
					GFxMouseEvent mevent(GFxEvent::MouseDown, 0, (Float)mx, (Float)my);
					pMovie->HandleEvent(mevent);
					processedMouseEvent = true;
				}
				else if(uMsg == WM_LBUTTONUP)
				{
					GFxMouseEvent mevent(GFxEvent::MouseUp, 0, (Float)mx, (Float)my);
					pMovie->HandleEvent(mevent);
					processedMouseEvent = true;
				}
				else if(uMsg == WM_RBUTTONDOWN)
				{
					GFxMouseEvent mevent(GFxEvent::MouseDown, 1, (Float)mx, (Float)my);
					pMovie->HandleEvent(mevent);
					processedMouseEvent = true;
				}
				else if(uMsg == WM_RBUTTONUP)
				{
					GFxMouseEvent mevent(GFxEvent::MouseUp, 1, (Float)mx, (Float)my);
					pMovie->HandleEvent(mevent);
					processedMouseEvent = true;
				}

				if(processedMouseEvent && pMovie->HitTest((Float)mx, (Float)my, GFxMovieView::HitTest_ShapesNoInvisible))
					*pbNoFurtherProcessing = true;

				if(uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP || uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR)
				{
					ProcessKeyEvent(pMovie, uMsg, wParam, lParam);
				}
			}
		}
	}
	return *pbNoFurtherProcessing;
}

// Implementa las funciones estáticas envolventes
bool CGFxManager::UpdateWrapper(void*& obj) { return static_cast<CGFxManager*>(obj)->Update(); }
bool CGFxManager::RenderWrapper(void*& obj) { return static_cast<CGFxManager*>(obj)->Render(); }
bool CGFxManager::RenderWithKeyWrapper(void*& obj, DWORD dwKey) { return static_cast<CGFxManager*>(obj)->Render(dwKey); }
bool CGFxManager::UpdateWithKeyWrapper(void*& obj, DWORD dwKey) { return static_cast<CGFxManager*>(obj)->Update(dwKey); }
bool CGFxManager::UpdateMouseEventWrapper(void*& obj, DWORD dwKey, int Event) { return static_cast<CGFxManager*>(obj)->UpdateMouseEvent(dwKey, Event); }

void CGFxManager::HideAllWrapper(void*& obj) { static_cast<CGFxManager*>(obj)->HideAll(); }
void CGFxManager::ShowWrapper(void*& obj, DWORD dwKey) { static_cast<CGFxManager*>(obj)->Show(dwKey); }
void CGFxManager::HideWrapper(void*& obj, DWORD dwKey) { static_cast<CGFxManager*>(obj)->Hide(dwKey); }
void CGFxManager::ToggleWrapper(void*& obj, DWORD dwKey) { static_cast<CGFxManager*>(obj)->Toggle(dwKey); }
bool CGFxManager::IsVisibleWrapper(void*& obj, DWORD dwKey) { return static_cast<CGFxManager*>(obj)->IsVisible(dwKey); }

bool CGFxManager::InvokeWrapper(void*& obj, DWORD index, const char* pmethodName, const char* pargFmt, va_list args)
{
	return static_cast<CGFxManager*>(obj)->Invoke(index, pmethodName, pargFmt, args);
}

bool CGFxManager::InvokeArgWrapper(void*& obj, DWORD index, const char* pmethodName, CEICommandContainer& args)
{
	return static_cast<CGFxManager*>(obj)->InvokeArg(index, pmethodName, args);
}

bool CGFxManager::InvokeArrayWrapper(void*& obj, DWORD index, const char* pmethodName, CEICommandContainer* args, int Size)
{
	return static_cast<CGFxManager*>(obj)->InvokeArray(index, pmethodName, args, Size);
}

bool CGFxManager::AddObjWrapper(void*& obj, DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, int aligntype)
{
	GFxMovieView::AlignType AlignType;

	if(aligntype >= GFxMovieView::Align_Center && aligntype <= GFxMovieView::Align_BottomRight)
		AlignType = (GFxMovieView::AlignType)aligntype;
	else
		AlignType = GFxMovieView::Align_TopLeft;

	return static_cast<CGFxManager*>(obj)->AddObj(index, pcommand, method, FileName, AlignType);
}

void CGFxManager::CreateDevice(void*& obj, DWORD iWindowWidth, DWORD iWindowHeight)
{
	static_cast<CGFxManager*>(obj)->Initialize();
	static_cast<CGFxManager*>(obj)->OnCreateDevice(iWindowWidth, iWindowHeight, 0, 0, iWindowWidth, iWindowHeight);
}

void CGFxManager::CallBackWrapper(void*& obj, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool pbNoFurtherProcessing;
	static_cast<CGFxManager*>(obj)->CallBack(hWnd, uMsg, wParam, lParam, &pbNoFurtherProcessing);
}
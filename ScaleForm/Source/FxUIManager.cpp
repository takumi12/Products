#include "stdafx.h"
#include "Util.h"
#include "CFrameEIHandler.h"
#include "FxUIManager.h"

GFileStat FontConfigFileStats;

CFxUIManager* CFxUIManager::Instance()
{
	static CFxUIManager _pGFx;
	return &_pGFx;
}

CFxUIManager::CFxUIManager()
{
	SizeWidth = 1024;
	SizeHeight = 768;
	m_iUISelection = 1;
	m_bNoFontConfig = FALSE;
	FontConfigIndex = -1;
	m_isCompletedLoad = FALSE;
	m_iNowSceneFlag = NON_SCENE;
	//-------------------------
	//mcHudNotice = NULL;
	pConfig = NULL;
}

CFxUIManager::~CFxUIManager()
{
	this->Release();
}

bool CFxUIManager::Initialize()
{
	if(m_isCompletedLoad != TRUE)
	{
		std::wstring pathW = GetExecutablePath() + L"\\Data\\Interface\\ScaleForm\\";


		m_AbsolutePath = WStringToString(pathW);

		if (LoadDefaultFontConfigFromPath(m_AbsolutePath.c_str()) == FALSE)
		{
			pLog->LogMessage(2, "font load Fail!!\nLoadDefaultFontConfigFromPath FAIL!!!! \n");
		}
		else
		{
			if (!GetLanguage(6, m_LanguageName))
			{
				strcpy(m_LanguageName, "Default");

				pLog->LogMessage(2, "font load Fail!!\nGetLanguage FAIL!!!! \n");
			}
		}

		//if(FontConfigs.GetSize() > 0)
		//	FontConfigIndex %= (SInt)FontConfigs.GetSize();
		//FontConfigIndex = 3;
		//FontConfigIndex %= (SInt)FontConfigs.GetSize();

		m_isCompletedLoad = TRUE;

		return !m_isCompletedLoad;
	}

	return m_isCompletedLoad;
}

bool CFxUIManager::AddObj(DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, GFxMovieView::AlignType AlignType)
{
	if(m_isCompletedLoad != TRUE)
		return false;

	type_map_uibase::iterator mi = m_mapUI.find(index);

	if(mi == m_mapUI.end())
	{
		//FontConfig* pconfig = NULL;
		//if (FontConfigIndex != -1) 
		//	pconfig = GetCurrentFontConfig();
		//
		////FontConfigIndex = 6;
		//
		//const char* language = (FontConfigIndex == -1) ? "Default": (FontConfigs)[FontConfigIndex]->ConfigName.ToCStr();
		//
		//pLog->LogMessage(2, "[%d][%s]", (SInt)FontConfigs.GetSize(), language);

		pLog->LogMessage(2, "[LanguageName: %s]", m_LanguageName);

		std::string filename = m_AbsolutePath;
		filename += FileName;
		return CFrameEIHandler::Make(index, this, FileName, filename.c_str(), pcommand, method, 0 , AlignType, pConfig, m_LanguageName);
	}
	return true;
}

void CFxUIManager::AddObj(DWORD index, CFxUIObjPtr pUIObj)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);
	if(mi == m_mapUI.end())
	{
		m_vecUI.push_back(pUIObj);
		m_mapUI.insert( type_map_uibase::value_type(index, pUIObj) );
	}
}

void CFxUIManager::RemoveUIObj(DWORD dwKey)
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

void CFxUIManager::RemoveAllUIObjs()
{
	type_vector_uibase::iterator vi = m_vecUI.begin();
	for (; vi != m_vecUI.end(); vi++)
	{
		SAFE_DELETE(*vi);
	}

	m_vecUI.clear();
	m_mapUI.clear();
}

CFxUIObjPtr CFxUIManager::FindUIObj(DWORD key)
{
	type_map_uibase::iterator mi = m_mapUI.find(key);

	if( mi != m_mapUI.end() )
	{
		return (*mi).second;
	}
	return NULL;
}

bool CFxUIManager::Update()
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

bool CFxUIManager::Render()
{
	if(!m_isCompletedLoad)
		return false;

	type_vector_uibase::iterator vi = m_vecUI.begin();

	for(; vi != m_vecUI.end(); vi++)
	{
		if((*vi)->IsVisible()) 
		{
			/*if(m_BackResolutionX != SizeWidth || m_BackResolutionY != SizeHeight)
			{
				m_BackResolutionX = SizeWidth;
				m_BackResolutionY = SizeHeight;
				(*vi)->OnCreateDevice(SizeWidth, SizeHeight, 0, 0, SizeWidth, SizeHeight);
			}*/
			(*vi)->Render();
		}
	}

	return true;
}

bool CFxUIManager::Update(DWORD dwKey)
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

bool CFxUIManager::Render(DWORD dwKey)
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

bool CFxUIManager::UpdateMouseEvent(DWORD dwKey, int Event)
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

void CFxUIManager::HideAll()
{
}

void CFxUIManager::Show(DWORD index)
{
	if(!m_isCompletedLoad)
		return;

	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Show(true);
	}
}

void CFxUIManager::Hide(DWORD index)
{
	if(!m_isCompletedLoad)
		return;

	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Show(false);
	}
}

void CFxUIManager::Toggle(DWORD index)
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

bool CFxUIManager::IsVisible(DWORD index)
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

bool CFxUIManager::Invoke(DWORD index, const char* pmethodName, const char* pargFmt, ...)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);

	if (mi != m_mapUI.end())
	{
		va_list args;
		va_start(args, pargFmt);
		(*mi).second->Invoke(pmethodName, pargFmt, args);
		va_end(args);
	}
	return true;
}

bool CFxUIManager::Invoke(DWORD index, const char* pmethodName, const char* pargFmt, va_list args)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->Invoke(pmethodName, pargFmt, args);
	}
	return true;
}

bool CFxUIManager::InvokeArg(DWORD index, const char* pmethodName, CEICommandContainer& args)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->InvokeObj(pmethodName, &args);
	}
	return true;
}

bool CFxUIManager::InvokeArray(DWORD index, const char* pmethodName, CEICommandContainer* Array, int Size)
{
	type_map_uibase::iterator mi = m_mapUI.find(index);

	if( mi != m_mapUI.end() )
	{
		(*mi).second->InvokeArray(pmethodName, Array, Size);
	}
	return true;
}

void CFxUIManager::OnCreateDevice(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags)
{
	SizeWidth = w;
	SizeHeight = h;

	m_BackResolutionX = SizeWidth;
	m_BackResolutionY = SizeHeight;
}

void CFxUIManager::OnResetDevice(HWND hWnd)
{
	if(!m_isCompletedLoad)
		return;

	RECT rect;

	SInt width = SizeWidth;
	SInt height = SizeHeight;

	if (GetWindowRect(hWnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	if (width != SizeWidth || height != SizeHeight)
	{
		SizeWidth = width;
		SizeHeight = height;

		type_vector_uibase::iterator vi = m_vecUI.begin();

		for (; vi != m_vecUI.end(); vi++)
		{
			if ((*vi)->IsVisible())
			{
				(*vi)->OnCreateDevice(this->SizeWidth, this->SizeHeight, 0, 0, this->SizeWidth, this->SizeHeight);
			}
		}
	}
}

void CFxUIManager::OnLostDevice()
{
	if(!m_isCompletedLoad)
		return;
}

void CFxUIManager::OnDestroyDevice()
{
	if(!m_isCompletedLoad)
		return;
}

bool CFxUIManager::GetLanguage(int LanguageId, char* LanguageName)
{
	pLog->LogMessage(2, "[FontConfigs: %d]", FontConfigs.GetSize());

	if(!FontConfigs.GetSize())
		return false;

	FontConfigIndex = LanguageId;
	FontConfigIndex %= (SInt)FontConfigs.GetSize();

	pConfig = NULL;

	if (FontConfigIndex != -1)
		pConfig = GetCurrentFontConfig();

	if (FontConfigIndex == -1)
	{
		strcpy(LanguageName, "Default");
	}
	else
	{
		strcpy(LanguageName, (FontConfigs)[FontConfigIndex]->ConfigName.ToCStr());
	}

	return pConfig != NULL;
}

bool CFxUIManager::LoadFontConfig(const GString& fontConfigFilePath)
{
	ConfigParser parser(fontConfigFilePath.ToCStr());
	SInt oldIdx = FontConfigIndex;
	LoadFontConfigs(&parser);

	if ((FontConfigIndex == 0) && (oldIdx != -1))
	{
		FontConfigIndex = oldIdx;
		FontConfigIndex %= (SInt)FontConfigs.GetSize();
	}

	return true;
}

void CFxUIManager::LoadFontConfigs(ConfigParser* conf)
{
	FontConfigs.Parse(conf);

	if (FontConfigs.GetSize() > 0)
		FontConfigIndex = 0;
	else
		FontConfigIndex = -1;
}


int CFxUIManager::GetFontConfigIndexByName(const char* pname)
{
	for (UInt i = 0; i < FontConfigs.GetSize(); i++)
	{
		if (G_stricmp(pname, (FontConfigs)[i]->ConfigName) == 0)
			return i;
	}
	return -1;
}

FontConfig* CFxUIManager::GetCurrentFontConfig()
{
	if (FontConfigIndex == -1)
		return NULL;

	FontConfig* fc = NULL;
	SInt sIdx = FontConfigIndex;
	bool ok = false;

	while (!ok)
	{
		ok = true;
		fc = FontConfigs[FontConfigIndex];
		// check if all fontlib files exist
		for (UInt i=0; i < fc->FontLibFiles.GetSize(); i++)
		{
			// check if file exists
			GSysFile file(fc->FontLibFiles[i]);

			pLog->LogMessage(2, "[FontLibFiles: %s]", fc->FontLibFiles[i].ToCStr());

			if (!file.IsValid())
			{
				ok = false;
				pLog->LogMessage(2, "Fontlib file '%s' cannot be found. Skipping config '%s'..\n", fc->FontLibFiles[i].ToCStr(), fc->ConfigName.ToCStr());

				//CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR","Fontlib file '%s' cannot be found. Skipping config '%s'..\\n", fc->FontLibFiles[i].ToCStr(), fc->ConfigName.ToCStr());
				break;
			}
		}

		if (!ok)
		{
			FontConfigIndex++;
			FontConfigIndex %= (SInt)FontConfigs.GetSize();
			if (FontConfigIndex == sIdx)
				return NULL;
		}
	}

	return FontConfigs[FontConfigIndex];
}

bool CFxUIManager::LoadDefaultFontConfigFromPath(const GString& path)
{
	if (!m_bNoFontConfig)
	{
		GString fontConfigFilePath;

		fontConfigFilePath.AppendString(path);

		m_bNoFontConfig = TRUE;
		if (!GFxURLBuilder::ExtractFilePath(&fontConfigFilePath))
		{
			fontConfigFilePath = "";
		}

		fontConfigFilePath += "fontconfig.txt";

		bool maintainIndex = false;

		if (FontConfigFilePath.GetLength() == 0)
		{
			pLog->LogMessage(2, "FontConfigFilePath.GetLength()[ %s]", fontConfigFilePath.ToCStr());
			GFileStat fileStats;
			if (GSysFile::GetFileStat(&fileStats, fontConfigFilePath))
			{
				FontConfigFilePath = fontConfigFilePath;
				FontConfigFileStats = fileStats;
			}
		}
		else // if the file was previously loaded and is modified
		{
			if (fontConfigFilePath == FontConfigFilePath)
			{
				// if modified time is not the same, then reload config file
				GFileStat fileStats;
				if (GSysFile::GetFileStat(&fileStats, fontConfigFilePath))
				{
					if (!(fileStats == FontConfigFileStats))
					{
						FontConfigFileStats = fileStats;
						maintainIndex = true;
					}
				}
			}
		}
		//FontConfigIndex;
		ConfigParser parser(fontConfigFilePath);
		SInt oldIdx = FontConfigIndex;
		LoadFontConfigs(&parser);

		// try to maintain previous font config index
		if (maintainIndex && (FontConfigIndex == 0) && (oldIdx != -1))
		{
			FontConfigIndex = oldIdx;
			FontConfigIndex %= (SInt)FontConfigs.GetSize();
		}
		return true;
	}
	return false;
}

bool CFxUIManager::CallBack(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *pbNoFurtherProcessing)
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
bool CFxUIManager::UpdateWrapper()
{
	return GMScaleForm->Update();
}

bool CFxUIManager::RenderWrapper()
{
	return GMScaleForm->Render();
}

bool CFxUIManager::RenderWithKeyWrapper(DWORD dwKey)
{
	return GMScaleForm->Render(dwKey);
}

bool CFxUIManager::UpdateWithKeyWrapper(DWORD dwKey)
{
	return GMScaleForm->Update(dwKey);
}

bool CFxUIManager::UpdateMouseEventWrapper(DWORD dwKey, int Event)
{
	return GMScaleForm->UpdateMouseEvent(dwKey, Event);
}

void CFxUIManager::HideAllWrapper()
{
	GMScaleForm->HideAll();
}

void CFxUIManager::ShowWrapper(DWORD dwKey)
{
	GMScaleForm->Show(dwKey);
}

void CFxUIManager::HideWrapper(DWORD dwKey)
{
	GMScaleForm->Hide(dwKey);
}

void CFxUIManager::ToggleWrapper(DWORD dwKey)
{
	GMScaleForm->Toggle(dwKey);
}

bool CFxUIManager::IsVisibleWrapper(DWORD dwKey)
{
	return GMScaleForm->IsVisible(dwKey);
}

bool CFxUIManager::InvokeWrapper(DWORD index, const char* pmethodName, const char* pargFmt, va_list args)
{
	return GMScaleForm->Invoke(index, pmethodName, pargFmt, args);
}

bool CFxUIManager::InvokeArgWrapper(DWORD index, const char* pmethodName, CEICommandContainer& args)
{
	return GMScaleForm->InvokeArg(index, pmethodName, args);
}

bool CFxUIManager::InvokeArrayWrapper(DWORD index, const char* pmethodName, CEICommandContainer* args, int Size)
{
	return GMScaleForm->InvokeArray(index, pmethodName, args, Size);
}

bool CFxUIManager::AddObjWrapper(DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, int aligntype)
{
	GFxMovieView::AlignType AlignType;

	if(aligntype >= GFxMovieView::Align_Center && aligntype <= GFxMovieView::Align_BottomRight)
		AlignType = (GFxMovieView::AlignType)aligntype;
	else
		AlignType = GFxMovieView::Align_TopLeft;

	return GMScaleForm->AddObj(index, pcommand, method, FileName, AlignType);
}

void CFxUIManager::CreateDevice(DWORD iWindowWidth, DWORD iWindowHeight)
{
	GMScaleForm->Initialize();
	GMScaleForm->OnCreateDevice(iWindowWidth, iWindowHeight, 0, 0, iWindowWidth, iWindowHeight);
	GMScaleForm->Create();
}

void CFxUIManager::ReleaseDevice()
{
	GMScaleForm->Release();
	GFxSystem::Destroy();
}

void CFxUIManager::CallBackWrapper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool pbNoFurtherProcessing;

	GMScaleForm->OnResetDevice(hWnd);

	GMScaleForm->CallBack(hWnd, uMsg, wParam, lParam, &pbNoFurtherProcessing);
}

void CFxUIManager::Create()
{
	mcHudNotice.Create();
	mcHudPlayer.Create();
	mcHudItemDrop.Create();
}

void CFxUIManager::Release()
{
	//SAFE_DELETE(mcHudNotice);
	//SAFE_DELETE(mcHudPlayer);
}
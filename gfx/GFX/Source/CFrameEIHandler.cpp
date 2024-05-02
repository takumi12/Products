#include "stdafx.h"
#include "Util.h"
#include "CFrameEIHandler.h"

bool CFrameEIHandler::Make(int _UIType, CGFxManager* _GfxRepo, const char* _pfilename, CInternalmethod* pcommand, CExternalmethod* method, UPInt _memoryArena, GFxMovieView::AlignType AlignType, FontConfig* _pconfig, const char* _language)
{
	if (_GfxRepo == NULL)
		return NULL;
	CFrameEIHandler* temp(new CFrameEIHandler);
	if (!temp->InitGFx(_pfilename, pcommand, method, _memoryArena, AlignType, _pconfig, _language))
	{
		SAFE_DELETE(temp);
		CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR", "Error(%s) Line: %d", __FILE__, __LINE__);
		return false;
	}
	temp->OnCreateDevice(_GfxRepo->m_ResolutionX, _GfxRepo->m_ResolutionY, 0, 0, _GfxRepo->m_ResolutionX, _GfxRepo->m_ResolutionY);

	_GfxRepo->AddObj(_UIType, temp);
	return temp != NULL;
}

CFrameEIHandler::CFrameEIHandler()
{
	m_bWireFrame = false;
	MovieLastTime = 0;
	pRenderer = NULL;
	memset(_strUID, 0, sizeof(_strUID));
}

CFrameEIHandler::~CFrameEIHandler()
{
	SAFE_DELETE(pContainerMovie);
	SAFE_DELETE(pContainerMovieDef);
}

bool CFrameEIHandler::OpenGLWin32App()
{
	if(!pRenderer)
	{
		pRenderer = *GRendererOGL::CreateRenderer();

		if (pRenderer)
			pRenderer->SetDependentVideoMode();
	}
	return (pRenderer.GetPtr() != NULL);
}

bool CFrameEIHandler::InitGFx(const char* pfilename, CInternalmethod* pcommand, CExternalmethod* method, UPInt memoryArena, GFxMovieView::AlignType AlignType, FontConfig* pconfig, const char* language)
{
	if (pconfig)
	{
		pconfig->Apply(gFxLoader);
	}

	GPtr<GFxFileOpener> pfileOpener = *new GFxFileOpener;

	gFxLoader->SetFileOpener(pfileOpener);

	if (OpenGLWin32App() == false)
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR", "Error(%s) Line: %d", __FILE__, __LINE__);
		return false;
	}

	UInt loadConstants = GFxLoader::LoadAll | GFxLoader::LoadKeepBindData | GFxLoader::LoadWaitFrame1;

	pRenderConfig = *new GFxRenderConfig(pRenderer);
	pRenderConfig->SetRenderFlags(GFxRenderConfig::RF_EdgeAA | GFxRenderConfig::RF_StrokeMask);

	gFxLoader->SetRenderConfig(pRenderConfig);

	pRenderStats = *new GFxRenderStats();
	gFxLoader->SetRenderStats(pRenderStats);

	GFxMovieInfo pMovieInfo;
	if (!gFxLoader->GetMovieInfo(pfilename, &pMovieInfo, false, loadConstants))
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR", "Error(%s) Line: %d\nError: Failed to get info about \"%s\"", __FILE__, __LINE__, pfilename);
		return false;
	}

	pContainerMovieDef = *(gFxLoader->CreateMovie(pfilename, loadConstants, memoryArena));
	if (!pContainerMovieDef)
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR", "Error(%s) Line: %d\nError: Failed to create a movie from \"%s\"", __FILE__, __LINE__, pfilename);
		return false;
	}

	pContainerMovie = *pContainerMovieDef->CreateInstance(GFxMovieDef::MemoryParams(), false);
	if (!pContainerMovie)
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, "ERROR", "Error(%s) Line: %d\nError: Failed to create movie instance", __FILE__, __LINE__);
		return false;
	}

	if (pContainerMovie->GetMeshCacheManager())
		pContainerMovie->GetMeshCacheManager()->ClearCache();

	GPtr<GFxActionControl> pActionControl = *new GFxActionControl();
	pContainerMovie->SetActionControl(pActionControl);

	CFSCommandHandler* pCommandHandler = new CFSCommandHandler(pcommand, this);
	pContainerMovie->SetFSCommandHandler(GPtr<GFxFSCommandHandler>(pCommandHandler));

	CExternalEIHandler* pEIHandler = new CExternalEIHandler(method, this);
	pContainerMovie->SetExternalInterface(GPtr<GFxExternalInterface>(pEIHandler));

	pContainerMovie->HandleEvent(GFxEvent::SetFocus);

	pContainerMovie->SetUserData(this);

	pContainerMovie->Advance(0.0f, 0);

	MovieLastTime = timeGetTime();

	pContainerMovie->SetBackgroundAlpha(0.0f);

	pContainerMovie->SetViewScaleMode(GFxMovieView::SM_NoScale);

	pContainerMovie->SetViewAlignment(AlignType);

	if (language == NULL)
		language = "Default";

	pContainerMovie->SetVariable("_global.gfxLanguage", language);

	pContainerMovie->SetVariable("_root.isClient", true);

	return true;
}

bool CFrameEIHandler::Update()
{
	if (!IsVisible() || !pContainerMovie)
		return false;

	DWORD mtime = timeGetTime();
	float deltaTime = ((float)(mtime - MovieLastTime)) / 1000.0f;
	MovieLastTime = mtime;
	pContainerMovie->Advance(deltaTime, 0);
	return true;
}

bool CFrameEIHandler::UpdateMouseEvent(int key)
{
	GFxMouseEvent mevent;
	switch (key)
	{
	case 1:
		mevent.Type = GFxEvent::MouseDown;
		mevent.Button = 0;
		mevent.x = (Float)MouseX;
		mevent.y = (Float)MouseY;
		pContainerMovie->HandleEvent(mevent);
		break;
	case 2:
		mevent.Type = GFxEvent::MouseUp;
		mevent.Button = 0;
		mevent.x = (Float)MouseX;
		mevent.y = (Float)MouseY;
		pContainerMovie->HandleEvent(mevent);
		break;
	case 3:
		mevent.Type = GFxEvent::MouseDown;
		mevent.Button = 1;
		mevent.x = (Float)MouseX;
		mevent.y = (Float)MouseY;
		pContainerMovie->HandleEvent(mevent);
		break;
	case 4:
		mevent.Type = GFxEvent::MouseUp;
		mevent.Button = 1;
		mevent.x = (Float)MouseX;
		mevent.y = (Float)MouseY;
		pContainerMovie->HandleEvent(mevent);
		break;
	}

	if (uMouse == WM_MOUSEWHEEL)
	{
		GFxMouseEvent mevent(GFxEvent::MouseWheel, 0, (Float)MouseX, (Float)MouseY, (Float)MouseWheel);
		pContainerMovie->HandleEvent(mevent);
	}
	return true;
}

bool CFrameEIHandler::UpdateKeyEvent()
{
	return true;
}

bool CFrameEIHandler::Render()
{
	if (!IsVisible() || !pContainerMovie)
		return false;

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	if (m_bWireFrame)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (pContainerMovie) //-- GFx Render
	{
		pContainerMovie->Display();

		pRenderer->EndDisplay();
	}

	if (m_bWireFrame)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glPopAttrib();
	glClear(GL_DEPTH_BUFFER_BIT);

	return true;
}

float CFrameEIHandler::GetLayerDepth()
{
	return 4.1f;
}

float CFrameEIHandler::GetKeyEventOrder()
{
	return 3.1f;
}

void* CFrameEIHandler::GetMovie()
{
	if (!pContainerMovie)
		return NULL;
	return pContainerMovie;
}

void CFrameEIHandler::InvokeObj(const char* pmethodName, CEICommandContainer* Obj)
{
	if (!pContainerMovie)
		return;

	UInt numArgs = 0;
	GFxValue pObjet(GFxValue::VT_Object);
	pContainerMovie->CreateObject(&pObjet, NULL, NULL, 0);

	for (int i = 0; i < Obj->NumArgs(); i++)
	{
		switch (Obj->GetHashType(i))
		{
		case GFxValue::VT_Boolean: // bool
			numArgs++;
			pObjet.SetMember(Obj->GetHashName(i), Obj->GetBoolean(i));
			break;
		case GFxValue::VT_Number: // double
			numArgs++;
			pObjet.SetMember(Obj->GetHashName(i), Obj->GetNumber(i));
			break;
		case GFxValue::VT_String: // string
			numArgs++;
			pObjet.SetMember(Obj->GetHashName(i), Obj->GetString(i));
			break;
		}
	}
	if (numArgs != 0)
	{
		char stCommand[250];
		sprintf_s(stCommand, "_root.%s.%s", _strUID, pmethodName);

		pContainerMovie->Invoke(pmethodName, &pObjet, 1);
	}
}

void CFrameEIHandler::InvokeArray(const char* pmethodName, CEICommandContainer* Obj, int size)
{
	if (!pContainerMovie)
		return;

	UInt numArgs = 0;
	GFxValue pArray(GFxValue::VT_Array);
	pContainerMovie->CreateArray(&pArray);

	pArray.SetArraySize(size);

	for(int i = 0; i < size; i++)
	{
		GFxValue pObjet(GFxValue::VT_Object);
		pContainerMovie->CreateObject(&pObjet, NULL, NULL, 0);

		for (int j = 0; j < Obj[i].NumArgs(); j++)
		{
			switch (Obj[i].GetHashType(j))
			{
			case GFxValue::VT_Boolean: // bool
				numArgs++;
				pObjet.SetMember(Obj[i].GetHashName(j), Obj[i].GetBoolean(j));
				break;
			case GFxValue::VT_Number: // double
				numArgs++;
				pObjet.SetMember(Obj[i].GetHashName(j), Obj[i].GetNumber(j));
				break;
			case GFxValue::VT_String: // string
				numArgs++;
				pObjet.SetMember(Obj[i].GetHashName(j), Obj[i].GetString(j));
				break;
			}
		}
		pArray.SetElement(i, pObjet);
	}
	if (numArgs != 0)
	{
		char stCommand[250];
		sprintf_s(stCommand, "_root.%s.%s", _strUID, pmethodName);

		pContainerMovie->Invoke(stCommand, &pArray, 1);
	}
}

void CFrameEIHandler::Invoke(const char* pmethodName, const char* pargFmt, va_list args)
{
	if (pContainerMovie)
	{
		char stCommand[250];
		sprintf_s(stCommand, "_root.%s.%s", _strUID, pmethodName);

		pContainerMovie->InvokeArgs(stCommand, pargFmt, args);
	}
}

bool CFrameEIHandler::OnCreateDevice(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags)
{
	if (!pContainerMovie)
		return false;

	if (!pRenderer->SetDependentVideoMode())
		return false;

	if (m_ResolutionX != w || m_ResolutionY != h)
	{
		m_ResolutionX = w;
		m_ResolutionY = h;

		pContainerMovie->SetViewport(m_ResolutionX, m_ResolutionY, left, top, w, h, flags);
	}
	return true;
}

void CFrameEIHandler::OnResetDevice()
{
	if (pRenderer)
	{
		pRenderer->ResetVideoMode();
		pRenderer->SetDependentVideoMode();
	}
}

void CFrameEIHandler::OnLostDevice()
{
	pRenderer->ResetVideoMode();
}

void CFrameEIHandler::OnDestroyDevice()
{
}

void CFrameEIHandler::ReceiveStrUID(const char* name)
{
	strcpy_s(_strUID, name);
}

void CFSCommandHandler::Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg)
{
	if (parg)
		pLog->LogMessage(0, "FsCallback: '%s' '%s'", pcommand, parg);
	else
		pLog->LogMessage(0, "FsCallback: '%s'", pcommand);

	if (pCommand)
	{
		pCommand->funcAction(pcommand, parg);
	}
}

void CExternalEIHandler::Callback(GFxMovieView* pmovieView, const char* methodName, const GFxValue* args, UInt numArgs)
{
	pLog->LogMessage(0, "Callback! %s, nargs = %d", (methodName) ? methodName : "(null)", numArgs);

	if (methodName && strcmp(methodName, "UIInit") == 0)
	{
		inheritance->ReceiveStrUID(args[0].GetString());
	}

	for (UInt i = 0; i < numArgs; ++i)
	{
		switch (args[i].GetType())
		{
		case GFxValue::VT_Boolean:
			pLog->LogMessage(0, "[VT_Boolean] arg(%d) = %s", i, (args[i].GetBool()) ? "true" : "false");
			break;
		case GFxValue::VT_Number:
			pLog->LogMessage(0, "[VT_Number] arg(%d) = %f", i, args[i].GetNumber());
			break;
		case GFxValue::VT_String:
			pLog->LogMessage(0, "[VT_String] arg(%d) = %s", i, args[i].GetString());
			break;
		case GFxValue::VT_Object:
			pLog->LogMessage(0, "[VT_Object] arg(%d):", i);
			args[i].HashTable(GetHashMembers, NULL, (GFxValue*)&args[i]);
			break;
		case GFxValue::VT_Array:
			break;
		default:
			pLog->LogMessage(0, "arg(%d) = [desconocido: %d]", i, args[i].GetType());
			break;
		}
	}
	pLog->LogMessage(0, "");

	if (method)
	{
		method->clear();
		method->SetName((methodName) ? methodName : "(null)");

		for (UInt i = 0; i < numArgs; ++i)
		{
			switch (args[i].GetType())
			{
			case GFxValue::VT_Boolean:
				method->push_back(args[i].GetBool());
				break;
			case GFxValue::VT_Number:
				method->push_back(args[i].GetNumber());
				break;
			case GFxValue::VT_String:
				method->push_back(args[i].GetString());
				break;
			case GFxValue::VT_Object:
			{
				CEICommandHandler info;
				type_member_value members;
				args[i].HashTable(GetHashMembers, &members, (GFxValue*)&args[i]);

				for (int l = 0; l < members.size(); l++)
				{
					MEMBER_VALUE* Member = &members[l];

					switch (Member->GetHashType())
					{
					case GFxValue::VT_Boolean:
						info.Addargumento(Member->GetName(), Member->GetBoolean());
						break;
					case GFxValue::VT_Number:
						info.Addargumento(Member->GetName(), Member->GetNumber());
						break;
					case GFxValue::VT_String:
						info.Addargumento(Member->GetName(), Member->GetString());
						break;
					}
				}
				method->push_back(info);
			}
			break;
			}
		}
		method->funcAction();
	}
}

void GetHashMembers(void* pClass, const char* member, void* val)
{
	GFxValue HashObj;
	GFxValue* value = static_cast<GFxValue*>(val);

	if (pClass != NULL)
	{
		type_member_value* vectorPtr = static_cast<type_member_value*>(pClass);

		if (value->GetMember(member, &HashObj))
		{
			vectorPtr->push_back(MEMBER_VALUE(member, HashObj));
		}
	}
	else
	{
		if (value->GetMember(member, &HashObj))
		{
			switch (HashObj.GetType())
			{
			case GFxValue::VT_Boolean:
				pLog->LogMessage(0, "->[VT_Boolean] member(%s) = %s", member, (HashObj.GetBool()) ? "true" : "false");
				break;
			case GFxValue::VT_Number:
				pLog->LogMessage(0, "->[VT_Number] member(%s) = %.2f", member, HashObj.GetNumber());
				break;
			case GFxValue::VT_String:
				pLog->LogMessage(0, "->[VT_String] member(%s) = %s", member, HashObj.GetString());
				break;
			case GFxValue::VT_Object:
				pLog->LogMessage(0, "->[VT_Object] member(%s)", member);
				break;
			case GFxValue::VT_Array:
				pLog->LogMessage(0, "->[VT_Array] member(%s)", member);
				break;
			case GFxValue::VT_DisplayObject:
				pLog->LogMessage(0, "->[VT_DisplayObject] member(%s)", member);
			default:
				pLog->LogMessage(0, "->[VT_Undefined] member(%s)", member);
				break;
			}
		}
	}
}
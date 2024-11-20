#include "StdAfx.h"
#include "FxUIManager.h"
#include "InterfaceNotice.h"

CInterfaceNotice::CInterfaceNotice(void)
{
}

CInterfaceNotice::~CInterfaceNotice(void)
{
}

bool CInterfaceNotice::Create()
{
	bool advance = GMScaleForm->AddObj(0, this, this, "hudNotice.gfx", GFxMovieView::Align_TopLeft);

	if (advance)
	{
		//function SetWindowSize(_iWidth, _iHeight)
		//GMScaleForm->Invoke(0, "changeScene", "%d", MAIN_SCENE + 1);
		//GMScaleForm->Invoke(0, "onLoad", "");
		//GMScaleForm->Invoke(1, "SetAlign", "%d", 5);
		//GMScaleForm->Invoke(1, "SetWindowSize", "%d %d", GMScaleForm->SizeWidth, GMScaleForm->SizeHeight);
		GMScaleForm->Show(0);
	}

	return advance;
}

bool CInterfaceNotice::funcAction()
{
	return false;
}

bool CInterfaceNotice::funcAction(const char* pcommand, const char* parg)
{
	return false;
}

bool CInterfaceNotice::funcAction(CFrameEIHandler* inheritance, const char* methodName, const GFxValue* args, unsigned int numArgs)
{
	return false;
}


CInterfacePlayer::CInterfacePlayer(void)
{
}

CInterfacePlayer::~CInterfacePlayer(void)
{
}

bool CInterfacePlayer::Create()
{
	bool advance = GMScaleForm->AddObj(1, this, this, "PlayerInfo.gfx", GFxMovieView::Align_TopLeft);
	GMScaleForm->Invoke(1, "InitScene", "");
	//GMScaleForm->Invoke(1, "changeScene", "%d", MAIN_SCENE + 1);
	//GMScaleForm->Invoke(1, "SetAlign", "%d", 5);
	//GMScaleForm->Invoke(1, "SetWindowSize", "%d %d", GMScaleForm->SizeWidth, GMScaleForm->SizeHeight);
	GMScaleForm->Show(1);
	return advance;
}

bool CInterfacePlayer::funcAction()
{
	return false;
}

bool CInterfacePlayer::funcAction(const char* pcommand, const char* parg)
{
	return false;
}

bool CInterfacePlayer::funcAction(CFrameEIHandler* inheritance, const char* methodName, const GFxValue* args, unsigned int numArgs)
{
	return false;
}

void CInterfacePlayer::Connection(const char* methodName, const char* pargFmt)
{
	if (methodName)
	{
		/*if (strcmp(methodName, "setItemView") == 0)
		{
			bool bView;
			sscanf(pargFmt, "%d", &bView); // Usamos sscanf para extraer los tres valores
			GMScaleForm->Invoke(1, "setItemView", "%b", bView);
		}
		else if (strcmp(methodName, "removeItem") == 0)
		{
			int bKey;
			sscanf(pargFmt, "%d", &bKey); // Usamos sscanf para extraer los tres valores
			GMScaleForm->Invoke(1, "removeItem", "%d", bKey);
		}
		else if (strcmp(methodName, "clearAllItem") == 0)
		{
			GMScaleForm->Invoke(1, "clearAllItem", "");
		}*/
	}
}

CInterfaceItemDrop::CInterfaceItemDrop(void)
{
}

CInterfaceItemDrop::~CInterfaceItemDrop(void)
{
}

bool CInterfaceItemDrop::Create()
{
	bool advance = GMScaleForm->AddObj(2, this, this, "ItemInfo.gfx", GFxMovieView::Align_TopLeft);
	GMScaleForm->Invoke(2, "InitScene", "");
	GMScaleForm->Show(2);
	return advance;
}

bool CInterfaceItemDrop::funcAction()
{
	return false;
}

bool CInterfaceItemDrop::funcAction(const char* pcommand, const char* parg)
{
	return false;
}

bool CInterfaceItemDrop::funcAction(CFrameEIHandler* inheritance, const char* methodName, const GFxValue* args, unsigned int numArgs)
{
	return false;
}

void CInterfaceItemDrop::Connection(const char* methodName, const char* pargFmt)
{
}

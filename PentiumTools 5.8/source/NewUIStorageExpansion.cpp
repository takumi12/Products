#include "stdafx.h"
#include "NewUISystem.h"
#include "CGMProtect.h"
#include "NewUIStorageExpansion.h"

SEASON3B::CNewUIStorageExpansion::CNewUIStorageExpansion()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;
	m_bLock = false;
}

SEASON3B::CNewUIStorageExpansion::~CNewUIStorageExpansion()
{
	Release();
}

bool SEASON3B::CNewUIStorageExpansion::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (pNewUIMng)
	{
		m_pNewUIMng = pNewUIMng;
		m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_STORAGE_EXTENSION, this);

		SetBtnInfo();

		SetPos(x, y);

		LoadImages();

		Show(false);

		m_bLock = false;

		return true;
	}
	return false;
}

void SEASON3B::CNewUIStorageExpansion::Release()
{
	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIStorageExpansion::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		SetButtonPosition(&m_BtnExit, m_Pos.x, m_Pos.y, 1);
	}
	else
	{
		m_BtnExit.SetPos(m_Pos.x + 13, m_Pos.y + 391);
	}
}

tagPOINT* SEASON3B::CNewUIStorageExpansion::GetPos()
{
	return &m_Pos;
}

bool SEASON3B::CNewUIStorageExpansion::UpdateMouseEvent()
{
	if (BtnProcess())
		return false;

	if (SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, 190, 429))
	{
	}

	return true;
}

bool SEASON3B::CNewUIStorageExpansion::UpdateKeyEvent()
{
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_STORAGE_EXTENSION) && SEASON3B::IsPress(VK_ESCAPE) == 1)
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_STORAGE_EXTENSION);
		return false;
	}
	return true;
}

bool SEASON3B::CNewUIStorageExpansion::Update()
{
	return true;
}

bool SEASON3B::CNewUIStorageExpansion::Render()
{
	EnableAlphaTest();

	glColor4f(1.0, 1.0, 1.0, 1.0);

	RenderFrame();

	RenderTexts();

	RenderButtons();

	g_pStorageInventory->RenderExpansionInven();

	DisableAlphaBlend();

	return true;
}

float SEASON3B::CNewUIStorageExpansion::GetLayerDepth()
{
	return 2.2f;
}

void SEASON3B::CNewUIStorageExpansion::LoadImages()
{
	LoadBitmap("Interface\\newui_msgbox_back.jpg", IMAGE_STORAGE_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back01.tga", IMAGE_STORAGE_TOP, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back02-L.tga", IMAGE_STORAGE_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back02-R.tga", IMAGE_STORAGE_RIGHT, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back03.tga", IMAGE_STORAGE_BOTTOM, GL_LINEAR);
}

void SEASON3B::CNewUIStorageExpansion::UnloadImages()
{
	DeleteBitmap(IMAGE_STORAGE_BOTTOM);
	DeleteBitmap(IMAGE_STORAGE_RIGHT);
	DeleteBitmap(IMAGE_STORAGE_LEFT);
	DeleteBitmap(IMAGE_STORAGE_TOP);
	DeleteBitmap(IMAGE_STORAGE_BACK);
}

void SEASON3B::CNewUIStorageExpansion::RenderFrame()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		RenderInventoryInterface(m_Pos.x, m_Pos.y, 0);
	}
	else
	{
		RenderImage(IMAGE_STORAGE_BACK, m_Pos.x, m_Pos.y, 190.0, 429.0);
		RenderImage(IMAGE_STORAGE_TOP, m_Pos.x, m_Pos.y, 190.0, 64.f);
		RenderImage(IMAGE_STORAGE_LEFT, m_Pos.x, m_Pos.y + 64, 21.f, 320.f);
		RenderImage(IMAGE_STORAGE_RIGHT, m_Pos.x + 190.0 - 21, m_Pos.y + 64, 21.f, 320.f);
		RenderImage(IMAGE_STORAGE_BOTTOM, m_Pos.x, m_Pos.y + 429.0 - 45, 190.0, 45.f);
	}
}

void SEASON3B::CNewUIStorageExpansion::RenderTexts()
{
	int PosX = GetPos()->x;
	int PosY = GetPos()->y;

	g_pRenderText->SetBgColor(0, 0, 0, 0);

	unicode::t_char pszText[100];

	if (g_pStorageInventory->IsLockStorage())
	{
		g_pRenderText->SetTextColor(0xF0u, 0x20u, 0x20u, 0xFFu);
		unicode::_sprintf(pszText, "%s (%s)", GlobalText[3339], GlobalText[241]);
	}
	else
	{
		g_pRenderText->SetTextColor(0xD8u, 0xD8u, 0xD8u, 0xFFu);
		unicode::_sprintf(pszText, "%s (%s)", GlobalText[3339], GlobalText[240]);
	}

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->RenderText(PosX, PosY + 11, pszText, 190, 0, RT3_SORT_CENTER);

}

void SEASON3B::CNewUIStorageExpansion::RenderButtons()
{
	m_BtnExit.Render();
}

void SEASON3B::CNewUIStorageExpansion::OpenningProcess()
{
	g_pStorageInventory->SetExpansionOpenState();
}

void SEASON3B::CNewUIStorageExpansion::ClosingProcess()
{
	g_pStorageInventory->SetExpansionCloseState();
}

bool SEASON3B::CNewUIStorageExpansion::CheckExpansionInventory()
{
	if (IsVisible())
		return CheckMouseIn(m_Pos.x, m_Pos.y, 190, 429) != false;
	else
		return false;
}

bool SEASON3B::CNewUIStorageExpansion::BtnProcess()
{
	int PosX = GetPos()->x;
	int PosY = GetPos()->y;

	if (SEASON3B::IsRelease(VK_LBUTTON) && SEASON3B::CheckMouseIn(PosX + 169, PosY + 7, 13, 12))
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_STORAGE_EXTENSION);
		return true;
	}
	if (m_BtnExit.UpdateMouseEvent())
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_STORAGE_EXTENSION);
		return true;
	}

	return false;
}

void SEASON3B::CNewUIStorageExpansion::SetBtnInfo()
{
	unicode::t_char pszText[100];
	unicode::_sprintf(pszText, GlobalText[927], "H");
	m_BtnExit.ChangeToolTipText(pszText, true);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		SetButtonInterface(&m_BtnExit, m_Pos.x, m_Pos.y, 1);
	}
	else
	{
		m_BtnExit.ChangeButtonImgState(true, IMAGE_STORAGE_EXIT_BTN, false);
		m_BtnExit.ChangeButtonInfo(m_Pos.x + 13, m_Pos.y + 391, 36, 29);
	}
}

bool SEASON3B::CNewUIStorageExpansion::IsExpansionStorage()
{
	if (GMProtect->IsBaulExt() == true)
	{
		return m_bLock;
	}
	else
	{
		return false;
	}
}

void SEASON3B::CNewUIStorageExpansion::SetExpansionStorage(bool Success)
{
	if (GMProtect->IsBaulExt() == true)
	{
		if (Success)
		{
			g_pStorageInventory->UnLockBtnExpansion();
		}
		else
		{
			g_pStorageInventory->LockBtnExpansion();
		}

		m_bLock = Success;
	}
}

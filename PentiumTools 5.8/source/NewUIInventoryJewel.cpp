#include "stdafx.h"
#include "WSclient.h"
#include "UIControls.h"
#include "ZzzInventory.h"
#include "wsclientinline.h"
#include "NewUIInventoryJewel.h"

SEASON3B::CNewUIInventoryJewel::CNewUIInventoryJewel()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;

	m_dwCurIndex = -1;
	m_dwSelIndex = -1;
	m_nSelPage = 0;
	m_nMaxPage = 0;
	m_nDescLine = 0;

	memset(m_aszJobDesc, 0, sizeof(m_aszJobDesc));


	m_bItems.push_back(WareHoly(0, 6159, 250000));
	m_bItems.push_back(WareHoly(1, 7181, 250000));
	m_bItems.push_back(WareHoly(2, 7182, 250000));
	m_bItems.push_back(WareHoly(3, 7184, 250000));
	m_bItems.push_back(WareHoly(4, 7190, 250000));
	m_bItems.push_back(WareHoly(5, 7199, 250000));
	m_bItems.push_back(WareHoly(6, 7210, 250000));
}

SEASON3B::CNewUIInventoryJewel::~CNewUIInventoryJewel()
{
	m_bItems.clear();
}

bool SEASON3B::CNewUIInventoryJewel::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (pNewUIMng)
	{
		m_pNewUIMng = pNewUIMng;

		m_pNewUIMng->AddUIObj(INTERFACE_INVENTORY_JEWEL, this);

		SetPos(x, y);

		LoadBitmap("Interface\\InGameShop\\IGS_Storage_Page.tga", CNewUIInGameShop::IMAGE_IGS_STORAGE_PAGE, GL_LINEAR);

		InitButtons();

		return true;
	}
	return false;
}

void SEASON3B::CNewUIInventoryJewel::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		m_Button1.SetPos(m_Pos.x + 25.00, m_Pos.y + 395.0);
		m_Button2.SetPos(m_Pos.x + 109.0, m_Pos.y + 395.0);
	}
	else
	{
		m_Button1.SetPos(m_Pos.x + 25.00, m_Pos.y + 390.0);
		m_Button2.SetPos(m_Pos.x + 112.0, m_Pos.y + 390.0);
	}
}

void SEASON3B::CNewUIInventoryJewel::InitButtons()
{
	m_Button1.ChangeText(GlobalText[235]);
	m_Button2.ChangeText(GlobalText[236]);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		SetButtonImagen(&m_Button1, Bitmap_text_button1, Bitmap_text_button2);
		SetButtonImagen(&m_Button2, Bitmap_text_button1, Bitmap_text_button2);
		m_Button1.ChangeButtonInfo(m_Pos.x + 25.00, m_Pos.y + 395.0, 56.0, 24.0);
		m_Button2.ChangeButtonInfo(m_Pos.x + 109.0, m_Pos.y + 395.0, 56.0, 24.0);
	}
	else
	{
		m_Button1.ChangeButtonImgState(true, CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL, true);
		m_Button2.ChangeButtonImgState(true, CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL, true);

		m_Button1.ChangeButtonInfo(m_Pos.x + 25.00, m_Pos.y + 390.0, 53.0, 23.0);
		m_Button2.ChangeButtonInfo(m_Pos.x + 112.0, m_Pos.y + 390.0, 53.0, 23.0);
	}
}

bool SEASON3B::CNewUIInventoryJewel::Render()
{
	EnableAlphaTest();

	glColor4f(1.0, 1.0, 1.0, 1.0);

	RenderFrame();

	RenderInter();

	RenderTexts();

	RenderButtons();

	RenderHoly();

	DisableAlphaBlend();

	return true;
}

bool SEASON3B::CNewUIInventoryJewel::Update()
{
	return true;
}

bool SEASON3B::CNewUIInventoryJewel::UpdateMouseEvent()
{
	m_dwCurIndex = -1;

	float RenderFrameX = m_Pos.x + 25.f;
	float RenderFrameY = m_Pos.y + 124.f;

	int index = m_nSelPage * 7;

	for (size_t i = index; i < m_bItems.size() && i < index + 7; i++)
	{
		if (SEASON3B::CheckMouseIn(RenderFrameX, RenderFrameY, 140.f, 30.f))
		{
			m_dwCurIndex = i;
			if (SEASON3B::IsRelease(VK_LBUTTON))
			{
				m_dwSelIndex = i;
				return false;
			}
		}
		RenderFrameY += 32.f;
	}


	if (m_Button1.UpdateMouseEvent())
	{
		if (m_dwSelIndex != -1)
		{
			SendRequestAddJewelOfInventory(m_dwSelIndex, 0, 0);
		}
		return false;
	}
	if (m_Button2.UpdateMouseEvent())
	{
		return false;
	}
	return true;
}

bool SEASON3B::CNewUIInventoryJewel::UpdateKeyEvent()
{
	return true;
}

float SEASON3B::CNewUIInventoryJewel::GetLayerDepth()
{
	return 4.25;
}

bool SEASON3B::CNewUIInventoryJewel::CheckExpansionInventory()
{
	if (IsVisible())
		return CheckMouseIn(m_Pos.x, m_Pos.y, 190, 429) != false;
	else
		return false;
}

void SEASON3B::CNewUIInventoryJewel::OpenningProcess()
{
	m_nSelPage = 0;
	m_dwSelIndex = -1;
	m_dwCurIndex = -1;

	if (m_nDescLine == 0)
	{
		m_nDescLine = ::SeparateTextIntoLines(GlobalText[2359], m_aszJobDesc[0], 4, 50);
	}
}

void SEASON3B::CNewUIInventoryJewel::ClosingProcess()
{
	m_nSelPage = 0;
	m_dwSelIndex = -1;
	m_dwCurIndex = -1;
}

void SEASON3B::CNewUIInventoryJewel::RemoveData()
{
	m_bItems.clear();
}

void SEASON3B::CNewUIInventoryJewel::setData(BYTE Index, short ItemIndex, __int64 count)
{
	if (ItemIndex >= 6144 && ItemIndex < MAX_ITEM)
	{
		m_bItems.push_back(WareHoly(Index, ItemIndex, count));
	}

	m_nMaxPage = (m_bItems.size() / 7);
}

void SEASON3B::CNewUIInventoryJewel::RenderFrame()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		RenderInventoryInterface(m_Pos.x, m_Pos.y, 0);
	}
	else
	{
		RenderImage(IMAGE_INVENTORY_BACK, this->m_Pos.x, this->m_Pos.y, 190.0, 429.0);

		RenderImage(IMAGE_INVENTORY_BACK_TOP2, this->m_Pos.x, this->m_Pos.y, 190.0, 64.0);

		RenderImage(IMAGE_INVENTORY_BACK_LEFT, (float)this->m_Pos.x, (float)(this->m_Pos.y + 64), 21.0, 320.0);

		RenderImage(IMAGE_INVENTORY_BACK_RIGHT, (float)(this->m_Pos.x + 169), (float)(this->m_Pos.y + 64), 21.0, 320.0);

		RenderImage(IMAGE_INVENTORY_BACK_BOTTOM, (float)this->m_Pos.x, (float)(this->m_Pos.y + 384), 190.0, 45.0);
	}
}

void SEASON3B::CNewUIInventoryJewel::RenderTexts()
{
	g_pRenderText->SetFont(g_hFontBold);

	g_pRenderText->SetBgColor(0, 0, 0, 0);

	g_pRenderText->SetTextColor(255, 255, 255, 0xFFu);

	char strText[100];
	sprintf_s(strText, "%s %s", GlobalText[102], GlobalText[223]);

	g_pRenderText->RenderText(m_Pos.x, m_Pos.y + 15, strText, 190, 0, 3, 0);

	float RenderFrameX = m_Pos.x + 25.f;
	float RenderFrameY = m_Pos.y + 60.f;

	for (int i = 0; i < m_nDescLine; i++)
	{
		g_pRenderText->RenderText(RenderFrameX - 5, RenderFrameY, m_aszJobDesc[i]);
		RenderFrameY += 10;
	}

	RenderFrameY = m_Pos.y + 124.f;

	int index = m_nSelPage * 7;

	for (size_t i = index; i < m_bItems.size() && i < index + 7; i++)
	{
		WareHoly* item = &m_bItems[i];

		Script_Item* item_info = GMItemMng->find(item->GetIndex());

		sprintf_s(strText, "x %lld", item->GetValue());

		if(item->GetValue() <= 0)
			g_pRenderText->SetTextColor(180, 180, 180, 0xFFu);
		else
			g_pRenderText->SetTextColor(CLRDW_GOLD);

		if (item_info->Name[0] != '\0')
		{
			g_pRenderText->RenderText(RenderFrameX + 30.f, RenderFrameY + 5.f, item_info->Name);
			g_pRenderText->RenderText(RenderFrameX + 40.f, RenderFrameY + 17.f, strText);
		}
		else
		{
			g_pRenderText->RenderText(RenderFrameX + 30.f, RenderFrameY + 5.f, "Jewel Unknown");

			g_pRenderText->RenderText(RenderFrameX + 40.f, RenderFrameY + 17.f, strText);
		}
		RenderFrameY += 32.f;
	}
}

void SEASON3B::CNewUIInventoryJewel::RenderInter()
{
	EnableAlphaTest(true);
	glColor4f(0.0, 0.0, 0.0, 0.7);

	float RenderFrameX = m_Pos.x + 25.f;
	float RenderFrameY = m_Pos.y + 124.f;

	int index = m_nSelPage * 7;

	for (size_t i = index; i < m_bItems.size() && i < index + 7; i++)
	{
		RenderColor(RenderFrameX, RenderFrameY, 140.0, 30.0);
		RenderFrameY += 32.f;
	}
	EndRenderColor();
	
	RenderFrameY = m_Pos.y + 124.f;
	if (m_dwCurIndex != -1)
	{
		FrameTarget(RenderFrameX, RenderFrameY + ((m_dwCurIndex - index) * 32.0), 140.0, 30.0, RGBA(98, 223, 31, 0xff));
	}
	if (m_dwSelIndex != -1 && m_dwSelIndex != m_dwCurIndex)
	{
		FrameTarget(RenderFrameX, RenderFrameY + ((m_dwSelIndex - index) * 32.0), 140.0, 30.0, RGBA(98, 223, 31, 0xff));
	}
}



void SEASON3B::CNewUIInventoryJewel::RenderButtons()
{
	float RenderFrameX = m_Pos.x + 25.0f;
	float RenderFrameY = m_Pos.y + 360.f;


	RenderImageF(CNewUIInGameShop::IMAGE_IGS_STORAGE_PAGE, m_Pos.x + 66, RenderFrameY-2, 58.0, 22.0, 0.0, 0.0, 80.0, 30.0);

	RenderImage(CNewUIQuestProgress::IMAGE_QP_BTN_L, RenderFrameX, RenderFrameY, 17.0, 18.0, 0.0, 0.0);

	RenderImage(CNewUIQuestProgress::IMAGE_QP_BTN_R, RenderFrameX + 123.0, RenderFrameY, 17.0, 18.0, 0.0, 0.0);

	m_Button1.Render();

	m_Button2.Render();
}

void SEASON3B::CNewUIInventoryJewel::RenderHoly()
{
	SEASON3B::Open3D();

	float RenderFrameX = m_Pos.x + 25.f;
	float RenderFrameY = m_Pos.y + 124.f;

	int index = m_nSelPage * 7;

	for (size_t i = index; i < m_bItems.size() && i < index + 7; i++)
	{
		WareHoly* item = &m_bItems[i];

		Render2Item3D(RenderFrameX + 5.f, RenderFrameY + 5.f, 20.0, 20.0, item->GetIndex(), 0, 0, 0, false);

		RenderFrameY += 32.f;
	}

	SEASON3B::Exit3D();
}

void SEASON3B::CNewUIInventoryJewel::FrameTarget(float iPos_x, float iPos_y, float width, float height, DWORD color)
{
	EnableAlphaTest(true);

	BYTE red = GetRed(color);
	BYTE green = GetGreen(color);
	BYTE blue = GetBlue(color);

	glColor4ub(red, green, blue, 30);
	RenderColor(iPos_x, iPos_y, width, height, 0.0, 0);

	glColor4ub(red, green, blue, 255);
	RenderColor(iPos_x, iPos_y, width, 1.f, 0.0, 0);
	RenderColor(iPos_x, iPos_y + height, width, 1.f, 0.0, 0);

	RenderColor(iPos_x, iPos_y + 1.f, 1.f, height - 1.f, 0.0, 0);
	RenderColor(iPos_x + width - 1.f, iPos_y + 1.f, 1.f, height - 1.f, 0.0, 0);

	EndRenderColor();
}
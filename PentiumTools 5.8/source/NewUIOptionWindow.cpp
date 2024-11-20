// NewUIOptionWindow.cpp: implementation of the CNewUIOptionWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NewUIOptionWindow.h"
#include "NewUISystem.h"
#include "ZzzTexture.h"
#include "DSPlaySound.h"

#include "Input.h"
#include "ZzzOpenData.h"
#include "ZzzInterface.h"

extern void SetResolution();

using namespace SEASON3B;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SEASON3B::CNewUIOptionWindow::CNewUIOptionWindow()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;

	m_bAutoAttack = true;
	m_bWhisperSound = false;
	m_bSlideHelp = true;
	m_iVolumeLevel = 0;
	m_iRenderLevel = 4;

	m_RenderEffect = true;
	m_RenderEquipment = true;
	m_RenderTerrain = true;
	m_RenderObjects = true;
}

SEASON3B::CNewUIOptionWindow::~CNewUIOptionWindow()
{
	Release();
}

bool SEASON3B::CNewUIOptionWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_OPTION, this);

	SetPos(x, y);

	LoadImages();

	SetButtonInfo();

	Show(false);

	return true;
}

void SEASON3B::CNewUIOptionWindow::SetButtonInfo()
{
	m_BtnClose.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnClose.ChangeButtonImgState(true, IMAGE_OPTION_BTN_CLOSE, true);
	m_BtnClose.ChangeButtonInfo(m_Pos.x + 68, m_Pos.y + 209, 54, 30);
	m_BtnClose.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnClose.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
}

void SEASON3B::CNewUIOptionWindow::Release()
{
	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIOptionWindow::SetPos(int x, int y)
{
	float RenderFrameX = PositionX_The_Mid(240.0);
	float RenderFrameY = PositionY_In_The_Mid(0) + y;

	m_Pos.x = RenderFrameX;
	m_Pos.y = RenderFrameY;
}

bool SEASON3B::CNewUIOptionWindow::UpdateMouseEvent()
{
	float RenderFrameX = m_Pos.x;
	float RenderFrameY = m_Pos.y;

	if (m_BtnClose.UpdateMouseEvent() == true)
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_OPTION);
		return false;
	}

	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 210.0, RenderFrameY + 40.0, 15, 15))
	{
		m_bAutoAttack = !m_bAutoAttack;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 210.0, RenderFrameY + 58.0, 15, 15))
	{
		m_bWhisperSound = !m_bWhisperSound;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 210.0, RenderFrameY + 76.0, 15, 15))
	{
		m_bSlideHelp = !m_bSlideHelp;
	}

	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 210.0, RenderFrameY + 94.0, 15, 15))
	{
		m_RenderEffect = !m_RenderEffect;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 210.0, RenderFrameY + 112.0, 15, 15))
	{
		m_RenderEquipment = !m_RenderEquipment;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 210.0, RenderFrameY + 130.0, 15, 15))
	{
		m_RenderTerrain = !m_RenderTerrain;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 210.0, RenderFrameY + 148.0, 15, 15))
	{
		m_RenderObjects = !m_RenderObjects;
	}

#ifdef SYSTEM_OPTION_RENDER
	if (SEASON3B::IsRelease(VK_LBUTTON) && CheckMouseIn(RenderFrameX + 10.0, RenderFrameY + 40.0, 56.0, 216.0))
	{
		float RenderButtonY = RenderFrameY + 40;

		for (size_t i = 0; i < 9; i++)
		{
			if (CheckMouseIn(RenderFrameX + 10, RenderButtonY, 56.0, 20.0))
			{
				this->change_resolution( i + 1);
				return false;
			}
			RenderButtonY += 24;
		}
	}
#endif // SYSTEM_OPTION_RENDER

	if (CheckMouseIn(RenderFrameX + 93 - 8, RenderFrameY + 185, 124 + 8, 16))
	{
		int iOldValue = m_iVolumeLevel;
		if (MouseWheel > 0)
		{
			MouseWheel = 0;
			m_iVolumeLevel++;
			if (m_iVolumeLevel > 10)
			{
				m_iVolumeLevel = 10;
			}
		}
		else if (MouseWheel < 0)
		{
			MouseWheel = 0;
			m_iVolumeLevel--;
			if (m_iVolumeLevel < 0)
			{
				m_iVolumeLevel = 0;
			}
		}
		if (SEASON3B::IsRepeat(VK_LBUTTON))
		{
			int x = MouseX - (RenderFrameX + 93);
			if (x < 0)
			{
				m_iVolumeLevel = 0;
			}
			else
			{
				float fValue = (10.f * x) / 124.f;
				m_iVolumeLevel = (int)fValue + 1;
			}
		}

		if (iOldValue != m_iVolumeLevel)
		{
			SetEffectVolumeLevel(m_iVolumeLevel);
		}
	}
	if (CheckMouseIn(RenderFrameX + 85, RenderFrameY + 220, 141, 29))
	{
		if (SEASON3B::IsRepeat(VK_LBUTTON))
		{
			int x = MouseX - (RenderFrameX + 85);
			float fValue = (5.f * x) / 141.f;
			m_iRenderLevel = (int)fValue;
		}
	}

	return !CheckMouseIn(RenderFrameX, RenderFrameY, 240.0, 270.0);
}

bool SEASON3B::CNewUIOptionWindow::UpdateKeyEvent()
{
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_OPTION) == true)
	{
		if (SEASON3B::IsPress(VK_ESCAPE) == true)
		{
			g_pNewUISystem->Hide(SEASON3B::INTERFACE_OPTION);
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	return true;
}

bool SEASON3B::CNewUIOptionWindow::Update()
{
	return true;
}

bool SEASON3B::CNewUIOptionWindow::Render()
{
	EnableAlphaTest();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RenderFrame();

	RenderContents();

	RenderButtons();

	DisableAlphaBlend();

	return true;
}

float SEASON3B::CNewUIOptionWindow::GetLayerDepth()	//. 10.5f
{
	return 10.5f;
}

float SEASON3B::CNewUIOptionWindow::GetKeyEventOrder()	// 10.f;
{
	return 10.0f;
}

void SEASON3B::CNewUIOptionWindow::OpenningProcess()
{

}

void SEASON3B::CNewUIOptionWindow::ClosingProcess()
{
}

void SEASON3B::CNewUIOptionWindow::LoadImages()
{
	LoadBitmap("Interface\\newui_button_close.tga", IMAGE_OPTION_BTN_CLOSE, GL_LINEAR);
	LoadBitmap("Interface\\newui_msgbox_back.jpg", IMAGE_OPTION_FRAME_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back03.tga", IMAGE_OPTION_FRAME_DOWN, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_top.tga", IMAGE_OPTION_FRAME_UP, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_back06(L).tga", IMAGE_OPTION_FRAME_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_back06(R).tga", IMAGE_OPTION_FRAME_RIGHT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_line.jpg", IMAGE_OPTION_LINE, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_point.tga", IMAGE_OPTION_POINT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_check.tga", IMAGE_OPTION_BTN_CHECK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_effect03.tga", IMAGE_OPTION_EFFECT_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_effect04.tga", IMAGE_OPTION_EFFECT_COLOR, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_volume01.tga", IMAGE_OPTION_VOLUME_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_volume02.tga", IMAGE_OPTION_VOLUME_COLOR, GL_LINEAR);

	LoadBitmap("Interface\\HUD\\checkbox.tga", IMAGE_CHECK_LIVE);
	LoadBitmap("Interface\\HUD\\uncheckbox.tga", IMAGE_UNCHECK_LIVE);
}

void SEASON3B::CNewUIOptionWindow::UnloadImages()
{
	DeleteBitmap(IMAGE_OPTION_BTN_CLOSE);
	DeleteBitmap(IMAGE_OPTION_FRAME_BACK);
	DeleteBitmap(IMAGE_OPTION_FRAME_DOWN);
	DeleteBitmap(IMAGE_OPTION_FRAME_UP);
	DeleteBitmap(IMAGE_OPTION_FRAME_LEFT);
	DeleteBitmap(IMAGE_OPTION_FRAME_RIGHT);
	DeleteBitmap(IMAGE_OPTION_LINE);
	DeleteBitmap(IMAGE_OPTION_POINT);
	DeleteBitmap(IMAGE_OPTION_BTN_CHECK);
	DeleteBitmap(IMAGE_OPTION_EFFECT_BACK);
	DeleteBitmap(IMAGE_OPTION_EFFECT_COLOR);
	DeleteBitmap(IMAGE_OPTION_VOLUME_BACK);
	DeleteBitmap(IMAGE_OPTION_VOLUME_COLOR);
}

void SEASON3B::CNewUIOptionWindow::RenderFrame()
{
	float RenderFrameX = m_Pos.x;
	float RenderFrameY = m_Pos.y;

	glColor4f(0.0, 0.0, 0.0, 0.8);

	RenderColor(RenderFrameX, RenderFrameY, 240.0, 270.0, 0.0, 0);

	glColor4f(0.0 / 255.0, 95.0 / 255.0, 232.0 / 255.0, 0.6);
	RenderColor(RenderFrameX + 10, RenderFrameY + 10, 220.0, 15.0, 0.0, 0);


	float RenderButtonY = RenderFrameY + 40;

	for (size_t i = 0; i < 9; i++)
	{
		RenderColor(RenderFrameX + 10, RenderButtonY, 56.0, 20.0, 0.0, 0);

		RenderButtonY += 24;
	}

	EndRenderColor();
	//RenderImage(IMAGE_OPTION_FRAME_BACK, x, y, 190.f, 249.f);
	//RenderImage(IMAGE_OPTION_FRAME_UP, x, y, 190.f, 64.f);
	//
	//y += 64.f;
	//for (int i = 0; i < 14; ++i)
	//{
	//	RenderImage(IMAGE_OPTION_FRAME_LEFT, x, y, 21.f, 10.f);
	//	RenderImage(IMAGE_OPTION_FRAME_RIGHT, x + 190 - 21, y, 21.f, 10.f);
	//	y += 10.f;
	//}
	//RenderImage(IMAGE_OPTION_FRAME_DOWN, x, y, 190.f, 45.f);
	//
	//y = m_Pos.y + 60.f;
	//RenderImage(IMAGE_OPTION_LINE, x + 18, y, 154.f, 2.f);
	//y += 22.f;
	//RenderImage(IMAGE_OPTION_LINE, x + 18, y, 154.f, 2.f);
	//y += 40.f;
	//RenderImage(IMAGE_OPTION_LINE, x + 18, y, 154.f, 2.f);
	//y += 22.f;
	//RenderImage(IMAGE_OPTION_LINE, x + 18, y, 154.f, 2.f);
}

void SEASON3B::CNewUIOptionWindow::RenderContents()
{
	float RenderFrameX = m_Pos.x;
	float RenderFrameY = m_Pos.y;

	g_pRenderText->SetBgColor(0);

	g_pRenderText->SetFont(g_hFontBold);

	g_pRenderText->SetTextColor(200, 200, 200, 255);

	g_pRenderText->RenderText(RenderFrameX + 10, RenderFrameY + 10, GlobalText[3450], 220, 15, 3);

#ifdef SYSTEM_OPTION_RENDER
	float RenderButtonY = RenderFrameY + 40;

	char* resolution[] = {"800x600","1024x768", "1280x1024", "1360x768", "1440x900", "1600x900", "1680x1050", "1920x1080", "3840x2160", };

	for (size_t i = 0; i < 9; i++)
	{
		g_pRenderText->RenderText(RenderFrameX + 10, RenderButtonY, resolution[i], 56.0, 20.0, 3);

		RenderButtonY += 24;
	}
#endif // SYSTEM_OPTION_RENDER

	g_pRenderText->SetFont(g_hFont);

	g_pRenderText->SetTextColor(255, 255, 255, 255);

	g_pRenderText->SetBgColor(0);

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 40, GlobalText[386], 0, 15); //-- Automatic Attack

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 58, GlobalText[387], 0, 15); //-- Beep sound for whispering

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 76, GlobalText[919], 0, 15); //-- Slide Help

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 94, "Render Effect", 0, 15); //-- Slide Help

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 112, "Render Equipment", 0, 15); //-- Slide Help

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 130, "Render Terrain", 0, 15); //-- Slide Help

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 148, "Render Objects", 0, 15); //-- Slide Help

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 175, GlobalText[389]); //-- Volume

	g_pRenderText->RenderText(RenderFrameX + 85, RenderFrameY + 210, GlobalText[1840]); //-- +Effect limitation

	/*
	float x, y;
	x = m_Pos.x + 20.f;
	y = m_Pos.y + 46.f;
	RenderImage(IMAGE_OPTION_POINT, x, y, 10.f, 10.f);
	y += 22.f;
	RenderImage(IMAGE_OPTION_POINT, x, y, 10.f, 10.f);
	y += 22.f;
	RenderImage(IMAGE_OPTION_POINT, x, y, 10.f, 10.f);
	y += 40.f;
	RenderImage(IMAGE_OPTION_POINT, x, y, 10.f, 10.f);
	y += 22.f;
	RenderImage(IMAGE_OPTION_POINT, x, y, 10.f, 10.f);

	g_pRenderText->SetFont(g_hFont);

	g_pRenderText->SetTextColor(255, 255, 255, 255);

	g_pRenderText->SetBgColor(0);

	g_pRenderText->RenderText(m_Pos.x + 40, m_Pos.y + 48, GlobalText[386]);

	g_pRenderText->RenderText(m_Pos.x + 40, m_Pos.y + 70, GlobalText[387]);

	g_pRenderText->RenderText(m_Pos.x + 40, m_Pos.y + 92, GlobalText[389]);

	g_pRenderText->RenderText(m_Pos.x + 40, m_Pos.y + 132, GlobalText[919]);

	g_pRenderText->RenderText(m_Pos.x + 40, m_Pos.y + 154, GlobalText[1840]);*/
}

void SEASON3B::CNewUIOptionWindow::RenderChecked(float RenderFrameX, float RenderFrameY, bool bEnable)
{
	if(bEnable)
		RenderImage(IMAGE_CHECK_LIVE, RenderFrameX, RenderFrameY, 15.0, 15.0, 0.0, 0.0, 0.75, 0.75);
	else
		RenderImage(IMAGE_UNCHECK_LIVE, RenderFrameX, RenderFrameY, 15.0, 15.0, 0.0, 0.0, 0.75, 0.75);
}

void SEASON3B::CNewUIOptionWindow::RenderButtons()
{
	float RenderFrameX = m_Pos.x;
	float RenderFrameY = m_Pos.y;

	RenderChecked(RenderFrameX + 210.0, RenderFrameY + 40.0, m_bAutoAttack);

	RenderChecked(RenderFrameX + 210.0, RenderFrameY + 58.0, m_bWhisperSound);

	RenderChecked(RenderFrameX + 210.0, RenderFrameY + 76.0, m_bSlideHelp);

	RenderChecked(RenderFrameX + 210.0, RenderFrameY + 94.0, m_RenderEffect);

	RenderChecked(RenderFrameX + 210.0, RenderFrameY + 112.0, m_RenderEquipment);

	RenderChecked(RenderFrameX + 210.0, RenderFrameY + 130.0, m_RenderTerrain);

	RenderChecked(RenderFrameX + 210.0, RenderFrameY + 148.0, m_RenderObjects);


	RenderImage(IMAGE_OPTION_VOLUME_BACK, RenderFrameX + 93, RenderFrameY + 185, 124.f, 16.f);

	if (m_iVolumeLevel > 0)
	{
		RenderImage(IMAGE_OPTION_VOLUME_COLOR, RenderFrameX + 93, RenderFrameY + 185, 124.f * 0.1f * (m_iVolumeLevel), 16.f);
	}

	RenderImage(IMAGE_OPTION_EFFECT_BACK, RenderFrameX + 85, RenderFrameY + 220, 141.f, 29.f);

	if (m_iRenderLevel >= 0)
	{
		RenderImage(IMAGE_OPTION_EFFECT_COLOR, RenderFrameX + 85, RenderFrameY + 220, 141.f * 0.2f * (m_iRenderLevel + 1), 29.f);
	}

	/*m_BtnClose.Render();

	RenderImage(IMAGE_OPTION_BTN_CHECK, m_Pos.x + 150, m_Pos.y + 43, 15, 15, 0, (m_bAutoAttack) ? 0.0 : 15.0);

	RenderImage(IMAGE_OPTION_BTN_CHECK, m_Pos.x + 150, m_Pos.y + 65, 15, 15, 0, (m_bWhisperSound) ? 0.0 : 15.0);

	RenderImage(IMAGE_OPTION_BTN_CHECK, m_Pos.x + 150, m_Pos.y + 127, 15, 15, 0, (m_bSlideHelp) ? 0.0 : 15.0);

	RenderImage(IMAGE_OPTION_VOLUME_BACK, m_Pos.x + 33, m_Pos.y + 104, 124.f, 16.f);

	if (m_iVolumeLevel > 0)
	{
		RenderImage(IMAGE_OPTION_VOLUME_COLOR, m_Pos.x + 33, m_Pos.y + 104, 124.f * 0.1f * (m_iVolumeLevel), 16.f);
	}

	RenderImage(IMAGE_OPTION_EFFECT_BACK, m_Pos.x + 25, m_Pos.y + 168, 141.f, 29.f);

	if (m_iRenderLevel >= 0)
	{
		RenderImage(IMAGE_OPTION_EFFECT_COLOR, m_Pos.x + 25, m_Pos.y + 168, 141.f * 0.2f * (m_iRenderLevel + 1), 29.f);
	}*/
}

void SEASON3B::CNewUIOptionWindow::SetAutoAttack(bool bAuto)
{
	m_bAutoAttack = bAuto;
}

bool SEASON3B::CNewUIOptionWindow::IsAutoAttack()
{
	return m_bAutoAttack;
}

void SEASON3B::CNewUIOptionWindow::SetWhisperSound(bool bSound)
{
	m_bWhisperSound = bSound;
}

bool SEASON3B::CNewUIOptionWindow::IsWhisperSound()
{
	return m_bWhisperSound;
}

void SEASON3B::CNewUIOptionWindow::SetSlideHelp(bool bHelp)
{
	m_bSlideHelp = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::IsSlideHelp()
{
	return m_bSlideHelp;
}

void SEASON3B::CNewUIOptionWindow::SetVolumeLevel(int iVolume)
{
	m_iVolumeLevel = iVolume;
}

int SEASON3B::CNewUIOptionWindow::GetVolumeLevel()
{
	return m_iVolumeLevel;
}

void SEASON3B::CNewUIOptionWindow::SetRenderLevel(int iRender)
{
	m_iRenderLevel = iRender;
}

int SEASON3B::CNewUIOptionWindow::GetRenderLevel()
{
	return m_iRenderLevel;
}

void SEASON3B::CNewUIOptionWindow::SetRemoveEffect(bool bHelp)
{
	m_RenderEffect = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRemoveEffect()
{
	return m_RenderEffect;
}

void SEASON3B::CNewUIOptionWindow::SetRemoveEquipment(bool bHelp)
{
	m_RenderEquipment = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRemoveEquipment()
{
	return m_RenderEquipment;
}

void SEASON3B::CNewUIOptionWindow::SetRemoveTerrain(bool bHelp)
{
	m_RenderTerrain = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRemoveTerrain()
{
	return m_RenderTerrain;
}

void SEASON3B::CNewUIOptionWindow::SetRemoveObjects(bool bHelp)
{
	m_RenderObjects = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRemoveObjects()
{
	return m_RenderObjects;
}

#ifdef SYSTEM_OPTION_RENDER
void SEASON3B::CNewUIOptionWindow::change_resolution(int index)
{
	if (m_Resolution != index)
	{
		if (FNREG_SZ("Resolution", REG_DWORD, (const BYTE*)&index, sizeof(index)))
		{
			m_Resolution = index;

			double backup_width = WinFrameX;

			double backup_hight = WinFrameY;

			SetResolution();

			ChangeWindowResolution(g_hWnd, WindowWidth, WindowHeight);

			OpenFont();

			ClearInput(TRUE);

			CInput& rInput = CInput::Instance();

			rInput.Create(g_hWnd, WindowWidth, WindowHeight);

			g_pMoveCommandWindow->SetPos(1, 1);

			this->SetPos(0, 70);

			g_pNewUISystem->RenderFrameUpdate(backup_width, backup_hight);

			g_pNewUI3DRenderMng->Reload3DEffectObject(WindowWidth, WindowHeight);
		}
	}
}

bool SEASON3B::CNewUIOptionWindow::FNREG_SZ(char* Name, DWORD dwType, const BYTE* Data, DWORD Size)
{
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, Name, 0, dwType, Data, Size);
		RegCloseKey(hKey);
		return true;
	}
	return false;
}
#endif // SYSTEM_OPTION_RENDER

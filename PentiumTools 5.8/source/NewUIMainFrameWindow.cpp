//////////////////////////////////////////////////////////////////////
// NewUIMainFrameWindow.cpp: implementation of the CNewUIMainFrameWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CGMProtect.h"
#include "NewUIMainFrameWindow.h"	// self
#include "NewUIOptionWindow.h"
#include "NewUISystem.h"
#include "UIBaseDef.h"
#include "DSPlaySound.h"
#include "ZzzInfomation.h"
#include "ZzzBMD.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "wsclientinline.h"
#include "CSItemOption.h"
#include "CSChaosCastle.h"
#include "MapManager.h"
#include "CharacterManager.h"
#include "SkillManager.h"
#include "GMDoppelGanger1.h"
#include "GMDoppelGanger2.h"
#include "GMDoppelGanger3.h"
#include "GMDoppelGanger4.h"
#include "./Time/CTimCheck.h"
#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
#include "MonkSystem.h"
#endif //PBG_ADD_NEWCHAR_MONK_SKILL

#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
#include "GameShop/InGameShopSystem.h"
#endif //PBG_ADD_INGAMESHOP_UI_MAINFRAME
#if MAIN_UPDATE == 303
#include "CAIController.h"
#endif // MAIN_UPDATE == 303


extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
extern int  MouseUpdateTime;
extern int  MouseUpdateTimeMax;
extern int SelectedCharacter;
extern int Attacking;


SEASON3B::CNewUIMainFrameWindow::CNewUIMainFrameWindow()
{
	m_pNewUIMng = NULL;
	m_pNewUI3DRenderMng = NULL;

	m_bExpEffect = false;
	m_dwExpEffectTime = 0;
	m_dwPreExp = 0;
	m_dwGetExp = 0;
	m_loPreExp = 0;
	m_loGetExp = 0;
	m_bButtonBlink = false;
	PosX = 0.0;
	PosY = 0.0;

	this->logo_tick = std::chrono::steady_clock::now();
}

SEASON3B::CNewUIMainFrameWindow::~CNewUIMainFrameWindow()
{
	Release();
}

void SEASON3B::CNewUIMainFrameWindow::LoadImages()
{
#if MAIN_UPDATE == 303
	LoadBitmap("Interface\\New_ui_interface.tga", IMAGE_MENU_1, GL_LINEAR);
	LoadBitmap("Interface\\Menu_Blue.tga", IMAGE_GAUGE_BLUE, GL_LINEAR);
	LoadBitmap("Interface\\Menu_Green.tga", IMAGE_GAUGE_GREEN, GL_LINEAR);
	LoadBitmap("Interface\\Menu_Red.tga", IMAGE_GAUGE_RED, GL_LINEAR);
	LoadBitmap("Interface\\Menu03_new_AG.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
	LoadBitmap("Interface\\menu01_new2_SD.jpg", IMAGE_GAUGE_SD, GL_LINEAR);
	LoadBitmap("Interface\\Mu_helper_buttons.tga", IMAGE_MENU_2, GL_LINEAR);

	LoadBitmap("Interface\\Menu_party.jpg", IMAGE_MENU_BTN_PARTY, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Interface\\Menu_character.jpg", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Interface\\Menu_inventory.jpg", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);

	LoadBitmap("Interface\\HUD\\Look-1\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\Look-1\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
#else
	if (gmProtect->LookAndFeel == 1)
	{
		LoadBitmap("Interface\\HUD\\Look-1\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu01_new.jpg", IMAGE_MENU_1, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu02.jpg", IMAGE_MENU_2, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu03_new.jpg", IMAGE_MENU_3, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu_blue.jpg", IMAGE_GAUGE_BLUE, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu_green.jpg", IMAGE_GAUGE_GREEN, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu_red.jpg", IMAGE_GAUGE_RED, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu03_new_ag.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu_character.jpg", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu_inventory.jpg", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-1\\Guild.jpg", IMAGE_MENU_BTN_GUILD, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-1\\Menu_party.jpg", IMAGE_MENU_BTN_PARTY, GL_LINEAR, GL_CLAMP_TO_EDGE);

		if (WindowWidth > 800 && gmProtect->ScreenType != 0)
		{
			LoadBitmap("Interface\\partCharge1\\newui_menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		}
		else
		{
			LoadBitmap("Interface\\HUD\\Look-1\\Menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		}
	}
	else if (gmProtect->LookAndFeel == 2)
	{
		LoadBitmap("Interface\\HUD\\Look-2\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu01_new.jpg", IMAGE_MENU_1, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu02.jpg", IMAGE_MENU_2, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu03_new.jpg", IMAGE_MENU_3, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu_blue.jpg", IMAGE_GAUGE_BLUE, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu_green.jpg", IMAGE_GAUGE_GREEN, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu_red.jpg", IMAGE_GAUGE_RED, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu03_new_ag.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu_character.jpg", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu_inventory.jpg", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-2\\Guild.jpg", IMAGE_MENU_BTN_GUILD, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-2\\Menu_party.jpg", IMAGE_MENU_BTN_PARTY, GL_LINEAR, GL_CLAMP_TO_EDGE);

		LoadBitmap("Interface\\HUD\\Look-2\\menu01_new2.jpg", IMAGE_MENU_BTN_WINDOW, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-2\\menu01_new2_SD.jpg", IMAGE_GAUGE_SD, GL_LINEAR);

		if (WindowWidth > 800 && gmProtect->ScreenType != 0)
		{
			LoadBitmap("Interface\\partCharge1\\newui_menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		}
		else
		{
			LoadBitmap("Interface\\HUD\\Look-2\\Menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		}

		//LoadBitmap("Interface\\HUD\\Look-2\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu01.jpg", IMAGE_MENU_1, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu02.jpg", IMAGE_MENU_2, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu03.jpg", IMAGE_MENU_3, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_blue.jpg", IMAGE_GAUGE_BLUE, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_green.jpg", IMAGE_GAUGE_GREEN, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_red.jpg", IMAGE_GAUGE_RED, GL_LINEAR);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_ag.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
		//
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_Bt01.jpg", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_Bt02.jpg", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_Bt03.jpg", IMAGE_MENU_BTN_FRIEND, GL_LINEAR, GL_CLAMP_TO_EDGE);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_Bt04.jpg", IMAGE_MENU_BTN_WINDOW, GL_LINEAR, GL_CLAMP_TO_EDGE);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_Bt06.jpg", IMAGE_MENU_BTN_GUILD, GL_LINEAR, GL_CLAMP_TO_EDGE);
		//LoadBitmap("Interface\\HUD\\Look-2\\newui_menu_Bt07.jpg", IMAGE_MENU_BTN_PARTY, GL_LINEAR, GL_CLAMP_TO_EDGE);
	}
	else if (gmProtect->LookAndFeel == 3)
	{
		LoadBitmap("Interface\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
		LoadBitmap("Interface\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu01.tga", IMAGE_MENU_1, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu02.tga", IMAGE_MENU_2, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu03.tga", IMAGE_MENU_3, GL_LINEAR);
		//LoadBitmap("Interface\\partCharge1\\newui_menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		//LoadBitmap("Interface\\newui_menu02-03.jpg", IMAGE_MENU_2_1, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_blue.tga", IMAGE_GAUGE_BLUE, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_green.tga", IMAGE_GAUGE_GREEN, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_red.tga", IMAGE_GAUGE_RED, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_ag.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_sd.jpg", IMAGE_GAUGE_SD, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_Bt01.tga", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_Bt02.tga", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_Bt03.tga", IMAGE_MENU_BTN_FRIEND, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_Bt04.tga", IMAGE_MENU_BTN_WINDOW, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_Bt05.tga", IMAGE_MENU_BTN_CSHOP, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_menu_Bt06.tga", IMAGE_MENU_BTN_GUILD, GL_LINEAR, GL_CLAMP_TO_EDGE);
	}
	else if (gmProtect->LookAndFeel == 4)
	{
		LoadBitmap("Interface\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
		LoadBitmap("Interface\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu01.tga", IMAGE_MENU_1, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu02.tga", IMAGE_MENU_2, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu03.tga", IMAGE_MENU_3, GL_LINEAR);
		//LoadBitmap("Interface\\partCharge1\\newui_menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		//LoadBitmap("Interface\\newui_menu02-03.jpg", IMAGE_MENU_2_1, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_blue.tga", IMAGE_GAUGE_BLUE, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_green.tga", IMAGE_GAUGE_GREEN, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_red.tga", IMAGE_GAUGE_RED, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_ag.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_sd.jpg", IMAGE_GAUGE_SD, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_Bt01.tga", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_Bt02.tga", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_Bt03.tga", IMAGE_MENU_BTN_FRIEND, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_Bt04.tga", IMAGE_MENU_BTN_WINDOW, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_Bt05.tga", IMAGE_MENU_BTN_CSHOP, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_menu_Bt06.tga", IMAGE_MENU_BTN_GUILD, GL_LINEAR, GL_CLAMP_TO_EDGE);
	}
	else
	{
		LoadBitmap("Interface\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
		LoadBitmap("Interface\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
		LoadBitmap("Interface\\newui_menu01.jpg", IMAGE_MENU_1, GL_LINEAR);
		LoadBitmap("Interface\\newui_menu02.jpg", IMAGE_MENU_2, GL_LINEAR);
		LoadBitmap("Interface\\partCharge1\\newui_menu03.jpg", IMAGE_MENU_3, GL_LINEAR);
		LoadBitmap("Interface\\partCharge1\\newui_menu04.tga", IMAGE_MENU_4, GL_LINEAR, GL_CLAMP, true, false);
		LoadBitmap("Interface\\newui_menu02-03.jpg", IMAGE_MENU_2_1, GL_LINEAR);
		LoadBitmap("Interface\\newui_menu_blue.jpg", IMAGE_GAUGE_BLUE, GL_LINEAR);
		LoadBitmap("Interface\\newui_menu_green.jpg", IMAGE_GAUGE_GREEN, GL_LINEAR);
		LoadBitmap("Interface\\newui_menu_red.jpg", IMAGE_GAUGE_RED, GL_LINEAR);
		LoadBitmap("Interface\\newui_menu_ag.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
		LoadBitmap("Interface\\newui_menu_sd.jpg", IMAGE_GAUGE_SD, GL_LINEAR);
		LoadBitmap("Interface\\partCharge1\\newui_menu_Bt01.jpg", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\partCharge1\\newui_menu_Bt02.jpg", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\partCharge1\\newui_menu_Bt03.jpg", IMAGE_MENU_BTN_FRIEND, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\partCharge1\\newui_menu_Bt04.jpg", IMAGE_MENU_BTN_WINDOW, GL_LINEAR, GL_CLAMP_TO_EDGE);
		LoadBitmap("Interface\\partCharge1\\newui_menu_Bt05.jpg", IMAGE_MENU_BTN_CSHOP, GL_LINEAR, GL_CLAMP_TO_EDGE);
	}
#endif // MAIN_UPDATE == 303
	if (gmProtect->RenderMuLogo != 0)
	{
		LoadBitmap("Interface\\HUD\\MU-logo.tga", BITMAP_IMAGE_FRAME_EMU, GL_LINEAR, GL_CLAMP_TO_EDGE);
	}
}

void SEASON3B::CNewUIMainFrameWindow::UnloadImages()
{
	DeleteBitmap(IMAGE_MENU_1);
	DeleteBitmap(IMAGE_MENU_2);
	DeleteBitmap(IMAGE_MENU_3);
	DeleteBitmap(IMAGE_MENU_4);
	DeleteBitmap(IMAGE_MENU_2_1);
	DeleteBitmap(IMAGE_GAUGE_BLUE);
	DeleteBitmap(IMAGE_GAUGE_GREEN);
	DeleteBitmap(IMAGE_GAUGE_RED);
	DeleteBitmap(IMAGE_GAUGE_AG);
	DeleteBitmap(IMAGE_GAUGE_SD);
	DeleteBitmap(IMAGE_GAUGE_EXBAR);
	DeleteBitmap(IMAGE_MENU_BTN_CHAINFO);
	DeleteBitmap(IMAGE_MENU_BTN_MYINVEN);
	DeleteBitmap(IMAGE_MENU_BTN_FRIEND);
	DeleteBitmap(IMAGE_MENU_BTN_WINDOW);

	if (gmProtect->LookAndFeel >= 1 && gmProtect->LookAndFeel <= 4)
	{
		DeleteBitmap(IMAGE_MENU_BTN_GUILD);
	}
	if (gmProtect->RenderMuLogo != 0)
	{
		DeleteBitmap(BITMAP_IMAGE_FRAME_EMU);
	}
}

bool SEASON3B::CNewUIMainFrameWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
{
	if (NULL == pNewUIMng || NULL == pNewUI3DRenderMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MAINFRAME, this);

	m_pNewUI3DRenderMng = pNewUI3DRenderMng;
	m_pNewUI3DRenderMng->Add3DRenderObj(this, ITEMHOTKEYNUMBER_CAMERA_Z_ORDER);

	PosX = PositionX_The_Mid(640.0);

	PosY = GetWindowsY;

	LoadImages();

	SetButtonInfo();

	m_ItemHotKey.Init();

	Show(true);

	return true;
}

void SEASON3B::CNewUIMainFrameWindow::SetButtonInfo()
{
#if MAIN_UPDATE == 303
	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	m_BtnWinParty.ChangeButtonInfo(RenderFrameX + 334.0, RenderFrameY + 450.5, 23.0, 22.0);
	m_BtnWinParty.ChangeToolTipText(GlobalText[361], true);

	m_BtnChaInfo.ChangeButtonInfo(RenderFrameX + 364.0, RenderFrameY + 450.5, 23.0, 22.0);
	m_BtnChaInfo.ChangeToolTipText(GlobalText[362], true);

	m_BtnMyInven.ChangeButtonInfo(RenderFrameX + 394.0, RenderFrameY + 450.5, 23.0, 22.0);
	m_BtnMyInven.ChangeToolTipText(GlobalText[363], true);
#else
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		float x_Next = PosX;
		float y_Next = PosY - 48;

		m_BtnWinParty.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnWinParty.ChangeButtonImgState(true, IMAGE_MENU_BTN_PARTY, true);
		m_BtnWinParty.ChangeButtonInfo(x_Next + 346.0, y_Next + 19.0, 28.0, 30.0);
		m_BtnWinParty.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnWinParty.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnWinParty.ChangeToolTipText(GlobalText[361], true);
		//--
		m_BtnChaInfo.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnChaInfo.ChangeButtonImgState(true, IMAGE_MENU_BTN_CHAINFO, true);
		m_BtnChaInfo.ChangeButtonInfo(x_Next + 378.0, y_Next + 19.0, 28.0, 30.0);
		m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnChaInfo.ChangeToolTipText(GlobalText[362], true);
		//--
		m_BtnMyInven.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnMyInven.ChangeButtonImgState(true, IMAGE_MENU_BTN_MYINVEN, true);
		m_BtnMyInven.ChangeButtonInfo(x_Next + 408.0, y_Next + 19.0, 28.0, 30.0);
		m_BtnMyInven.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnMyInven.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnMyInven.ChangeToolTipText(GlobalText[363], true);
		//--
		m_BtnWinGuild.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnWinGuild.ChangeButtonImgState(true, IMAGE_MENU_BTN_GUILD, true);
		m_BtnWinGuild.ChangeButtonInfo(x_Next + 579.0, y_Next + 26.5, 56.0, 24.0);
		m_BtnWinGuild.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnWinGuild.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnWinGuild.ChangeToolTipText(GlobalText[364], true);

		if (gmProtect->LookAndFeel == 2)
		{
			//m_BtnFriend.ChangeTextBackColor(RGBA(255, 255, 255, 0));
			//m_BtnFriend.ChangeButtonImgState(true, IMAGE_MENU_BTN_FRIEND, true);
			//m_BtnFriend.ChangeButtonInfo(x_Next + 579.0, y_Next + 1.5, 56.0, 24.0);
			//m_BtnFriend.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
			//m_BtnFriend.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
			//m_BtnFriend.ChangeToolTipText(GlobalText[1043], true);

			m_BtnWindow.ChangeTextBackColor(RGBA(255, 255, 255, 0));
			m_BtnWindow.ChangeButtonImgState(true, IMAGE_MENU_BTN_WINDOW, true);
			m_BtnWindow.ChangeButtonInfo(x_Next + 5.0, y_Next + 1.5, 56.0, 24.0);
			m_BtnWindow.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
			m_BtnWindow.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
			m_BtnWindow.ChangeToolTipText(GlobalText[1744], true);

			g_pFriendMenu->Init();
		}
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		float x_Next = PosX + 41;
		float y_Next = (PosY - 72) + 39;
		float x_Add = 24;
		float y_Add = 26;

		m_BtnCShop.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnCShop.ChangeButtonImgState(true, IMAGE_MENU_BTN_CSHOP, true);
		m_BtnCShop.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnCShop.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnCShop.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnCShop.ChangeToolTipText(GlobalText[3421], true);
		x_Next += x_Add;
		m_BtnChaInfo.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnChaInfo.ChangeButtonImgState(true, IMAGE_MENU_BTN_CHAINFO, true);
		m_BtnChaInfo.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnChaInfo.ChangeToolTipText(GlobalText[3423], true);
		x_Next += x_Add;
		//--
		m_BtnMyInven.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnMyInven.ChangeButtonImgState(true, IMAGE_MENU_BTN_MYINVEN, true);
		m_BtnMyInven.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnMyInven.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnMyInven.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnMyInven.ChangeToolTipText(GlobalText[3425], true);
		//--
		x_Next = PosX + 525;
		m_BtnWinGuild.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnWinGuild.ChangeButtonImgState(true, IMAGE_MENU_BTN_GUILD, true);
		m_BtnWinGuild.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnWinGuild.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnWinGuild.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnWinGuild.ChangeToolTipText(GlobalText[3427], true);
		x_Next += x_Add;

		m_BtnFriend.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnFriend.ChangeButtonImgState(true, IMAGE_MENU_BTN_FRIEND, true);
		m_BtnFriend.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnFriend.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnFriend.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnFriend.ChangeToolTipText(GlobalText[1043], true);
		x_Next += x_Add;

		m_BtnWindow.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnWindow.ChangeButtonImgState(true, IMAGE_MENU_BTN_WINDOW, true);
		m_BtnWindow.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnWindow.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnWindow.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnWindow.ChangeToolTipText(GlobalText[1744], true);
	}
	else
	{
		float x_Next = PosX + 489;
		float y_Next = PosY - 51;
		float x_Add = 30;
		float y_Add = 41;

		m_BtnCShop.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnCShop.ChangeButtonImgState(true, IMAGE_MENU_BTN_CSHOP, true);
		m_BtnCShop.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnCShop.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnCShop.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnCShop.ChangeToolTipText(GlobalText[2277], true);
		x_Next += x_Add;

		m_BtnChaInfo.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnChaInfo.ChangeButtonImgState(true, IMAGE_MENU_BTN_CHAINFO, true);
		m_BtnChaInfo.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnChaInfo.ChangeToolTipText(GlobalText[362], true);
		x_Next += x_Add;

		m_BtnMyInven.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnMyInven.ChangeButtonImgState(true, IMAGE_MENU_BTN_MYINVEN, true);
		m_BtnMyInven.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnMyInven.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnMyInven.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnMyInven.ChangeToolTipText(GlobalText[363], true);
		x_Next += x_Add;

		m_BtnFriend.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnFriend.ChangeButtonImgState(true, IMAGE_MENU_BTN_FRIEND, true);
		m_BtnFriend.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnFriend.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnFriend.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnFriend.ChangeToolTipText(GlobalText[1043], true);
		x_Next += x_Add;

		m_BtnWindow.ChangeTextBackColor(RGBA(255, 255, 255, 0));
		m_BtnWindow.ChangeButtonImgState(true, IMAGE_MENU_BTN_WINDOW, true);
		m_BtnWindow.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		m_BtnWindow.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
		m_BtnWindow.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
		m_BtnWindow.ChangeToolTipText(GlobalText[1744], true);
	}
#endif // MAIN_UPDATE == 303
}

void SEASON3B::CNewUIMainFrameWindow::Release()
{
	UnloadImages();

	if (m_pNewUI3DRenderMng)
	{
		m_pNewUI3DRenderMng->Remove3DRenderObj(this);
		m_pNewUI3DRenderMng = NULL;
	}

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

bool SEASON3B::CNewUIMainFrameWindow::Render()
{
	EnableAlphaTest();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RenderFrame();

#if MAIN_UPDATE == 303
	RenderLifeMana();

	RenderGuageSD();

	RenderGuageAG();

	RenderButtons();

	RenderExperience();

	g_pSkillList->RenderCurrentSkillAndHotSkillList();

#else

	m_pNewUI3DRenderMng->RenderUI2DEffect(ITEMHOTKEYNUMBER_CAMERA_Z_ORDER, UI2DEffectCallback, this, 0, 0);

	g_pSkillList->RenderCurrentSkillAndHotSkillList();

	EnableAlphaTest();

	RenderLifeMana();

	RenderGuageSD();

	RenderGuageAG();

	RenderButtons();

	RenderExperience();
#endif // MAIN_UPDATE == 303


	if (gmProtect->RenderMuLogo != 0 && GrabEnable)
	{
		this->logo_tick = std::chrono::steady_clock::now();
	}
	RenderLogo();

	DisableAlphaBlend();

	return true;
}

void SEASON3B::CNewUIMainFrameWindow::Render3D()
{
#if MAIN_UPDATE != 303
	m_ItemHotKey.RenderItems();
#endif // MAIN_UPDATE == 303
}

void SEASON3B::CNewUIMainFrameWindow::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
	g_pMainFrame->RenderHotKeyItemCount();
}

bool SEASON3B::CNewUIMainFrameWindow::IsVisible() const
{
	return CNewUIObj::IsVisible();
}

#if MAIN_UPDATE == 303
void Renderfragment(int imgindex, float RenderFrameX, float RenderFrameY, float RenderWidth, float RenderHight, double Percent, int Rendermode)
{
	BITMAP_t* pText = &Bitmaps[imgindex];

	if (Rendermode == 1)
	{
		float RenderFramex = RenderFrameX + (RenderWidth * Percent);
		float Renderwidth = RenderWidth - (RenderWidth * Percent);

		float output_width = (pText->output_height * Percent);

		RenderBitmap(imgindex, RenderFramex, RenderFrameY, Renderwidth, RenderHight, output_width / pText->Width, 0.0, (pText->output_width - output_width) / pText->Width, pText->output_height / pText->Height, true, true, 0.0);
	}
	else if (Rendermode == 2)
	{
		float RenderFramex = RenderFrameX + (RenderWidth * Percent);
		float Renderwidth = RenderWidth - (RenderWidth * Percent);

		float output_width = (pText->output_height * Percent);

		RenderBitmap(imgindex, RenderFramex, RenderFrameY, Renderwidth, RenderHight, 0.0, 0.0, (pText->output_width - output_width) / pText->Width, pText->output_height / pText->Height, true, true, 0.0);
	}
	else
	{
		float RenderFramey = RenderFrameY + (RenderHight * Percent);
		float Renderheight = RenderHight  - (RenderHight * Percent);

		float output_height = (pText->output_height * Percent);

		RenderBitmap(imgindex, RenderFrameX, RenderFramey, RenderWidth, Renderheight, 0.0, output_height / pText->Height, pText->output_width / pText->Width, (pText->output_height - output_height) / pText->Height, true, true, 0.0);
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderFrame()
{
	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);
	//-- scale: 1.8125
	SEASON3B::RenderImageF(IMAGE_MENU_1, RenderFrameX + 168.0, RenderFrameY + 432.0, 336.0, 48.0, 0.0, 0.0, 550.0, 87.0);

	//-- Mu Helper
	if (SEASON3B::CheckMouseIn(RenderFrameX + 423.0, RenderFrameY + 450, 18.0, 11.0) && MouseLButton)
	{
		SEASON3B::RenderImage(IMAGE_MENU_2, RenderFrameX + 423.0, RenderFrameY + 450, 18.0, 11.0, 1.0 / 120.0, 2.0 / 64.0, 32.0 / 120.0, 22 / 64.0, RGBA(100, 100, 100, 255)); //-- play
	}
	else
	{
		SEASON3B::RenderImage(IMAGE_MENU_2, RenderFrameX + 423.0, RenderFrameY + 450, 18.0, 11.0, 1.0 / 120.0, 2.0 / 64.0, 32.0 / 120.0, 22 / 64.0); //-- play
	}

	if (SEASON3B::CheckMouseIn(RenderFrameX + 423.0, RenderFrameY + 462.0, 18.0, 11.0) && MouseLButton)
	{
		SEASON3B::RenderImage(IMAGE_MENU_2, RenderFrameX + 423.0, RenderFrameY + 462.0, 18.0, 11.0, 1.0 / 120.0, 26.0 / 64.0, 32.0 / 120.0, 22 / 64.0, RGBA(100, 100, 100, 255)); //-- config
	}
	else
	{
		SEASON3B::RenderImage(IMAGE_MENU_2, RenderFrameX + 423.0, RenderFrameY + 462.0, 18.0, 11.0, 1.0 / 120.0, 26.0 / 64.0, 32.0 / 120.0, 22 / 64.0); //-- config
	}

	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_PARTY))
	{
		SEASON3B::RenderImage(IMAGE_MENU_BTN_PARTY, RenderFrameX + 334.0, RenderFrameY + 450.5, 23.0, 22.0, 0.0, 0.0, 24.0 / 32.0, 24.0 / 32.0); //-- Party
	}

	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
	{
		SEASON3B::RenderImage(IMAGE_MENU_BTN_CHAINFO, RenderFrameX + 364.0, RenderFrameY + 450.5, 23.0, 22.0, 0.0, 0.0, 24.0 / 32.0, 24.0 / 32.0); //-- Character
	}
	//
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
	{
		SEASON3B::RenderImage(IMAGE_MENU_BTN_MYINVEN, RenderFrameX + 394.0, RenderFrameY + 450.5, 23.0, 22.0, 0.0, 0.0, 24.0 / 32.0, 24.0 / 32.0); //-- Inventory
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderLifeMana()
{
	float fLife = 0.f;
	float fMana = 0.f;
	int indeximg = 0;

	DWORD wLifeMax = CharacterAttribute->LifeMax;
	DWORD wLife = CharacterAttribute->Life;
	DWORD wManaMax = CharacterAttribute->ManaMax;
	DWORD wMana = CharacterAttribute->Mana;

	if (wLifeMax > 0 && (wLife > 0 && (wLife / (float)wLifeMax) < 0.2f))
	{
		PlayBuffer(SOUND_HEART);
	}

	if (wLifeMax > 0)
		fLife = (wLifeMax - wLife) / (float)wLifeMax;

	if (wManaMax > 0)
		fMana = (wManaMax - wMana) / (float)wManaMax;

	if (g_isCharacterBuff((&Hero->Object), eDeBuff_Poison))
		indeximg = IMAGE_GAUGE_GREEN;
	else
		indeximg = IMAGE_GAUGE_RED;

	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	Renderfragment(indeximg, RenderFrameX + 168.0, RenderFrameY + 432.0, 53.0, 41.0, fLife, 0);

	indeximg = IMAGE_GAUGE_BLUE;

	Renderfragment(indeximg, RenderFrameX + 451.0, RenderFrameY + 432.0, 53.0, 41.0, fMana, 0);


	char strTipText[256];

	if (SEASON3B::CheckMouseIn(RenderFrameX + 168.0, RenderFrameY + 432.0, 53.0, 41.0))
	{
		sprintf(strTipText, GlobalText[358], wLife, wLifeMax);

		RenderTipText(RenderFrameX + 168.0, RenderFrameY + 412.0, strTipText);
	}

	if (SEASON3B::CheckMouseIn(RenderFrameX + 451.0, RenderFrameY + 432.0, 53.0, 41.0))
	{
		sprintf(strTipText, GlobalText[359], wMana, wManaMax);

		RenderTipText(RenderFrameX + 451.0, RenderFrameY + 412.0, strTipText);
	}

	glColor3f(1.0, 0.94999999, 0.75);

	SEASON3B::RenderNumber(RenderFrameX + 192.0, RenderFrameY + 465.0, wLife, 0.90);

	SEASON3B::RenderNumber(RenderFrameX + 477.0, RenderFrameY + 465.0, wMana, 0.90);

}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageAG()
{
	float fSkillMana = 0.0f;
	DWORD dwSkillMana = CharacterAttribute->SkillMana;
	DWORD dwMaxSkillMana = CharacterAttribute->SkillManaMax;

	if (dwMaxSkillMana > 0)
	{
		fSkillMana = (dwMaxSkillMana - dwSkillMana) / (float)dwMaxSkillMana;
	}

	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	glColor3f(1.0, 1.0, 1.0);

	Renderfragment(IMAGE_GAUGE_AG, RenderFrameX + 338.0, RenderFrameY + 434.5, 75.0, 8.0, fSkillMana, 1);

	if (SEASON3B::CheckMouseIn(RenderFrameX + 338.0, RenderFrameY + 434.5, 75.0, 8.0))
	{
		char strTipText[256];
		sprintf(strTipText, GlobalText[214], dwSkillMana, dwMaxSkillMana);
		RenderTipText(RenderFrameX + 338.0, RenderFrameY + 420, strTipText);
	}

	glColor3f(1.0, 0.94999999, 0.75);

	SEASON3B::RenderNumber(RenderFrameX + 373.0, RenderFrameY + 434.5, dwSkillMana, 0.8);
}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageSD()
{
	float fShield = 0.0f;
	DWORD wShield = CharacterAttribute->Shield;
	DWORD wMaxShield = CharacterAttribute->ShieldMax;

	if (wMaxShield > 0)
	{
		fShield = (wMaxShield - wShield) / (float)wMaxShield;
	}


	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	glColor3f(1.0, 1.0, 1.0);

	Renderfragment(IMAGE_GAUGE_SD, RenderFrameX + 259.0, RenderFrameY + 434.5, 75.0, 8.0, fShield, 2);

	if (SEASON3B::CheckMouseIn(RenderFrameX + 259.0, RenderFrameY + 434.5, 75.0, 8.0))
	{
		char strTipText[256];
		sprintf(strTipText, GlobalText[2037], wShield, wMaxShield);
		RenderTipText(RenderFrameX + 259.0, RenderFrameY + 420, strTipText);
	}

	glColor3f(1.0, 0.94999999, 0.75);

	SEASON3B::RenderNumber(RenderFrameX + 291.0, RenderFrameY + 434.5, wShield, 0.8);
}

void SEASON3B::CNewUIMainFrameWindow::RenderExperience()
{
	__int64 wLevel;
	__int64 dwNexExperience;
	__int64 dwExperience;

	double x_Next = 0.0;
	double Expwidth = 0.0;

	//double RenderFrameX = PositionX_The_Mid(640.0) + 2.0;
	//double RenderFrameY = PositionY_Add(0) + 473.0;
	//double Expwidth = 629.0;
	//double Expheight = 4.0;
	//double ExpBar = 629.0;RenderFrameX + 168.0, RenderFrameY + 432.0

	double RenderFrameX = PositionX_The_Mid(640.0) + 217.0;
	double RenderFrameY = PositionY_Add(0) + 476.5;
	double Expheight = 3.0;
	double ExpBar = 212.0;

	if (gCharacterManager.IsMasterLevel(CharacterAttribute->Class) == true && CharacterAttribute->Level >= 400)
	{
		wLevel = (__int64)Master_Level_Data.nMLevel;
		dwNexExperience = (__int64)Master_Level_Data.lNext_MasterLevel_Experince;
		dwExperience = (__int64)Master_Level_Data.lMasterLevel_Experince;
	}
	else
	{
		wLevel = CharacterAttribute->Level;
		dwNexExperience = CharacterAttribute->NextExperince;
		dwExperience = CharacterAttribute->Experience;
	}

	int iExp = 0;

	if (gCharacterManager.IsMasterLevel(CharacterAttribute->Class) == true && CharacterAttribute->Level >= 400)
	{
		__int64 iTotalLevel = wLevel + 400;
		__int64 iTOverLevel = iTotalLevel - 255;
		__int64 iBaseExperience = 0;

		__int64 iData_Master =
			(((__int64)9 + (__int64)iTotalLevel) * (__int64)iTotalLevel * (__int64)iTotalLevel * (__int64)10)
			+
			(((__int64)9 + (__int64)iTOverLevel) * (__int64)iTOverLevel * (__int64)iTOverLevel * (__int64)1000);

		iBaseExperience = (iData_Master - (__int64)3892250000) / (__int64)2;

		double fNeedExp = (double)dwNexExperience - (double)iBaseExperience;
		double fExp = (double)dwExperience - (double)iBaseExperience;

		if (dwExperience < iBaseExperience)
		{
			fExp = 0.f;
		}

		double fExpBarNum = 0.f;

		if (fExp > 0.f && fNeedExp > 0)
		{
			fExpBarNum = ((double)fExp / (double)fNeedExp) * (double)10.f;
		}

		double fProgress = fExpBarNum - __int64(fExpBarNum);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		if (m_bExpEffect == true)
		{
			double fPreProgress = 0.f;
			double fPreExp = (double)m_loPreExp - (double)iBaseExperience;

			if (m_loPreExp >= iBaseExperience)
			{
				int iPreExpBarNum = 0;
				int iExpBarNum = 0;
				if (fPreExp > 0.f && fNeedExp > 0.f)
				{
					fPreProgress = ((double)fPreExp / (double)fNeedExp) * (double)10.f;
					iPreExpBarNum = (int)fPreProgress;
					fPreProgress = (double)fPreProgress - __int64(fPreProgress);
				}
				iExpBarNum = (int)fExpBarNum;

				if (iExpBarNum <= iPreExpBarNum)
				{
					double fGapProgress = (fProgress - fPreProgress);

					Expwidth = (fPreProgress * ExpBar);
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, 1, 1, 0.0);

					//x_Next = Expwidth + RenderFrameX;
					Expwidth += fGapProgress * ExpBar;
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, 1, 1, 0.0);

					glColor4f(1.0, 1.0, 1.0, 0.60000002);
					RenderColor(RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0);
					EndRenderColor();
				}
				else
				{
					Expwidth = fProgress * ExpBar;
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, 1, 1, 0.0);
					glColor4f(1.0, 1.0, 1.0, 0.60000002);
					RenderColor(RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0);
					EndRenderColor();
				}
			}
			else
			{
				Expwidth = fProgress * ExpBar;
				RenderBitmap(IMAGE_MASTER_GAUGE_BAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, 1, 1, 0.0);
				glColor4f(1.0, 1.0, 1.0, 0.60000002);
				RenderColor(RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0);
				EndRenderColor();
			}
		}
		else
		{
			Expwidth = fProgress * ExpBar;
			RenderBitmap(IMAGE_MASTER_GAUGE_BAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, 1, 1, 0.0);
		}

		iExp = (int)fExpBarNum;
	}
	else
	{
		WORD wPriorLevel = wLevel - 1;
		DWORD dwPriorExperience = 0;

		if (wPriorLevel > 0)
		{
			dwPriorExperience = (9 + wPriorLevel) * wPriorLevel * wPriorLevel * 10;

			if (wPriorLevel > 255)
			{
				int iLevelOverN = wPriorLevel - 255;
				dwPriorExperience += (9 + iLevelOverN) * iLevelOverN * iLevelOverN * 1000;
			}
		}

		float fNeedExp = dwNexExperience - dwPriorExperience;
		float fExp = dwExperience - dwPriorExperience;

		if (dwExperience < dwPriorExperience)
		{
			fExp = 0.f;
		}

		float fExpBarNum = 0.f;
		if (fExp > 0.f && fNeedExp > 0)
		{
			fExpBarNum = (fExp / fNeedExp) * 10.f;
		}

		float fProgress = fExpBarNum;
		fProgress = fProgress - (int)fProgress;

		if (m_bExpEffect == true)
		{
			float fPreProgress = 0.f;
			fExp = m_dwPreExp - dwPriorExperience;


			if (m_dwPreExp >= dwPriorExperience)
			{
				int iPreExpBarNum = 0;
				int iExpBarNum = 0;
				if (fExp > 0.f && fNeedExp > 0.f)
				{
					fPreProgress = (fExp / fNeedExp) * 10.f;
					iPreExpBarNum = (int)fPreProgress;
					fPreProgress = fPreProgress - (int)fPreProgress;
				}

				iExpBarNum = (int)fExpBarNum;

				if (iExpBarNum <= iPreExpBarNum)
				{
					float fGapProgress = 0.f;

					fGapProgress = fProgress - fPreProgress;

					Expwidth = (fPreProgress * ExpBar);
					RenderBitmap(IMAGE_GAUGE_EXBAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, 1, 1, 0.0);

					x_Next = RenderFrameX;
					Expwidth += fGapProgress * ExpBar;
					RenderBitmap(IMAGE_GAUGE_EXBAR, x_Next, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, 1, 1, 0.0);

					glColor4f(1.0, 1.0, 1.0, 0.40000001);
					RenderColor(x_Next, RenderFrameY, Expwidth, Expheight, 0.0, 0);
					EndRenderColor();
				}
				else
				{
					Expwidth = fProgress * ExpBar;
					RenderBitmap(IMAGE_GAUGE_EXBAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, true, true, 0.0);
					glColor4f(1.0, 1.0, 1.0, 0.40000001);
					RenderColor(RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0);
					EndRenderColor();
				}
			}
			else
			{
				Expwidth = fProgress * ExpBar;
				RenderBitmap(IMAGE_GAUGE_EXBAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, true, true, 0.0);
				glColor4f(1.0, 1.0, 1.0, 0.40000001);
				RenderColor(RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0);
				EndRenderColor();
			}
		}
		else
		{
			Expwidth = fProgress * ExpBar;
			RenderBitmap(IMAGE_GAUGE_EXBAR, RenderFrameX, RenderFrameY, Expwidth, Expheight, 0.0, 0.0, 0.75, 1.0, true, true, 0.0);
		}
		iExp = (int)fExpBarNum;
	}


	EnableAlphaTest(true);
	glColor3f(0.91000003, 0.81, 0.60000002);
	SEASON3B::RenderNumber(RenderFrameX + 239, RenderFrameY - 4, iExp, 0.8);
	DisableAlphaBlend();
	glColor3f(1.0, 1.0, 1.0);

	if (SEASON3B::CheckMouseIn(RenderFrameX, RenderFrameY, ExpBar, Expheight) == true)
	{
		char strTipText[256];
		x_Next = RenderFrameX + (ExpBar / 2) - 40;
		sprintf(strTipText, GlobalText[1748], dwExperience, dwNexExperience);
		RenderTipText(x_Next, RenderFrameY - 58, strTipText);
	}
	/*width = ExpBar;
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		EnableAlphaTest(true);
		glColor3f(0.91000003, 0.81, 0.60000002);
		SEASON3B::RenderNumber(RenderFrameX + 425.0, RenderFrameY + 434.0, iExp);
		DisableAlphaBlend();
		glColor3f(1.0, 1.0, 1.0);
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		if (gmProtect->LookAndFeel == 3)
			SEASON3B::RenderImageF(IMAGE_MENU_2, x_Next - 1, y_Next - 2.2, width + 1, 9.0, 0.0, 0.0, 739.0, 13.0);

		x = x_Next + width + 27.0;
		y = y_Next - 4.0;
		SEASON3B::RenderNumber(x, y, iExp, 0.85);
	}
	else
	{
		x = x_Next + width + 6.0;
		y = y_Next - 4.0;
		SEASON3B::RenderNumber(x, y, iExp);
	}

	*/
}

#else

void RenderBouding(GLuint uiImageType, float posx, float y, float width, float height, float su, float sv, float uw, float vh, float percent, float NumberX, float NumberY, int current, int maxStat, int index)
{
	float fH = height - (percent * height);

	float RenderFrameX = posx;
	float RenderFrameY = (percent * height) + y;

	SEASON3B::RenderImage(uiImageType, RenderFrameX, RenderFrameY, width, fH, su, sv + (percent * height / vh), width / uw, (1.0f - percent) * height / vh);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		glColor3f(1.0, 0.94999999, 0.75);
		SEASON3B::RenderNumber(RenderFrameX + NumberX, y + NumberY, current, 0.90);
		glColor3f(1.0f, 1.0f, 1.0f);
	}
	else
	{
		SEASON3B::RenderNumber(RenderFrameX + NumberX, y + NumberY, current, 0.90);
	}

	if (SEASON3B::CheckMouseIn(RenderFrameX, y, width, height))
	{
		char strTipText[256];

		sprintf(strTipText, GlobalText[index], current, maxStat);

		RenderTipText(RenderFrameX, (int)(y - 16), strTipText);
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderFrame()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		float RenderFrameX = PosX;
		float RenderFrameY = PosY - 48.0;

		RenderBitmap(IMAGE_MENU_1, RenderFrameX, RenderFrameY, 256.0, 48.0, 0, 0, 1.0, 0.75);
		RenderFrameX += 256.0;
		RenderBitmap(IMAGE_MENU_2, RenderFrameX, RenderFrameY, 128.0, 48.0, 0, 0, 1.0, 0.75);
		RenderFrameX += 128.0;
		RenderBitmap(IMAGE_MENU_3, RenderFrameX, RenderFrameY, 256.0, 48.0, 0, 0, 1.0, 0.75);

		if (WindowWidth > 800 && gmProtect->ScreenType != 0)
		{
			RenderFrameX = PosX;
			SEASON3B::RenderImageF(IMAGE_MENU_4, RenderFrameX - 66.0, RenderFrameY, 66.0, 48.0, 0.0, 0.0, 113.0, 82.0);
			RenderFrameX += 640.0;
			SEASON3B::RenderImageF(IMAGE_MENU_4, RenderFrameX, RenderFrameY, 66.0, 48.0, 113.0, 0.0, -113.0, 82.0);
		}
		else
		{
			RenderFrameX = PosX;
			RenderFrameY -= 45.;
			RenderBitmap(IMAGE_MENU_4, RenderFrameX, RenderFrameY, 108.0, 45.0, 0, 0, 0.84375, 0.703125);

			if (GetScreenWidth() == (int)GetWindowsX)
			{
				RenderFrameX += 532.0;
				RenderBitmap(IMAGE_MENU_4, RenderFrameX, RenderFrameY, 108.0, 45.0, 0.84375, 0, -0.84375, 0.703125);
			}
		}

		this->RenderPosition();
	}
	else
	{
		float frameX = PosX;
		float frameY = PosY - 51.0;
		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			SEASON3B::RenderImageF(IMAGE_MENU_1, frameX, frameY, 256.0, 51.0);
			frameX += 256.0;
			SEASON3B::RenderImageF(IMAGE_MENU_2, frameX, frameY, 128.0, 50.95);
			frameX += 128.0;
			SEASON3B::RenderImageF(IMAGE_MENU_3, frameX, frameY, 256.0, 51.0);
			frameX += 256.0;

			frameX -= 640.0;
			if (WindowWidth > 800)
			{
				RenderImageF(IMAGE_MENU_4, frameX - 053.0, frameY - 13.0, 130.0, 64.0, 0.0, 0.0, 128.0, 64.0);

				RenderImageF(IMAGE_MENU_4, frameX + 564.0, frameY - 13.0, 130.0, 64.0, 128.0, 0.0, -128.0, 64.0);
			}
			glColor4f(0.6, 0.6, 0.6, 1.0f);
			SEASON3B::RenderNumber(frameX + 20, frameY + 34, Hero->PositionX, 0.90);
			SEASON3B::RenderNumber(frameX + 45, frameY + 34, Hero->PositionY, 0.90);
		}
		else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
		{
			frameY = PosY - 72.0;

			Render64LifeMana();

			SEASON3B::RenderImageF(IMAGE_MENU_1, frameX, frameY, 640.0, 72.0, 0.0, 22.0, 934.0, 106.0);

			if (g_pSkillList->IsSkillListUp() == true)
			{
				SEASON3B::RenderImageF(IMAGE_MENU_2, frameX + 444, frameY + 42.0, 6.0, 12.0, 83.0, 19.0, -12.0, 22.0);
			}
			else
			{
				SEASON3B::RenderImageF(IMAGE_MENU_2, frameX + 445, frameY + 42.0, 6.0, 12.0, 71.0, 19.0, 12.0, 22.0);
			}
		}
		else
		{
			if (WindowWidth > 800)
			{
				SEASON3B::RenderImageF(IMAGE_MENU_4, frameX - 70.28, frameY, 70.28, 51.0, 0.0, 0.0, 113.0, 82.0);
			}

			SEASON3B::RenderImage(IMAGE_MENU_1, frameX, frameY, 256.0, 51.0);
			frameX += 256.0;
			SEASON3B::RenderImage(IMAGE_MENU_2, frameX, frameY, 128.0, 51.0);
			frameX += 128.0;
			SEASON3B::RenderImage(IMAGE_MENU_3, frameX, frameY, 256.0, 51.0);
			frameX += 256.0;

			if (WindowWidth > 800)
			{
				SEASON3B::RenderImageF(IMAGE_MENU_4, frameX, frameY, 70.28, 51.0, 113.0, 0.0, -113.0, 82.0);
			}
			if (g_pSkillList->IsSkillListUp() == true)
			{
				SEASON3B::RenderImage(IMAGE_MENU_2_1, frameX - 418.0, frameY, 160.0, 40.0);
			}
		}
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderPosition()
{
	float RenderFrameX = PosX;
	float RenderFrameY = PosY - 18.0;

	glColor3f(0.60000002, 0.60000002, 0.60000002);
	SEASON3B::RenderNumber(RenderFrameX + 24.0, RenderFrameY, Hero->PositionX, 0.90);

	SEASON3B::RenderNumber(RenderFrameX + 48.0, RenderFrameY, Hero->PositionY, 0.90);
}

void SEASON3B::CNewUIMainFrameWindow::Render64LifeMana()
{
	float fLife = 0.f;
	float fMana = 0.f;

	DWORD wLifeMax = CharacterAttribute->LifeMax;
	DWORD wLife = min(max(0, CharacterAttribute->Life), wLifeMax);
	DWORD wManaMax = CharacterAttribute->ManaMax;
	DWORD wMana = min(max(0, CharacterAttribute->Mana), wManaMax);

	if (wLifeMax > 0 && (wLife > 0 && (wLife / (float)wLifeMax) < 0.2f))
	{
		PlayBuffer(SOUND_HEART);
	}

	if (wLifeMax > 0)
	{
		fLife = (wLifeMax - wLife) / (float)wLifeMax;
	}
	if (wManaMax > 0)
	{
		fMana = (wManaMax - wMana) / (float)wManaMax;
	}

	float frameX = PosX;
	float frameY = PosY - 72.0;

	double width = 60.0;
	double height = 60.0;
	double porcento = (1.0 - fLife);

	float iPosX = frameX + 108;
	float iPosY = frameY + (double)(fLife * height);
	float Height = (double)(porcento * height);

	if (gmProtect->LookAndFeel == 3)
	{
		SEASON3B::RenderImageF(IMAGE_MENU_3, iPosX, frameY, 68, 68, 0.0, 0.0, 96.0, 96.0);

		if (g_isCharacterBuff((&Hero->Object), eDeBuff_Poison))
			SEASON3B::RenderFrameAnimation(IMAGE_GAUGE_GREEN, iPosX, iPosY, width, Height, 0.0, (fLife * 128.0), 128.0, (porcento * 128.0), 2.44, 8, 64, 0.0, 128.0);
		else
			SEASON3B::RenderFrameAnimation(IMAGE_GAUGE_RED, iPosX, iPosY, width, Height, 0.0, (fLife * 128.0), 128.0, (porcento * 128.0), 2.44, 8, 64, 0.0, 128.0);

		porcento = (1.0 - fMana);

		iPosX = frameX + 465;
		iPosY = frameY + (double)(fMana * height);
		Height = (double)(porcento * height);

		SEASON3B::RenderImageF(IMAGE_MENU_3, iPosX, frameY, 68, 68, 0.0, 0.0, 96.0, 96.0);

		SEASON3B::RenderFrameAnimation(IMAGE_GAUGE_BLUE, iPosX, iPosY, width, Height, 0.0, (fMana * 128.0), 128.0, (porcento * 128.0), 2.44, 8, 64, 0.0, 128.0);

	}
	if (gmProtect->LookAndFeel == 4)
	{
		SEASON3B::RenderImageF(IMAGE_MENU_3, iPosX, frameY, width, height, 2.0, 2.0, 158.0, 158.0);

		if (g_isCharacterBuff((&Hero->Object), eDeBuff_Poison))
			RenderFrameAnimation(IMAGE_GAUGE_GREEN, iPosX, iPosY, width, Height, 0.0, (fLife * 152.0), 152.0, (porcento * 152.0), 1.44, 6, 36, 0.0, 152.0);
		else
			RenderFrameAnimation(IMAGE_GAUGE_RED, iPosX, iPosY, width, Height, 0.0, (fLife * 152.0), 152.0, (porcento * 152.0), 1.44, 6, 36, 0.0, 152.0);

		porcento = (1.0 - fMana);

		iPosX = frameX + 465;
		iPosY = frameY + (double)(fMana * height);
		Height = (double)(porcento * height);

		SEASON3B::RenderImageF(IMAGE_MENU_3, iPosX, frameY, width, height, 160, 2.0, -158.0, 158.0);

		SEASON3B::RenderFrameAnimation(IMAGE_GAUGE_BLUE, iPosX, iPosY, width, Height, 0.0, (fMana * 152.0), 152.0, (porcento * 152.0), 1.44, 6, 36, 0.0, 152.0);
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderLifeMana()
{
	float fLife = 0.f;
	float fMana = 0.f;
	int indeximg = 0;

	DWORD wLifeMax = CharacterAttribute->LifeMax;
	DWORD wLife = CharacterAttribute->Life;
	DWORD wManaMax = CharacterAttribute->ManaMax;
	DWORD wMana = CharacterAttribute->Mana;

	if (wLifeMax > 0 && (wLife > 0 && (wLife / (float)wLifeMax) < 0.2f))
	{
		PlayBuffer(SOUND_HEART);
	}

	if (wLifeMax > 0)
		fLife = (wLifeMax - wLife) / (float)wLifeMax;

	if (wManaMax > 0)
		fMana = (wManaMax - wMana) / (float)wManaMax;

	if (g_isCharacterBuff((&Hero->Object), eDeBuff_Poison))
		indeximg = IMAGE_GAUGE_GREEN;
	else
		indeximg = IMAGE_GAUGE_RED;

	if (gmProtect->LookAndFeel != 3 && gmProtect->LookAndFeel != 4)
	{
		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			RenderBouding(indeximg, PosX + 97.0, PosY - 48.0, 53.0, 48.0, 0.0, 0.0, 64.0, 64.0, fLife, 25.0, 30, wLife, wLifeMax, 358);

			RenderBouding(IMAGE_GAUGE_BLUE, PosX + 489.0, PosY - 48.0, 53.0, 48.0, 0.0, 0.0, 64.0, 64.0, fMana, 25.0, 30, wMana, wManaMax, 359);
		}
		else
		{
			RenderBouding(indeximg, PosX + 158.0, PosY - 48.0, 45.0, 39.0, 0.0, 0.0, 64.0, 64.0, fLife, 25.0, 15, wLife, wLifeMax, 358);

			RenderBouding(IMAGE_GAUGE_BLUE, PosX + 437.0, PosY - 48.0, 45.0, 39.0, 0.0, 0.0, 64.0, 64.0, fMana, 25.0, 15, wMana, wManaMax, 359);
		}
	}
	else
	{
		float frameX = PosX + 108.0;
		float frameY = PosY - 72.0;

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		SEASON3B::RenderNumber(frameX + 30.0, frameY + 30.0, wLife, 0.8);

		if (SEASON3B::CheckMouseIn(frameX, frameY, 60.0, 60.0))
		{
			char strTipText[256];
			sprintf(strTipText, GlobalText[358], wLife, wLifeMax);
			RenderTipText(frameX, (int)(frameY - 16), strTipText);
		}

		frameX += 357.0;

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		SEASON3B::RenderNumber(frameX + 35.0, frameY + 30.0, wMana, 0.8);

		if (SEASON3B::CheckMouseIn(frameX, frameY, 60.0, 60.0))
		{
			char strTipText[256];
			sprintf(strTipText, GlobalText[359], wMana, wManaMax);
			RenderTipText(frameX, (int)(frameY - 16), strTipText);
		}
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageAG()
{
	float fSkillMana = 0.0f;
	DWORD dwSkillMana = CharacterAttribute->SkillMana;
	DWORD dwMaxSkillMana = CharacterAttribute->SkillManaMax;

	if (dwMaxSkillMana > 0)
	{
		fSkillMana = (dwMaxSkillMana - dwSkillMana) / (float)dwMaxSkillMana;
	}

	if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		float frameX = PosX + 396.0;
		float frameY = (PosY - 72.0) + 19.0;

		double width = 68.0;
		double height = 9.0;
		double porcento = (1.0 - fSkillMana);

		float iPosX = frameX + (double)(fSkillMana * width);
		float Width = (double)(porcento * width);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		SEASON3B::RenderImageF(IMAGE_GAUGE_AG, iPosX, frameY, Width, height, (fSkillMana * 122.0), 0.0, (porcento * 122.0), 16.0);

		SEASON3B::RenderNumber(frameX + 35.0, frameY + 1.0, dwSkillMana, 0.8);

		if (SEASON3B::CheckMouseIn(frameX, frameY, width, height))
		{
			char strTipText[256];
			sprintf(strTipText, GlobalText[214], dwSkillMana, dwMaxSkillMana);
			RenderTipText(frameX, (int)(frameY - 16), strTipText);
		}
	}
	else
	{
		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			RenderBouding(IMAGE_GAUGE_AG, PosX + 551.0, PosY - 42.0, 15.0, 36.0, 0.0, 0.0, 16.0, 64.f, fSkillMana, 10, 28, dwSkillMana, dwMaxSkillMana, 214);
		}
		else
		{
			RenderBouding(IMAGE_GAUGE_AG, PosX + 420.0, PosY - 49.f, 16.f, 39.f, 0.0, 0.0, 16.0, 64.f, fSkillMana, 10, 30, dwSkillMana, dwMaxSkillMana, 214);
		}
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageSD()
{
	if (gmProtect->LookAndFeel != 1)
	{

		float fShield = 0.0f;
		DWORD wShield = CharacterAttribute->Shield;
		DWORD wMaxShield = CharacterAttribute->ShieldMax;

		if (wMaxShield > 0)
		{
			fShield = (wMaxShield - wShield) / (float)wMaxShield;
		}

		if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
		{
			float frameX = PosX + 178.0;
			float frameY = (PosY - 72.0) + 19.0;

			double width = 68.0;
			double height = 9.0;
			double porcento = (1.0 - fShield);
			float Width = (double)(porcento * width);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			SEASON3B::RenderImageF(IMAGE_GAUGE_SD, frameX, frameY, Width, height, 0.0, 0.0, (porcento * 122.0), 16.0);

			SEASON3B::RenderNumber(frameX + 35.0, frameY + 1.0, wShield, 0.8);

			if (SEASON3B::CheckMouseIn(frameX, frameY, width, height))
			{
				char strTipText[256];
				sprintf(strTipText, GlobalText[2037], wShield, wMaxShield);
				RenderTipText(frameX, (int)(frameY - 16), strTipText);
			}
		}
		else if (gmProtect->LookAndFeel == 2)
		{
			float RenderFrameX = PositionX_The_Mid(640.0);
			float RenderFrameY = PositionY_Add(0);

			RenderBouding(IMAGE_GAUGE_SD, RenderFrameX + 73.0, RenderFrameY + 438.0, 15.0, 36.0, 0.0, 0.0, 16.0, 64.f, fShield, 10, 28, wShield, wMaxShield, 2037);
		}
		else
		{
			RenderBouding(IMAGE_GAUGE_SD, PosX + 204.0, PosY - 49.f, 16.f, 39.f, 0.0, 0.0, 16.0, 64.f, fShield, 10, 30, wShield, wMaxShield, 2037);
		}
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderExperience()
{
	__int64 wLevel;
	__int64 dwNexExperience;
	__int64 dwExperience;

	double x_Next;
	double y_Next;
	double ExpBar;
	double height;
	double x, y, width;

	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		//glColor3f(0.92000002, 0.80000001, 0.34);
		x_Next = RenderFrameX + 221.0;
		y_Next = RenderFrameY + 439.0;
		ExpBar = 198.0;
		height = 4.0;
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		x_Next = PosX + 61.0;
		y_Next = PosY - 6.0;
		ExpBar = 516.0;
		height = 3.5;
	}
	else
	{
		x_Next = PosX + 2.0;
		y_Next = PosY - 7.0;
		ExpBar = 629.0;
		height = 4.0;
	}

	if (gCharacterManager.IsMasterLevel(CharacterAttribute->Class) == true && CharacterAttribute->Level >= 400)
	{
		wLevel = (__int64)Master_Level_Data.nMLevel;
		dwNexExperience = (__int64)Master_Level_Data.lNext_MasterLevel_Experince;
		dwExperience = (__int64)Master_Level_Data.lMasterLevel_Experince;
	}
	else
	{
		wLevel = CharacterAttribute->Level;
		dwNexExperience = CharacterAttribute->NextExperince;
		dwExperience = CharacterAttribute->Experience;
	}

	int iExp = 0;

	if (gCharacterManager.IsMasterLevel(CharacterAttribute->Class) == true && CharacterAttribute->Level >= 400)
	{
		__int64 iTotalLevel = wLevel + 400;
		__int64 iTOverLevel = iTotalLevel - 255;
		__int64 iBaseExperience = 0;

		__int64 iData_Master =
			(((__int64)9 + (__int64)iTotalLevel) * (__int64)iTotalLevel * (__int64)iTotalLevel * (__int64)10)
			+
			(((__int64)9 + (__int64)iTOverLevel) * (__int64)iTOverLevel * (__int64)iTOverLevel * (__int64)1000);

		iBaseExperience = (iData_Master - (__int64)3892250000) / (__int64)2;

		double fNeedExp = (double)dwNexExperience - (double)iBaseExperience;
		double fExp = (double)dwExperience - (double)iBaseExperience;

		if (dwExperience < iBaseExperience)
		{
			fExp = 0.f;
		}

		double fExpBarNum = 0.f;

		if (fExp > 0.f && fNeedExp > 0)
		{
			fExpBarNum = ((double)fExp / (double)fNeedExp) * (double)10.f;
		}

		double fProgress = fExpBarNum - __int64(fExpBarNum);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		if (m_bExpEffect == true)
		{
			double fPreProgress = 0.f;
			double fPreExp = (double)m_loPreExp - (double)iBaseExperience;
			if (m_loPreExp < iBaseExperience)
			{
				width = fProgress * ExpBar;
				RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

				glColor4f(1.f, 1.f, 1.f, 0.6f);
				RenderColor(x_Next, y_Next, width, height);
				EndRenderColor();
			}
			else
			{
				int iPreExpBarNum = 0;
				int iExpBarNum = 0;
				if (fPreExp > 0.f && fNeedExp > 0.f)
				{
					fPreProgress = ((double)fPreExp / (double)fNeedExp) * (double)10.f;
					iPreExpBarNum = (int)fPreProgress;
					fPreProgress = (double)fPreProgress - __int64(fPreProgress);
				}
				iExpBarNum = (int)fExpBarNum;

				if (iExpBarNum > iPreExpBarNum)
				{
					width = fProgress * ExpBar;
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

					glColor4f(1.f, 1.f, 1.f, 0.6f);
					RenderColor(x_Next, y_Next, width, height);
					EndRenderColor();
				}
				else
				{
					double fGapProgress = (double)fProgress - (double)fPreProgress;
					width = (double)fPreProgress * ExpBar;
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

					x = x_Next + width;
					width = (double)fGapProgress * ExpBar;

					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

					glColor4f(1.f, 1.f, 1.f, 0.6f);
					RenderColor(x, y_Next, width, height);
					EndRenderColor();
				}
			}
		}
		else
		{
			width = fProgress * ExpBar;
			RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
		}

		iExp = (int)fExpBarNum;
	}
	else
	{
		WORD wPriorLevel = wLevel - 1;
		DWORD dwPriorExperience = 0;

		if (wPriorLevel > 0)
		{
			dwPriorExperience = (9 + wPriorLevel) * wPriorLevel * wPriorLevel * 10;

			if (wPriorLevel > 255)
			{
				int iLevelOverN = wPriorLevel - 255;
				dwPriorExperience += (9 + iLevelOverN) * iLevelOverN * iLevelOverN * 1000;
			}
		}

		float fNeedExp = dwNexExperience - dwPriorExperience;
		float fExp = dwExperience - dwPriorExperience;

		if (dwExperience < dwPriorExperience)
		{
			fExp = 0.f;
		}

		float fExpBarNum = 0.f;
		if (fExp > 0.f && fNeedExp > 0)
		{
			fExpBarNum = (fExp / fNeedExp) * 10.f;
		}

		float fProgress = fExpBarNum;
		fProgress = fProgress - (int)fProgress;

		if (m_bExpEffect == true)
		{
			float fPreProgress = 0.f;
			fExp = m_dwPreExp - dwPriorExperience;
			if (m_dwPreExp < dwPriorExperience)
			{
				width = fProgress * ExpBar;
				RenderBitmap(IMAGE_GAUGE_EXBAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

				glColor4f(1.f, 1.f, 1.f, 0.4f);
				RenderColor(x_Next, y_Next, width, height);
				EndRenderColor();
			}
			else
			{
				int iPreExpBarNum = 0;
				int iExpBarNum = 0;
				if (fExp > 0.f && fNeedExp > 0.f)
				{
					fPreProgress = (fExp / fNeedExp) * 10.f;
					iPreExpBarNum = (int)fPreProgress;
					fPreProgress = fPreProgress - (int)fPreProgress;
				}

				iExpBarNum = (int)fExpBarNum;

				if (iExpBarNum > iPreExpBarNum)
				{
					width = fProgress * ExpBar;

					RenderBitmap(IMAGE_GAUGE_EXBAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

					glColor4f(1.f, 1.f, 1.f, 0.4f);
					RenderColor(x_Next, y_Next, width, height);
					EndRenderColor();
				}
				else
				{
					float fGapProgress = 0.f;

					fGapProgress = fProgress - fPreProgress;
					width = fProgress * ExpBar;

					RenderBitmap(IMAGE_GAUGE_EXBAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

					x = x_Next + width;
					width = fGapProgress * ExpBar;

					RenderBitmap(IMAGE_GAUGE_EXBAR, x, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

					glColor4f(1.f, 1.f, 1.f, 0.4f);
					RenderColor(x, y_Next, width, height);
					EndRenderColor();
				}
			}
		}
		else
		{
			width = fProgress * ExpBar;
			RenderBitmap(IMAGE_GAUGE_EXBAR, x_Next, y_Next, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
		}
		iExp = (int)fExpBarNum;
	}

	width = ExpBar;
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		EnableAlphaTest(true);
		glColor3f(0.91000003, 0.81, 0.60000002);
		SEASON3B::RenderNumber(RenderFrameX + 425.0, RenderFrameY + 434.0, iExp);
		DisableAlphaBlend();
		glColor3f(1.0, 1.0, 1.0);
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		if (gmProtect->LookAndFeel == 3)
			SEASON3B::RenderImageF(IMAGE_MENU_2, x_Next - 1, y_Next - 2.2, width + 1, 9.0, 0.0, 0.0, 739.0, 13.0);

		x = x_Next + width + 27.0;
		y = y_Next - 4.0;
		SEASON3B::RenderNumber(x, y, iExp, 0.85);
	}
	else
	{
		x = x_Next + width + 6.0;
		y = y_Next - 4.0;
		SEASON3B::RenderNumber(x, y, iExp);
	}

	if (SEASON3B::CheckMouseIn(x_Next, y_Next, width, height) == true)
	{
		char strTipText[256];
		x = x_Next + (width / 2) - 40;
		sprintf(strTipText, GlobalText[1748], dwExperience, dwNexExperience);
		RenderTipText(x, PosY - 60, strTipText);
	}
}
#endif

void SEASON3B::CNewUIMainFrameWindow::RenderHotKeyItemCount()
{
	m_ItemHotKey.RenderItemCount();
}

#if MAIN_UPDATE == 303
void SEASON3B::CNewUIMainFrameWindow::RenderButtons()
{
	m_BtnMyInven.Render();

	m_BtnChaInfo.Render();

	m_BtnWinParty.Render();
}
#else
void SEASON3B::CNewUIMainFrameWindow::RenderButtons()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		float RenderFrameX = PositionX_The_Mid(640.0);
		float RenderFrameY = PositionY_Add(0);

		//-- party
		if (SEASON3B::CheckMouseIn(RenderFrameX + 348.0, RenderFrameY + 452.0, 24.0, 24.0))
		{
			RenderTipText(RenderFrameX + 348, RenderFrameY + 417, GlobalText[361]);
		}
		if (g_pNewUISystem->IsVisible(INTERFACE_PARTY))
		{
			RenderBitmap(IMAGE_MENU_BTN_PARTY, RenderFrameX + 348.0, RenderFrameY + 452.0, 24.0, 24.0, 0, 0, 0.75, 0.75, true, true, 0.0);
		}
		//-- character
		if (SEASON3B::CheckMouseIn(RenderFrameX + 379.0, RenderFrameY + 452.0, 24.0, 24.0))
		{
			RenderTipText(RenderFrameX + 379, RenderFrameY + 417, GlobalText[362]);
		}
		if (g_pNewUISystem->IsVisible(INTERFACE_CHARACTER))
		{
			RenderBitmap(IMAGE_MENU_BTN_CHAINFO, RenderFrameX + 379.0, RenderFrameY + 452.0, 24.0, 24.0, 0, 0, 0.75, 0.75, true, true, 0.0);
		}
		//-- inventory
		if (SEASON3B::CheckMouseIn(RenderFrameX + 410.0, RenderFrameY + 452.0, 24.0, 24.0))
		{
			RenderTipText(RenderFrameX + 410, RenderFrameY + 417, GlobalText[363]); // Inventory (I,V)
		}
		if (g_pNewUISystem->IsVisible(INTERFACE_INVENTORY))
		{
			RenderBitmap(IMAGE_MENU_BTN_MYINVEN, RenderFrameX + 410.0, RenderFrameY + 452.0, 24.0, 24.0, 0, 0, 0.75, 0.75, true, true, 0.0);
		}
		//-- guild
		if (SEASON3B::CheckMouseIn(RenderFrameX + 582.0, RenderFrameY + 459.0, 52.0, 18.0))
		{
			RenderTipText(RenderFrameX + 582, RenderFrameY + 444, GlobalText[364]); // Guild (G)
		}
		if (g_pNewUISystem->IsVisible(INTERFACE_GUILDINFO))
		{
			RenderBitmap(IMAGE_MENU_BTN_GUILD, RenderFrameX + 582.0, RenderFrameY + 459.0, 52.0, 18.0, 0, 0, 0.8125, 0.5625, true, true, 0.0);
		}

		if (gmProtect->LookAndFeel == 2)
		{
			//-- command
			if (SEASON3B::CheckMouseIn(RenderFrameX + 6.0, RenderFrameY + 435.0, 53.0, 19.0))
			{
				RenderTipText(RenderFrameX + 6.0, RenderFrameY + 417, GlobalText[939]); // Command (D)
			}
			if (g_pNewUISystem->IsVisible(INTERFACE_COMMAND))
			{
				RenderBitmap(IMAGE_MENU_BTN_WINDOW, RenderFrameX + 6.0, RenderFrameY + 435.0, 53.0, 19.0, 0, 0, 0.828125, 0.59375, true, true, 0.0);
			}

			g_pFriendMenu->RenderFriendButton();
		}
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		RenderCharInfoButton();

		m_BtnCShop.Render(35.0, 38.0);

		m_BtnMyInven.Render(35.0, 38.0);

		m_BtnWinGuild.Render(35.0, 38.0); //-- quest

		RenderFriendButton();

		m_BtnWindow.Render(35.0, 38.0);
	}
	else
	{
		m_BtnCShop.Render();

		RenderCharInfoButton();

		m_BtnMyInven.Render();

		RenderFriendButton();

		m_BtnWindow.Render();
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderCharInfoButton()
{
	if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		m_BtnChaInfo.Render(35.0, 38.0);
	}
	else
	{
		m_BtnChaInfo.Render();

		if (g_QuestMng.IsQuestIndexByEtcListEmpty())
			return;

		if (g_Time.GetTimeCheck(5, 500))
			m_bButtonBlink = !m_bButtonBlink;

		if (m_bButtonBlink)
		{
			if (!(g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_QUEST_PROGRESS_ETC)
				|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER)))
			{
				if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
				{
					RenderImage(IMAGE_MENU_BTN_CHAINFO, PosX + 378.0, (PosY - 51.0) + 19.0, 28.0, 30.0, 0.0f, 30.0);
				}
				else
				{
					RenderImage(IMAGE_MENU_BTN_CHAINFO, PosX + 489 + 30.0, (PosY - 51.0), 30, 41, 0.0f, 41.f);
				}
			}
		}
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderFriendButton()
{
	if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		m_BtnFriend.Render(35.0, 38.0);
	}
	else
	{
		m_BtnFriend.Render();

		int iBlinkTemp = g_pFriendMenu->GetBlinkTemp();
		BOOL bIsAlertTime = (iBlinkTemp % 24 < 12);

		if (g_pFriendMenu->IsNewChatAlert() && bIsAlertTime)
		{
			RenderFriendButtonState();
		}
		if (g_pFriendMenu->IsNewMailAlert())
		{
			if (bIsAlertTime)
			{
				RenderFriendButtonState();

				if (iBlinkTemp % 24 == 11)
				{
					g_pFriendMenu->IncreaseLetterBlink();
				}
			}
		}
		else if (g_pLetterList->CheckNoReadLetter())
		{
			RenderFriendButtonState();
		}

		g_pFriendMenu->IncreaseBlinkTemp();
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderFriendButtonState()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		if (gmProtect->LookAndFeel == 2)
		{
			float x_Next = PosX + 579.0;
			float y_Next = (PosY - 51) + 1.5;

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_FRIEND) == true)
			{
				RenderImage(IMAGE_MENU_BTN_FRIEND, x_Next, y_Next, 56.0, 24.0, 0.0f, 72.0);
			}
			else
			{
				RenderImage(IMAGE_MENU_BTN_FRIEND, x_Next, y_Next, 56.0, 24.0, 0.0f, 24.0);
			}
		}
	}
	else
	{
		if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_FRIEND) == true)
		{
			RenderImage(IMAGE_MENU_BTN_FRIEND, PosX + 489 + (30 * 3), PosY - 51, 30, 41, 0.0f, 123.f);
		}
		else
		{
			RenderImage(IMAGE_MENU_BTN_FRIEND, PosX + 489 + (30 * 3), PosY - 51, 30, 41, 0.0f, 41.f);
		}
	}
}
#endif

void SEASON3B::CNewUIMainFrameWindow::RenderLogo()
{
	if (gmProtect->RenderMuLogo != 0)
	{
		auto runtimeThread = std::chrono::steady_clock::now();
		double difTime = std::chrono::duration<double>(runtimeThread - this->logo_tick).count();

		if (difTime < 1.5)
		{
			EnableAlphaTest();
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			if (gmProtect->RenderMuLogo == 1)
			{
				SEASON3B::RenderImage(BITMAP_IMAGE_FRAME_EMU, 10, 10, 150, 114);
			}
			if (gmProtect->RenderMuLogo == 2)
			{
				SEASON3B::RenderImage(BITMAP_IMAGE_FRAME_EMU, GetWindowsX - 160, 10, 150, 114);
			}
			if (gmProtect->RenderMuLogo == 3)
			{
				SEASON3B::RenderImage(BITMAP_IMAGE_FRAME_EMU, 10, GetWindowsY - 144, 150, 114);
			}
			if (gmProtect->RenderMuLogo == 4)
			{
				SEASON3B::RenderImage(BITMAP_IMAGE_FRAME_EMU, GetWindowsX - 160, GetWindowsY - 144, 150, 114);
			}
			if (gmProtect->RenderMuLogo == 5)
			{
				SEASON3B::RenderImage(BITMAP_IMAGE_FRAME_EMU, PositionX_The_Mid(150), PositionY_The_Mid(114) - 51, 150, 114);
			}
		}
	}
}


bool SEASON3B::CNewUIMainFrameWindow::UpdateMouseEvent()
{
	if (g_pNewUIHotKey->IsStateGameOver() == true)
	{
		return true;
	}

	if (BtnProcess() == true)
	{
		return false;
	}

	return true;
}

#if MAIN_UPDATE == 303
bool SEASON3B::CNewUIMainFrameWindow::BtnProcess()
{
	if (g_pNewUIHotKey->CanUpdateKeyEventRelatedMyInventory() == true)
	{
		if (m_BtnMyInven.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
	}
	else if (g_pNewUIHotKey->CanUpdateKeyEvent() == true)
	{
		float RenderFrameX = PositionX_The_Mid(640.0);
		float RenderFrameY = PositionY_Add(0);

		if (m_BtnMyInven.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}

		if (m_BtnChaInfo.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER);

			PlayBuffer(SOUND_CLICK01);

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
			{
				g_QuestMng.SendQuestIndexByEtcSelection();
			}
			return true;
		}

		if (m_BtnWinParty.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_PARTY);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}

		if (SEASON3B::IsRelease(VK_LBUTTON))
		{
			if (SEASON3B::CheckMouseIn(RenderFrameX + 423.0, RenderFrameY + 450, 18.0, 11.0))
			{
				if (gmAIController->IsRunning() != true)
				{
					if (gmAIController->CanUseAIController())
					{
						SendRequestStartHelper(false);
					}
					return true;
				}
				else
				{
					SendRequestStartHelper(true);
					return true;
				}
			}

			if (SEASON3B::CheckMouseIn(RenderFrameX + 423.0, RenderFrameY + 462.0, 18.0, 11.0))
			{
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_MACRO_OFICIAL);
				PlayBuffer(SOUND_CLICK01);

				if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MACRO_OFICIAL))
				{
					g_pNewUISystem->Hide(SEASON3B::INTERFACE_MACRO_OFICIAL_SUB);
				}
				return true;
			}
		}
	}

	return false;
}
#else
bool SEASON3B::CNewUIMainFrameWindow::BtnProcess()
{
	if (g_pNewUIHotKey->CanUpdateKeyEventRelatedMyInventory() == true)
	{
		if (m_BtnMyInven.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
	}
	else if (g_pNewUIHotKey->CanUpdateKeyEvent() == true)
	{
		if (m_BtnMyInven.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
		else if (m_BtnChaInfo.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER);

			PlayBuffer(SOUND_CLICK01);

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
				g_QuestMng.SendQuestIndexByEtcSelection();

			return true;
		}
		else if (m_BtnFriend.UpdateMouseEvent() == true && gmProtect->LookAndFeel != 1)
		{
			if (gMapManager.InChaosCastle() == true)
			{
				PlayBuffer(SOUND_CLICK01);
				return true;
			}

			int iLevel = CharacterAttribute->Level;

			if (iLevel < 6)
			{
				if (g_pChatListBox->CheckChatRedundancy(GlobalText[1067]) == FALSE)
				{
					g_pChatListBox->AddText("", GlobalText[1067], SEASON3B::TYPE_SYSTEM_MESSAGE);
				}
			}
			else
			{
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_FRIEND);
			}
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
		else if (m_BtnWindow.UpdateMouseEvent() == true && gmProtect->LookAndFeel != 1 && gmProtect->LookAndFeel == 2)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_COMMAND);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
		else if (m_BtnCShop.UpdateMouseEvent() == true && (gmProtect->LookAndFeel != 1 && gmProtect->LookAndFeel != 2))
		{
			if (GMProtect->IsInGameShop() == false)
				return false;

			if (g_pInGameShop->IsInGameShopOpen() == false)
				return false;

			if (g_InGameShopSystem->IsScriptDownload() == true)
			{
				if (g_InGameShopSystem->ScriptDownload() == false)
					return false;
			}

			if (g_InGameShopSystem->IsBannerDownload() == true)
			{
				g_InGameShopSystem->BannerDownload();
			}
			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == false)
			{
				if (g_InGameShopSystem->GetIsRequestShopOpenning() == false)
				{
					SendRequestIGS_CashShopOpen(0);
					g_InGameShopSystem->SetIsRequestShopOpenning(true);
					g_pMainFrame->SetBtnState(MAINFRAME_BTN_PARTCHARGE, true);
				}
			}
			else
			{
				SendRequestIGS_CashShopOpen(1);
				g_pNewUISystem->Hide(SEASON3B::INTERFACE_INGAMESHOP);
			}
			return true;
		}

		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			if (m_BtnWinGuild.UpdateMouseEvent() == true)
			{
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_GUILDINFO);
				PlayBuffer(SOUND_CLICK01);
				return true;
			}
			if (m_BtnWinParty.UpdateMouseEvent() == true)
			{
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_PARTY);
				PlayBuffer(SOUND_CLICK01);
				return true;
			}
		}
		if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
		{
			if (m_BtnWinGuild.UpdateMouseEvent() == true)
			{
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_MYQUEST);
				PlayBuffer(SOUND_CLICK01);
				return true;
			}
		}
	}

	return false;
}
#endif // MAIN_UPDATE == 303

bool SEASON3B::CNewUIMainFrameWindow::UpdateKeyEvent()
{
	if (m_ItemHotKey.UpdateKeyEvent() == false)
	{
		return false;
	}
	return true;
}

bool SEASON3B::CNewUIMainFrameWindow::Update()
{
	if (m_bExpEffect == true)
	{
		if (timeGetTime() - m_dwExpEffectTime > 2000)
		{
			m_bExpEffect = false;
			m_dwExpEffectTime = 0;
			m_dwGetExp = 0;
		}
	}

	return true;
}

float SEASON3B::CNewUIMainFrameWindow::GetLayerDepth()
{
	return 10.6f;
}

float SEASON3B::CNewUIMainFrameWindow::GetKeyEventOrder()
{
	return 2.9f;
}

void SEASON3B::CNewUIMainFrameWindow::EventOrderWindows(double WindowsX, double WindowsY)
{
#if MAIN_UPDATE == 303
	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	m_BtnWinParty.SetPos(RenderFrameX + 334.0, RenderFrameY + 450.5);

	m_BtnChaInfo.SetPos(RenderFrameX + 364.0, RenderFrameY + 450.5);

	m_BtnMyInven.SetPos(RenderFrameX + 394.0, RenderFrameY + 450.5);
#else
	PosX = PositionX_The_Mid(640.0);
	PosY = GetWindowsY;

	m_ItemHotKey.Init();

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		float x_Next = PosX;
		float y_Next = PosY - 48;

		m_BtnWinParty.ChangeButtonInfo(x_Next + 346.0, y_Next + 19.0, 28.0, 30.0);
		m_BtnChaInfo.ChangeButtonInfo(x_Next + 378.0, y_Next + 19.0, 28.0, 30.0);
		m_BtnMyInven.ChangeButtonInfo(x_Next + 408.0, y_Next + 19.0, 28.0, 30.0);
		m_BtnWinGuild.ChangeButtonInfo(x_Next + 579.0, y_Next + 26.5, 56.0, 24.0);

		if (gmProtect->LookAndFeel == 2)
		{
			m_BtnFriend.ChangeButtonInfo(x_Next + 579.0, y_Next + 1.5, 56.0, 24.0);
			m_BtnWindow.ChangeButtonInfo(x_Next + 5.0, y_Next + 1.5, 56.0, 24.0);
			g_pFriendMenu->Init();
		}
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		float x_Next = PosX + 41;
		float y_Next = (PosY - 72) + 39;
		float x_Add = 24;
		float y_Add = 26;

		m_BtnCShop.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnChaInfo.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnMyInven.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		//--
		x_Next = PosX + 525;
		m_BtnWinGuild.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnFriend.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnWindow.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	}
	else
	{
		float x_Next = PosX + 489;
		float y_Next = PosY - 51;
		float x_Add = 30;
		float y_Add = 41;

		m_BtnCShop.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnChaInfo.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnMyInven.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnFriend.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
		x_Next += x_Add;
		m_BtnWindow.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	}
#endif
}

void SEASON3B::CNewUIMainFrameWindow::SetItemHotKey(int iHotKey, int iItemType, int iItemLevel)
{
	m_ItemHotKey.SetHotKey(iHotKey, iItemType, iItemLevel);
}

int SEASON3B::CNewUIMainFrameWindow::GetItemHotKey(int iHotKey)
{
	return m_ItemHotKey.GetHotKey(iHotKey);
}

int SEASON3B::CNewUIMainFrameWindow::GetItemHotKeyLevel(int iHotKey)
{
	return m_ItemHotKey.GetHotKeyLevel(iHotKey);
}

void SEASON3B::CNewUIMainFrameWindow::UseHotKeyItemRButton()
{
	m_ItemHotKey.UseItemRButton();
}

void SEASON3B::CNewUIMainFrameWindow::UpdateItemHotKey()
{
	m_ItemHotKey.UpdateKeyEvent();
}

void SEASON3B::CNewUIMainFrameWindow::ResetSkillHotKey()
{
	g_pSkillList->Reset();
}

void SEASON3B::CNewUIMainFrameWindow::SetSkillHotKey(int iHotKey, int iSkillType)
{
	g_pSkillList->SetHotKey(iHotKey, iSkillType);
}

int SEASON3B::CNewUIMainFrameWindow::GetSkillHotKey(int iHotKey)
{
	return g_pSkillList->GetHotKey(iHotKey);
}

int SEASON3B::CNewUIMainFrameWindow::GetSkillHotKeyIndex(int iSkillType)
{
	return g_pSkillList->GetSkillIndex(iSkillType);
}

SEASON3B::CNewUIItemHotKey::CNewUIItemHotKey()
{
	for (int i = 0; i < HOTKEY_COUNT; ++i)
	{
		m_iHotKey[i].Type = -1;
		m_iHotKey[i].Level = 0;
	}

	rendercount = HOTKEY_COUNT;
}

SEASON3B::CNewUIItemHotKey::~CNewUIItemHotKey()
{
}

void SEASON3B::CNewUIItemHotKey::Init()
{
	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		rendercount = HOTKEY_R;

		SetPosition(HOTKEY_Q, RenderFrameX + 210, RenderFrameY + 453.0, RenderFrameX + 226.0, RenderFrameY + 447.0);

		SetPosition(HOTKEY_W, RenderFrameX + 240, RenderFrameY + 453.0, RenderFrameX + 257.0, RenderFrameY + 447.0);

		SetPosition(HOTKEY_E, RenderFrameX + 270, RenderFrameY + 453.0, RenderFrameX + 288.0, RenderFrameY + 447.0);

		//SetPosition(HOTKEY_R, RenderFrameX + 300, RenderFrameY + 453.0, RenderFrameX + 334.0, RenderFrameY + 447.0);
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		SetPosition(HOTKEY_Q, RenderFrameX + 188, RenderFrameY + 445.0, RenderFrameX + 203, RenderFrameY + 457.0);

		SetPosition(HOTKEY_W, RenderFrameX + 217, RenderFrameY + 445.0, RenderFrameX + 232, RenderFrameY + 457.0);

		SetPosition(HOTKEY_E, RenderFrameX + 246, RenderFrameY + 445.0, RenderFrameX + 261, RenderFrameY + 457.0);

		SetPosition(HOTKEY_R, RenderFrameX + 275, RenderFrameY + 445.0, RenderFrameX + 290, RenderFrameY + 457.0);
	}
	else
	{
		SetPosition(HOTKEY_Q, RenderFrameX + 10.00, RenderFrameY + 443.0, RenderFrameX + 30.0, RenderFrameY + 457.0);

		SetPosition(HOTKEY_W, RenderFrameX + 48.00, RenderFrameY + 443.0, RenderFrameX + 68.0, RenderFrameY + 457.0);

		SetPosition(HOTKEY_E, RenderFrameX + 86.00, RenderFrameY + 443.0, RenderFrameX + 106.0, RenderFrameY + 457.0);

		SetPosition(HOTKEY_R, RenderFrameX + 124.0, RenderFrameY + 443.0, RenderFrameX + 144.0, RenderFrameY + 457.0);
	}
}

void SEASON3B::CNewUIItemHotKey::SetPosition(int iHotKey, float x, float y, float x2, float y2)
{
	if (iHotKey >= 0 && iHotKey < rendercount)
	{
		m_iHotKey[iHotKey].m_ItemX = x;
		m_iHotKey[iHotKey].m_ItemY = y;
		m_iHotKey[iHotKey].m_NumbX = x2;
		m_iHotKey[iHotKey].m_NumbY = y2;
	}
}

bool SEASON3B::CNewUIItemHotKey::UpdateKeyEvent()
{
	int iIndex = -1;

	if (SEASON3B::IsPress('Q') == true)
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_Q);
	}
	else if (SEASON3B::IsPress('W') == true)
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_W);
	}
	else if (SEASON3B::IsPress('E') == true)
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_E);
	}
	else if (SEASON3B::IsPress('R') == true && (gmProtect->LookAndFeel != 1 && gmProtect->LookAndFeel != 2))
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_R);
	}

	if (iIndex != -1)
	{
		ITEM* pItem = NULL;
		pItem = g_pMyInventory->FindItem(iIndex);
		if ((pItem->Type >= ITEM_POTION + 78 && pItem->Type <= ITEM_POTION + 82))
		{
			std::list<eBuffState> secretPotionbufflist;
			secretPotionbufflist.push_back(eBuff_SecretPotion1);
			secretPotionbufflist.push_back(eBuff_SecretPotion2);
			secretPotionbufflist.push_back(eBuff_SecretPotion3);
			secretPotionbufflist.push_back(eBuff_SecretPotion4);
			secretPotionbufflist.push_back(eBuff_SecretPotion5);

			if (g_isCharacterBufflist((&Hero->Object), secretPotionbufflist) != eBuffNone)
			{
				SEASON3B::CreateOkMessageBox(GlobalText[2530], RGBA(255, 30, 0, 255));
			}
			else
			{
				SendRequestUse(iIndex, 0);
			}
		}
		else

		{
			SendRequestUse(iIndex, 0);
		}
		return false;
	}

	return true;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyItemIndex(int iType, bool bItemCount)
{
	int iStartItemType = 0, iEndItemType = 0;
	int i, j;

	switch (iType)
	{
	case HOTKEY_Q:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
		{
			if (m_iHotKey[iType].Type >= ITEM_POTION + 4 && m_iHotKey[iType].Type <= ITEM_POTION + 6)
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
			else
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
		}
		break;
	case HOTKEY_W:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
		{
			if (m_iHotKey[iType].Type >= ITEM_POTION + 0 && m_iHotKey[iType].Type <= ITEM_POTION + 3)
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
			else
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
		}
		break;
	case HOTKEY_E:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
		{
			if (m_iHotKey[iType].Type >= ITEM_POTION + 0 && m_iHotKey[iType].Type <= ITEM_POTION + 3)
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
			else if (m_iHotKey[iType].Type >= ITEM_POTION + 4 && m_iHotKey[iType].Type <= ITEM_POTION + 6)
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
			else
			{
				iStartItemType = ITEM_POTION + 8; iEndItemType = ITEM_POTION + 8;
			}
		}
		break;
	case HOTKEY_R:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false && (gmProtect->LookAndFeel != 1 && gmProtect->LookAndFeel != 2))
		{
			if (m_iHotKey[iType].Type >= ITEM_POTION + 0 && m_iHotKey[iType].Type <= ITEM_POTION + 3)
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
			else if (m_iHotKey[iType].Type >= ITEM_POTION + 4 && m_iHotKey[iType].Type <= ITEM_POTION + 6)
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
			else
			{
				iStartItemType = ITEM_POTION + 37; iEndItemType = ITEM_POTION + 35;
			}
		}
		break;
	}

	int iItemCount = 0;
	ITEM* pItem = NULL;

	int iNumberofItems = g_pMyInventory->GetNumberOfItems();

	for (i = iStartItemType; i >= iEndItemType; --i)
	{
		if (bItemCount)
		{
			for (j = 0; j < iNumberofItems; ++j)
			{
				pItem = g_pMyInventory->GetItem(j);

				if (pItem == NULL)
				{
					continue;
				}

				if (
					(pItem->Type == i && ((pItem->Level >> 3) & 15) == m_iHotKey[iType].Level)
					|| (pItem->Type == i && (pItem->Type >= ITEM_POTION + 0 && pItem->Type <= ITEM_POTION + 3))
					)
				{
					if (pItem->Type == ITEM_POTION + 9
						|| pItem->Type == ITEM_POTION + 10
						|| pItem->Type == ITEM_POTION + 20
						)
					{
						iItemCount++;
					}
					else
					{
						iItemCount += pItem->Durability;
					}
				}
			}
		}
		else
		{
			int iIndex = -1;
			if (i >= ITEM_POTION + 0 && i <= ITEM_POTION + 3)
			{
				iIndex = g_pMyInventory->FindItemReverseIndex(i);
			}
			else
			{
				iIndex = g_pMyInventory->FindItemReverseIndex(i, m_iHotKey[iType].Level);
			}

			if (-1 != iIndex)
			{
				pItem = g_pMyInventory->FindItem(iIndex);
				if ((pItem->Type != ITEM_POTION + 7
					&& pItem->Type != ITEM_POTION + 10
					&& pItem->Type != ITEM_POTION + 20)
					|| ((pItem->Level >> 3) & 15) == m_iHotKey[iType].Level
					)
				{
					return iIndex;
				}
			}
		}
	}

	if (bItemCount == true)
	{
		return iItemCount;
	}

	return -1;
}

bool SEASON3B::CNewUIItemHotKey::GetHotKeyCommonItem(IN int iHotKey, OUT int& iStart, OUT int& iEnd)
{
	switch (m_iHotKey[iHotKey].Type)
	{
	case ITEM_POTION + 7:
	case ITEM_POTION + 8:
	case ITEM_POTION + 9:
	case ITEM_POTION + 10:
	case ITEM_POTION + 20:
	case ITEM_POTION + 46:
	case ITEM_POTION + 47:
	case ITEM_POTION + 48:
	case ITEM_POTION + 49:
	case ITEM_POTION + 50:
	case ITEM_POTION + 70:
	case ITEM_POTION + 71:
	case ITEM_POTION + 78:
	case ITEM_POTION + 79:
	case ITEM_POTION + 80:
	case ITEM_POTION + 81:
	case ITEM_POTION + 82:
	case ITEM_POTION + 94:
	case ITEM_POTION + 85:
	case ITEM_POTION + 86:
	case ITEM_POTION + 87:
	case ITEM_POTION + 133:
		if (m_iHotKey[iHotKey].Type != ITEM_POTION + 20 || m_iHotKey[iHotKey].Level == 0)
		{
			iStart = iEnd = m_iHotKey[iHotKey].Type;
			return true;
		}
		break;
	default:
		if (m_iHotKey[iHotKey].Type >= ITEM_POTION + 35 && m_iHotKey[iHotKey].Type <= ITEM_POTION + 37)
		{
			iStart = ITEM_POTION + 37; iEnd = ITEM_POTION + 35;
			return true;
		}
		else if (m_iHotKey[iHotKey].Type >= ITEM_POTION + 38 && m_iHotKey[iHotKey].Type <= ITEM_POTION + 40)
		{
			iStart = ITEM_POTION + 40; iEnd = ITEM_POTION + 38;
			return true;
		}
		break;
	}
	return false;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyItemCount(int iType)
{
	return 0;
}

void SEASON3B::CNewUIItemHotKey::SetHotKey(int iHotKey, int iItemType, int iItemLevel)
{
	if (iHotKey >= 0 && iHotKey < rendercount)
	{
		if (CNewUIMyInventory::CanRegisterItemHotKey(iItemType) == true)
		{
			m_iHotKey[iHotKey].Type = iItemType;
			m_iHotKey[iHotKey].Level = iItemLevel;
		}
		else
		{
			m_iHotKey[iHotKey].Type = -1;
			m_iHotKey[iHotKey].Level = 0;
		}
	}
}

int SEASON3B::CNewUIItemHotKey::GetHotKey(int iHotKey)
{
	if (iHotKey >= 0 && iHotKey < rendercount)
	{
		return m_iHotKey[iHotKey].Type;
	}

	return -1;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyLevel(int iHotKey)
{
	if (iHotKey >= 0 && iHotKey < rendercount)
	{
		return m_iHotKey[iHotKey].Level;
	}

	return 0;
}

void SEASON3B::CNewUIItemHotKey::RenderItems()
{
	for (int i = 0; i < rendercount; ++i)
	{
		int iIndex = GetHotKeyItemIndex(i);
		if (iIndex != -1)
		{
			ITEM* pItem = g_pMyInventory->FindItem(iIndex);
			if (pItem)
			{
				float x = m_iHotKey[i].m_ItemX;
				float y = m_iHotKey[i].m_ItemY;

				RenderItem3D(x, y, 20.0, 20.0, pItem->Type, pItem->Level, 0, 0);
			}
		}
	}
}

void SEASON3B::CNewUIItemHotKey::RenderItemCount()
{
	float x, y;

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		glColor3f(1.0, 0.94999999, 0.75);
	}
	else
	{
		glColor4f(1.f, 1.f, 1.f, 1.f);
	}

	for (int i = 0; i < rendercount; ++i)
	{
		int iCount = GetHotKeyItemIndex(i, true);
		if (iCount > 0)
		{
			x = m_iHotKey[i].m_NumbX;
			y = m_iHotKey[i].m_NumbY;

			if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
			{
				SEASON3B::RenderNumber(x, y, iCount, 0.85);
			}
			else
			{
				SEASON3B::RenderNumber(x, y, iCount);
			}
		}
	}
}

void SEASON3B::CNewUIItemHotKey::UseItemRButton()
{
	int x, y;
	for (int i = 0; i < rendercount; ++i)
	{
		x = m_iHotKey[i].m_ItemX;
		y = m_iHotKey[i].m_ItemY;

		if (SEASON3B::CheckMouseIn(x, y, 20, 20) == true)
		{
			if (MouseRButtonPush)
			{
				MouseRButtonPush = false;
				int iIndex = GetHotKeyItemIndex(i);
				if (iIndex != -1)
				{
					SendRequestUse(iIndex, 0);
					break;
				}
			}
		}
	}
}

SEASON3B::CNewUISkillList::CNewUISkillList()
{
	m_pNewUIMng = NULL;
	Reset();
}

SEASON3B::CNewUISkillList::~CNewUISkillList()
{
	Release();
}

bool SEASON3B::CNewUISkillList::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_SKILL_LIST, this);

	m_pNewUI3DRenderMng = pNewUI3DRenderMng;

	LoadImages();

	Show(true);

	return true;
}

void SEASON3B::CNewUISkillList::Release()
{
	if (m_pNewUI3DRenderMng)
	{
		m_pNewUI3DRenderMng->DeleteUI2DEffectObject(UI2DEffectCallback);
	}

	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUISkillList::Reset()
{
	m_bSkillList = false;
	m_bHotKeySkillListUp = false;

	m_bRenderSkillInfo = false;
	m_iRenderSkillInfoType = 0;
	m_iRenderSkillInfoPosX = 0;
	m_iRenderSkillInfoPosY = 0;

	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		m_iHotKeySkillType[i] = -1;
	}

	m_EventState = EVENT_NONE;
}

void SEASON3B::CNewUISkillList::LoadImages()
{
	if (gmProtect->LookAndFeel == 1)
	{
		LoadBitmap("Interface\\HUD\\Look-1\\Skill.jpg", IMAGE_SKILL1, GL_LINEAR); //-- 31310
		LoadBitmap("Interface\\HUD\\Look-1\\Skill2.jpg", IMAGE_SKILL2, GL_LINEAR); //-- 31311
		LoadBitmap("Interface\\HUD\\Look-1\\Skill.jpg", IMAGE_NON_SKILL1, GL_LINEAR); //-- 31316
		LoadBitmap("Interface\\HUD\\Look-1\\Skill2.jpg", IMAGE_NON_SKILL2, GL_LINEAR); //-- 31317
		LoadBitmap("Interface\\HUD\\Look-1\\skillbox.jpg", IMAGE_SKILLBOX, GL_LINEAR); //-- 31314
		LoadBitmap("Interface\\HUD\\Look-1\\skillbox2.jpg", IMAGE_SKILLBOX_USE, GL_LINEAR); //-- 31315
		LoadBitmap("Interface\\HUD\\Look-1\\command.jpg", IMAGE_COMMAND, GL_LINEAR); //-- 31312
		LoadBitmap("Interface\\HUD\\Look-1\\command.jpg", IMAGE_NON_COMMAND, GL_LINEAR); //-- 31318
	}
	else if (gmProtect->LookAndFeel == 2)
	{
		LoadBitmap("Interface\\HUD\\Look-2\\Skill.jpg", IMAGE_SKILL1, GL_LINEAR); //-- 31310
		LoadBitmap("Interface\\HUD\\Look-2\\Skill2.jpg", IMAGE_SKILL2, GL_LINEAR); //-- 31311
		LoadBitmap("Interface\\HUD\\Look-2\\Skill.jpg", IMAGE_NON_SKILL1, GL_LINEAR); //-- 31316
		LoadBitmap("Interface\\HUD\\Look-2\\Skill2.jpg", IMAGE_NON_SKILL2, GL_LINEAR); //-- 31317
		LoadBitmap("Interface\\HUD\\Look-2\\skillbox.jpg", IMAGE_SKILLBOX, GL_LINEAR); //-- 31314
		LoadBitmap("Interface\\HUD\\Look-2\\skillbox2.jpg", IMAGE_SKILLBOX_USE, GL_LINEAR); //-- 31315
		LoadBitmap("Interface\\HUD\\Look-2\\command.jpg", IMAGE_COMMAND, GL_LINEAR); //-- 31312
		LoadBitmap("Interface\\HUD\\Look-2\\command.jpg", IMAGE_NON_COMMAND, GL_LINEAR); //-- 31318
	}
	else
	{
		LoadBitmap("Interface\\newui_skill.jpg", IMAGE_SKILL1, GL_LINEAR); //-- 31310
		LoadBitmap("Interface\\newui_skill2.jpg", IMAGE_SKILL2, GL_LINEAR); //-- 31311
		LoadBitmap("Interface\\newui_non_skill.jpg", IMAGE_NON_SKILL1, GL_LINEAR); //-- 31316
		LoadBitmap("Interface\\newui_non_skill2.jpg", IMAGE_NON_SKILL2, GL_LINEAR); //-- 31317
		LoadBitmap("Interface\\newui_skillbox.jpg", IMAGE_SKILLBOX, GL_LINEAR); //-- 31314
		LoadBitmap("Interface\\newui_skillbox2.jpg", IMAGE_SKILLBOX_USE, GL_LINEAR); //-- 31315
		LoadBitmap("Interface\\newui_command.jpg", IMAGE_COMMAND, GL_LINEAR); //-- 31312
		LoadBitmap("Interface\\newui_non_command.jpg", IMAGE_NON_COMMAND, GL_LINEAR); //-- 31318
	}

	LoadBitmap("Interface\\newui_skill3.jpg", IMAGE_SKILL3, GL_LINEAR); //-- 31313
	LoadBitmap("Interface\\newui_non_skill3.jpg", IMAGE_NON_SKILL3, GL_LINEAR); //-- 31319

	if (gmProtect->LookAndFeel == 3)
	{
		LoadBitmap("Interface\\HUD\\Look-3\\newui_skillbox1.tga", IMAGE_SKILLBOX_EX7001, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-3\\newui_skillbox2.tga", IMAGE_SKILLBOX_EX7002, GL_LINEAR);
	}
	if (gmProtect->LookAndFeel == 4)
	{
		LoadBitmap("Interface\\HUD\\Look-4\\newui_skillbox1.tga", IMAGE_SKILLBOX_EX7001, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\Look-4\\newui_skillbox2.tga", IMAGE_SKILLBOX_EX7002, GL_LINEAR);
	}
}

void SEASON3B::CNewUISkillList::UnloadImages()
{
	DeleteBitmap(IMAGE_SKILL1);
	DeleteBitmap(IMAGE_SKILL2);
	DeleteBitmap(IMAGE_COMMAND);
	DeleteBitmap(IMAGE_SKILLBOX);
	DeleteBitmap(IMAGE_SKILLBOX_USE);
	DeleteBitmap(IMAGE_NON_SKILL1);
	DeleteBitmap(IMAGE_NON_SKILL2);
	DeleteBitmap(IMAGE_NON_COMMAND);
	DeleteBitmap(IMAGE_SKILL3);
	DeleteBitmap(IMAGE_NON_SKILL3);
}

bool SEASON3B::CNewUISkillList::UpdateMouseEvent()
{
	if (g_isCharacterBuff((&Hero->Object), eBuff_DuelWatch))
	{
		m_bSkillList = false;
		return true;
	}

	BYTE bySkillNumber = CharacterAttribute->SkillNumber;
	BYTE bySkillMasterNumber = CharacterAttribute->SkillMasterNumber;

	float x, y, width, height;

	m_bRenderSkillInfo = false;

	if (bySkillNumber <= 0)
	{
		return true;
	}

#if MAIN_UPDATE == 303
	float FrameX = PositionX_The_Mid(640.0);
	float FrameY = GetWindowsY - 51.0;

	float RenderFrameX = PositionX_The_Mid(640.0);
	float RenderFrameY = PositionY_Add(0);

	x = RenderFrameX + 302;
	y = RenderFrameY + 451;
	width = 16.0;
	height = 20.0;
#else

	float FrameX = PositionX_The_Mid(640.0);
	float FrameY = GetWindowsY - 51.0;

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		x = PositionX_In_The_Mid((310.0 - 6.0));
		y = WinFrameY - (33.0 + 6.0);
		width = 32.f; height = 38.f;
	}
	else if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		FrameY = GetWindowsY - 72.0;

		x = FrameX + 305.0;
		y = FrameY + 34.0;
		width = 22; height = 26.f;
	}
	else
	{
		x = FrameX + 385.f;
		y = FrameY + 2.0;
		width = 32.f; height = 38.f;
	}
#endif
	if (m_EventState == EVENT_NONE && MouseLButtonPush == false && SEASON3B::CheckMouseIn(x, y, width, height) == true)
	{
		m_EventState = EVENT_BTN_HOVER_CURRENTSKILL;
		return true;
	}
	if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL && MouseLButtonPush == false
		&& SEASON3B::CheckMouseIn(x, y, width, height) == false)
	{
		m_EventState = EVENT_NONE;
		return true;
	}
	if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL && (MouseLButtonPush == true || MouseLButtonDBClick == true)
		&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
	{
		m_EventState = EVENT_BTN_DOWN_CURRENTSKILL;
		return false;
	}
	if (m_EventState == EVENT_BTN_DOWN_CURRENTSKILL)
	{
		if (MouseLButtonPush == false && MouseLButtonDBClick == false)
		{
			if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
			{
				m_bSkillList = !m_bSkillList;
				PlayBuffer(SOUND_CLICK01);
				m_EventState = EVENT_NONE;
				return false;
			}
			m_EventState = EVENT_NONE;
			return true;
		}
	}

	if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL)
	{
		m_bRenderSkillInfo = true;
		m_iRenderSkillInfoType = Hero->CurrentSkill;
		m_iRenderSkillInfoPosX = x - 5;
		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			m_iRenderSkillInfoPosY = y - 25.0;
		}
		else
		{
			m_iRenderSkillInfoPosY = y;
		}

		return false;
	}
	else if (m_EventState == EVENT_BTN_DOWN_CURRENTSKILL)
	{
		return false;
	}
#if MAIN_UPDATE != 303
	//-- skill current hot and skill
	if (gmProtect->LookAndFeel != 1 && gmProtect->LookAndFeel != 2)
	{
		if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
		{
			x = FrameX + 331.5;
			y = FrameY + 34.0;
			width = 22.0 * 5.f; height = 26.0;
		}
		else
		{
			x = FrameX + 222.f;
			y = FrameY + 2.0;
			width = 32.f * 5.f; height = 38.f;
		}

		if (m_EventState == EVENT_NONE && MouseLButtonPush == false
			&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			m_EventState = EVENT_BTN_HOVER_SKILLHOTKEY;
			return true;
		}
		if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY && MouseLButtonPush == false
			&& SEASON3B::CheckMouseIn(x, y, width, height) == false)
		{
			m_EventState = EVENT_NONE;
			return true;
		}
		if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY && MouseLButtonPush == true
			&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			m_EventState = EVENT_BTN_DOWN_SKILLHOTKEY;
			return false;
		}

		if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
		{
			width = 22.0; height = 26.0;
		}
		else
		{
			width = 32.f; height = 38.f;
		}

		int iStartIndex = (m_bHotKeySkillListUp == true) ? 6 : 1;

		for (int i = 0, iIndex = iStartIndex; i < 5; ++i, iIndex++)
		{
			if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
			{
				x = FrameX + (width * i) + 331.5;
			}
			else
			{
				x = FrameX + (width * i) + 222.0;
			}
			if (iIndex == 10)
				iIndex = 0;

			if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
			{
				if (m_iHotKeySkillType[iIndex] == -1)
				{
					if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY)
					{
						m_bRenderSkillInfo = false;
						m_iRenderSkillInfoType = -1;
					}
					if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY && MouseLButtonPush == false)
					{
						m_EventState = EVENT_NONE;
					}
					continue;
				}

				WORD bySkillType = CharacterAttribute->Skill[m_iHotKeySkillType[iIndex]];

				if (bySkillType == 0 || (bySkillType >= AT_SKILL_STUN && bySkillType <= AT_SKILL_REMOVAL_BUFF))
					continue;

				BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;

				if (bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
				{
					continue;
				}

				if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY)
				{
					m_bRenderSkillInfo = true;
					m_iRenderSkillInfoType = m_iHotKeySkillType[iIndex];
					m_iRenderSkillInfoPosX = x - 5;
					m_iRenderSkillInfoPosY = y;
					return true;
				}
				if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY)
				{
					if (MouseLButtonPush == false)
					{
						if (m_iRenderSkillInfoType == m_iHotKeySkillType[iIndex])
						{
							m_EventState = EVENT_NONE;
							m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];
							Hero->CurrentSkill = m_iHotKeySkillType[iIndex];
							PlayBuffer(SOUND_CLICK01);
							return false;
						}
						else
						{
							m_EventState = EVENT_NONE;
						}
					}
				}
			}
		}

		if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
		{
			x = FrameX + 331.5;
			y = FrameY + 34.0;
			width = 22.0 * 5.f; height = 26.0;
		}
		else
		{
			x = FrameX + 222.f;
			y = FrameY + 2.0;
			width = 32.f * 5.f; height = 38.f;
		}

		if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY)
		{
			if (MouseLButtonPush == false && SEASON3B::CheckMouseIn(x, y, width, height) == false)
			{
				m_EventState = EVENT_NONE;
				return true;
			}
			return false;
		}
	}
#endif
	if (m_bSkillList == false)
		return true;

	WORD bySkillType = 0;

	int iSkillCount = 0;
	bool bMouseOnSkillList = false;

	float fOrigX = 0;
	x = AddMiddleX() + 385.f; width = 32; height = 38;

	if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		y = AddPositionY() + 365;
		fOrigX = AddMiddleX() + 335.f;
	}
	else
	{
		y = AddPositionY() + 390;
		fOrigX = AddMiddleX() + 385.f;
	}

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		fOrigX = PositionX_The_Mid(640.0) + 320.0;
		y = PositionY_Add(0) + 370.0;
		width = 32.0; height = 36.0;
	}

	EVENT_STATE PrevEventState = m_EventState;

	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		bySkillType = CharacterAttribute->Skill[i];

		if (bySkillType == 0 || (bySkillType >= AT_SKILL_STUN && bySkillType <= AT_SKILL_REMOVAL_BUFF))
			continue;
#if MAIN_UPDATE == 303
		if (Hero->m_pPet == NULL && bySkillType >= AT_PET_COMMAND_DEFAULT && bySkillType < AT_PET_COMMAND_END)
			continue;
		
#else
		if (bySkillType >= AT_PET_COMMAND_DEFAULT && bySkillType < AT_PET_COMMAND_END)
			continue;
#endif

		BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;

		if (bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
		{
			continue;
		}


		if (iSkillCount == 18)
		{
			y -= height;
		}

		if (iSkillCount < 14)
		{
			int iRemainder = iSkillCount % 2;
			int iQuotient = iSkillCount / 2;

			if (iRemainder == 0)
			{
				x = fOrigX + iQuotient * width;
			}
			else
			{
				x = fOrigX - (iQuotient + 1) * width;
			}
		}
		else if (iSkillCount >= 14 && iSkillCount < 18)
		{
			x = fOrigX - (8 * width) - ((iSkillCount - 14) * width);
		}
		else
		{
			x = fOrigX - (12 * width) + ((iSkillCount - 17) * width);
		}

		iSkillCount++;

		if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			bMouseOnSkillList = true;
			if (m_EventState == EVENT_NONE && MouseLButtonPush == false)
			{
				m_EventState = EVENT_BTN_HOVER_SKILLLIST;
				break;
			}
		}

		if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == true
			&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			m_EventState = EVENT_BTN_DOWN_SKILLLIST;
			break;
		}

		if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == false
			&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			m_bRenderSkillInfo = true;
			m_iRenderSkillInfoType = i;
			m_iRenderSkillInfoPosX = x;
			m_iRenderSkillInfoPosY = y;
		}

		if (m_EventState == EVENT_BTN_DOWN_SKILLLIST && MouseLButtonPush == false
			&& m_iRenderSkillInfoType == i && SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			m_EventState = EVENT_NONE;

			m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

			Hero->CurrentSkill = i;
			m_bSkillList = false;

			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	if (PrevEventState != m_EventState)
	{
		if (m_EventState == EVENT_NONE || m_EventState == EVENT_BTN_HOVER_SKILLLIST)
			return true;
		return false;
	}
#if MAIN_UPDATE != 303
	if (Hero->m_pPet != NULL)
	{
		width = 32; height = 38;

		if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
		{
			x = AddMiddleX() + 303.f;
			y = AddPositionY() + 327;
		}
		else
		{
			x = AddMiddleX() + 353.f;
			y = AddPositionY() + 352;
		}
		for (int i = AT_PET_COMMAND_DEFAULT; i < AT_PET_COMMAND_END; ++i)
		{
			if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
			{
				bMouseOnSkillList = true;

				if (m_EventState == EVENT_NONE && MouseLButtonPush == false)
				{
					m_EventState = EVENT_BTN_HOVER_SKILLLIST;
					return true;
				}
				if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == true)
				{
					m_EventState = EVENT_BTN_DOWN_SKILLLIST;
					return false;
				}

				if (m_EventState == EVENT_BTN_HOVER_SKILLLIST)
				{
					m_bRenderSkillInfo = true;
					m_iRenderSkillInfoType = i;
					m_iRenderSkillInfoPosX = x;
					m_iRenderSkillInfoPosY = y;
				}
				if (m_EventState == EVENT_BTN_DOWN_SKILLLIST && MouseLButtonPush == false
					&& m_iRenderSkillInfoType == i)
				{
					m_EventState = EVENT_NONE;

					m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

					Hero->CurrentSkill = i;
					m_bSkillList = false;
					PlayBuffer(SOUND_CLICK01);
					return false;
				}
			}
			x += width;
		}
	}
#endif
	if (bMouseOnSkillList == false && m_EventState == EVENT_BTN_HOVER_SKILLLIST)
	{
		m_EventState = EVENT_NONE;
		return true;
	}
	if (bMouseOnSkillList == false && MouseLButtonPush == false
		&& m_EventState == EVENT_BTN_DOWN_SKILLLIST)
	{
		m_EventState = EVENT_NONE;
		return false;
	}
	if (m_EventState == EVENT_BTN_DOWN_SKILLLIST)
	{
		if (MouseLButtonPush == false)
		{
			m_EventState = EVENT_NONE;
			return true;
		}
		return false;
	}

	return true;
}

bool SEASON3B::CNewUISkillList::UpdateKeyEvent()
{
	for (int i = 0; i < 9; ++i)
	{
		if (SEASON3B::IsPress('1' + i))
		{
			UseHotKey(i + 1);
		}
	}

	if (SEASON3B::IsPress('0'))
	{
		UseHotKey(0);
	}

	if (m_EventState == EVENT_BTN_HOVER_SKILLLIST)
	{
		if (SEASON3B::IsRepeat(VK_CONTROL))
		{
			for (int i = 0; i < 9; ++i)
			{
				if (SEASON3B::IsPress('1' + i))
				{
					SetHotKey(i + 1, m_iRenderSkillInfoType);

					return false;
				}
			}

			if (SEASON3B::IsPress('0'))
			{
				SetHotKey(0, m_iRenderSkillInfoType);

				return false;
			}
		}
	}

	if (SEASON3B::IsRepeat(VK_SHIFT))
	{
		for (int i = 0; i < 4; ++i)
		{
			if (SEASON3B::IsPress('1' + i))
			{
				Hero->CurrentSkill = AT_PET_COMMAND_DEFAULT + i;
				return false;
			}
		}
	}

	return true;
}

bool SEASON3B::CNewUISkillList::IsArrayUp(BYTE bySkill)
{
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == bySkill)
		{
			if (i == 0 || i > 5)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}

bool SEASON3B::CNewUISkillList::IsArrayIn(BYTE bySkill)
{
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == bySkill)
		{
			return true;
		}
	}

	return false;
}

void SEASON3B::CNewUISkillList::SetHotKey(int iHotKey, int iSkillType)
{
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == iSkillType)
		{
			m_iHotKeySkillType[i] = -1;
			break;
		}
	}

	m_iHotKeySkillType[iHotKey] = iSkillType;
}

int SEASON3B::CNewUISkillList::GetHotKey(int iHotKey)
{
	return m_iHotKeySkillType[iHotKey];
}

int SEASON3B::CNewUISkillList::GetSkillIndex(int iSkillType)
{
	int iReturn = -1;
	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		if (CharacterAttribute->Skill[i] == iSkillType)
		{
			iReturn = i;
			break;
		}
	}

	return iReturn;
}

void SEASON3B::CNewUISkillList::UseHotKey(int iHotKey)
{
	if (m_iHotKeySkillType[iHotKey] != -1)
	{
		if (m_iHotKeySkillType[iHotKey] >= AT_PET_COMMAND_DEFAULT && m_iHotKeySkillType[iHotKey] < AT_PET_COMMAND_END)
		{
			if (Hero->m_pPet == NULL)
			{
				return;
			}
		}

		WORD wHotKeySkill = CharacterAttribute->Skill[m_iHotKeySkillType[iHotKey]];

		if (wHotKeySkill == 0)
		{
			return;
		}

		m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

		Hero->CurrentSkill = m_iHotKeySkillType[iHotKey];

		WORD bySkill = CharacterAttribute->Skill[Hero->CurrentSkill];


		if (
			g_pOption->IsAutoAttack() == true
			&& gMapManager.WorldActive != WD_6STADIUM
			&& gMapManager.InChaosCastle() == false
			&& (bySkill == AT_SKILL_TELEPORT || bySkill == AT_SKILL_TELEPORT_B))
		{
			SelectedCharacter = -1;
			Attacking = -1;
		}
	}
}

bool SEASON3B::CNewUISkillList::Update()
{
	if (IsArrayIn(Hero->CurrentSkill) == true)
	{
		if (IsArrayUp(Hero->CurrentSkill) == true)
		{
			m_bHotKeySkillListUp = true;
		}
		else
		{
			m_bHotKeySkillListUp = false;
		}
	}

	if (Hero->m_pPet == NULL)
	{
		if (Hero->CurrentSkill >= AT_PET_COMMAND_DEFAULT && Hero->CurrentSkill < AT_PET_COMMAND_END)
		{
			Hero->CurrentSkill = 0;
		}
	}

	return true;
}

void SEASON3B::CNewUISkillList::RenderCurrentSkillAndHotSkillList()
{
#if MAIN_UPDATE == 303
	BYTE bySkillNumber = CharacterAttribute->SkillNumber;
	if (bySkillNumber > 0)
	{
		float RenderFrameX = PositionX_The_Mid(640.0);
		float RenderFrameY = PositionY_Add(0);

		RenderSkillIcon(Hero->CurrentSkill, RenderFrameX + 302, RenderFrameY + 451, 16.0, 20.0, false, 0.0, 0.0, true);
	}
#else
	int i;
	float x, y, width, height;

	BYTE bySkillNumber = CharacterAttribute->SkillNumber;

	if (bySkillNumber > 0)
	{
		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			float RenderFrameX = PositionX_The_Mid(640.0);
			float RenderFrameY = PositionY_Add(0);

			RenderSkillIcon(Hero->CurrentSkill, RenderFrameX + 304 + 6, RenderFrameY + 444 + 4, 20.0, 28.0, true, 20.0, 20.0, true);
		}
		else
		{
			int iStartSkillIndex = 1;
			if (m_bHotKeySkillListUp)
			{
				iStartSkillIndex = 6;
			}
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
			{
				float FrameX = PositionX_The_Mid(640.0);
				float FrameY = GetWindowsY - 72.0;

				width = 22.0;
				height = 26.0;

				x = FrameX + 331.5;
				y = FrameY + 34.0;

				for (i = 0; i < 5; ++i)
				{
					x = FrameX + (i * width) + 331.5;

					int iIndex = iStartSkillIndex + i;

					if (iIndex == 10)
						iIndex = 0;

					if (!(m_iHotKeySkillType[iIndex] == -1
						|| m_iHotKeySkillType[iIndex] >= AT_PET_COMMAND_DEFAULT && m_iHotKeySkillType[iIndex] < AT_PET_COMMAND_END && Hero->m_pPet == NULL))
					{
						RenderSkillIcon(m_iHotKeySkillType[iIndex], x + 3, y + 3, 16.0, 21.0, false);
						if (Hero->CurrentSkill == m_iHotKeySkillType[iIndex])
						{
							SEASON3B::RenderImageF(IMAGE_SKILLBOX_EX7002, x, y, 22.0, 26.0, 0.0, 0.0, 36.0, 44.0);
						}
					}
					SEASON3B::RenderNumber(x + 11, y - 5, iIndex, 0.75);
				}

				x = FrameX + 305.0;
				y = FrameY + 34.0;
				RenderSkillIcon(Hero->CurrentSkill, x + 3, y + 3, 16.0, 21.0, false);
				SEASON3B::RenderImageF(IMAGE_SKILLBOX_EX7001, x, y, 22.0, 26.0, 0.0, 0.0, 36.0, 44.0);
			}
			else
			{
				x = AddMiddleX() + 190; y = AddPositionY() + 431; width = 32; height = 38;

				for (i = 0; i < 5; ++i)
				{
					x += width;

					int iIndex = iStartSkillIndex + i;
					if (iIndex == 10)
					{
						iIndex = 0;
					}

					if (m_iHotKeySkillType[iIndex] == -1)
					{
						continue;
					}

					if (m_iHotKeySkillType[iIndex] >= AT_PET_COMMAND_DEFAULT && m_iHotKeySkillType[iIndex] < AT_PET_COMMAND_END)
					{
						if (Hero->m_pPet == NULL)
						{
							continue;
						}
					}

					if (Hero->CurrentSkill == m_iHotKeySkillType[iIndex])
					{
						SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
					}
					RenderSkillIcon(m_iHotKeySkillType[iIndex], x + 6, y + 6, 20, 28);
				}

				width = 20; x = AddMiddleX() + 392;
				height = 28; y = AddPositionY() + 437;

				RenderSkillIcon(Hero->CurrentSkill, x, y, width, height);
			}
		}
	}
#endif // MAIN_UPDATE == 303
}

bool SEASON3B::CNewUISkillList::Render()
{
	int i;
	float x, y, width, height;

	BYTE bySkillNumber = CharacterAttribute->SkillNumber;

	if (bySkillNumber > 0)
	{
		if (m_bSkillList == true)
		{
			float fOrigX = AddMiddleX() + 385.f;
			int iSkillType = 0;
			int iSkillCount = 0;

			if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
			{
				fOrigX = AddMiddleX() + 335.f;
				y = AddPositionY() + 365;
			}
			else
			{
				fOrigX = AddMiddleX() + 385.f;
				y = AddPositionY() + 390;
			}

			width = 32; height = 38;

			if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
			{
				fOrigX = PositionX_The_Mid(640.0) + 320.0;
				y = PositionY_Add(0) + 370.0;
				width = 32.0; height = 36.0;
			}

			for (i = 0; i < MAX_MAGIC; ++i)
			{
				iSkillType = CharacterAttribute->Skill[i];
#if MAIN_UPDATE == 303
				if (Hero->m_pPet == NULL && iSkillType >= AT_PET_COMMAND_DEFAULT && iSkillType < AT_PET_COMMAND_END)
					continue;

#else
				if (iSkillType >= AT_PET_COMMAND_DEFAULT && iSkillType < AT_PET_COMMAND_END)
					continue;
#endif

				if (iSkillType != 0 && (iSkillType < AT_SKILL_STUN || iSkillType > AT_SKILL_REMOVAL_BUFF))
				{
					BYTE bySkillUseType = SkillAttribute[iSkillType].SkillUseType;

					if (bySkillUseType == SKILL_USE_TYPE_MASTER || bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
					{
						continue;
					}

					if (iSkillCount == 18)
					{
						y -= height;
					}

					if (iSkillCount < 14)
					{
						int iRemainder = iSkillCount % 2;
						int iQuotient = iSkillCount / 2;

						if (iRemainder == 0)
						{
							x = fOrigX + iQuotient * width;
						}
						else
						{
							x = fOrigX - (iQuotient + 1) * width;
						}
					}
					else if (iSkillCount >= 14 && iSkillCount < 18)
					{
						x = fOrigX - (8 * width) - ((iSkillCount - 14) * width);
					}
					else
					{
						x = fOrigX - (12 * width) + ((iSkillCount - 17) * width);
					}

					iSkillCount++;

					if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
					{
						RenderSkillIcon(i, x + 6.0, y + 4.0, 20.0, 28.0, true, 20.0, 20.0, true);
					}
					else
					{
						if (i == Hero->CurrentSkill)
						{
							SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
						}
						else
						{
							SEASON3B::RenderImage(IMAGE_SKILLBOX, x, y, width, height);
						}

						RenderSkillIcon(i, x + 6, y + 6, 20, 28);
					}
				}
			}
#if MAIN_UPDATE != 303
			RenderPetSkill();
#endif
		}
	}

	if (m_bRenderSkillInfo == true && m_pNewUI3DRenderMng)
	{
		m_pNewUI3DRenderMng->RenderUI2DEffect(INVENTORY_CAMERA_Z_ORDER, UI2DEffectCallback, this, 0, 0);
	}

	return true;
}

void SEASON3B::CNewUISkillList::RenderSkillInfo()
{
	::RenderSkillInfo(m_iRenderSkillInfoPosX + 15, m_iRenderSkillInfoPosY - 10, m_iRenderSkillInfoType);
}

float SEASON3B::CNewUISkillList::GetLayerDepth()
{
	return 5.2f;
}

WORD SEASON3B::CNewUISkillList::GetHeroPriorSkill()
{
	return m_wHeroPriorSkill;
}

void SEASON3B::CNewUISkillList::SetHeroPriorSkill(BYTE bySkill)
{
	m_wHeroPriorSkill = bySkill;
}

void SEASON3B::CNewUISkillList::RenderPetSkill()
{
	if (Hero->m_pPet == NULL)
	{
		return;
	}

	float x, y, width, height;

	width = 32; height = 38;

	if (gmProtect->LookAndFeel == 3 || gmProtect->LookAndFeel == 4)
	{
		x = AddMiddleX() + 303.f;
		y = AddPositionY() + 327;
	}
	else
	{
		x = AddMiddleX() + 353.f;
		y = AddPositionY() + 352;
	}
	for (int i = AT_PET_COMMAND_DEFAULT; i < AT_PET_COMMAND_END; ++i)
	{
		if (i == Hero->CurrentSkill)
		{
			SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
		}
		else
		{
			SEASON3B::RenderImage(IMAGE_SKILLBOX, x, y, width, height);
		}

		RenderSkillIcon(i, x + 6, y + 6, 20, 28);
		x += width;
	}
}

void SEASON3B::CNewUISkillList::RenderSkillIcon(int iIndex, float x, float y, float width, float height, bool RenderNumber, float NBX, float NBY, bool Renderback)
{
	WORD bySkillType = CharacterAttribute->Skill[iIndex];

	if (bySkillType == 0)
	{
		return;
	}

	bool bCantSkill = false;
	int Skill_Icon = SkillAttribute[bySkillType].Magic_Icon;
	BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;

	if (!gSkillManager.DemendConditionCheckSkill(bySkillType))
		bCantSkill = true;

	if (!IsCanBCSkill(bySkillType))
		bCantSkill = true;

	if (g_isCharacterBuff((&Hero->Object), eBuff_AddSkill) && bySkillUseType == SKILL_USE_TYPE_BRAND)
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_SPEAR
		&& (Hero->Helper.Type < (MODEL_HELPER + 2) || Hero->Helper.Type >(MODEL_HELPER + 3))
		&& Hero->Helper.Type != (MODEL_HELPER + 37))
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_SPEAR
		&& (Hero->Helper.Type == (MODEL_HELPER + 2) || Hero->Helper.Type == (MODEL_HELPER + 3) || Hero->Helper.Type == (MODEL_HELPER + 37)))
	{
		int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
		int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;
		if ((iTypeL < ITEM_SPEAR || iTypeL >= ITEM_BOW) && (iTypeR < ITEM_SPEAR || iTypeR >= ITEM_BOW))
		{
			bCantSkill = true;
		}
	}

	if (bySkillType >= AT_SKILL_BLOCKING
		&& bySkillType <= AT_SKILL_SWORD5
		&& (Hero->Helper.Type == MODEL_HELPER + 2 || Hero->Helper.Type == MODEL_HELPER + 3 || Hero->Helper.Type == MODEL_HELPER + 37))
	{
		bCantSkill = true;
	}

	if ((bySkillType == AT_SKILL_ICE_BLADE || bySkillType == Skill_Power_Slash_Strengthener)
		&& (Hero->Helper.Type == MODEL_HELPER + 2 || Hero->Helper.Type == MODEL_HELPER + 3 || Hero->Helper.Type == MODEL_HELPER + 37))
	{
		bCantSkill = true;
	}

	int iEnergy = CharacterAttribute->Energy + CharacterAttribute->AddEnergy;

	if (g_csItemOption.IsDisableSkill(bySkillType, iEnergy))
		bCantSkill = true;


	if (bySkillType == AT_SKILL_PARTY_TELEPORT && PartyNumber <= 0)
		bCantSkill = true;


	if (bySkillType == AT_SKILL_PARTY_TELEPORT && gMapManager.IsDoppelRenewal())
		bCantSkill = true;

	if (bySkillType == AT_SKILL_DARK_HORSE || bySkillType == 510 || bySkillType == 516 || bySkillType == 512)
	{
		BYTE byDarkHorseLife = CharacterMachine->Equipment[EQUIPMENT_HELPER].Durability;
		if (byDarkHorseLife == 0 || Hero->Helper.Type != MODEL_HELPER + 4)
		{
			bCantSkill = true;
		}
	}

	if (bySkillType == 77 || bySkillType == 233 || bySkillType == 380 || bySkillType == 383 || bySkillType == 441)
	{
		if (g_csItemOption.IsDisableSkill(bySkillType, iEnergy))
			bCantSkill = true;

		if (g_isCharacterBuff((&Hero->Object), eBuff_InfinityArrow)
			|| g_isCharacterBuff((&Hero->Object), EFFECT_INFINITY_ARROW_IMPROVED)
			|| g_isCharacterBuff((&Hero->Object), eBuff_SwellOfMagicPower)
			|| g_isCharacterBuff((&Hero->Object), EFFECT_MAGIC_CIRCLE_IMPROVED)
			|| g_isCharacterBuff((&Hero->Object), EFFECT_MAGIC_CIRCLE_ENHANCED)
			)
		{
			bCantSkill = true;
		}
	}

	if (bySkillType == 55 || bySkillType == 490)
	{
		DWORD Strength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;

		if (Strength < 596)
			bCantSkill = true;

		int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
		int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;

		if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
		{
			bCantSkill = true;
		}
	}

	if ((bySkillType == AT_SKILL_PARALYZE || bySkillType == Skill_Ice_Arrow_Strengthener)
		&& (CharacterAttribute->Dexterity + CharacterAttribute->AddDexterity) < 646)
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_WHEEL || bySkillType == Skill_Twisting_Slash_Strengthener || bySkillType == Skill_Twisting_Slash_Strengthener2 || bySkillType == Skill_Twisting_Slash_Mastery)
	{
		int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
		int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;

		if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
		{
			bCantSkill = true;
		}
	}

	if (gMapManager.InChaosCastle() == true)
	{
		if (bySkillType == AT_SKILL_DARK_HORSE
			|| bySkillType == AT_SKILL_RIDER
			|| bySkillType == Skill_Dark_Horse_Strengthener
			|| bySkillType == Skill_Earthshake_Mastery
			|| (bySkillType >= AT_PET_COMMAND_DEFAULT && bySkillType <= AT_PET_COMMAND_TARGET)
			|| bySkillType == Skill_Earthshake_Strengthener)
		{
			bCantSkill = true;
		}
	}
	else if (bySkillType == AT_SKILL_DARK_HORSE || bySkillType == Skill_Dark_Horse_Strengthener || bySkillType == Skill_Earthshake_Mastery || bySkillType == Skill_Earthshake_Strengthener)
	{
		BYTE byDarkHorseLife = CharacterMachine->Equipment[EQUIPMENT_HELPER].Durability;
		if (byDarkHorseLife == 0)
		{
			bCantSkill = true;
		}
	}

	DWORD iCharisma = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;

	if (g_csItemOption.IsDisableSkill(bySkillType, iEnergy, iCharisma))
		bCantSkill = true;

	if (!g_CMonkSystem.IsSwordformGlovesUseSkill(bySkillType))
		bCantSkill = true;

	if (g_CMonkSystem.IsRideNotUseSkill(bySkillType, Hero->Helper.Type))
		bCantSkill = true;

	ITEM* pLeftRing = &CharacterMachine->Equipment[EQUIPMENT_RING_LEFT];
	ITEM* pRightRing = &CharacterMachine->Equipment[EQUIPMENT_RING_RIGHT];

	if (g_CMonkSystem.IsChangeringNotUseSkill(pLeftRing->Type, pRightRing->Type, pLeftRing->Level, pRightRing->Level)
		&& gCharacterManager.GetBaseClass(Hero->Class) == CLASS_RAGEFIGHTER)
	{
		bCantSkill = true;
	}

	float fU = 0.0;
	float fV = 0.0;
	int iKindofSkill = 0;

	if (!g_csItemOption.Special_Option_Check(bySkillType))
		bCantSkill = true;

	if (!g_csItemOption.Special_Option_Check(bySkillType))
		bCantSkill = true;

	float uWidth = 20.0;
	float vHeight = 28.0;

	if ((signed int)bySkillType < Skill_PetRaven_Default || (signed int)bySkillType > Skill_PetRaven_Target+1)
	{
		if (bySkillType == 76)
		{
			fU = uWidth * 4.0 / 256.0;
			fV = 0.0;
			iKindofSkill = KOS_COMMAND;
		}
		else if ((signed int)bySkillType < Skill_Drain_Life || (signed int)bySkillType > Skill_Damage_Reflection)
		{
			if ((signed int)bySkillType < Skill_Sleep || (signed int)bySkillType > Skill_Sleep+1)
			{
				if (bySkillType == Skill_Berserker)
				{
					fU = uWidth * 10.0 / 256.0;
					fV = vHeight * 3.0 / 256.0;
					iKindofSkill = KOS_SKILL2;
				}
				else if ((signed int)bySkillType < Skill_Weakness || (signed int)bySkillType > Skill_Innovation)
				{
					if ((signed int)bySkillType < Skill_Explosion_Sahamutt || (signed int)bySkillType > Skill_Requiem)
					{
						if (bySkillType == Skill_Pollution)
						{
							fU = uWidth * 11.0 / 256.0;
							fV = vHeight * 3.0 / 256.0;
							iKindofSkill = KOS_SKILL2;
						}
						else if (bySkillType == Skill_Strike_of_Destruction)
						{
							fU = uWidth * 7.0 / 256.0;
							fV = vHeight * 2.0 / 256.0;
							iKindofSkill = KOS_SKILL2;
						}
						else if (bySkillType != Skill_Chaotic_Diseier_Strengthener)
						{
							switch (bySkillType)
							{
							case Skill_Chaotic_Diseier:
								fU = uWidth * 3.0 / 256.0;
								fV = vHeight * 8.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
								break;
							case Skill_Recovery:
								fU = uWidth * 9.0 / 256.0;
								fV = vHeight * 2.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
								break;
							case Skill_Multi_Shot:
								if (gCharacterManager.GetEquipedBowType_Skill() == BOWTYPE_NONE)
									bCantSkill = 1;
								fU = uWidth * 0.0 / 256.0;
								fV = vHeight * 8.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
								break;
							case Skill_Flame_Strike:
							{
								int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
								int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;
								if (iTypeR == -1 || iTypeR >= ITEM_STAFF && iTypeR < ITEM_STAFF + MAX_ITEM_INDEX || iTypeL >= iTypeR && iTypeL < ITEM_STAFF + MAX_ITEM_INDEX)
									bCantSkill = true;
								fU = uWidth * 1.0 / 256.0;
								fV = vHeight * 8.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
							}
							break;
							case Skill_Gigantic_Storm:
								fU = uWidth * 2.0 / 256.0;
								fV = vHeight * 8.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
								break;
							case Skill_Lightning_Shock:
								fU = uWidth * 2.0 / 256.0;
								fV = vHeight * 3.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
								break;
							case Skill_Chain_Lightning_Strengthener:
								fU = uWidth * 6.0 / 256.0;
								fV = vHeight * 8.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
								break;
							case Skill_Expansion_of_Wizardry:
								fU = uWidth * 8.0 / 256.0;
								fV = vHeight * 2.0 / 256.0;
								iKindofSkill = KOS_SKILL2;
								break;
							default:
								if (bySkillUseType == Skill_Fire_Ball)
								{
									fU = uWidth / 256.0 * (double)(Skill_Icon % 12);
									fV = vHeight / 256.0 * (double)(Skill_Icon / 12 + 4);
									iKindofSkill = KOS_SKILL2;
								}
								else if ((signed int)bySkillType < Skill_Killing_Blow)
								{
									if ((signed int)bySkillType < 57)
									{
										fU = (double)((bySkillType - 1) % 8) * uWidth / 256.0;
										fV = (double)((bySkillType - 1) / 8) * vHeight / 256.0;
										iKindofSkill = KOS_SKILL1;
									}
									else
									{
										fU = (double)((bySkillType - 57) % 8) * uWidth / 256.0;
										fV = (double)((bySkillType - 57) / 8) * vHeight / 256.0;
										iKindofSkill = KOS_SKILL2;
									}
								}
								else
								{
									fU = (double)((bySkillType - 260) % 12) * uWidth / 256.0;
									fV = (double)((bySkillType - 260) / 12) * vHeight / 256.0;
									iKindofSkill = KOS_SKILL3;
								}
								break;
							}
						}
					}
					else
					{
						fU = (double)((bySkillType - 217) % 8) * uWidth / 256.0;
						fV = vHeight * 3.0 / 256.0;
						iKindofSkill = KOS_SKILL2;
					}
				}
				else
				{
					fU = (double)(bySkillType - 213) * uWidth / 256.0;
					fV = vHeight * 3.0 / 256.0;
					iKindofSkill = KOS_SKILL2;
				}
			}
			else
			{
				fU = (double)((bySkillType - 215) % 8) * uWidth / 256.0;
				fV = vHeight * 3.0 / 256.0;
				iKindofSkill = KOS_SKILL2;
			}
		}
		else
		{
			fU = (double)((bySkillType - 214) % 8) * uWidth / 256.0;
			fV = vHeight * 3.0 / 256.0;
			iKindofSkill = KOS_SKILL2;
		}
	}
	else
	{
		fU = (double)((bySkillType - 120) % 8) * uWidth / 256.0;
		fV = (double)((bySkillType - 120) / 8) * vHeight / 256.0;
		iKindofSkill = KOS_COMMAND;
	}

	if (bySkillUseType == SKILL_USE_TYPE_MASTERACTIVE)
		iKindofSkill = KOS_SKILL4;

	int iSkillIndex = 0;
	switch (iKindofSkill)
	{
	case KOS_COMMAND:
		iSkillIndex = IMAGE_COMMAND;
		if (bCantSkill == 1)
			iSkillIndex = IMAGE_NON_COMMAND;
		break;
	case KOS_SKILL1:
		iSkillIndex = IMAGE_SKILL1;
		if (bCantSkill == 1)
			iSkillIndex = IMAGE_NON_SKILL1;
		break;
	case KOS_SKILL2:
		iSkillIndex = IMAGE_SKILL2;
		if (bCantSkill == 1)
			iSkillIndex = IMAGE_NON_SKILL2;
		break;
	case KOS_SKILL3:
		iSkillIndex = IMAGE_SKILL3;
		if (bCantSkill == 1)
			iSkillIndex = IMAGE_NON_SKILL3;
		break;
	case KOS_SKILL4:
		iSkillIndex = IMAGE_MASTER_SKILL;
		if (bCantSkill == 1)
			iSkillIndex = IMAGE_NON_MASTER_SKILL;
		break;
	default:
		break;
	}
	if (iSkillIndex)
	{
		float uw = 0.0;
		float vh = 0.0;
		if (bySkillUseType == 4)
		{
			fU = uWidth / 512.0 * (double)(Skill_Icon % 25);
			fV = vHeight / 512.0 * (double)(Skill_Icon / 25);
			uw = uWidth / 512.0;
			vh = vHeight / 512.0;
			RenderBitmap(iSkillIndex, x, y, width, height, fU, fV, uw, vh, true, true, 0.0);
		}
		else
		{
			uw = uWidth / 256.0;
			vh = vHeight / 256.0;

			if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
			{
				if (bCantSkill)
					glColor3f(1.0, 0.5, 0.5);
				else
					glColor3f(1.0, 0.89999998, 0.80000001);

				if (Renderback)
				{
					float percent = height / 28.0;
					float nwidth = 32.0 * percent;
					float nhight = 36.0 * percent;

					SEASON3B::RenderImage(IMAGE_SKILLBOX, x + ((width - nwidth) / 2.f), y + ((height - nhight) / 2.f), nwidth, nhight, 0.0, 0.0, 1.0, 36/64.0);
				}
			}
			RenderBitmap(iSkillIndex, x, y, width, height, fU, fV, uw, vh, true, true, 0.0);
		}
	}

	int iHotKey = -1;
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == iIndex)
		{
			iHotKey = i;
			break;
		}
	}

	if (iHotKey != -1 && RenderNumber == TRUE)
	{
		glColor3f(1.f, 0.9f, 0.8f);
		SEASON3B::RenderNumber(x + NBX, y + NBY, iHotKey);
	}

	if ((bySkillType != 262 && bySkillType != 265 && bySkillType != 264 && bySkillType != 558 && bySkillType != 560 || !bCantSkill)
		&& bySkillType != 77
		&& bySkillType != 233
		&& bySkillType != 380
		&& bySkillType != 383
		&& bySkillType != 441)
	{
		RenderSkillDelay(iIndex, x, y, width, height);
	}
}

void SEASON3B::CNewUISkillList::RenderSkillDelay(int iIndex, float x, float y, float width, float height)
{
	int iSkillDelay = CharacterAttribute->SkillDelay[iIndex];

	if (iSkillDelay > 0)
	{
		int iSkillType = CharacterAttribute->Skill[iIndex];

		int iSkillMaxDelay = SkillAttribute[iSkillType].Delay;

		float fPersent = (float)(iSkillDelay / (float)iSkillMaxDelay);

		EnableAlphaTest();
		glColor4f(1.f, 0.5f, 0.5f, 0.5f);
		float fdeltaH = height * fPersent;
		RenderColor(x, y + height - fdeltaH, width, fdeltaH);
		EndRenderColor();
	}
}

bool SEASON3B::CNewUISkillList::IsSkillListUp()
{
	return m_bHotKeySkillListUp;
}

void SEASON3B::CNewUISkillList::ResetMouseLButton()
{
	MouseLButton = false;
	MouseLButtonPop = false;
	MouseLButtonPush = false;
}

void SEASON3B::CNewUISkillList::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
	if (pClass)
	{
		CNewUISkillList* pSkillList = (CNewUISkillList*)(pClass);
		pSkillList->RenderSkillInfo();
	}
}

void SEASON3B::CNewUIMainFrameWindow::SetPreExp_Wide(__int64 dwPreExp)
{
	m_loPreExp = dwPreExp;
}

void SEASON3B::CNewUIMainFrameWindow::SetGetExp_Wide(__int64 dwGetExp)
{
	m_loGetExp = dwGetExp;

	if (m_loGetExp > 0)
	{
		m_bExpEffect = true;
		m_dwExpEffectTime = timeGetTime();
	}
}

void SEASON3B::CNewUIMainFrameWindow::SetPreExp(DWORD dwPreExp)
{
	m_dwPreExp = dwPreExp;
}

void SEASON3B::CNewUIMainFrameWindow::SetGetExp(DWORD dwGetExp)
{
	m_dwGetExp = dwGetExp;

	if (m_dwGetExp > 0)
	{
		m_bExpEffect = true;
		m_dwExpEffectTime = timeGetTime();
	}
}

void SEASON3B::CNewUIMainFrameWindow::SetBtnState(int iBtnType, bool bStateDown)
{
#if MAIN_UPDATE != 303
	switch (iBtnType)
	{
#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
	case MAINFRAME_BTN_PARTCHARGE:
	{
		if (gmProtect->LookAndFeel != 1 && gmProtect->LookAndFeel != 2)
		{
			if (bStateDown)
			{
				m_BtnCShop.UnRegisterButtonState();
				m_BtnCShop.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CSHOP, 2);
				m_BtnCShop.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CSHOP, 3);
				m_BtnCShop.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CSHOP, 2);
				m_BtnCShop.ChangeImgIndex(IMAGE_MENU_BTN_CSHOP, 2);
			}
			else
			{
				m_BtnCShop.UnRegisterButtonState();
				m_BtnCShop.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CSHOP, 0);
				m_BtnCShop.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CSHOP, 1);
				m_BtnCShop.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CSHOP, 2);
				m_BtnCShop.ChangeImgIndex(IMAGE_MENU_BTN_CSHOP, 0);
			}
		}
	}
	break;
#endif //defined defined PBG_ADD_INGAMESHOP_UI_MAINFRAME
	case MAINFRAME_BTN_CHAINFO:
	{
		if (bStateDown)
		{
			m_BtnChaInfo.UnRegisterButtonState();
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CHAINFO, 2);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CHAINFO, 3);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CHAINFO, 2);
			m_BtnChaInfo.ChangeImgIndex(IMAGE_MENU_BTN_CHAINFO, 2);

		}
		else
		{
			m_BtnChaInfo.UnRegisterButtonState();
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CHAINFO, 0);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CHAINFO, 1);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CHAINFO, 2);
			m_BtnChaInfo.ChangeImgIndex(IMAGE_MENU_BTN_CHAINFO, 0);
		}
	}
	break;
	case MAINFRAME_BTN_MYINVEN:
	{
		if (bStateDown)
		{
			m_BtnMyInven.UnRegisterButtonState();
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_MYINVEN, 2);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_MYINVEN, 3);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_MYINVEN, 2);
			m_BtnMyInven.ChangeImgIndex(IMAGE_MENU_BTN_MYINVEN, 2);
		}
		else
		{
			m_BtnMyInven.UnRegisterButtonState();
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_MYINVEN, 0);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_MYINVEN, 1);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_MYINVEN, 2);
			m_BtnMyInven.ChangeImgIndex(IMAGE_MENU_BTN_MYINVEN, 0);
		}
	}
	break;
	case MAINFRAME_BTN_FRIEND:
	{
		if (gmProtect->LookAndFeel != 1)
		{
			if (bStateDown)
			{
				m_BtnFriend.UnRegisterButtonState();
				m_BtnFriend.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_FRIEND, 2);
				m_BtnFriend.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_FRIEND, 3);
				m_BtnFriend.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_FRIEND, 2);
				m_BtnFriend.ChangeImgIndex(IMAGE_MENU_BTN_FRIEND, 2);
			}
			else
			{
				m_BtnFriend.UnRegisterButtonState();
				m_BtnFriend.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_FRIEND, 0);
				m_BtnFriend.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_FRIEND, 1);
				m_BtnFriend.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_FRIEND, 2);
				m_BtnFriend.ChangeImgIndex(IMAGE_MENU_BTN_FRIEND, 0);
			}
		}
	}
	break;
	case MAINFRAME_BTN_WINDOW:
	{
		if (gmProtect->LookAndFeel != 1)
		{
			if (bStateDown)
			{
				m_BtnWindow.UnRegisterButtonState();
				m_BtnWindow.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_WINDOW, 2);
				m_BtnWindow.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_WINDOW, 3);
				m_BtnWindow.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_WINDOW, 2);
				m_BtnWindow.ChangeImgIndex(IMAGE_MENU_BTN_WINDOW, 2);
			}
			else
			{
				m_BtnWindow.UnRegisterButtonState();
				m_BtnWindow.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_WINDOW, 0);
				m_BtnWindow.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_WINDOW, 1);
				m_BtnWindow.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_WINDOW, 2);
				m_BtnWindow.ChangeImgIndex(IMAGE_MENU_BTN_WINDOW, 0);
			}
		}
	}
	break;
	case MAINFRAME_BTN_GUILD:
	{
		if (gmProtect->LookAndFeel >= 1 && gmProtect->LookAndFeel <= 4)
		{
			if (bStateDown)
			{
				m_BtnWinGuild.UnRegisterButtonState();
				m_BtnWinGuild.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_GUILD, 2);
				m_BtnWinGuild.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_GUILD, 3);
				m_BtnWinGuild.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_GUILD, 2);
				m_BtnWinGuild.ChangeImgIndex(IMAGE_MENU_BTN_GUILD, 2);
			}
			else
			{
				m_BtnWinGuild.UnRegisterButtonState();
				m_BtnWinGuild.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_GUILD, 0);
				m_BtnWinGuild.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_GUILD, 1);
				m_BtnWinGuild.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_GUILD, 2);
				m_BtnWinGuild.ChangeImgIndex(IMAGE_MENU_BTN_GUILD, 0);
			}
		}
	}
	break;
	case MAINFRAME_BTN_PARTY:
	{
		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			if (bStateDown)
			{
				m_BtnWinParty.UnRegisterButtonState();
				m_BtnWinParty.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_PARTY, 2);
				m_BtnWinParty.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_PARTY, 3);
				m_BtnWinParty.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_PARTY, 2);
				m_BtnWinParty.ChangeImgIndex(IMAGE_MENU_BTN_PARTY, 2);
			}
			else
			{
				m_BtnWinParty.UnRegisterButtonState();
				m_BtnWinParty.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_PARTY, 0);
				m_BtnWinParty.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_PARTY, 1);
				m_BtnWinParty.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_PARTY, 2);
				m_BtnWinParty.ChangeImgIndex(IMAGE_MENU_BTN_PARTY, 0);
			}
		}
	}
	break;
	}
#endif // MAIN_UPDATE != 303

}
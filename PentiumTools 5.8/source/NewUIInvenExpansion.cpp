#include "stdafx.h"
#include "DSPlaySound.h"
#include "NewUISystem.h"
#include "wsclientinline.h"
#include "NewUIInvenExpansion.h"

SEASON3B::CInvenOrgInterface::CInvenOrgInterface()
{
	this->InterfaceList = 0;
	this->posX = 0.0f;
	this->posY = 0.0f;
	this->myPosX = 0.0f;
	this->myPosY = 0.0f;
}

SEASON3B::CInvenOrgInterface::~CInvenOrgInterface()
{
}

void SEASON3B::CInvenOrgInterface::SetOrgInvenPos(int InterfaceList, float posX, float posY, float myPosX, float myPosY)
{
	this->InterfaceList = InterfaceList;
	this->posX = posX;
	this->posY = posY;
	this->myPosX = myPosX;
	this->myPosY = myPosY;
}

float SEASON3B::CInvenOrgInterface::GetOrgInvenPosX()
{
	return this->posX;
}

float SEASON3B::CInvenOrgInterface::GetOrgInvenPosY()
{
	return this->posY;
}

float SEASON3B::CInvenOrgInterface::GetOrgYourInvenPosX()
{
	return this->myPosX;
}

float SEASON3B::CInvenOrgInterface::GetOrgYourInvenPosY()
{
	return this->myPosY;
}

int SEASON3B::CInvenOrgInterface::GetInterfaceList()
{
	return this->InterfaceList;
}

SEASON3B::CNewUIInvenExpansion::CNewUIInvenExpansion()
{
	m_pNewUIMng = NULL;

	for (int i = 0; i < MAX_INVENT_EXPANSION; ++i)
	{
		m_InvBoxPos[i].x = 0;
		m_InvBoxPos[i].y = 0;
		isExtEnabled[i] = false;
	}

	m_Pos.x = 0;
	m_Pos.y = 0;

	ThirdInterface = false;

	InvenOrgInterface.clear();

	m_List.clear();

	RegLinkageUI();
}

SEASON3B::CNewUIInvenExpansion::~CNewUIInvenExpansion()
{
	Release();
}

bool SEASON3B::CNewUIInvenExpansion::Create(CNewUIManager* pUIManager, int posX, int posY)
{
	if (pUIManager && g_pNewItemMng)
	{
		m_pNewUIMng = pUIManager;

		m_pNewUIMng->AddUIObj(INTERFACE_INVENTORY_EXTENSION, this);

		for (int i = 0; i < MAX_INVENT_EXPANSION; i++)
		{
			m_InvBoxPos[i].x = posX + 14;

			m_InvBoxPos[i].y = posY + 87 * i + 46;
		}

		SetButtonInfo();

		SetPos(posX, posY);

		LoadImages();

		Show(false);

		return true;
	}

	return false;
}

void SEASON3B::CNewUIInvenExpansion::Release()
{
	InvenOrgInterface.clear();

	m_List.clear();

	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
	}
}

void SEASON3B::CNewUIInvenExpansion::SetPos(int posX, int posY)
{
	m_Pos.x = posX;
	m_Pos.y = posY;

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		SetButtonPosition(&m_BtnExit, m_Pos.x, m_Pos.y, 1);
	}
	else
	{
		m_BtnExit.SetPos(m_Pos.x + 13, m_Pos.y + 391);
	}

	for (int i = 0; i < MAX_INVENT_EXPANSION; i++)
	{
		m_InvBoxPos[i].x = posX + 14;
		m_InvBoxPos[i].y = posY + 87 * i + 46;
	}
	g_pMyInventory->SetInvenPos();

	if (IsVisible() == false)
		return;

	for (int i = 1; i < g_pMyInventory->GetInvenEnableCnt(); i++)
	{
		g_pMyInventory->UnlockInventory(i);

		g_pMyInventory->GetInventoryCtrl(i)->ShowInventory();
	}
}

void SEASON3B::CNewUIInvenExpansion::SetButtonInfo()
{
	unicode::t_char pszText[100];

	unicode::_sprintf(pszText, GlobalText[927], "K");
	m_BtnExit.ChangeToolTipText(pszText, true);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		SetButtonInterface(&m_BtnExit, m_Pos.x, m_Pos.y, 1);
	}
	else
	{
		m_BtnExit.ChangeButtonImgState(true, CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN, false);
		m_BtnExit.ChangeButtonInfo(m_Pos.x + 13, m_Pos.y + 391, 36, 29);
	}
}

bool SEASON3B::CNewUIInvenExpansion::Render()
{
	EnableAlphaTest();

	glColor4f(1.0, 1.0, 1.0, 1.0);

	RenderFrame();

	RenderTexts();

	RenderInvenExpansion();

	RenderButtons();

	DisableAlphaBlend();

	return true;
}

bool SEASON3B::CNewUIInvenExpansion::Update()
{
	return true;
}

bool SEASON3B::CNewUIInvenExpansion::UpdateMouseEvent()
{
	bool success = false;

	if (BtnProcess() == true)
		return false;

	if (CheckMouseIn(m_Pos.x, m_Pos.y, 190, 429))
	{

		/*if (IsPress(VK_RBUTTON))
		{
			ResetRButton();
		}
		else
		{
			if (!g_pMyInventory->GetRepairMode())
				ResetLButton();
		}*/
	}

	return true;
}

bool SEASON3B::CNewUIInvenExpansion::UpdateKeyEvent()
{
	if (g_pNewUISystem->IsVisible(INTERFACE_INVENTORY_EXTENSION) && IsPress(VK_ESCAPE))
	{
		if (!g_pNPCShop->IsSellingItem())
		{
			g_pNewUISystem->Hide(INTERFACE_INVENTORY);
			g_pNewUISystem->Hide(INTERFACE_INVENTORY_EXTENSION);
			PlayBuffer(SOUND_CLICK01);
		}
	}
	return true;
}

float SEASON3B::CNewUIInvenExpansion::GetLayerDepth()
{
	return 4.25;
}

bool SEASON3B::CNewUIInvenExpansion::BtnProcess()
{
	int PosX = GetPos()->x + 169;

	int PosY = GetPos()->y + 7;

	if (IsRelease(VK_LBUTTON) == true && CheckMouseIn(PosX, PosY, 13, 12) == true)
	{
		g_pNewUISystem->Hide(INTERFACE_INVENTORY_EXTENSION);
		return true;
	}

	if (m_BtnExit.UpdateMouseEvent())
	{
		g_pNewUISystem->Hide(INTERFACE_INVENTORY_EXTENSION);
		return true;
	}

	return false;
}

void SEASON3B::CNewUIInvenExpansion::ResetLButton()
{
	MouseLButton = false;

	MouseLButtonPop = false;

	MouseLButtonPush = false;
}

void SEASON3B::CNewUIInvenExpansion::ResetRButton()
{
	MouseRButton = false;

	MouseRButtonPop = false;

	MouseRButtonPush = false;
}

bool SEASON3B::CNewUIInvenExpansion::CheckExpansionInventory()
{
	if (IsVisible())
		return CheckMouseIn(m_Pos.x, m_Pos.y, 190, 429) != false;
	else
		return false;
}

void SEASON3B::CNewUIInvenExpansion::InitExpansion()
{
	for (int i = 0; i < MAX_INVENT_EXPANSION; i++)
	{
		isExtEnabled[i] = false;
	}
}

void SEASON3B::CNewUIInvenExpansion::LoadImages()
{
	LoadBitmap("Interface\\newui_msgbox_back.jpg", CNewUIMyInventory::IMAGE_INVENTORY_BACK, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_back04.tga", CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP2, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_back02-L.tga", CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_back02-R.tga", CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_back03.tga", CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_exit_00.tga", CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_add_table.tga", item_add_table, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_add_marking_non.JPG", item_add_marking_non, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_add_marking_no01.tga", item_add_marking_no01, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_add_marking_no02.tga", item_add_marking_no02, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_add_marking_no03.tga", item_add_marking_no03, GL_LINEAR, GL_CLAMP, true, false);
	LoadBitmap("Interface\\newui_item_add_marking_no04.tga", item_add_marking_no04, GL_LINEAR, GL_CLAMP, true, false);
}

void SEASON3B::CNewUIInvenExpansion::UnloadImages()
{
	DeleteBitmap(CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN);
	DeleteBitmap(CNewUIMyInventory::IMAGE_INVENTORY_BACK);
	DeleteBitmap(CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP2);
	DeleteBitmap(CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT);
	DeleteBitmap(CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT);
	DeleteBitmap(CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM);
	for (int i = item_add_table; i <= item_add_marking_no04; ++i)
	{
		DeleteBitmap(i);
	}
}

void SEASON3B::CNewUIInvenExpansion::RenderFrame()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		RenderInventoryInterface(m_Pos.x, m_Pos.y, 0);
	}
	else
	{
		RenderImage(CNewUIMyInventory::IMAGE_INVENTORY_BACK, this->m_Pos.x, this->m_Pos.y, 190.0, 429.0);

		RenderImage(CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP2, this->m_Pos.x, this->m_Pos.y, 190.0, 64.0);

		RenderImage(CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT, (float)this->m_Pos.x, (float)(this->m_Pos.y + 64), 21.0, 320.0);

		RenderImage(CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT, (float)(this->m_Pos.x + 169), (float)(this->m_Pos.y + 64), 21.0, 320.0);

		RenderImage(CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM, (float)this->m_Pos.x, (float)(this->m_Pos.y + 384), 190.0, 45.0);
	}
}

void SEASON3B::CNewUIInvenExpansion::RenderTexts()
{
	g_pRenderText->SetFont(g_hFontBold);

	g_pRenderText->SetBgColor(0, 0, 0, 0);

	g_pRenderText->SetTextColor(255, 255, 255, 0xFFu);

	g_pRenderText->RenderText(GetPos()->x, GetPos()->y + 15, GlobalText[3323], 190, 0, 3, 0);
}

void SEASON3B::CNewUIInvenExpansion::RenderButtons()
{
	m_BtnExit.Render();
}

void SEASON3B::CNewUIInvenExpansion::RenderInvenExpansion()
{
	g_pMyInventory->RenderExpansionInven();

	bool onlyRender = false;

	for (int i = 0; i < MAX_INVENT_EXPANSION; i++)
	{
		RenderImage(item_add_table, (float)(m_InvBoxPos[i].x - 4), (float)(m_InvBoxPos[i].y - 3), 173.0, 87.0);

		if (isExtEnabled[i] != false)
		{
			continue;
		}
		RenderImage(item_add_marking_non, (float)m_InvBoxPos[i].x, (float)m_InvBoxPos[i].y, 161.0, 81.0);

		RenderImage(i + item_add_marking_no01, (float)(m_InvBoxPos[i].x + 68), (float)(m_InvBoxPos[i].y + 26), 25.0, 28.0);
	}
}

void SEASON3B::CNewUIInvenExpansion::OpeningProcess()
{
	g_pMyInventory->SetExpansionOpenState();
}

void SEASON3B::CNewUIInvenExpansion::ClosingProcess()
{
	g_pMyInventory->SetExpansionCloseState();

	InitMultiPosition();

	g_pMyInventory->SetInvenPos();
}

void SEASON3B::CNewUIInvenExpansion::InitMultiPosition()
{
	int WinOpen2 = (int)(GetWindowsX - 380);

	g_pMyShopInventory->SetPos(WinOpen2, 0);

	g_pInventoryJewel->SetPos(WinOpen2, 0);

	int PosY = this->GetUIPosition(INTERFACE_MYSHOP_INVENTORY, 0)->y;

	int PosX = this->GetUIPosition(INTERFACE_MYSHOP_INVENTORY, 0)->x + WinOpen2;

	g_pMyShopInventory->GetInventoryCtrl()->SetPos(PosX, PosY);

	g_pMixInventory->SetPos(WinOpen2, 0);

	PosY = this->GetUIPosition(INTERFACE_MIXINVENTORY, 0)->y;

	PosX = this->GetUIPosition(INTERFACE_MIXINVENTORY, 0)->x + WinOpen2;

	g_pMixInventory->GetInventoryCtrl()->SetPos(PosX, PosY);

	g_pNPCShop->SetPos(WinOpen2, 0);

	PosY = this->GetUIPosition(INTERFACE_NPCSHOP, 0)->y;

	PosX = this->GetUIPosition(INTERFACE_NPCSHOP, 0)->x + WinOpen2;

	g_pNPCShop->GetInventoryCtrl()->SetPos(PosX, PosY);

	g_pPurchaseShopInventory->SetPos(WinOpen2, 0);

	PosY = this->GetUIPosition(INTERFACE_PURCHASESHOP_INVENTORY, 0)->y;

	PosX = this->GetUIPosition(INTERFACE_PURCHASESHOP_INVENTORY, 0)->x + WinOpen2;

	g_pPurchaseShopInventory->GetInventoryCtrl()->SetPos(PosX, PosY);

	//g_pLuckyItemWnd->SetPos(WinOpen2, 0);

	PosY = this->GetUIPosition(INTERFACE_LUCKYITEMWND, 0)->y;

	PosX = this->GetUIPosition(INTERFACE_LUCKYITEMWND, 0)->x + WinOpen2;

	//g_pLuckyItemWnd->GetInventoryCtrl()->SetPos(PosX, PosY);

	g_pStorageInventory->SetPos(WinOpen2, 0);

	PosY = this->GetUIPosition(INTERFACE_STORAGE, 0)->y;

	PosX = this->GetUIPosition(INTERFACE_STORAGE, 0)->x + WinOpen2;

	g_pStorageInventory->GetInventoryCtrl()->SetPos(PosX, PosY);

	g_pTrade->SetPos(WinOpen2, 0);

	PosY = this->GetUIPosition(INTERFACE_TRADE, 1)->y;

	PosX = this->GetUIPosition(INTERFACE_TRADE, 1)->x + WinOpen2;

	g_pTrade->GetYourInvenCtrl()->SetPos(PosX, PosY);

	PosY = this->GetUIPosition(INTERFACE_TRADE, 0)->y;

	PosX = this->GetUIPosition(INTERFACE_TRADE, 0)->x + WinOpen2;

	g_pTrade->GetMyInvenCtrl()->SetPos(PosX, PosY);

	this->ThirdInterface = false;

	this->SetPos(WinOpen2, 0);
}

bool SEASON3B::CNewUIInvenExpansion::SetThirdPosition(DWORD winIndex)
{
	int WinOpen2 = (int)(GetWindowsX - 380);
	int WinOpen3 = (int)(GetWindowsX - 570);

	SetPos(WinOpen2, 0);

	int ThirdIndex = IsThirdInterface(winIndex);

	switch (ThirdIndex)
	{
	case INTERFACE_TRADE:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		g_pTrade->SetPos(WinOpen3, 0);
		g_pTrade->GetMyInvenCtrl()->SetPos(GetUIPosition(ThirdIndex, 0)->x + WinOpen3, GetUIPosition(ThirdIndex, 0)->y);
		g_pTrade->GetYourInvenCtrl()->SetPos(GetUIPosition(ThirdIndex, 1)->x + WinOpen3, GetUIPosition(ThirdIndex, 1)->y);
		break;
	case INTERFACE_STORAGE:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		g_pStorageInventory->SetPos(WinOpen3, 0);
		g_pStorageInventory->GetInventoryCtrl()->SetPos(GetUIPosition(ThirdIndex, 0)->x + WinOpen3, GetUIPosition(ThirdIndex, 0)->y);
		break;
	case INTERFACE_MIXINVENTORY:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		g_pMixInventory->SetPos(WinOpen3, 0);
		g_pMixInventory->GetInventoryCtrl()->SetPos(GetUIPosition(ThirdIndex, 0)->x + WinOpen3, GetUIPosition(ThirdIndex, 0)->y);
		break;
	case INTERFACE_NPCSHOP:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		g_pNPCShop->SetPos(WinOpen3, 0);
		g_pNPCShop->GetInventoryCtrl()->SetPos(GetUIPosition(ThirdIndex, 0)->x + WinOpen3, GetUIPosition(ThirdIndex, 0)->y);
		break;
	case INTERFACE_MYSHOP_INVENTORY:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		g_pMyShopInventory->SetPos(WinOpen3, 0);
		g_pMyShopInventory->GetInventoryCtrl()->SetPos(GetUIPosition(ThirdIndex, 0)->x + WinOpen3, GetUIPosition(ThirdIndex, 0)->y);
		break;
	case INTERFACE_PURCHASESHOP_INVENTORY:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		g_pPurchaseShopInventory->SetPos(WinOpen3, 0);
		g_pPurchaseShopInventory->GetInventoryCtrl()->SetPos(GetUIPosition(ThirdIndex, 0)->x + WinOpen3, GetUIPosition(ThirdIndex, 0)->y);
		break;
	case INTERFACE_CHARACTER:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		SetPos(WinOpen3, 0);
		break;
	case INTERFACE_LUCKYITEMWND:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		//g_pLuckyItemWnd->SetPos(WinOpen3, 0);
		//g_pLuckyItemWnd->GetInventoryCtrl()->SetPos(GetUIPosition(ThirdIndex, 0)->x + WinOpen3, GetUIPosition(ThirdIndex, 0)->y);
		break;
	case INTERFACE_INVENTORY_JEWEL:
		g_pNewUIMng->ShowInterface(INTERFACE_INVENTORY);
		g_pNewUIMng->ShowInterface(ThirdIndex);
		g_pInventoryJewel->SetPos(WinOpen3, 0);
		break;
	default:
		return false;
	}

	return true;
}

int SEASON3B::CNewUIInvenExpansion::IsThirdInterface(int winIndex)
{
	return FindLinkageUI(winIndex);
}

void SEASON3B::CNewUIInvenExpansion::RegLinkageUI()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		//-- store
		myOrgInterface.SetOrgInvenPos(INTERFACE_MYSHOP_INVENTORY, 15.0, 110.0, 0.0, 0.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());

		//-- warehouse
		myOrgInterface.SetOrgInvenPos(INTERFACE_STORAGE, 15.0, 50.0, 0.0, 0.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());

		//-- trade
		myOrgInterface.SetOrgInvenPos(INTERFACE_TRADE, 15.0, 270.0, 15.0, 70.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());

		//-- store others
		myOrgInterface.SetOrgInvenPos(INTERFACE_PURCHASESHOP_INVENTORY, 15.0, 110.0, 0.0, 0.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());
	}
	else
	{
		//-- store
		myOrgInterface.SetOrgInvenPos(INTERFACE_MYSHOP_INVENTORY, 15.0, 90.0, 0.0, 0.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());

		//-- warehouse
		myOrgInterface.SetOrgInvenPos(INTERFACE_STORAGE, 15.0, 36.0, 0.0, 0.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());

		//-- trade
		myOrgInterface.SetOrgInvenPos(INTERFACE_TRADE, 15.0, 274.0, 15.0, 68.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());

		//-- store others
		myOrgInterface.SetOrgInvenPos(INTERFACE_PURCHASESHOP_INVENTORY, 15.0, 90.0, 0.0, 0.0);
		InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
		m_List.push_back(myOrgInterface.GetInterfaceList());
	}

	//-- npc shop
	myOrgInterface.SetOrgInvenPos(INTERFACE_NPCSHOP, 15.0, 50.0, 0.0, 0.0);
	InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
	m_List.push_back(myOrgInterface.GetInterfaceList());

	//-- mix inventory
	myOrgInterface.SetOrgInvenPos(INTERFACE_MIXINVENTORY, 15.0, 110.0, 0.0, 0.0);
	InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
	m_List.push_back(myOrgInterface.GetInterfaceList());

	//-- lucky
	myOrgInterface.SetOrgInvenPos(INTERFACE_LUCKYITEMWND, 15.0, 110.0, 0.0, 0.0);
	InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
	m_List.push_back(myOrgInterface.GetInterfaceList());

	//-- character info
	myOrgInterface.SetOrgInvenPos(INTERFACE_CHARACTER, 0.0, 0.0, 0.0, 0.0);
	InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
	m_List.push_back(myOrgInterface.GetInterfaceList());

	myOrgInterface.SetOrgInvenPos(INTERFACE_INVENTORY_JEWEL, 0.0, 0.0, 0.0, 0.0);
	InvenOrgInterface.insert(type_map_inven::value_type(myOrgInterface.GetInterfaceList(), myOrgInterface));
	m_List.push_back(myOrgInterface.GetInterfaceList());
}

int SEASON3B::CNewUIInvenExpansion::FindLinkageUI(int winIndex)
{
	for (std::list<int>::iterator it = m_List.begin(); it != m_List.end(); it++)
	{
		if (g_pNewUISystem->IsVisible(*it) == false)
		{
			continue;
		}
		ThirdInterface = 1;
		return *it;
	}

	ThirdInterface = 0;

	return winIndex;
}

tagPOINT* SEASON3B::CNewUIInvenExpansion::GetUIPosition(int index, bool isMyPos)
{
	static tagPOINT lpPoint = { 0,0 };

	std::map<int, CInvenOrgInterface>::iterator it = this->InvenOrgInterface.find(index);

	if (it == this->InvenOrgInterface.end())
	{
		return &lpPoint;
	}

	if (isMyPos == true)
	{
		lpPoint.x = it->second.GetOrgYourInvenPosX();

		lpPoint.y = it->second.GetOrgYourInvenPosY();

		return &lpPoint;
	}

	lpPoint.x = it->second.GetOrgInvenPosX();

	lpPoint.y = it->second.GetOrgInvenPosY();

	return &lpPoint;
}

bool SEASON3B::CNewUIInvenExpansion::SetEnableExpansionInven(BYTE count)
{
	bool result = false;

	int MaxCount = count;

	this->InitExpansion();

	if (count >= (MAX_INVENT_EXPANSION + 1))
	{
		MaxCount = 0;
	}

	for (int i = 0; i < MaxCount; i++)
	{
		this->isExtEnabled[i] = true;

		result = 1;
	}
	g_pMyInventory->SetInvenEnableCnt(MaxCount);

	return result;
}

tagPOINT* SEASON3B::CNewUIInvenExpansion::GetPos()
{
	return &m_Pos;
}

tagPOINT* SEASON3B::CNewUIInvenExpansion::GetIEStartPos(int index)
{
	return &m_InvBoxPos[index];
}


// NewUINameWindow.cpp: implementation of the CNewUINameWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NewUINameWindow.h"
#include "ZzzBmd.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "UIControls.h"
#include "CSChaosCastle.h"
#include "PersonalShopTitleImp.h"
#include "MatchEvent.h"
#include "MapManager.h"


using namespace SEASON3B;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SEASON3B::CNewUINameWindow::CNewUINameWindow()
{
	m_pNewUIMng = NULL;
	m_Pos.x = m_Pos.y = 0;

	m_bShowItemName = false;
}

SEASON3B::CNewUINameWindow::~CNewUINameWindow()
{
	Release();
}

bool SEASON3B::CNewUINameWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_NAME_WINDOW, this);

	LoadImages();

	SetPos(x, y);

	Show(true);

	return true;
}

void SEASON3B::CNewUINameWindow::Release()
{
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUINameWindow::LoadImages()
{
	LoadBitmap("Interface\\HUD\\TooltipHUD.tga", IMAGE_TOOLTIP_ID, GL_LINEAR);
}

void SEASON3B::CNewUINameWindow::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;
}

bool SEASON3B::CNewUINameWindow::UpdateMouseEvent()
{
	return true;
}

bool SEASON3B::CNewUINameWindow::UpdateKeyEvent()
{
	if (SEASON3B::IsPress(VK_MENU) == true)
	{
		m_bShowItemName = !m_bShowItemName;
	}

	return true;
}

bool SEASON3B::CNewUINameWindow::Update()
{
	return true;
}

bool SEASON3B::CNewUINameWindow::Render()
{
	EnableAlphaTest();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RenderName();

	RenderTimes();

	matchEvent::RenderMatchTimes();

	RenderBooleans();

	DrawPersonalShopTitleImp();

	DisableAlphaBlend();

	return true;
}

void SEASON3B::CNewUINameWindow::RenderTooltip(float x, float y, float width, float height)
{

	int wcnt = (int)width / 5;
	int hcnt = (int)height / 5;

	if (width > (wcnt*5))
	{
		wcnt++;

		x -= ((wcnt * 5) - width) / 2.0;
	}
	if (height > (hcnt * 5))
	{
		hcnt++;
		y -= ((hcnt * 5) - height) / 2.0;
	}

	float pos = x;

	glColor4f(0.0, 0.0, 0.0, 0.84);
	RenderColor(x + 5.0, y + 5.0, width - 10.0, height - 10.0);
	EndRenderColor();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, pos, y, 5.0, 5.0, 20.0, 0.0, 28.0, 8.0);
	SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, pos, y + (height - 5.0), 5.0, 5.0, 50.0, 0.0, 58.0, 8.0);

	pos += 5.0;
	for (int i = 0; i < wcnt-2; i++)
	{
		SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, pos, y, 5.0, 5.0, 10.0, 0.0, 18.0, 8.0);
		SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, pos, y + (height - 5.0), 5.0, 5.0, 40.0, 0.0, 48.0, 8.0);
		pos += 5.0;
	}
	SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, pos, y, 5.0, 5.0, 0.0, 0.0, 8.0, 8.0);
	SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, pos, y + (height - 5.0), 5.0, 5.0, 30.0, 0.0, 38.0, 8.0);

	pos = y + 5.0;
	for (int i = 0; i < hcnt - 2; i++)
	{
		SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, x, pos, 5.0, 5.0, 80.0, 0.0, 88.0, 8.0);
		SEASON3B::RenderImages(IMAGE_TOOLTIP_ID, x + (width - 5.0), pos, 5.0, 5.0, 60.0, 0.0, 68.0, 8.0);
		pos += 5.0;
	}
}

void SEASON3B::CNewUINameWindow::RenderName()
{
	if (g_bGMObservation == true)
	{
		for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
		{
			CHARACTER* c = gmCharacters->GetCharacter(i);
			OBJECT* o = &c->Object;
			if (o->Live && o->Kind == KIND_PLAYER)
			{
				if (IsShopTitleVisible(c) == false)
				{
					CreateChat(c->ID, "", c);
				}
			}
		}
	}

#ifndef GUILD_WAR_EVENT
	if (gMapManager.InChaosCastle() == true)
	{
		if (FindText(Hero->ID, "webzen") == false)
		{
			if (SelectedNpc != -1 || SelectedCharacter != -1)
			{
				return;
			}
		}
	}
#endif//GUILD_WAR_EVENT

	if (SelectedItem != -1 || SelectedNpc != -1 || SelectedCharacter != -1)
	{
		if (SelectedNpc != -1)
		{
			CHARACTER* c = gmCharacters->GetCharacter(SelectedNpc);
			OBJECT* o = &c->Object;
			CreateChat(c->ID, "", c);
		}
		else if (SelectedCharacter != -1)
		{
			CHARACTER* c = gmCharacters->GetCharacter(SelectedCharacter);

			OBJECT* o = &c->Object;
			if (o->Kind == KIND_MONSTER)
			{
				g_pRenderText->SetTextColor(255, 230, 200, 255);
				g_pRenderText->SetBgColor(100, 0, 0, 255);
				g_pRenderText->RenderText((GetWindowsX / 2), 2, c->ID, 0, 0, RT3_WRITE_CENTER);
			}
			else
#ifdef ASG_ADD_GENS_SYSTEM
#ifndef PBG_MOD_STRIFE_GENSMARKRENDER
				if (!::IsStrifeMap(World) || Hero->m_byGensInfluence == c->m_byGensInfluence)
#endif //PBG_MOD_STRIFE_GENSMARKRENDER
#endif	// ASG_ADD_GENS_SYSTEM
				{
					if (IsShopTitleVisible(c) == false)
					{
						CreateChat(c->ID, "", c);
					}
				}
		}
		else if (SelectedItem != -1)
		{
			RenderItemName(&Items[SelectedItem], SelectedItem, &Items[SelectedItem].Object, Items[SelectedItem].Item.Level, Items[SelectedItem].Item.Option1, Items[SelectedItem].Item.ExtOption, false);
		}
	}

	if (m_bShowItemName || SEASON3B::IsRepeat(VK_MENU))
	{
		for (int i = 0; i < MAX_ITEMS; i++)
		{
			OBJECT* o = &Items[i].Object;
			if (o->Live)
			{
				if (o->Visible && i != SelectedItem)
				{
					RenderItemName(&Items[i], i, o, Items[i].Item.Level, Items[i].Item.Option1, Items[i].Item.ExtOption, true);
				}
			}
		}
	}
}

float SEASON3B::CNewUINameWindow::GetLayerDepth()
{
	return 1.0f;
}


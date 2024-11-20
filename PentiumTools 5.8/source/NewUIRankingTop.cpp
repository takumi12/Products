#include "stdafx.h"
#include "NewUISystem.h"
#include "NewUIRankingTop.h"
#include "ZzzTexture.h"

SEASON3B::CNewUIRankingTop::CNewUIRankingTop()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;
}

SEASON3B::CNewUIRankingTop::~CNewUIRankingTop()
{
	Release();
}

bool SEASON3B::CNewUIRankingTop::Create(CNewUIManager* pNewUIMng, float x, float y)
{
	bool Success = false;

	if (pNewUIMng)
	{
		m_pNewUIMng = pNewUIMng;

		m_pNewUIMng->AddUIObj(INTERFACE_RANKING_TOP, this);

		this->LoadImages();

		this->SetPos(x, y);

		this->Show(false);

		m_Player.Init(0);
		m_Player.SetArrangeType(1, 119, 30);
		m_Player.SetSize(141, 184);
		m_Player.CopyPlayer();
		m_Player.SetAutoupdatePlayer(TRUE);
		m_Player.SetAnimation(AT_STAND1);
		m_Player.SetAngle(90.f);
		m_Player.SetZoom(0.80f);
		Success = true;
	}
	return Success;
}

void SEASON3B::CNewUIRankingTop::Release()
{
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);

		this->UnloadImages();
	}
}

void SEASON3B::CNewUIRankingTop::SetPos(float x, float y)
{
	m_Pos.x = x;
	m_Pos.y = y;
	m_Player.SetPosition(x + 12, y + 34);
}

void SEASON3B::CNewUIRankingTop::LoadImages()
{
	LoadBitmap("Interface\\HUD\\top_back_1.tga", IMAGE_TOP_BACK1, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\top_back_2.tga", IMAGE_TOP_BACK2, GL_LINEAR);
}

void SEASON3B::CNewUIRankingTop::UnloadImages()
{
	DeleteBitmap(IMAGE_TOP_BACK1);
	DeleteBitmap(IMAGE_TOP_BACK2);
}

bool SEASON3B::CNewUIRankingTop::UpdateKeyEvent()
{
	if (IsVisible() == true)
	{
		if (SEASON3B::IsPress(VK_ESCAPE))
		{
			g_pNewUISystem->Hide(INTERFACE_RANKING_TOP);
			return false;
		}
	}

	return true;
}

bool SEASON3B::CNewUIRankingTop::UpdateMouseEvent()
{
	if (SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, 366, 230))
	{
		if (SEASON3B::CheckMouseIn(m_Pos.x + 344.f, m_Pos.y + 3.5f, 19.f, 19.f) && SEASON3B::IsRelease(VK_LBUTTON))
		{
			g_pNewUISystem->Hide(INTERFACE_RANKING_TOP);
			return false;
		}
		m_Player.DoMouseAction();
	}

	return !SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, 366, 230);
}

bool SEASON3B::CNewUIRankingTop::Render()
{
	EnableAlphaTest(true);

	glColor4f(1.f, 1.f, 1.f, 1.f);

	this->RenderFrame();

	this->RenderTexte();

	DisableAlphaBlend();

	m_Player.Render();

	return true;
}

bool SEASON3B::CNewUIRankingTop::Update()
{
	return true;
}

float SEASON3B::CNewUIRankingTop::GetLayerDepth()
{
	return 10.0f;
}

void SEASON3B::CNewUIRankingTop::OpenningProcess()
{
	m_Player.CopyPlayer();
}

void SEASON3B::CNewUIRankingTop::ClosingProcess()
{
}

void SEASON3B::CNewUIRankingTop::RenderFrame()
{
	float x = m_Pos.x;
	float y = m_Pos.y;

	RenderImageF(IMAGE_TOP_BACK1, x, y, 183.f, 230.f, 0.0, 0.0, 732.0, 917.0); //-- background

	x += 183.f;
	RenderImageF(IMAGE_TOP_BACK2, x, y, 183.f, 230.f, 0.0, 0.0, 732.0, 917.0); //-- background
}

void SEASON3B::CNewUIRankingTop::RenderTexte()
{
	int x = m_Pos.x;
	int y = m_Pos.y;

	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(-1);
	g_pRenderText->SetFont(g_hFontBold);

	g_pRenderText->RenderFont(x + 12, y + 10, "Ranking - Level", 0, 0, RT3_SORT_LEFT);

	x = m_Pos.x + 162;
	g_pRenderText->SetBgColor(52, 57, 67, 255);
	g_pRenderText->RenderFont(x, y + 32, "#", 25, 18, RT3_SORT_CENTER);

	x += 25;
	g_pRenderText->RenderFont(x, y + 32, "Name", 50, 18, RT3_SORT_CENTER);

	x += 50;
	g_pRenderText->RenderFont(x, y + 32, "Class", 60, 18, RT3_SORT_CENTER);

	x += 60;
	g_pRenderText->RenderFont(x, y + 32, "Score", 60, 18, RT3_SORT_CENTER);
}

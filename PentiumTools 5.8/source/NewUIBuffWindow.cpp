// NewUIBuffWindow.cpp: implementation of the CNewUIBuffWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "NewUIBuffWindow.h"
#include "ZzzBMD.h"
#include "ZzzCharacter.h"
#include "ZzzTexture.h"
#include "ZzzInventory.h"
#include "UIControls.h"
#include "NewUICommonMessageBox.h"

using namespace SEASON3B;

namespace
{
	const float BUFF_IMG_WIDTH = 16.0f;
	const float BUFF_IMG_HEIGHT = 20.0f;
	const int BUFF_MAX_LINE_COUNT = 8;
	const int BUFF_IMG_SPACE = 5;
};

SEASON3B::CNewUIBuffWindow::CNewUIBuffWindow()
{
	m_pNewUIMng = NULL;
	m_Pos.x = m_Pos.y = 0;
}

SEASON3B::CNewUIBuffWindow::~CNewUIBuffWindow()
{
	Release();
}

bool SEASON3B::CNewUIBuffWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_BUFF_WINDOW, this);

	SetPos(x, y);

	LoadImages();

	Show(true);

	return true;
}

void SEASON3B::CNewUIBuffWindow::Release()
{
	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIBuffWindow::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;
}

void SEASON3B::CNewUIBuffWindow::SetPos(int iScreenWidth)
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		if (iScreenWidth == WinFrameX)
		{
			SetPos(PositionX_In_The_Mid(220), 15);
		}
		else if (iScreenWidth == (WinFrameX - 190))
		{
			SetPos(PositionX_In_The_Mid(125), 15);
		}
		else if (iScreenWidth == (WinFrameX - 380))
		{
			SetPos(PositionX_In_The_Mid(86), 15);
		}
		else if (iScreenWidth == (WinFrameX - 570))
		{
			SetPos(PositionX_In_The_Mid(30), 15);
		}
		else
		{
			SetPos(PositionX_In_The_Mid(220), 15);
		}
	}
	else
	{

		SetPos(PositionX_The_Mid(640), (int)GetWindowsY - 51);
	}
}

void SEASON3B::CNewUIBuffWindow::BuffSort(std::list<eBuffState>& buffstate)
{
	OBJECT* pHeroObject = &Hero->Object;

	int iBuffSize = g_CharacterBuffSize(pHeroObject);

	for (int i = 0; i < iBuffSize; ++i)
	{
		eBuffState eBuffType = g_CharacterBuff(pHeroObject, i);

		if (SetDisableRenderBuff(eBuffType))	continue;

		if (eBuffType != eBuffNone) {
			eBuffClass eBuffClassType = g_IsBuffClass(eBuffType);

			if (eBuffClassType == eBuffClass_Buff)
			{
				buffstate.push_front(eBuffType);
			}
			else if (eBuffClassType == eBuffClass_DeBuff)
			{
				buffstate.push_back(eBuffType);
			}
			else {
				assert(!"SetDisableRenderBuff");
			}
		}
	}
}

bool SEASON3B::CNewUIBuffWindow::SetDisableRenderBuff(const eBuffState& _BuffState)
{
	switch (_BuffState)
	{
#ifdef PBG_ADD_PKSYSTEM_INGAMESHOP
	case eDeBuff_MoveCommandWin:
#endif //PBG_ADD_PKSYSTEM_INGAMESHOP
	case eDeBuff_FlameStrikeDamage:
	case eDeBuff_GiganticStormDamage:
	case eDeBuff_LightningShockDamage:
	case eDeBuff_Discharge_Stamina:
		return true;
	default:
		return false;
}
	return false;
}

bool SEASON3B::CNewUIBuffWindow::UpdateMouseEvent()
{
	float x = 0.0f, y = 0.0f;
	int buffwidthcount = 0, buffheightcount = 1;

	std::list<eBuffState> buffstate;
	BuffSort(buffstate);

	std::list<eBuffState>::iterator iter;


	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		buffheightcount = 0;
	}

	for (iter = buffstate.begin(); iter != buffstate.end(); )
	{
		std::list<eBuffState>::iterator tempiter = iter;
		++iter;
		eBuffState buff = (*tempiter);

		x = m_Pos.x + (buffwidthcount * (BUFF_IMG_WIDTH + BUFF_IMG_SPACE));

		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			y = m_Pos.y + (buffheightcount * (BUFF_IMG_HEIGHT + BUFF_IMG_SPACE));
		}
		else
		{
			y = m_Pos.y - (buffheightcount * (BUFF_IMG_HEIGHT + BUFF_IMG_SPACE));
		}


		if (SEASON3B::CheckMouseIn(x, y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT))
		{
			if (buff == eBuff_InfinityArrow)
			{
				if (SEASON3B::IsRelease(VK_RBUTTON))
				{
					SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CInfinityArrowCancelMsgBoxLayout));
				}
			}
			else if (buff == eBuff_SwellOfMagicPower)
			{
				if (SEASON3B::IsRelease(VK_RBUTTON))
				{
					SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CBuffSwellOfMPCancelMsgBoxLayOut));
				}
			}
			else if (buff == EFFECT_MAGIC_CIRCLE_IMPROVED)
			{
				if (SEASON3B::IsRelease(VK_RBUTTON))
				{
					SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CBuffSwellOfMPUpCancelMsgBoxLayOut));
				}
			}
			else if (buff == EFFECT_MAGIC_CIRCLE_ENHANCED)
			{
				if (SEASON3B::IsRelease(VK_RBUTTON))
				{
					SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CBuffSwellOfMPMasteryCancelMsgBoxLayOut));
				}
			}
			return false;
		}

		if (++buffwidthcount >= BUFF_MAX_LINE_COUNT) {
			buffwidthcount = 0;
			++buffheightcount;
		}
	}
	return true;
}

bool SEASON3B::CNewUIBuffWindow::UpdateKeyEvent()
{
	return true;
}

bool SEASON3B::CNewUIBuffWindow::Update()
{
	return true;
}

bool SEASON3B::CNewUIBuffWindow::Render()
{
	EnableAlphaTest();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	RenderBuffStatus(BUFF_RENDER_ICON);

	RenderBuffStatus(BUFF_RENDER_TOOLTIP);

	DisableAlphaBlend();

	return true;
}

void SEASON3B::CNewUIBuffWindow::RenderBuffStatus(BUFF_RENDER renderstate)
{
	OBJECT* pHeroObject = &Hero->Object;

	float x = 0.0f, y = 0.0f;
	int buffwidthcount = 0, buffheightcount = 1;

	std::list<eBuffState> buffstate;
	BuffSort(buffstate);

	std::list<eBuffState>::iterator iter;

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		buffheightcount = 0;
	}

	for (iter = buffstate.begin(); iter != buffstate.end(); )
	{
		std::list<eBuffState>::iterator tempiter = iter;
		++iter;
		eBuffState buff = (*tempiter);

		x = m_Pos.x + (buffwidthcount * (BUFF_IMG_WIDTH + BUFF_IMG_SPACE));
		if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
		{
			y = m_Pos.y + (buffheightcount * (BUFF_IMG_HEIGHT + BUFF_IMG_SPACE));
		}
		else
		{
			y = m_Pos.y - (buffheightcount * (BUFF_IMG_HEIGHT + BUFF_IMG_SPACE));
		}

		if (renderstate == BUFF_RENDER_ICON)
		{
			RenderBuffIcon(buff, x, y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT);
#ifdef _DEBUG
			int iBuffReferenceCount = g_CharacterBuffCount(pHeroObject, buff);
			RenderNumber(x + 5, y + 5, iBuffReferenceCount, 1.f);
#endif //_DEBUG
	}
		else if (renderstate == BUFF_RENDER_TOOLTIP)
		{
			// ¹öÇÁ ÅøÆÁ ·»´õ¸µ
			if (SEASON3B::CheckMouseIn(x, y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT))
			{
				float fTooltip_x = x + (BUFF_IMG_WIDTH / 2);
				float fTooltip_y = y;

				if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
				{
					y += (BUFF_IMG_HEIGHT * 2);
				}
				else
				{
					y -= (BUFF_IMG_HEIGHT * 2);
				}

				eBuffClass buffclass = g_IsBuffClass(buff);
				RenderBuffTooltip(buffclass, buff, fTooltip_x, fTooltip_y);
			}
		}

		if (++buffwidthcount >= BUFF_MAX_LINE_COUNT) {
			buffwidthcount = 0;
			++buffheightcount;
		}
}
}

void SEASON3B::CNewUIBuffWindow::RenderBuffIcon(eBuffState& eBuffType, float x, float y, float width, float height)
{
	float u, v;

	if (eBuffType > eBuffNone && eBuffType < eBuff_Count) // eBuff_Berserker
	{
		int skillindex = ((int)eBuffType - 1) % 80;

		u = (double)(skillindex % 10) * 20.f / 256.0;
		v = (double)(skillindex / 10) * 28.f / 256.0;
		int imgindex = ((int)eBuffType - 1) / 80 + IMAGE_BUFF_STATUS;
		RenderBitmap(imgindex, x, y, width, height, u, v, 20.f / 256.f, 28.f / 256.f);
	}
}

void SEASON3B::CNewUIBuffWindow::RenderBuffTooltip(eBuffClass& eBuffClassType, eBuffState& eBuffType, float x, float y)
{
	int TextNum = 0;
	::memset(TextList, 0, sizeof(char) * 30 * 100);
	::memset(TextListColor, 0, sizeof(int) * 30);
	::memset(TextBold, 0, sizeof(int) * 30);

	std::list<std::string> tooltipinfo;
	g_BuffToolTipString(tooltipinfo, eBuffType);

	for (std::list<std::string>::iterator iter = tooltipinfo.begin(); iter != tooltipinfo.end(); ++iter)
	{
		std::string& temp = *iter;

		unicode::_sprintf(TextList[TextNum], temp.c_str());

		if (TextNum == 0)
		{
			TextListColor[TextNum] = TEXT_COLOR_BLUE;
			TextBold[TextNum] = true;
		}
		else
		{
			TextListColor[TextNum] = TEXT_COLOR_WHITE;
			TextBold[TextNum] = false;
		}
		TextNum += 1;
	}

	std::string bufftime;
	g_BuffStringTime(eBuffType, bufftime);

	if (bufftime.size() != 0)
	{
		unicode::_sprintf(TextList[TextNum++], "\n");

		unicode::_sprintf(TextList[TextNum], GlobalText[2533], bufftime.c_str());
		TextListColor[TextNum] = TEXT_COLOR_PURPLE;
		TextBold[TextNum] = false;
		TextNum += 1;
	}

	SIZE TextSize = { 0, 0 };
	g_pMultiLanguage->_GetTextExtentPoint32(g_pRenderText->GetFontDC(), TextList[0], 1, &TextSize);
	RenderTipTextList(x, y, TextNum, 0);
}

float SEASON3B::CNewUIBuffWindow::GetLayerDepth()	//. 5.3f
{
	return 0.95f;
}

void SEASON3B::CNewUIBuffWindow::OpenningProcess()
{

}

void SEASON3B::CNewUIBuffWindow::ClosingProcess()
{

}

void SEASON3B::CNewUIBuffWindow::LoadImages()
{
	LoadBitmap("Interface\\newui_statusicon.jpg", IMAGE_BUFF_STATUS, GL_LINEAR);
	LoadBitmap("Interface\\newui_statusicon2.jpg", IMAGE_BUFF_STATUS2, GL_LINEAR);
	LoadBitmap("Interface\\newui_statusicon3.jpg", IMAGE_BUFF_STATUS3, GL_LINEAR);
}

void SEASON3B::CNewUIBuffWindow::UnloadImages()
{
	DeleteBitmap(IMAGE_BUFF_STATUS);
	DeleteBitmap(IMAGE_BUFF_STATUS2);
	DeleteBitmap(IMAGE_BUFF_STATUS3);
}

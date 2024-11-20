// NewUIGuildInfoWindow.cpp: implementation of the CNewUIGuildInfoWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "NewUIMiniMap.h"
#include "NewUISystem.h"
#include "NewUICommonMessageBox.h"
#include "NewUICustomMessageBox.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"
#include "NewUIGuildInfoWindow.h"
#include "NewUIButton.h"
#include "NewUIMyInventory.h"
#include "CSitemOption.h"
#include "MapManager.h"

#include "ZzzAI.h"
#include "ZzzLodTerrain.h"


using namespace SEASON3B;

extern int TargetX;
extern int TargetY;
extern BYTE m_OccupationState;
extern WORD TerrainWall[TERRAIN_SIZE * TERRAIN_SIZE];

SEASON3B::CNewUIMiniMap::CNewUIMiniMap()
{
	m_pNewUIMng = NULL;

	m_Lenth[0].x = 800;
	m_Lenth[1].x = 1000;
	m_Lenth[2].x = 1200;
	m_Lenth[3].x = 1400;
	m_Lenth[4].x = 1600;
	m_Lenth[5].x = 1800;
	m_Lenth[0].y = 800;
	m_Lenth[1].y = 1000;
	m_Lenth[2].y = 1200;
	m_Lenth[3].y = 1400;
	m_Lenth[4].y = 1600;
	m_Lenth[5].y = 1800;
	m_MiniPos = 0;
	m_bSuccess = false;

	m_Pos.x = 0;
	m_Pos.y = 0;

	m_TooltipText = "";
	m_hToolTipFont = g_hFont;
	m_TooltipTextColor = -1;

	m_bMoveEnable = false;

	m_bNavigation = 0;
	m_bWorldBackup = -1;
	m_bCurPositionX = -1;
	m_bCurPositionY = -1;
	m_bDesPositionX = -1;
	m_bDesPositionY = -1;
	m_bMovemTimeWating = 0;
}

SEASON3B::CNewUIMiniMap::~CNewUIMiniMap()
{
	Release();
}

bool SEASON3B::CNewUIMiniMap::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MINI_MAP, this);

	this->LoadImages();

	m_BtnExit.ChangeButtonImgState(true, IMAGE_MINIMAP_INTERFACE + 6, false);
	m_BtnExit.ChangeButtonInfo(m_Pos.x + 610, m_Pos.y + 3, 85, 85);
	m_BtnExit.ChangeToolTipText(GlobalText[1002], true);

	this->SetPos(x, y);

	return true;
}

void SEASON3B::CNewUIMiniMap::Release()
{
	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIMiniMap::LoadImages()
{
	LoadBitmap("Interface\\mini_map_ui_corner.tga", IMAGE_MINIMAP_INTERFACE + 1, GL_LINEAR);
	LoadBitmap("Interface\\mini_map_ui_line.jpg", IMAGE_MINIMAP_INTERFACE + 2, GL_LINEAR);
	LoadBitmap("Interface\\mini_map_ui_cha.tga", IMAGE_MINIMAP_INTERFACE + 3, GL_LINEAR);
	LoadBitmap("Interface\\mini_map_ui_portal.tga", IMAGE_MINIMAP_INTERFACE + 4, GL_LINEAR);
	LoadBitmap("Interface\\mini_map_ui_npc.tga", IMAGE_MINIMAP_INTERFACE + 5, GL_LINEAR);
	LoadBitmap("Interface\\mini_map_ui_cancel.tga", IMAGE_MINIMAP_INTERFACE + 6, GL_LINEAR);
}

void SEASON3B::CNewUIMiniMap::UnloadImages()
{
	DeleteBitmap(IMAGE_MINIMAP_INTERFACE);

	DeleteBitmap(IMAGE_MINIMAP_INTERFACE + 1);
	DeleteBitmap(IMAGE_MINIMAP_INTERFACE + 2);
	DeleteBitmap(IMAGE_MINIMAP_INTERFACE + 3);
	DeleteBitmap(IMAGE_MINIMAP_INTERFACE + 4);
	DeleteBitmap(IMAGE_MINIMAP_INTERFACE + 5);
	DeleteBitmap(IMAGE_MINIMAP_INTERFACE + 6);
}

float SEASON3B::CNewUIMiniMap::GetLayerDepth()
{
	return 8.1f;
}

void SEASON3B::CNewUIMiniMap::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;
	m_BtnExit.ChangeButtonInfo(GetWindowsX - 27, 3, 30, 25);
}

void SEASON3B::CNewUIMiniMap::SetBtnPos(int Num, float x, float y, float nx, float ny)
{
	m_Btn_Loc[Num][0] = x;
	m_Btn_Loc[Num][1] = y;
	m_Btn_Loc[Num][2] = nx;
	m_Btn_Loc[Num][3] = ny;
}

bool SEASON3B::CNewUIMiniMap::UpdateKeyEvent()
{
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MINI_MAP))
	{
		if (IsPress(VK_ESCAPE) == true || IsPress(VK_TAB) == true)
		{
			g_pNewUISystem->Hide(SEASON3B::INTERFACE_MINI_MAP);
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}
	return true;
}

bool SEASON3B::CNewUIMiniMap::Render()
{
	float Rot = 45.f;

	if (m_bSuccess == false)
		return m_bSuccess;

	EnableAlphaTest();
	RenderColor(0, 0, GetWindowsX, GetWindowsY, 0.85f, 1);
	DisableAlphaBlend();
	EnableAlphaTest();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	float Ty1;
	float Tx1;
	float uvxy = (41.7f / 64.f);
	float uvxy_Line = 8.f / 8.f;
	float Ui_wid = 35.f;
	float Ui_Hig = 6.f;
	float Rot_Loc = 45.f;
	int i = 0;

	float Ty = (float)(((float)Hero->PositionX / 256.f) * m_Lenth[m_MiniPos].y);
	float Tx = (float)(((float)Hero->PositionY / 256.f) * m_Lenth[m_MiniPos].x);

	RenderBitRotate(IMAGE_MINIMAP_INTERFACE, m_Lenth[m_MiniPos].x - Tx, m_Lenth[m_MiniPos].y - Ty, m_Lenth[m_MiniPos].x, m_Lenth[m_MiniPos].y, Rot);

	int NpcWidth = 15;
	int NpcWidthP = 30;
	for (i = 0; i < MAX_MINI_MAP_DATA; i++)
	{
		if (m_Mini_Map_Data[i].Kind > 0)
		{
			Ty1 = (float)(((float)m_Mini_Map_Data[i].Location[0] / 256.f) * m_Lenth[m_MiniPos].y);
			Tx1 = (float)(((float)m_Mini_Map_Data[i].Location[1] / 256.f) * m_Lenth[m_MiniPos].x);
			Rot_Loc = (float)m_Mini_Map_Data[i].Rotation;

			if (m_Mini_Map_Data[i].Kind == 1) //npc
			{
				if (!(gMapManager.WorldActive == WD_34CRYWOLF_1ST && m_OccupationState > 0) || (m_Mini_Map_Data[i].Location[0] == 228 && m_Mini_Map_Data[i].Location[1] == 48 && gMapManager.WorldActive == WD_34CRYWOLF_1ST))
					RenderPointRotate(IMAGE_MINIMAP_INTERFACE + 5, Tx1, Ty1, NpcWidth, NpcWidth, m_Lenth[m_MiniPos].x - Tx, m_Lenth[m_MiniPos].y - Ty, m_Lenth[m_MiniPos].x, m_Lenth[m_MiniPos].y, Rot, Rot_Loc, 17.5f / 32.f, 17.5f / 32.f, i);
			}
			else
				if (m_Mini_Map_Data[i].Kind == 2)
					RenderPointRotate(IMAGE_MINIMAP_INTERFACE + 4, Tx1, Ty1, NpcWidthP, NpcWidthP, m_Lenth[m_MiniPos].x - Tx, m_Lenth[m_MiniPos].y - Ty, m_Lenth[m_MiniPos].x, m_Lenth[m_MiniPos].y, Rot, Rot_Loc, 17.5f / 32.f, 17.5f / 32.f, 100 + i);
		}
		else
			break;
	}

	float Ch_wid = 12;
	RenderImage(IMAGE_MINIMAP_INTERFACE + 3, PositionX_The_Mid(Ch_wid), PositionY_The_Mid(Ch_wid), Ch_wid, Ch_wid, 0.f, 0.f, 17.5f / 32.f, 17.5f / 32.f);


	int iNumX = (int)(GetWindowsX / 35) + 1;
	int iNumY = (int)(GetWindowsY / 6) + 1;

	for (i = 0; i < iNumX; i++)
	{
		RenderImage(IMAGE_MINIMAP_INTERFACE + 2, i * Ui_wid, 0, Ui_wid, Ui_Hig, 0.f, 1.f, uvxy, -uvxy_Line);
		RenderImage(IMAGE_MINIMAP_INTERFACE + 2, i * Ui_wid, GetWindowsY - Ui_Hig, Ui_wid, Ui_Hig, 0.f, 0.f, uvxy, uvxy_Line);
	}
	for (i = 0; i < iNumY; i++)
	{
		RenderBitmapRotate(IMAGE_MINIMAP_INTERFACE + 2, (Ui_Hig / 2.f), i * (Ui_wid - 3.f), Ui_wid, Ui_Hig, -90.f, 0.f, 0.f, uvxy, uvxy_Line);
		RenderBitmapRotate(IMAGE_MINIMAP_INTERFACE + 2, GetWindowsX - (Ui_Hig / 2.f), i * (Ui_wid - 3.f), Ui_wid, Ui_Hig, 90.f, 0.f, 0.f, uvxy, uvxy_Line);
	}

	RenderImage(IMAGE_MINIMAP_INTERFACE + 1, 0, 0, Ui_wid, Ui_wid, 0.f, 0.f, uvxy, uvxy);
	RenderImage(IMAGE_MINIMAP_INTERFACE + 1, GetWindowsX - Ui_wid, 0, Ui_wid, Ui_wid, uvxy, 0.f, -uvxy, uvxy);
	RenderImage(IMAGE_MINIMAP_INTERFACE + 1, 0, GetWindowsY - Ui_wid, Ui_wid, Ui_wid, 0.f, uvxy, uvxy, -uvxy);
	RenderImage(IMAGE_MINIMAP_INTERFACE + 1, GetWindowsX - Ui_wid, GetWindowsY - Ui_wid, Ui_wid, Ui_wid, uvxy, uvxy, -uvxy, -uvxy);

	m_BtnExit.Render(true);

	DisableAlphaBlend();

	Check_Btn(MouseX, MouseY);
	return true;
}

bool SEASON3B::CNewUIMiniMap::Update()
{
	return true;
}

void SEASON3B::CNewUIMiniMap::LoadImages(const char* Filename)
{
	char Fname[300];
	sprintf(Fname, "Data\\%s\\mini_map.ozt", Filename);
	FILE* pFile = fopen(Fname, "rb");

	DeleteBitmap(IMAGE_MINIMAP_INTERFACE);

	if (pFile == NULL)
	{
		m_bSuccess = false;
		g_pItemEnduranceInfo->HideInterfaceWorld(false);
		return;
	}
	else
	{
		m_bSuccess = true;
		g_pItemEnduranceInfo->HideInterfaceWorld(true);

		fclose(pFile);
		sprintf(Fname, "%s\\mini_map.tga", Filename);
		LoadBitmap(Fname, IMAGE_MINIMAP_INTERFACE, GL_LINEAR);
	}

#if MAIN_UPDATE == 303
	sprintf(Fname, "Data\\Local\\Minimap\\Minimap_%s.bmd", Filename);
#else
	sprintf(Fname, "Data\\Local\\%s\\Minimap\\Minimap_%s_%s.bmd", g_strSelectedML.c_str(), Filename, g_strSelectedML.c_str());
#endif // MAIN_UPDATE == 303


	for (int i = 0; i < MAX_MINI_MAP_DATA; i++)
	{
		m_Mini_Map_Data[i].Kind = 0;
	}

	FILE* fp = fopen(Fname, "rb");

	if (fp != NULL)
	{
		int Size = sizeof(MINI_MAP);
		BYTE* Buffer = new BYTE[Size * MAX_MINI_MAP_DATA + 45];
		fread(Buffer, (Size * MAX_MINI_MAP_DATA) + 45, 1, fp);

		DWORD dwCheckSum;
		fread(&dwCheckSum, sizeof(DWORD), 1, fp);
		fclose(fp);

		if (dwCheckSum != GenerateCheckSum2(Buffer, (Size * MAX_MINI_MAP_DATA) + 45, 0x2BC1))
		{
			char Text[256];
			sprintf(Text, "%s - File corrupted.", Fname);
			g_ErrorReport.Write(Text);
			MessageBox(g_hWnd, Text, NULL, MB_OK);
			SendMessage(g_hWnd, WM_DESTROY, 0, 0);
		}
		else
		{
			BYTE* pSeek = Buffer;

			for (int i = 0; i < MAX_MINI_MAP_DATA; i++)
			{
				BuxConvert(pSeek, Size);
				memcpy(&(m_Mini_Map_Data[i]), pSeek, Size);

				pSeek += Size;
			}
		}

		delete[] Buffer;
	}
}

bool SEASON3B::CNewUIMiniMap::IsSuccess()
{
	return m_bSuccess;
}

bool SEASON3B::CNewUIMiniMap::UpdateMouseEvent()
{
	bool ret = true;

	if (m_BtnExit.UpdateMouseEvent() == true)
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_MINI_MAP);
		return true;
	}

	if (IsPress(VK_LBUTTON))
	{
		ret = Check_Mouse(MouseX, MouseY);
		if (ret == false)
		{
			PlayBuffer(SOUND_CLICK01);
		}

	}

	if (CheckMouseIn(0, 0, GetWindowsX, GetWindowsY))
	{
		return false;
	}

	return ret;
}

bool SEASON3B::CNewUIMiniMap::Check_Mouse(int mx, int my)
{
	return true;
}

bool SEASON3B::CNewUIMiniMap::Check_Btn(int mx, int my)
{
	int i = 0;
	for (i = 0; i < MAX_MINI_MAP_DATA; i++)
	{
		if (m_Mini_Map_Data[i].Kind > 0)
		{
			if (mx > m_Btn_Loc[i][0] && mx < (m_Btn_Loc[i][0] + m_Btn_Loc[i][2]) && my > m_Btn_Loc[i][1] && my < (m_Btn_Loc[i][1] + m_Btn_Loc[i][3]))
			{
				SIZE Fontsize;
				m_TooltipText = (unicode::t_string)m_Mini_Map_Data[i].Name;
				g_pRenderText->SetFont(g_hFont);
				g_pMultiLanguage->_GetTextExtentPoint32(g_pRenderText->GetFontDC(), m_TooltipText.c_str(), m_TooltipText.size(), &Fontsize);

				Fontsize.cx = Fontsize.cx / ((float)WindowWidth / 640);
				Fontsize.cy = Fontsize.cy / ((float)WindowHeight / 480);

				int x = m_Btn_Loc[i][0] + ((m_Btn_Loc[i][2] / 2) - (Fontsize.cx / 2));
				int y = m_Btn_Loc[i][1] + m_Btn_Loc[i][3] + 2;

				y = m_Btn_Loc[i][1] - (Fontsize.cy + 2);

				DWORD backuptextcolor = g_pRenderText->GetTextColor();
				DWORD backuptextbackcolor = g_pRenderText->GetBgColor();

				g_pRenderText->SetTextColor(RGBA(255, 255, 255, 255));
				g_pRenderText->SetBgColor(RGBA(0, 0, 0, 180));
				g_pRenderText->RenderText(x, y, m_TooltipText.c_str(), Fontsize.cx + 6, 0, RT3_SORT_CENTER);

				g_pRenderText->SetTextColor(backuptextcolor);
				g_pRenderText->SetBgColor(backuptextbackcolor);

				return true;
			}
		}
		else
			break;
	}
	return false;
}

bool SEASON3B::CNewUIMiniMap::movement_automatic()
{
	if (Hero->Dead == 0 && m_bMoveEnable && m_bWorldBackup == gMapManager.WorldActive)
	{
		size_t max_travel = toTravel.size();

		if (max_travel > 0 && (m_bNavigation + 1) < max_travel)
		{
			type_pair_coord Walk = toTravel[m_bNavigation + 1];

			if (navigate_on_the_route(Walk.first, Walk.second))
			{
				m_bNavigation += 1;
			}
			else
			{
				if (GetTickCount() > m_bMovemTimeWating + 1000)
				{
					BYTE MoveRandom[4][2];
					m_bMovemTimeWating = GetTickCount();
					MoveRandom[0][0] = 4; MoveRandom[1][0] = 4;
					MoveRandom[0][1] = 4; MoveRandom[3][1] = 4;
					MoveRandom[2][0] = -4; MoveRandom[3][0] = -4;
					MoveRandom[2][1] = -4; MoveRandom[1][1] = -4;
					for (int i = 0; i < 4; i++)
					{
						if (navigate_on_the_route(Walk.first + MoveRandom[i][0], Walk.second + MoveRandom[i][1]))
						{
							m_bNavigation += 1;
							break;
						}
					}
				}
			}
		}
		else
		{
			m_bMoveEnable = false;
		}
	}
	else
	{
		m_bMoveEnable = false;
	}

	if ((Hero->Movement == false && m_bMoveEnable == false) || m_bMoveEnable == false)
		return true;

	return false;
}

void SEASON3B::CNewUIMiniMap::cancel_movement_automatic()
{
	if (m_bMoveEnable)
	{
		m_bMoveEnable = false;
		m_bDesPositionX = -1;
		m_bDesPositionY = -1;
		m_bNavigation = 0;
		toTravel.clear();
	}
}

void SEASON3B::CNewUIMiniMap::how_to_get(int PositionX, int PositionY)
{
	m_bMoveEnable = false;
	m_bDesPositionX = PositionX;
	m_bDesPositionY = PositionY;
	m_bWorldBackup = gMapManager.WorldActive;

	if (PositionX >= 0 && PositionX < 256 && PositionY >= 0 && PositionY < 256)
	{
		if (PositionX != Hero->PositionX && PositionY != Hero->PositionY)
		{
			size_t max_travel = route_to_destination(type_pair_coord(Hero->PositionX, Hero->PositionY));

			if (max_travel > 0 && (m_bNavigation + 1) < max_travel)
			{
				m_bMoveEnable = true;
				type_pair_coord Walk = toTravel[m_bNavigation + 1];

				if (navigate_on_the_route(Walk.first, Walk.second))
				{
					m_bNavigation++;
				}
			}
		}
	}
}

bool SEASON3B::CNewUIMiniMap::navigate_on_the_route(int PositionX, int PositionY)
{
	bool Success = false;
	if (PositionX >= 0 && PositionX < 256 && PositionY >= 0 && PositionY < 256)
	{
		OBJECT* o = &Hero->Object;

		if (o->Live)
		{
			TargetX = PositionX;
			TargetY = PositionY;

			WORD CurrAtt = TerrainWall[TargetY * 256 + TargetX];
			if (CurrAtt >= TW_NOGROUND && (CurrAtt & TW_ACTION) != TW_ACTION && (CurrAtt & TW_HEIGHT) != TW_HEIGHT)
				DontMove = true;
			else
				DontMove = false;

			int xPos = (int)(0.01f * o->Position[0]);
			int yPos = (int)(0.01f * o->Position[1]);

			if (!Hero->Movement || (abs((Hero->PositionX) - xPos) < 2 && abs((Hero->PositionY) - yPos) < 2))
			{
				if (((Hero->PositionX) != TargetX || (Hero->PositionY) != TargetY || !Hero->Movement) &&
					PathFinding2((Hero->PositionX), (Hero->PositionY), TargetX, TargetY, &Hero->Path))
				{
					Hero->MovementType = MOVEMENT_MOVE;

					Success = true;
					SendMove(Hero, o);
					m_bMovemTimeWating = GetTickCount();
				}
			}
		}
	}
	return Success;
}

size_t SEASON3B::CNewUIMiniMap::route_to_destination(const type_pair_coord& walkstart)
{
	std::queue<type_pair_coord> PriorityQueue;
	std::vector<bool> visitados((TERRAIN_SIZE * TERRAIN_SIZE), false);
	std::vector<type_pair_coord> padres((TERRAIN_SIZE * TERRAIN_SIZE), type_pair_coord(-1, -1));

	PriorityQueue.push(walkstart);

	int curIndex = TERRAIN_INDEX_REPEAT(walkstart.first, walkstart.second);

	visitados[curIndex] = true;

	while (!PriorityQueue.empty())
	{
		type_pair_coord Next = PriorityQueue.front();

		PriorityQueue.pop();

		if (Next == type_pair_coord(m_bDesPositionX, m_bDesPositionY))
		{
			break;
		}

		std::vector<type_pair_coord> roads;
		roads.push_back(type_pair_coord(Next.first + 1, Next.second));
		roads.push_back(type_pair_coord(Next.first - 1, Next.second));
		roads.push_back(type_pair_coord(Next.first, Next.second + 1));
		roads.push_back(type_pair_coord(Next.first, Next.second - 1));

		roads.push_back(type_pair_coord(Next.first + 1, Next.second + 1));
		roads.push_back(type_pair_coord(Next.first - 1, Next.second - 1));
		roads.push_back(type_pair_coord(Next.first - 1, Next.second + 1));
		roads.push_back(type_pair_coord(Next.first + 1, Next.second - 1));

		for (std::vector<type_pair_coord>::const_iterator it = roads.begin(); it != roads.end(); ++it)
		{
			const type_pair_coord& Mark = *it;
			int PositionX = Mark.first;
			int PositionY = Mark.second;

			if (PositionX >= 0 && PositionX < TERRAIN_SIZE && PositionY >= 0 && PositionY < TERRAIN_SIZE)
			{
				curIndex = TERRAIN_INDEX_REPEAT(PositionX, PositionY);

				if (TerrainWall[curIndex] < TW_CHARACTER && !visitados[curIndex])
				{
					PriorityQueue.push(Mark);
					padres[curIndex] = Next;
					visitados[curIndex] = true;
				}
			}
		}
	}

	toTravel.clear();
	m_bNavigation = 0;

	curIndex = TERRAIN_INDEX_REPEAT(m_bDesPositionX, m_bDesPositionY);

	if (padres[curIndex] != type_pair_coord(-1, -1))
	{
		type_pair_coord Next = type_pair_coord(m_bDesPositionX, m_bDesPositionY);

		while (Next != walkstart)
		{
			toTravel.push_back(Next);

			curIndex = TERRAIN_INDEX_REPEAT(Next.first, Next.second);

			Next = padres[curIndex];
		}
		toTravel.push_back(walkstart);
	}

	reverse(toTravel.begin(), toTravel.end());

	return toTravel.size();
}

void SEASON3B::CNewUIMiniMap::ClosingProcess()
{
}

void SEASON3B::CNewUIMiniMap::OpenningProcess()
{
}
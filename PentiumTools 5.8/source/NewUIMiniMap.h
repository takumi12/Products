//////////////////////////////////////////////////////////////////////
// NewUIGuildInfoWindow.h: interface for the CNewUIGuildInfoWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUIMainFrameWindow.h"
#include "NewUIChatLogWindow.h"
#include "NewUIMyInventory.h"

namespace SEASON3B
{
	typedef std::pair<int, int> type_pair_coord;

	class CNewUIMiniMap : public CNewUIObj
	{
	public:
		enum IMAGE_LIST
		{
			IMAGE_MINIMAP_INTERFACE		= BITMAP_MINI_MAP_BEGIN,
		};

		enum MASTER_DATA
		{
			SKILL_ICON_DATA_WDITH = 4,
			SKILL_ICON_DATA_HEIGHT = 8,
			SKILL_ICON_WIDTH = 20,
			SKILL_ICON_HEIGHT = 28,
			SKILL_ICON_STARTX1 = 75,
			SKILL_ICON_STARTY1 = 75,
		};

		enum EVENT_STATE
		{
			EVENT_NONE = 0,
			EVENT_SCROLL_BTN_DOWN,
		};
	private:
		unicode::t_string		m_TooltipText;
		HFONT					m_hToolTipFont;
		DWORD					m_TooltipTextColor;
		CNewUIManager*			m_pNewUIMng;
		POINT					m_Pos;
		POINT					m_Lenth[6];
		int						m_MiniPos;
		CNewUIButton			m_BtnExit;
		MINI_MAP				m_Mini_Map_Data[MAX_MINI_MAP_DATA];
		float					m_Btn_Loc[MAX_MINI_MAP_DATA][4];
		CNewUIButton			m_BtnToolTip;
		bool					m_bSuccess;

		bool m_bMoveEnable;
		size_t m_bNavigation;
		int m_bWorldBackup;
		int m_bCurPositionX;
		int m_bCurPositionY;
		int m_bDesPositionX;
		int m_bDesPositionY;
		DWORD m_bMovemTimeWating;
		std::vector<type_pair_coord> toTravel;
		//std::vector<std::vector<int>> moveTerrain;
	public:
		CNewUIMiniMap();
		virtual ~CNewUIMiniMap();
		
		bool Create(CNewUIManager* pNewUIMng, int x, int y);
		void Release();
		void LoadImages();
		void UnloadImages();
		void SetPos(int x, int y);
		void SetBtnPos(int Num ,float x, float y, float nx,float ny);

		bool UpdateMouseEvent();
		bool UpdateKeyEvent();
		bool Update();
		bool Render();

		float GetLayerDepth();	//. 8.1f
		void LoadImages(const char* Filename );

		bool IsSuccess();
		void how_to_get(int PositionX, int PositionY);
		bool movement_automatic();
		void cancel_movement_automatic();
		void OpenningProcess();
		void ClosingProcess();
	private:
		bool Check_Mouse(int mx,int my);
		bool Check_Btn(int mx,int my);
		bool navigate_on_the_route(int PositionX, int PositionY);
		size_t route_to_destination(const type_pair_coord& walkstart);
	};
}




#if !defined(AFX_NEWUIINVENTORYJEWEL_H__1151C4F9_04A5_47B1_A717_E7905BEEAD08__INCLUDED_)
#define AFX_NEWUIINVENTORYJEWEL_H__1151C4F9_04A5_47B1_A717_E7905BEEAD08__INCLUDED_
#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUIButton.h"
#include "NewUIMyInventory.h"

namespace SEASON3B
{
	class WareHoly
	{
	public:
		WareHoly() :bKeyIndex(0), bItemindex(-1), bItemCount(0) {};
		WareHoly(BYTE KeyIndex, int itemindex, __int64 ItemCount) :
			bKeyIndex(KeyIndex), bItemindex(itemindex), bItemCount(ItemCount){
		};
		~WareHoly() {};

		void setIndex(int index) {
			bItemindex = index;
		};
		int GetIndex() {
			return bItemindex;
		};
		void setValue(__int64 value) {
			bItemCount = value;
		};
		__int64 GetValue() {
			return bItemCount;
		};

	private:
		BYTE bKeyIndex;
		int bItemindex;
		__int64 bItemCount;
	};

	class CNewUIInventoryJewel : public CNewUIObj
	{
		enum IMAGE_LIST
		{
			IMAGE_INVENTORY_BACK = CNewUIMyInventory::IMAGE_INVENTORY_BACK,
			IMAGE_INVENTORY_BACK_TOP2 = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP2,
			IMAGE_INVENTORY_BACK_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
			IMAGE_INVENTORY_BACK_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
			IMAGE_INVENTORY_BACK_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
		};
	public:
		CNewUIInventoryJewel();
		virtual~CNewUIInventoryJewel();
	private:
	/*+012*/ CNewUIManager* m_pNewUIMng;
	/*+020*/ POINT m_Pos;
	int		m_dwCurIndex;
	int		m_dwSelIndex;
	int		m_nSelPage;
	int		m_nMaxPage;

	int		m_nDescLine;
	char	m_aszJobDesc[4][50];

	CNewUIButton m_Button1;
	CNewUIButton m_Button2;

	std::vector<WareHoly>m_bItems;
	public:
		bool Create(CNewUIManager* pNewUIMng, int x, int y);
		void SetPos(int x, int y);
		void InitButtons();
		bool Render();
		bool Update();
		bool UpdateMouseEvent();
		bool UpdateKeyEvent();
		float GetLayerDepth();
		bool CheckExpansionInventory();
		void OpenningProcess();
		void ClosingProcess();
		void RemoveData();
		void setData(BYTE Index, short ItemIndex, __int64 count);
	private:
		void RenderFrame();
		void RenderTexts();
		void RenderInter();
		void RenderButtons();
		void RenderHoly();
		void FrameTarget(float iPos_x, float iPos_y, float width, float height, DWORD color);
	};
}

#endif
// NewUIOptionWindow.h: interface for the CNewUIOptionWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NEWUIOPTIONWINDOW_H__1469FA1D_7C15_4AFE_AD6E_59C303E72BC0__INCLUDED_)
#define AFX_NEWUIOPTIONWINDOW_H__1469FA1D_7C15_4AFE_AD6E_59C303E72BC0__INCLUDED_

#pragma once

#include "NewUIManager.h"
#include "NewUIMyInventory.h"
#include "NewUIQuestProgress.h"

namespace SEASON3B
{
	
	class CNewUIOptionWindow  : public CNewUIObj 
	{
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
	public:
#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM
		enum IMAGE_LIST
		{
			IMAGE_OPTION_FRAME_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
			IMAGE_OPTION_BTN_CLOSE = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_CLOSE,
			IMAGE_OPTION_FRAME_DOWN = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,

			IMAGE_OPTION_FRAME_UP = BITMAP_OPTION_BEGIN,
			IMAGE_OPTION_FRAME_LEFT,
			IMAGE_OPTION_FRAME_RIGHT,
			IMAGE_OPTION_LINE,
			IMAGE_OPTION_POINT,
			IMAGE_OPTION_BTN_CHECK,
			IMAGE_OPTION_EFFECT_BACK,
			IMAGE_OPTION_EFFECT_COLOR,
			IMAGE_OPTION_VOLUME_BACK,
			IMAGE_OPTION_VOLUME_COLOR,
			IMAGE_ND_BTN_L = CNewUIQuestProgress::IMAGE_QP_BTN_L,				// Quest_bt_L.tga	(17,36)
			IMAGE_ND_BTN_R = CNewUIQuestProgress::IMAGE_QP_BTN_R,
			IMAGE_CHECK_LIVE = BITMAP_IMAGE_FRAME_EMU + 10,
			IMAGE_UNCHECK_LIVE = BITMAP_IMAGE_FRAME_EMU + 11,
		};

	public:
		CNewUIOptionWindow();
		virtual ~CNewUIOptionWindow();

		bool Create(CNewUIManager* pNewUIMng, int x, int y);
		void Release();
		
		void SetPos(int x, int y);
		
		bool UpdateMouseEvent();
		bool UpdateKeyEvent();
		bool Update();
		bool Render();

		float GetLayerDepth();	//. 10.5f
		float GetKeyEventOrder();	// 10.f;
		
		void OpenningProcess();
		void ClosingProcess();

		void SetAutoAttack(bool bAuto);
		bool IsAutoAttack();
		void SetWhisperSound(bool bSound);
		bool IsWhisperSound();
		void SetSlideHelp(bool bHelp);
		bool IsSlideHelp();
		void SetVolumeLevel(int iVolume);
		int GetVolumeLevel();
		void SetRenderLevel(int iRender);
		int GetRenderLevel();

		void SetRemoveEffect(bool bHelp);
		bool GetRemoveEffect();
		void SetRemoveEquipment(bool bHelp);
		bool GetRemoveEquipment();
		void SetRemoveTerrain(bool bHelp);
		bool GetRemoveTerrain();
		void SetRemoveObjects(bool bHelp);
		bool GetRemoveObjects();
	private:
		void LoadImages();
		void UnloadImages();

		void SetButtonInfo();

		void RenderFrame();
		void RenderContents();
		void RenderChecked(float x, float y, bool bEnable);
		void RenderButtons();

#ifdef SYSTEM_OPTION_RENDER
		void change_resolution(int index);
		bool FNREG_SZ(char* Name, DWORD dwType, const BYTE* Data, DWORD Size);
#endif // SYSTEM_OPTION_RENDER


	private:
		CNewUIManager*				m_pNewUIMng;
		POINT						m_Pos;

		CNewUIButton m_BtnClose;

		bool m_bAutoAttack;
		bool m_bWhisperSound;
		bool m_bSlideHelp;
		int m_iVolumeLevel;
		int m_iRenderLevel;
		bool m_RenderEffect;
		bool m_RenderEquipment;
		bool m_RenderTerrain;
		bool m_RenderObjects;
	};
	
}

#endif // !defined(AFX_NEWUIOPTIONWINDOW_H__1469FA1D_7C15_4AFE_AD6E_59C303E72BC0__INCLUDED_)

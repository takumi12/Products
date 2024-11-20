#pragma once
#include "NewUIBase.h"
#include "NewUIManager.h"

namespace SEASON3B
{
	class CNewUIRankingTop : public CNewUIObj
	{
		enum IMAGE_LIST
		{
			IMAGE_TOP_BACK1 = BITMAP_IMAGE_FRAME_EMU + 1,
			IMAGE_TOP_BACK2 = BITMAP_IMAGE_FRAME_EMU + 2,
		};
	private:
		CNewUIManager* m_pNewUIMng;
		POINT m_Pos;
		CUIPhotoViewer m_Player;
	public:
		CNewUIRankingTop();
		virtual ~CNewUIRankingTop();

		bool Create(CNewUIManager* pNewUIMng, float x, float y);
		void Release();
		void SetPos(float x, float y);
		void LoadImages();
		void UnloadImages();
		bool UpdateKeyEvent();
		bool UpdateMouseEvent();
		bool Render();
		bool Update();
		float GetLayerDepth(); //. 10.5f



		void OpenningProcess();
		void ClosingProcess();

		void RenderFrame();
		void RenderTexte();
	};
}


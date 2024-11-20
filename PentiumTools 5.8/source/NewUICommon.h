//////////////////////////////////////////////////////////////////////
// NewUICommon.h: interface for the CNewUICommon class.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NEWUICOMMON_H__0668BCBC_7537_454B_82FD_9D6BBBBDBA84__INCLUDED_)
#define AFX_NEWUICOMMON_H__0668BCBC_7537_454B_82FD_9D6BBBBDBA84__INCLUDED_

#pragma once
#include "NewUIScrollBar.h"

namespace SEASON3B
{
	bool CreateOkMessageBox(const unicode::t_string& strMsg, DWORD dwColor = 0xffffffff, float fPriority = 3.f);

	int IsPurchaseShop(); 
	#define g_IsPurchaseShop SEASON3B::IsPurchaseShop()

	bool CheckMouseIn(float x, float y, float width, float height);
	void RenderImageF(GLuint Image, float x, float y, float width, float height);
	void RenderImageF(GLuint uiImageType, float x, float y, float width, float height, float su, float sv, float uw, float vh);
	void RenderImageF(GLuint uiImageType, float x, float y, float width, float height, DWORD color, float su, float sv, float uw, float vh);

	void RenderImage(GLuint uiImageType, float x, float y);
	void RenderImages(GLuint uiImageType, float x, float y, float width, float height, float su, float sv, float uw, float vh);
	void RenderFrameAnimation(GLuint Image, float x, float y, float width, float height, float u, float v, float uw, float vh, float Time, int Numcol, int nFrame, float uFrame = 0.0, float vFrame = 0.0);
	void RenderFrameAnimation2(GLuint Image, float x, float y, float width, float height, float Rotate, float uw, float vh, float Time, int Numcol, int nFrame);
	
	void RenderImage(GLuint uiImageType, float x, float y, float width, float height);
	void RenderImage(GLuint uiImageType, float x, float y, float width, float height, float su, float sv);
	void RenderImage(GLuint uiImageType, float x, float y, float width, float height, float su, float sv, DWORD color);
	void RenderImage(GLuint uiImageType, float x, float y, float width, float height, float su, float sv,float uw, float vh, DWORD color = RGBA(255,255,255,255));
	
	float RenderNumber(float x, float y, int iNum, float fScale = 1.0f);

	void Open3D();
	void Exit3D();
	void RenderLocalItem3D(float sx, float sy, float Width, float Height, int Type, int Level = 0, int Option1 = 0, int ExtOption = 0, bool PickUp = false);
	void Render2Item3D(float sx, float sy, float Width, float Height, int Type, int Level = 0, int Option1 = 0, int ExtOption = 0, bool PickUp = false);


	bool IsNone(int iVirtKey);
	bool IsRelease(int iVirtKey);
	bool IsPress(int iVirtKey);
	bool IsRepeat(int iVirtKey);
	
	class CNewKeyInput
	{
		struct INPUTSTATEINFO 
		{
			BYTE byKeyState;
		} m_pInputInfo[256];

#ifndef ASG_FIX_ACTIVATE_APP_INPUT
		void Init();
#endif

	public:
		enum KEY_STATE 
		{ 
			KEY_NONE=0,
			KEY_RELEASE,
			KEY_PRESS,
			KEY_REPEAT
		};
		~CNewKeyInput();
		
		static CNewKeyInput* GetInstance();
#ifdef ASG_FIX_ACTIVATE_APP_INPUT
		void Init();
#endif	
		void ScanAsyncKeyState();

		bool IsNone(int iVirtKey);
		bool IsRelease(int iVirtKey);
		bool IsPress(int iVirtKey); 
		bool IsRepeat(int iVirtKey);
		void SetKeyState(int iVirtKey, KEY_STATE KeyState);
	protected:
		CNewKeyInput();
	};



	class stSkillList
	{
	public:
		stSkillList() {
			iIndex = -1;
			iX = 0; iY = 0;
			Type = 0;
		};
		virtual~stSkillList() {};
	public:
		int iIndex;
		int iX;
		int iY;
		int Type;
	};

	class CCheckBox
	{
	public:
		CCheckBox() {
			Initialize();
		}
		virtual~CCheckBox() {
			Destroy();
		}
	private:
		HFONT m_hTextFont;
		int m_ImgIndex;
		WORD output_width;
		WORD output_height;
		POINT m_Pos;
		SIZE m_Size;
		unicode::t_string m_Name;
		bool m_bEnable;
	public:
		void Initialize();
		void Destroy();
		void RegisterImgIndex(int imgindex);
		void ChangeCheckBoxInfo(int x, int y, int sx, int sy);
		void ChangeCheckBoxText(unicode::t_string btname);;
		bool Create(int imgindex, int x, int y, int sx, int sy, unicode::t_string btname);
		bool Render();
		bool UpdateMouseEvent();
		void SetPos(int x, int y);
		void SetSize(int width, int height);
		bool IsSelected() {
			return m_bEnable;
		};
		void SetSelected(bool enable) {
			m_bEnable = enable != false;
		}
	};

	class stMacroUIImage
	{
	public:
		stMacroUIImage();
		virtual~stMacroUIImage();
	public:
		POINT m_Pos;
		float output_width;
		float output_height;
		int m_ImgIndex;
	public:
		void Register(int imgindex, int x, int y, int width, int height);
		void Render() {
			RenderImage(m_ImgIndex, m_Pos.x, m_Pos.y, output_width, output_height);
		};
	};

	class stMacroUIText
	{
	public:
		stMacroUIText();
		virtual~stMacroUIText();
	public:
		POINT m_Pos;
		unicode::t_string m_Name;
	public:
		void Register(int x, int y, const char* pzText);
		void Render();
	};

	class CSlideBar
	{
	public:
		CSlideBar();
		virtual~CSlideBar();
	private:
		POINT m_Pos;
		__int16 m_iValue;
		__int16 m_MaxLength;
		__int16 m_MinLength;
		int m_Imgindex;
		int m_ImgBack;
		__int16 m_Width;
		__int16 m_Height;
	public:
		void Create(int imgback, int imgindex, int x, int y, __int16 sx, __int16 sy, __int16 iMaxLength, __int16 start);
		bool MouseUpdate();
		void Render();
		int GetSlideLevel();
		void SetSlideLevel(__int16 Value);
	};

	class COptionButtonGroup
	{
	public:
		COptionButtonGroup();
		virtual~COptionButtonGroup();
	private:
		std::vector<CCheckBox*> m_box;
		int m_imgindex;
		int m_index;
		int m_x;
		int m_y;
		int m_w;
		int m_h;
	public:
		void Create(int imgindex, int x, int y, int w, int h, unsigned char count);
		void SetOptionText(unsigned char index, int offset_x, int offset_y, const char* text);
		bool UpdateMouseEvent();
		bool Render();
		void SetIndex(int index);
		int GetIndex();
	};

	struct ItemComboBox {
		int id;
		std::string text;

		// Constructor de conveniencia
		ItemComboBox(int id, const std::string& text) : id(id), text(text) {}
	};

#ifdef EFFECT_MNG_HANDLE
	class CInputComboBox
	{
		enum IMAGE_LIST
		{
			IMAGE_BTN_UP = BITMAP_CURSEDTEMPLE_BEGIN + 14,
			IMAGE_BTN_DOWN,
		};
	private:
		CNewUIScrollBar m_ScrollBar;
		bool isVisible;
		size_t m_iViewCount;
		int selectedIndex;
		int posX, posY, width, height;
		std::vector<ItemComboBox> objects;
	public:
		CInputComboBox();
		virtual~CInputComboBox();
		void setLocation(int x, int y);
		void setBounds(int x, int y, int sx, int sy);
		void addItem(const ItemComboBox& object);
		void SelectedIndex(int index);
		void RemoveAll();
		int getSelectedIndex() const;
		const char* getSelectedText();
		const char* getIndexText(int index);
		int getSelectedId();


		void Render();
		bool UpdateMouse();
		bool Update();
	private:
		void RenderFrameWork();
	};
#endif // EFFECT_MNG_HANDLE

#ifdef SYSTEM_OPTION_RENDER
	void ChangeWindowResolution(HWND& hWnd, int newWidth, int newHeight);
#endif // SYSTEM_OPTION_RENDER
}

#define g_pNewKeyInput	SEASON3B::CNewKeyInput::GetInstance()

#endif // !defined(AFX_NEWUICOMMON_H__0668BCBC_7537_454B_82FD_9D6BBBBDBA84__INCLUDED_)

// NewUICommon.cpp: implementation of the CNewUICommon class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "NewUICommon.h"
#include "NewUIRenderNumber.h"
#include "NewUISystem.h"
#include "NewUICommonMessageBox.h"
#include "ZzzTexture.h"
#include "ZzzOpenglUtil.h"

extern bool g_bWndActive;
extern BOOL g_bUseWindowMode;
extern float MouseX, MouseY;

bool SEASON3B::CreateOkMessageBox(const unicode::t_string& strMsg, DWORD dwColor, float fPriority)
{
	CNewUICommonMessageBox* pMsgBox = g_MessageBox->NewMessageBox(MSGBOX_CLASS(CNewUICommonMessageBox));
	if(pMsgBox)
	{
		return pMsgBox->Create(MSGBOX_COMMON_TYPE_OK, strMsg, dwColor);
	}
	return false;
}

int SEASON3B::IsPurchaseShop() 
{ 
	if( g_pMyShopInventory->IsVisible() ) 
	{
		return 1;
	}
	else if( g_pPurchaseShopInventory->IsVisible() ) 
	{
		return 2;
	}

	return -1;
}

bool SEASON3B::CheckMouseIn(float x, float y, float width, float height)
{
	if(MouseX >= x && MouseX < x + width && MouseY >= y && MouseY < y + height)
		return true;
	return false;
}

void SEASON3B::RenderImageF(GLuint uiImageType, float x, float y, float width, float height)
{
	BITMAP_t* pText = &Bitmaps[uiImageType];

	RenderBitmap(uiImageType, x, y, width, height, 0.0, 0.0, pText->output_width / pText->Width, pText->output_height / pText->Height, true, true, 0.0);
}

void SEASON3B::RenderImageF(GLuint uiImageType, float x, float y, float width, float height, float su, float sv, float uw, float vh)
{
	BITMAP_t* pText = &Bitmaps[uiImageType];
	RenderBitmap(uiImageType, x, y, width, height, su / pText->Width, sv / pText->Height, uw / pText->Width, vh / pText->Height, true, true, 0.0);
}

void SEASON3B::RenderImageF(GLuint uiImageType, float x, float y, float width, float height, DWORD color, float su, float sv, float uw, float vh)
{
	BITMAP_t* pText = &Bitmaps[uiImageType];
	RenderColorBitmap(uiImageType, x, y, width, height, su / pText->Width, sv / pText->Height, uw / pText->Width, vh / pText->Height, color);
}

void SEASON3B::RenderImage(GLuint uiImageType, float x, float y)
{
	BITMAP_t* pImage = &Bitmaps[uiImageType];
	float u = 0.5 / pImage->Width;
	float v = 0.5 / pImage->Height;

	float temp_Width = ((float)pImage->output_width - 0.5) / pImage->Width;
	float temp_Height = ((float)pImage->output_height - 0.5) / pImage->Height;

	float vHeight = temp_Height - v;
	float uWidth = temp_Width - u;

	float Height = (double)pImage->output_height;
	float Width = (double)pImage->output_width;

	RenderBitmap(uiImageType, x, y, Width, Height, u, v, uWidth, vHeight);
}

void SEASON3B::RenderImages(GLuint uiImageType, float x, float y, float width, float height, float x1, float y1, float x2, float y2)
{
	BITMAP_t* pText = &Bitmaps[uiImageType];

	float uw = x2 - x1;
	float vh = y2 - y1;
	RenderBitmap(uiImageType, x, y, width, height, x1 / pText->Width, y1 / pText->Height, uw / pText->Width, vh / pText->Height, true, true, 0.0);
}

void SEASON3B::RenderFrameAnimation(GLuint Image, float x, float y, float width, float height, float u, float v, float uw, float vh, float Time, int Numcol, int nFrame, float uFrame, float vFrame)
{
	float factor = 0.0;
	float FrameX, FrameY;

	DWORD currentTime = timeGetTime();
	factor = nFrame / (Time * CLOCKS_PER_SEC);

	int frame = (int)(currentTime * factor) % nFrame;

	if (uFrame == 0.0)
		FrameX = (double)(frame % Numcol) * uw;
	else
		FrameX = (double)(frame % Numcol) * uFrame;

	if (vFrame == 0.0)
		FrameY = (double)(frame / Numcol) * vh;
	else
		FrameY = (double)(frame / Numcol) * vFrame;

	RenderImageF(Image, x, y, width, height, u + FrameX, v + FrameY, uw, vh);
}

void SEASON3B::RenderFrameAnimation2(GLuint Image, float x, float y, float width, float height, float Rotate, float uw, float vh, float Time, int Numcol, int nFrame)
{
	float factor = 0.0;
	float FrameX, FrameY;

	DWORD currentTime = timeGetTime();
	factor = nFrame / (Time * CLOCKS_PER_SEC);

	int frame = (int)(currentTime * factor) % nFrame;

	FrameX = (double)(frame % Numcol) * uw;

	FrameY = (double)(frame / Numcol) * vh;

	RenderBitmapLocalRotate2(Image, x, y, width, height, Rotate, FrameX, FrameY, uw, vh);
}

void SEASON3B::RenderImage(GLuint uiImageType, float x, float y, float width, float height)
{
	BITMAP_t *pImage = &Bitmaps[uiImageType];

	float u, v, uw, vh;

	u = 0.5f / (float)pImage->Width;
	v = 0.5f / (float)pImage->Height;
	uw = (width - 0.5f) / (float)pImage->Width;
	vh = (height - 0.5f) / (float)pImage->Height;

	RenderBitmap(uiImageType, x, y, width, height, u, v, uw - u, vh - v);	
}

void SEASON3B::RenderImage(GLuint uiImageType, float x, float y, float width, float height, float su, float sv,float uw, float vh, DWORD color)
{
	RenderColorBitmap(uiImageType, x, y, width, height, su, sv, uw, vh, color);
}

void SEASON3B::RenderImage(GLuint uiImageType, float x, float y, float width, float height, float su, float sv)
{
	BITMAP_t *pImage = &Bitmaps[uiImageType];

	float u, v, uw, vh;
	u = ((su + 0.5f) / (float)pImage->Width);
	v = ((sv + 0.5f) / (float)pImage->Height);
	uw = (width - 0.5f) / (float)pImage->Width - (0.5f / (float)pImage->Width);
	vh = (height - 0.5f) / (float)pImage->Height - (0.5f / (float)pImage->Height);

	RenderBitmap(uiImageType, x, y, width, height, u, v, uw, vh);
}

void SEASON3B::RenderImage(GLuint uiImageType, float x, float y, float width, float height, float su, float sv, DWORD color)
{
	BITMAP_t *pImage = &Bitmaps[uiImageType];

	float u, v, uw, vh;
	u = ((su + 0.5f) / (float)pImage->Width);
	v = ((sv + 0.5f) / (float)pImage->Height);
	uw = (width - 0.5f) / (float)pImage->Width - (0.5f / (float)pImage->Width);
	vh = (height - 0.5f) / (float)pImage->Height - (0.5f / (float)pImage->Height);

	RenderColorBitmap(uiImageType, x, y, width, height, u, v, uw, vh, color);	
}

float SEASON3B::RenderNumber(float x, float y, int iNum, float fScale)
{
	return g_RenderNumber->RenderNumber(x, y, iNum, fScale);
}

void SEASON3B::Open3D()
{
	EndBitmap();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glViewport2(0, 0, WindowWidth, WindowHeight);
	gluPerspective2(1.f, (float)(WindowWidth) / (float)(WindowHeight), RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	GetOpenGLMatrix(CameraMatrix);
	EnableDepthTest();
	EnableDepthMask();
	glClear(GL_DEPTH_BUFFER_BIT);
}

void SEASON3B::Exit3D()
{
	UpdateMousePositionn();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	BeginBitmap();
}

void SEASON3B::RenderLocalItem3D(float sx, float sy, float Width, float Height, int Type, int Level, int Option1, int ExtOption, bool PickUp)
{
	SEASON3B::Open3D();

	RenderItem3D(sx, sy, Width, Height, Type, Level, Option1, ExtOption, PickUp);

	SEASON3B::Exit3D();
}

void SEASON3B::Render2Item3D(float sx, float sy, float Width, float Height, int Type, int Level, int Option1, int ExtOption, bool PickUp)
{
	Script_Item* item_info = GMItemMng->find(Type);

	if (item_info && item_info->SubIndex != -1)
	{
		float width = item_info->Width * 20;
		float hight = item_info->Height * 20;

		sx = sx + ((width - Width) / 2.f);
		sy = sy + ((hight - Height) / 2.f);

		RenderItem3D(sx, sy, width, hight, Type, Level, Option1, ExtOption, PickUp);
	}
}

bool SEASON3B::IsNone(int iVirtKey)
{ 
	return g_pNewKeyInput->IsNone(iVirtKey); 
}

bool SEASON3B::IsRelease(int iVirtKey)
{ 
	return g_pNewKeyInput->IsRelease(iVirtKey); 
}

bool SEASON3B::IsPress(int iVirtKey)
{ 
	return g_pNewKeyInput->IsPress(iVirtKey); 
}

bool SEASON3B::IsRepeat(int iVirtKey)
{ 
	return g_pNewKeyInput->IsRepeat(iVirtKey); 
}

SEASON3B::CNewKeyInput::CNewKeyInput()
{
	Init();
}

SEASON3B::CNewKeyInput::~CNewKeyInput()
{

}

void SEASON3B::CNewKeyInput::Init()
{
	memset(&m_pInputInfo, 0, sizeof(INPUTSTATEINFO)*256);
}

SEASON3B::CNewKeyInput* SEASON3B::CNewKeyInput::GetInstance()
{
	static SEASON3B::CNewKeyInput s_Instance;
	return &s_Instance;
}

void SEASON3B::CNewKeyInput::ScanAsyncKeyState()
{
	if (g_bWndActive)
	{
		for (int key = 0; key < 256; key++)
		{
			if (HIBYTE(GetAsyncKeyState(key)) & 0x80)
			{
				if (m_pInputInfo[key].byKeyState == KEY_NONE || m_pInputInfo[key].byKeyState == KEY_RELEASE)
				{
					// press event (key was up before but down now)
					m_pInputInfo[key].byKeyState = KEY_PRESS;
				}
				else if (m_pInputInfo[key].byKeyState == KEY_PRESS)
				{
					// drag event (key is still down)
					m_pInputInfo[key].byKeyState = KEY_REPEAT;
				}
			}
			else // Key is not currently pressed
			{
				if (m_pInputInfo[key].byKeyState == KEY_REPEAT || m_pInputInfo[key].byKeyState == KEY_PRESS)
				{
					// release event (key was down before but up now)
					m_pInputInfo[key].byKeyState = KEY_RELEASE;
				}
				else if (m_pInputInfo[key].byKeyState == KEY_RELEASE)
				{
					m_pInputInfo[key].byKeyState = KEY_NONE;
				}
			}
		}

		if (IsPress(VK_RETURN) && IsEnterPressed() == false) {
			m_pInputInfo[VK_RETURN].byKeyState = KEY_NONE;
		}
		SetEnterPressed(false);
	}
}

bool SEASON3B::CNewKeyInput::IsNone(int iVirtKey)
{
	if (g_bWndActive)
		return (m_pInputInfo[iVirtKey].byKeyState == KEY_NONE) ? true : false;
	else
		return false;
}

bool SEASON3B::CNewKeyInput::IsRelease(int iVirtKey)
{
	if (g_bWndActive)
		return (m_pInputInfo[iVirtKey].byKeyState == KEY_RELEASE) ? true : false;
	else
		return false;
}

bool SEASON3B::CNewKeyInput::IsPress(int iVirtKey)
{
	if (g_bWndActive)
		return (m_pInputInfo[iVirtKey].byKeyState == KEY_PRESS) ? true : false;
	else
		return false;
}

bool SEASON3B::CNewKeyInput::IsRepeat(int iVirtKey)
{
	if (g_bWndActive)
		return (m_pInputInfo[iVirtKey].byKeyState == KEY_REPEAT) ? true : false;
	else
		return false;
}

void SEASON3B::CNewKeyInput::SetKeyState(int iVirtKey, KEY_STATE KeyState)
{
	m_pInputInfo[iVirtKey].byKeyState = KeyState;
}


void SEASON3B::CCheckBox::Initialize()
{
	m_hTextFont = g_hFont;
	m_ImgIndex = 0;
	output_width = 0;
	output_height = 0;
	m_bEnable = 0;
}

void SEASON3B::CCheckBox::Destroy()
{
}

void SEASON3B::CCheckBox::RegisterImgIndex(int imgindex)
{
	m_ImgIndex = imgindex;

	if (m_ImgIndex != -1)
	{
		BITMAP_t* pImage = &Bitmaps[m_ImgIndex];

		output_width = pImage->Width;
		output_height = pImage->Height;
	}
}

void SEASON3B::CCheckBox::ChangeCheckBoxInfo(int x, int y, int sx, int sy)
{
	SetPos(x, y);
	SetSize(sx, sy);
}

void SEASON3B::CCheckBox::ChangeCheckBoxText(unicode::t_string btname)
{
	m_Name = btname;
}

bool SEASON3B::CCheckBox::Create(int imgindex, int x, int y, int sx, int sy, unicode::t_string btname)
{
	RegisterImgIndex(imgindex);
	ChangeCheckBoxInfo(x, y, sx, sy);
	ChangeCheckBoxText(btname);
	return true;
}

bool SEASON3B::CCheckBox::Render()
{
	float sv = (double)(m_Size.cy * (m_bEnable == false));
	RenderImage(m_ImgIndex, (float)m_Pos.x, (float)m_Pos.y, (float)m_Size.cx, (float)m_Size.cy, 0.0, sv);
	g_pRenderText->RenderText(m_Pos.x + m_Size.cx, m_Pos.y + 4, m_Name.c_str(), 0, 0, RT3_SORT_LEFT, 0);

	return true;
}

bool SEASON3B::CCheckBox::UpdateMouseEvent()
{
	bool success = false;

	if (IsRelease(VK_LBUTTON) && CheckMouseIn(m_Pos.x, m_Pos.y, m_Size.cx, m_Size.cy))
	{
		success = true;
		m_bEnable = m_bEnable == false;
	}

	return success;
}

void SEASON3B::CCheckBox::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;
}

void SEASON3B::CCheckBox::SetSize(int width, int height)
{
	m_Size.cx = width;
	m_Size.cy = height;
}

SEASON3B::stMacroUIImage::stMacroUIImage()
{
	m_ImgIndex = -1;
	m_Pos.x = m_Pos.y = 0;
	output_width = 0.0;
	output_height = 0.0;
}

SEASON3B::stMacroUIImage::~stMacroUIImage()
{
}

void SEASON3B::stMacroUIImage::Register(int imgindex, int x, int y, int width, int height)
{
	m_ImgIndex = imgindex;
	m_Pos.x = x;
	m_Pos.y = y;
	output_width = width;
	output_height = height;
}

SEASON3B::stMacroUIText::stMacroUIText()
{
	m_Pos.x = m_Pos.y = 0;
	m_Name = "";
}

SEASON3B::stMacroUIText::~stMacroUIText()
{
}

void SEASON3B::stMacroUIText::Register(int x, int y, const char* pzText)
{
	m_Pos.x = x;
	m_Pos.y = y;
	m_Name = pzText;
}

void SEASON3B::stMacroUIText::Render()
{
	g_pRenderText->RenderText(m_Pos.x, m_Pos.y, m_Name.c_str(), 0, 0, RT3_SORT_LEFT, 0);
}

SEASON3B::CSlideBar::CSlideBar()
{
	m_Imgindex = 0;
	m_ImgBack = 0;
	m_Pos.x = 0;
	m_Pos.y = 0;
	m_Width = 0;
	m_Height = 0;
	m_MaxLength = 0;
	m_iValue = 0;
	m_MinLength = 0;
}

SEASON3B::CSlideBar::~CSlideBar()
{
}

void SEASON3B::CSlideBar::Create(int imgback, int imgindex, int x, int y, __int16 sx, __int16 sy, __int16 iMaxLength, __int16 start)
{
	m_Imgindex = imgindex;
	m_ImgBack = imgback;
	m_Pos.x = x;
	m_Pos.y = y;
	m_Width = sx;
	m_Height = sy;
	m_MaxLength = iMaxLength;
	m_MinLength = start;
}

bool SEASON3B::CSlideBar::MouseUpdate()
{
	int min = m_Width / m_MaxLength;
	if (CheckMouseIn(m_Pos.x - min, m_Pos.y, min + m_Width, m_Height) && IsRepeat(VK_LBUTTON))
	{
		int current = MouseX - m_Pos.x;
		if (current >= m_MinLength)
		{
			m_iValue = (current * m_MaxLength / m_Width) + 1;
		}
		else
		{
			m_iValue = m_MinLength;
		}
	}
	return true;
}

void SEASON3B::CSlideBar::Render()
{
	RenderImage(m_ImgBack, m_Pos.x, m_Pos.y, m_Width, m_Height);
	if (m_iValue > 0)
	{
		float width = (double)m_Width * 0.1 * m_iValue;

		RenderImage(m_Imgindex, m_Pos.x, m_Pos.y, width, m_Height);
	}
}

int SEASON3B::CSlideBar::GetSlideLevel()
{
	return m_iValue;
}

void SEASON3B::CSlideBar::SetSlideLevel(__int16 Value)
{
	m_iValue = Value;
}

SEASON3B::COptionButtonGroup::COptionButtonGroup()
{
	m_x = 0;
	m_y = 0;
	m_w = 0;
	m_h = 0;
	m_index = 0;
	m_imgindex = 0;
}

SEASON3B::COptionButtonGroup::~COptionButtonGroup()
{
	std::vector<CCheckBox*>::iterator iter = this->m_box.begin();
	while (iter != this->m_box.end())
	{
		delete* iter;
		iter++;
	}
	this->m_box.shrink_to_fit();
	this->m_box.clear();
}

void SEASON3B::COptionButtonGroup::Create(int imgindex, int x, int y, int w, int h, unsigned char count)
{
	this->m_x = x;
	this->m_y = y;
	this->m_w = w;
	this->m_h = h;
	for (int i = 0; i < count; i++)
	{
		CCheckBox* box = new CCheckBox;
		box->RegisterImgIndex(imgindex);
		box->ChangeCheckBoxInfo(0, 0, w, h);
		this->m_box.push_back(box);
	}
}

void SEASON3B::COptionButtonGroup::SetOptionText(unsigned char index, int offset_x, int offset_y, const char* text)
{
	unicode::t_string strText = text;
	this->m_box[index]->SetPos(this->m_x + offset_x, this->m_y + offset_y);
	this->m_box[index]->ChangeCheckBoxText(strText);
}

bool SEASON3B::COptionButtonGroup::UpdateMouseEvent()
{
	for (size_t i = 0; i < this->m_box.size(); i++)
	{
		if (true == this->m_box[i]->UpdateMouseEvent())
		{
			if (true == this->m_box[i]->IsSelected())
			{
				SetIndex(i);
				return true;
			}
		}
	}
	return false;
}

bool SEASON3B::COptionButtonGroup::Render()
{
	for (size_t i = 0; i < this->m_box.size(); i++)
	{
		this->m_box[i]->Render();
	}
	return true;
}

void SEASON3B::COptionButtonGroup::SetIndex(int index)
{
	if ((unsigned int)index < this->m_box.size())
		m_index = index;
	else
		m_index = 0;

	for (size_t i = 0; i < this->m_box.size(); i++)
	{
		if (i == this->m_index)
			this->m_box[i]->SetSelected(true);
		else
			this->m_box[i]->SetSelected(false);
	}
}

int SEASON3B::COptionButtonGroup::GetIndex()
{
	return m_index;
}

#ifdef EFFECT_MNG_HANDLE
SEASON3B::CInputComboBox::CInputComboBox()
{
	isVisible = false;
	m_iViewCount = 6;
	selectedIndex = -1;
	posX = 0;
	posY = 0;
	width = 50;
	height = 13;

	m_ScrollBar.Create(0, 0, height* m_iViewCount);
	m_ScrollBar.SetPercent(0);
}

SEASON3B::CInputComboBox::~CInputComboBox()
{
	RemoveAll();
}

void SEASON3B::CInputComboBox::setLocation(int x, int y)
{
	posX = x;
	posY = y;
	m_ScrollBar.SetPos(posX + (width - 14), posY + (height + 2));
}

void SEASON3B::CInputComboBox::setBounds(int x, int y, int sx, int sy)
{
	width = sx;
	height = sy;
	setLocation(x, y);
}

void SEASON3B::CInputComboBox::addItem(const ItemComboBox& object)
{
	objects.push_back(object);
}

void SEASON3B::CInputComboBox::SelectedIndex(int index)
{
	if (index >= 0 && index < (int)objects.size())
	{
		selectedIndex = index;
		m_ScrollBar.SetPercent(0);
	}
}

void SEASON3B::CInputComboBox::RemoveAll()
{
	selectedIndex = -1;
	m_ScrollBar.SetPercent(0);
	objects.clear();
}

int SEASON3B::CInputComboBox::getSelectedIndex() const
{
	return selectedIndex;
}

const char* SEASON3B::CInputComboBox::getSelectedText()
{
	if (selectedIndex >= 0 && selectedIndex < (int)objects.size())
	{
		return objects[selectedIndex].text.c_str();
	}
	else {
		return "None";
	}
}

const char* SEASON3B::CInputComboBox::getIndexText(int index)
{
	if (index >= 0 && index < (int)objects.size())
	{
		return objects[index].text.c_str();
	}
	else {
		return "None";
	}
}

int SEASON3B::CInputComboBox::getSelectedId()
{
	if (selectedIndex >= 0 && selectedIndex < (int)objects.size())
	{
		return objects[selectedIndex].id;
	}
	else {
		return -1;
	}
}

void SEASON3B::CInputComboBox::Render()
{
	EnableAlphaTest();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	//SEASON3B::RenderImage(BITMAP_GUILDMAKE_BEGIN, posX, posY, 108.f, 23.f);

	int btX = posX + width - 13;
	int btY = posY;

	if (isVisible == true)
	{
		if (SEASON3B::CheckMouseIn(btX, posY, 15.f, 13.f))
		{
			if (MouseLButton)
				SEASON3B::RenderImage(IMAGE_BTN_UP, btX, btY, 15.f, 13.f, 0.0, 26.f);
			else
				SEASON3B::RenderImage(IMAGE_BTN_UP, btX, btY, 15.f, 13.f, 0.0, 13.f);
		}
		else
		{
			SEASON3B::RenderImage(IMAGE_BTN_UP, btX, btY, 15.f, 13.f, 0.0, 0.0);
		}
	}
	else
	{
		if (SEASON3B::CheckMouseIn(btX, btY, 15.f, 13.f))
		{
			if (MouseLButton)
				SEASON3B::RenderImage(IMAGE_BTN_DOWN, btX, btY, 15.f, 13.f, 0.0, 26.f);
			else
				SEASON3B::RenderImage(IMAGE_BTN_DOWN, btX, btY, 15.f, 13.f, 0.0, 13.f);
		}
		else
		{
			SEASON3B::RenderImage(IMAGE_BTN_DOWN, btX, btY, 15.f, 13.f, 0.0, 0.0);
		}
	}

	this->RenderFrameWork();
}

void SEASON3B::CInputComboBox::RenderFrameWork()
{
	g_pRenderText->SetBgColor(0);

	g_pRenderText->SetTextColor(-1);

	g_pRenderText->SetFont(g_hFont);

	g_pRenderText->RenderFont(posX + 4, posY, getSelectedText(), (width - 16), height, 1);

	if (isVisible == true)
	{
		size_t secure = 0;
		size_t currentLoop = 0;
		size_t Loop = objects.size();

		if (Loop > m_iViewCount)
		{
			double prev = m_ScrollBar.GetPercent();
			currentLoop = (size_t)((double)(unsigned int)(Loop - m_iViewCount) * prev);
		}

		int Y = posY + (height + 2);

		glColor4f(0.f, 0.f, 0.f, 0.8f);
		RenderColor(posX, Y, width, (height * m_iViewCount));
		EndRenderColor();

		for (size_t i = currentLoop; i < Loop && secure < m_iViewCount; i++, secure++)
		{
			if (SEASON3B::CheckMouseIn(posX, Y, (width - 16), height))
			{
				if (SEASON3B::IsRelease(VK_LBUTTON))
				{
					SelectedIndex(i);
					isVisible = false;
				}
			}
			g_pRenderText->RenderFont(posX + 4, Y, getIndexText(i), (width - 16), height, 1);
			Y += height;
		}

		m_ScrollBar.Render();
	}
}

bool SEASON3B::CInputComboBox::UpdateMouse()
{
	int btX = posX + width - 13;

	if (SEASON3B::CheckMouseIn(btX, posY, 15.f, 13.f))
	{
		if (SEASON3B::IsRelease(VK_LBUTTON))
		{
			isVisible = !isVisible;
		}
		return true;
	}
	if (isVisible == true)
	{
		m_ScrollBar.Update();
		m_ScrollBar.UpdateMouseEvent();

		if (SEASON3B::CheckMouseIn(posX, posY + (height + 2), width, (height * m_iViewCount)))
		{
			return true;
		}
	}

	return false;
}

bool SEASON3B::CInputComboBox::Update()
{
	return false;
}
#endif // EFFECT_MNG_HANDLE



#ifdef SYSTEM_OPTION_RENDER
void SEASON3B::ChangeWindowResolution(HWND& hWnd, int newWidth, int newHeight)
{
	RECT rc = { 0, 0, newWidth, newHeight };

	// Ajustar el rectángulo de acuerdo al modo de ventana o pantalla completa
	if (g_bUseWindowMode == TRUE)
	{
		AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN, NULL);
	}
	else
	{
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL);
	}

	rc.right -= rc.left;
	rc.bottom -= rc.top;

	// Obtener la posición centrada para la ventana en modo ventana
	int posX = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
	int posY = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

	if (hWnd != NULL)
	{
		SetWindowPos(hWnd, HWND_TOP, posX, posY, rc.right, rc.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
	}
}
#endif // SYSTEM_OPTION_RENDER

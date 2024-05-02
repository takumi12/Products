#pragma once
typedef void (*GFxEIHandler__OnLoad)(int, int);
typedef void (*GFxEIHandler__OnRender)();



typedef void (*GFxEIHandler__Show)(int);
typedef void (*GFxEIHandler__Toggle)(int);
typedef bool (*GFxEIHandler__IsVisible)(int);
typedef bool (*GFxEIHandler__Update)(int);
typedef bool (*GFxEIHandler__Render)(int);
typedef bool (*GFxEIHandler__AddObj)(int, const char*);
//typedef void (*GFxEIHandler__WndProc)(HWND, UINT, WPARAM, LPARAM);

class CGFxEIHandler
{
public:
	CGFxEIHandler();
	virtual~CGFxEIHandler();
	void DinamicLibrary();
	static CGFxEIHandler* Instance();
public:
	void OnLoad(int width, int height);
	void OnRender();
	void OnUpdate();
	bool onLoad;
	/*void Hide(int index);
	void Show(int index);
	void Toggle(int index);
	bool Update(int index);
	bool Render(int index);
	bool IsVisible(int index);
	bool AddObj(int index, const char* Filename);

	void SrcOfFuntion(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);*/
};

#define GFxEIHandler				(CGFxEIHandler::Instance())

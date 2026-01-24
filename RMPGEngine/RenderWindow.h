#pragma once
#include "ErrorLogger.h"

class WindowContainer;

class RenderWindow
{
public:
	bool Initialize(WindowContainer* pWindowContainer, HINSTANCE hInstance, std::string window_title, std::string window_class, int width, int height);
	bool ProcessMessages();
	HWND GetHWND() const;
	bool SetWindowSize(int width, int height);
	bool SetFullScreen(bool fullscreen);
	~RenderWindow();

private:
	void RegisterWindowClass();
	HWND handle = NULL;
	HINSTANCE hInstance = NULL;
	std::string window_title = "";
	std::wstring window_title_wide = L"";
	std::string window_class = "";
	std::wstring window_class_wide = L"";
	int width = 0;
	int height = 0;

	bool isFullscreen = false;
	RECT windowRectBeforeFullscreen = { 0 };
	LONG_PTR savedStyle = 0;
	LONG_PTR savedExStyle = 0;

};

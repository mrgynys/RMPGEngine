#include "WindowContainer.h"

bool RenderWindow::Initialize(WindowContainer* pWindowContainer, HINSTANCE hInstance, std::string window_title, std::string window_class, int width, int height)
{
	this->hInstance = hInstance;
	this->window_title = window_title;
	this->window_title_wide = StringConverter::StringToWide(this->window_title);
	this->window_class = window_class;
	this->window_class_wide = StringConverter::StringToWide(this->window_class);
	this->width = width;
	this->height = height;
	
	this->RegisterWindowClass();

	int centerScreenX = GetSystemMetrics(SM_CXSCREEN) / 2 - this->width / 2;
	int centerScreenY = GetSystemMetrics(SM_CYSCREEN) / 2 - this->height / 2;

	RECT wr;
	wr.left = centerScreenX;
	wr.top = centerScreenY;
	wr.right = wr.left + this->width;
	wr.bottom = wr.top + this->height;
	AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);

	this->handle = CreateWindowEx(
		0,
		this->window_class_wide.c_str(),
		this->window_title_wide.c_str(),
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		wr.left,
		wr.top,
		wr.right - wr.left,
		wr.bottom - wr.top,
		NULL,
		NULL,
		this->hInstance,
		pWindowContainer
	);

	if (this->handle == NULL)
	{
		ErrorLogger::Log(GetLastError(), "CreateWindowEX Failed for window: " + this->window_title);
		return false;
	}

	ShowWindow(this->handle, SW_SHOW);
	SetForegroundWindow(this->handle);
	SetFocus(this->handle);
	
	return true;
}

bool RenderWindow::ProcessMessages()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (PeekMessage(&msg,
		this->handle,
		0,
		0,
		PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_NULL)
	{
		if (!IsWindow(this->handle))
		{
			this->handle = NULL;
			UnregisterClass(this->window_class_wide.c_str(), this->hInstance);
			return false;
		}
	}

	return true;
}

HWND RenderWindow::GetHWND() const
{
	return this->handle;
}

bool RenderWindow::SetWindowSize(int width, int height)
{
	if (this->handle == NULL) return false;
	if (width <= 0 || height <= 0) return false;

	this->width = width;
	this->height = height;

	RECT wr = { 0, 0, this->width, this->height };
	DWORD style = (DWORD)GetWindowLongPtr(this->handle, GWL_STYLE);
	AdjustWindowRect(&wr, style, FALSE);

	int w = wr.right - wr.left;
	int h = wr.bottom - wr.top;

	int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
	int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

	BOOL ok = SetWindowPos(this->handle, NULL, x, y, w, h, SWP_NOZORDER | SWP_SHOWWINDOW);
	if (!ok)
	{
		ErrorLogger::Log(GetLastError(), "SetWindowPos failed in SetWindowSize.");
		return false;
	}

	return true;
}

bool RenderWindow::SetFullScreen(bool fullscreen)
{
	if (this->handle == NULL) return false;

	if (fullscreen && !this->isFullscreen)
	{
		this->savedStyle = GetWindowLongPtr(this->handle, GWL_STYLE);
		this->savedExStyle = GetWindowLongPtr(this->handle, GWL_EXSTYLE);
		GetWindowRect(this->handle, &this->windowRectBeforeFullscreen);

		LONG_PTR newStyle = this->savedStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
		LONG_PTR newExStyle = this->savedExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

		SetWindowLongPtr(this->handle, GWL_STYLE, newStyle);
		SetWindowLongPtr(this->handle, GWL_EXSTYLE, newExStyle);

		HMONITOR hmon = MonitorFromWindow(this->handle, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = {};
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hmon, &mi);

		int width = mi.rcMonitor.right - mi.rcMonitor.left;
		int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

		BOOL ok = SetWindowPos(this->handle, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, width, height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		if (!ok)
		{
			ErrorLogger::Log(GetLastError(), "SetWindowPos failed in SetFullScreen enter.");
			return false;
		}

		this->isFullscreen = true;
		return true;
	}
	else if (!fullscreen && this->isFullscreen)
	{
		SetWindowLongPtr(this->handle, GWL_STYLE, this->savedStyle);
		SetWindowLongPtr(this->handle, GWL_EXSTYLE, this->savedExStyle);

		BOOL ok = SetWindowPos(this->handle, HWND_NOTOPMOST,
			this->windowRectBeforeFullscreen.left,
			this->windowRectBeforeFullscreen.top,
			this->windowRectBeforeFullscreen.right - this->windowRectBeforeFullscreen.left,
			this->windowRectBeforeFullscreen.bottom - this->windowRectBeforeFullscreen.top,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);

		if (!ok)
		{
			ErrorLogger::Log(GetLastError(), "SetWindowPos failed in SetFullScreen restore.");
			return false;
		}

		this->isFullscreen = false;
		return true;
	}

	return true;
}

RenderWindow::~RenderWindow()
{
	if (this->handle != NULL)
	{
		UnregisterClass(this->window_class_wide.c_str(), this->hInstance);
		DestroyWindow(handle);
	}
}

LRESULT CALLBACK HandleMsgRedirect(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hwnd);
		return 0;
	}
	default:
	{
		WindowContainer* const pWindow = reinterpret_cast<WindowContainer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return pWindow->WindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
}

LRESULT CALLBACK HandleMessageSetup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		WindowContainer* pWindow = reinterpret_cast<WindowContainer*>(pCreate->lpCreateParams);
		if (pWindow == nullptr)
		{
			ErrorLogger::Log("Critical Error: Pointer to window container is null during WM_NCCREATE.");
			exit(-1);
		}
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HandleMsgRedirect));
		return pWindow->WindowProc(hwnd, uMsg, wParam, lParam);
	}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void RenderWindow::RegisterWindowClass()
{
	WNDCLASSEX wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = HandleMessageSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = this->hInstance;
	wc.hIcon = NULL;
	wc.hIconSm = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = this->window_class_wide.c_str();
	wc.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&wc);
}

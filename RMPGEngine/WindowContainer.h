#pragma once
#include <memory>
#include "RenderWindow.h"
#include "Keyboard/KeyboardClass.h"
#include "Mouse/MouseClass.h"
#include "Graphics/Graphics.h"
#include "Audio/AudioManager.h"

class WindowContainer
{
public:
	WindowContainer();
	LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:
	RenderWindow render_window;
	KeyboardClass keyboard;
	MouseClass mouse;
	RMPG::Graphics gfx;
	RMPG::Audio audio;

private:

};
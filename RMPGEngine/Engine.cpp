#include "Engine.h"

bool Engine::Initialize(HINSTANCE hInstance, std::string window_title, std::string window_class, int width, int height)
{
	timer.Start();

	if (!this->render_window.Initialize(this, hInstance, window_title, window_class, width, height))
		return false;

	if (!gfx.Initialize(this->render_window.GetHWND(), width, height))
		return false;

	return true;
}

bool Engine::ProcessMessages()
{
	return this->render_window.ProcessMessages();
}

void Engine::EngineUpdate()
{
	float frameMs = timer.GetMilisecondsElapsed();
	timer.Restart();

	float dt = frameMs / 1000.0f;

	if (dt > maxFrameTime) dt = maxFrameTime;

	accumulator += dt;

	while (!keyboard.CharBufferIsEmpty())
	{
		unsigned char ch = keyboard.ReadChar();
	}

	while (!keyboard.KeyBufferIsEmpty())
	{
		KeyboardEvent kbe = keyboard.ReadKey();
		unsigned char keycode = kbe.GetKeyCode();
	}

	while (!mouse.EventBufferIsEmpty())
	{
		MouseEvent me = mouse.ReadEvent();
	}

	Update();

	while (accumulator >= fixedDelta)
	{
		FixedUpdate();
		accumulator -= fixedDelta;
	}
}

void Engine::RenderFrame()
{
	gfx.RenderFrame();
}

float Engine::GetDelta() const
{
	return this->fixedDelta;
}

void Engine::Update()
{
}

void Engine::FixedUpdate()
{
}

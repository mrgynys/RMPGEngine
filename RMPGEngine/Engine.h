#pragma once
#include "WindowContainer.h"
#include "Timer.h"

class Engine : public WindowContainer
{
public:
	bool Initialize(HINSTANCE hInstance, std::string window_title, std::string window_class, int width, int height);
	bool ProcessMessages();
	void EngineUpdate();
	void RenderFrame();

	float GetDelta() const;

protected:
	virtual void Update();
	virtual void FixedUpdate();

private:
	Timer timer;

	float accumulator = 0.0f;
	const float fixedDelta = 1.0f / 60.0f;
	const float maxFrameTime = 0.25f;
};

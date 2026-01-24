#pragma once
#include "Engine.h"

class Test : public Engine
{
public:
	bool GameInitialize()
	{
		idtex1 = gfx.AddTexture(L"Data\\Textures\\skeleton.png");
		idtex2 = gfx.AddTexture(L"Data\\Textures\\image.png");

		ido1 = gfx.AddObject(1.0f, 1.0f, 0.0f, gfx.textures[idtex1].get());
		gfx.objects[ido1]->SetAtlas(4.0f, 1.0f);
		ido2 = gfx.AddObject(0.5f, 0.5f, 0.0f, gfx.textures[idtex2].get());
		gfx.objects[ido2]->SetAtlas(1.0f, 1.0f);
		ido3 = gfx.AddObject(2.0f, 0.3f, 0.0f, gfx.textures[idtex1].get());
		gfx.objects[ido3]->SetAtlas(4.0f, 1.0f);

		idt1 = gfx.AddTextObjectFromFontFile(RMPG::ttf(RMPG::FONTS::LORA_SEMI_BOLD_ITALIC), L"Привет,\nworld! 123 &", 48, 0.0f, 0.01f);
		runs.push_back({ L"FPS: ", RMPG::ttf(RMPG::FONTS::LORA_BOLD), 60, 0xFF000000u });
		runs.push_back({ L"0", RMPG::ttf(RMPG::FONTS::LORA_REGULAR), 48, 0xFF00FF00u });
		idt2 = gfx.AddStyledTextObject(runs, 0.0f, 0.01f);

		return true;
	}

	void Run() {
		while (this->ProcessMessages())
		{
			this->EngineUpdate();
			this->RenderFrame();
		}
	}

	void Update() override
	{
		{
			MousePoint mp = mouse.GetPos();
			static int prevHovered = -1;
			int hovered = gfx.PickObjectAt(mp.x, mp.y);

			if (hovered != prevHovered)
			{
				if (prevHovered == 0) gfx.objects[ido1]->SetCol(0.0f);
				else if (prevHovered == 2) gfx.objects[ido3]->SetCol(0.0f);

				if (hovered == 0) gfx.objects[ido1]->SetCol(1.0f);
				//else if (hovered == 1 && mouse.IsLeftDown()) this->rotate = 0;
				else if (hovered == 2) gfx.objects[ido3]->SetCol(1.0f);

				prevHovered = hovered;
			}
		}

		if (keyboard.KeyIsPressed('Q'))
		{
			gfx.Resize(800, 600);
			render_window.SetFullScreen(false);
			render_window.SetWindowSize(800, 600);
		}
		if (keyboard.KeyIsPressed('W'))
		{
			gfx.Resize(1000, 500);
			render_window.SetFullScreen(false);
			render_window.SetWindowSize(1000, 500);
		}
		if (keyboard.KeyIsPressed('E'))
		{
			gfx.Resize(800, 600);
			render_window.SetFullScreen(false);
			render_window.SetWindowSize(800, 600);
		}
		if (keyboard.KeyIsPressed('R'))
		{
			gfx.Resize(1366, 768);
			render_window.SetFullScreen(true);
		}
		if (keyboard.KeyIsPressed('A'))
		{
			gfx.SetVSync(true);
		}
		if (keyboard.KeyIsPressed('S'))
		{
			gfx.SetVSync(false);
		}

		runs[1].text = std::to_wstring(gfx.GetFps());
		gfx.UpdateStyledTextObject(idt2, runs);
	}

	void FixedUpdate() override
	{
		static float rot = 0.0f;
		rot += GetDelta();

		static XMFLOAT3 mouseInWorld;

		if (mouse.IsLeftDown())
		{
			mouseInWorld = gfx.ScreenToWorldOnPlane(mouse.GetPosX(), mouse.GetPosY());
		}

		gfx.objects[ido1]->SetMatrix(XMMatrixTranslation(rot * 0.5f, 0.0f, 0.0f));
		gfx.objects[ido2]->SetMatrix(XMMatrixRotationZ(rot) * XMMatrixTranslation(mouseInWorld.x, mouseInWorld.y, 0.0f));
		gfx.objects[ido3]->SetMatrix(XMMatrixTranslation(1.25f, -0.35f, 0.0f));
		gfx.SetObjectMatrix(idt1, XMMatrixTranslation(-2.0f, 1.5f, 0.0f));
		gfx.SetObjectMatrix(idt2, XMMatrixRotationZ(rot / 2));
	}

private:
	// objects identificators
	int ido1;
	int ido2;
	int ido3;

	// text objects identificators
	int idt1;
	int idt2;

	// container of styled text object
	std::vector<TextRun> runs;

	// textures identificators
	int idtex1;
	int idtex2;

};

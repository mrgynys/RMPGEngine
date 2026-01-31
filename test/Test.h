#pragma once
#include "Engine.h"

class Test : public Engine
{
public:
	bool GameInitialize()
	{
		/*idtex1 = gfx.AddTexture(L"Data\\Textures\\skeleton.png");
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
		idt2 = gfx.AddStyledTextObject(runs, 0.0f, 0.01f);*/

		static auto t = gfx.AddTexture(L"Data\\Textures\\green_button.png");
		static auto qw = gfx.AddObject(3.0f, 0.6f, 0.0f, t);

		tex = gfx.AddTexture(L"Data\\Textures\\skeleton.png");
		obj = gfx.AddObject(1.0f, 1.0f, 0.0f, tex);
		gfx.GetObjectPtr(obj)->SetAtlas(4.0f, 1.0f);

		runs.push_back({ L"FPS: ", RMPG::ttf(RMPG::FONTS::LORA_REGULAR), 32, 0xFF00FF00u });
		runs.push_back({ L"0", RMPG::ttf(RMPG::FONTS::LORA_REGULAR), 32, 0xFFFFFFFFu });
		txt = gfx.AddStyledTextObject(runs, 0.0f, 0.01f);
		gfx.GetObjectPtr(txt)->texture->filter = RMPG::TextureFilterMode::Linear;

		static auto o = gfx.AddObject(0.5f, 0.5f, 0.0f, tex);
		gfx.GetObjectPtr(o)->SetAtlas(4.0f, 1.0f);
		gfx.GetObjectPtr(o)->SetMatrix(XMMatrixTranslation(1.0f, 0.0f, 0.0f));

		grp = gfx.CreateGroup();
		gfx.AddObjectToGroup(grp, qw);
		gfx.AddObjectToGroup(grp, o);

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
			std::vector<RMPG::ObjectID> hovered = gfx.PickObjectsAt(mp.x, mp.y);
			if (!hovered.empty())
			{
				int nearestHovered = hovered.back();
				if (nearestHovered != prevHovered)
				{
					std::string outstrUnhover = "\nUnhovered: ";
					outstrUnhover += std::to_string(prevHovered);
					std::string outstrHover = "\nHovered: ";
					outstrHover += std::to_string(nearestHovered);
					OutputDebugStringA(outstrUnhover.c_str());
					OutputDebugStringA(outstrHover.c_str());

					//if (prevHovered == 0) gfx.objects[ido1]->SetCol(0.0f);
					//else if (prevHovered == 2) gfx.objects[ido3]->SetCol(0.0f);

					//if (hovered == 0) gfx.objects[ido1]->SetCol(1.0f);
					//else if (hovered == 1 && mouse.IsLeftDown()) this->rotate = 0;
					//else if (hovered == 2) gfx.objects[ido3]->SetCol(1.0f);

					prevHovered = nearestHovered;
				}
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
		if (keyboard.KeyIsPressed('D'))
		{
			if (gfx.ObjectExists(obj))
			{
				gfx.RemoveObject(obj);
			}

			/*if (useObjects)
			{
				gfx.RemoveObject(idt2);
				useObjects = false;
			}*/
			//ido1 = gfx.AddObject(1.0f, 1.0f, 0.0f, gfx.textures[idtex1].get());
		}

		static float redLayout = 0.5f;
		if (mouse.GetWheelDelta() < 0)
		{
			redLayout -= 0.1f;
			if (redLayout < 0.0f) redLayout = 0.0f;
		}
		if (mouse.GetWheelDelta() > 0)
		{
			redLayout += 0.1f;
			if (redLayout > 1.0f) redLayout = 1.0f;
		}
		gfx.SetObjectTintColor(obj, XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), redLayout);

		mouse.ResetWheelDelta();

		runs[1].text = std::to_wstring(gfx.GetFps());
		gfx.UpdateStyledTextObject(txt, runs);
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

		gfx.GetObjectPtr(obj)->SetMatrix(XMMatrixRotationZ(rot) * XMMatrixTranslation(0.3f, 0.0f, 0.0f));

		gfx.GetObjectPtr(txt)->SetMatrix(XMMatrixTranslation(gfx.GetTopLeftWorldCoord().x + 0.6f, gfx.GetTopLeftWorldCoord().y - 0.3f, 0.0f));

		gfx.AdjustGroupRotation(grp, 0.0f, 0.0f, 0.01f);

		/*gfx.objects[ido1]->SetMatrix(XMMatrixTranslation(rot * 0.5f, 0.0f, 0.0f));
		gfx.objects[ido2]->SetMatrix(XMMatrixRotationZ(rot) * XMMatrixTranslation(mouseInWorld.x, mouseInWorld.y, 0.0f));
		gfx.objects[ido3]->SetMatrix(XMMatrixTranslation(1.25f, -0.35f, 0.0f));
		gfx.SetObjectMatrix(idt1, XMMatrixTranslation(-2.0f, 1.5f, 0.0f));
		gfx.SetObjectMatrix(idt2, XMMatrixRotationZ(rot / 2));*/
	}

private:
	RMPG::TextureID tex;
	RMPG::ObjectID obj;
	RMPG::ObjectID txt;
	std::vector<TextRun> runs;

	RMPG::GroupID grp;

	//// objects identificators
	//int ido1;
	//int ido2;
	//int ido3;

	//// text objects identificators
	//int idt1;
	//int idt2;

	//// container of styled text object
	//std::vector<TextRun> runs;

	//// textures identificators
	//int idtex1;
	//int idtex2;

	//bool useObjects = true;
};

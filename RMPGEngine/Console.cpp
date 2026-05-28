#include "Console.h"

void RMPG::Console::Initialize(Engine* engine, RMPG::TextureID inputBoxTexture, RMPG::TextureID bgTexture, RMPG::FONTS font)
{
	this->isOpen = false;
	this->engine = engine;
	this->input_box_tx = inputBoxTexture;
	this->bg_tx = bgTexture;
	this->font = font;

	inputRuns.push_back({ L"> ", RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFF00FF00u });
	inputRuns.push_back({ L"", RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFFFFFFu });
	historyRuns.push_back({ L"Type \"help\" to see commands", RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFFFFFFu });

	this->commandList[L"help"] = CommandTemplate({ L"help", Args({L"command"}), L"shows info about command" });
	this->commandList[L"clear"] = CommandTemplate({ L"clear", Args(), L"console clear" });
	this->commandList[L"clear_history"] = CommandTemplate({ L"clear_history", Args(), L"history clear" });
}

RMPG::Console::~Console()
{
	if (isOpen) Close();
}

void RMPG::Console::SetCommands(std::vector<CommandTemplate> commands)
{
	commandList.clear();
	this->commandList[L"help"] = CommandTemplate({ L"help", Args({L"command"}), L"shows info about command" });
	this->commandList[L"clear"] = CommandTemplate({ L"clear", Args(), L"console clear" });
	this->commandList[L"clear_history"] = CommandTemplate({ L"clear_history", Args(), L"history clear" });
	for (CommandTemplate ct : commands)
	{
		this->commandList[ct.name] = ct;
	}
}

void RMPG::Console::AddCommand(CommandTemplate command)
{
	this->commandList[command.name] = command;
}

void RMPG::Console::DelCommand(std::wstring commandName)
{
	this->commandList.erase(commandName);
}

void RMPG::Console::Open(int renderOrder)
{
	bg_id = engine->gfx.AddObject(
		(engine->gfx.GetBottomRightWorldCoord().x - engine->gfx.GetBottomLeftWorldCoord().x) / 2,
		engine->gfx.GetTopLeftWorldCoord().y - engine->gfx.GetBottomLeftWorldCoord().y,
		0.0001f,
		this->bg_tx
	);

	input_text_id = engine->gfx.AddStyledTextObject(inputRuns, textZ, 0.003f);

	input_box_id = engine->gfx.AddObject(
		(engine->gfx.GetBottomRightWorldCoord().x - engine->gfx.GetBottomLeftWorldCoord().x) / 2,
		engine->gfx.GetObjectPtr(input_text_id)->GetHeight(),
		0.0f,
		this->input_box_tx
	);

	history_text_id = engine->gfx.AddStyledTextObject(historyRuns, textZ, 0.003f);

	engine->gfx.GetObjectPtr(history_text_id)->texture->filter = TextureFilterMode::Linear;
	engine->gfx.GetObjectPtr(input_text_id)->texture->filter = TextureFilterMode::Linear;

	engine->gfx.GetObjectPtr(bg_id)->depthEnabled = false;
	engine->gfx.GetObjectPtr(history_text_id)->depthEnabled = false;
	engine->gfx.GetObjectPtr(input_box_id)->depthEnabled = false;
	engine->gfx.GetObjectPtr(input_text_id)->depthEnabled = false;
	engine->gfx.GetObjectPtr(bg_id)->renderOrder = renderOrder + 0;
	engine->gfx.GetObjectPtr(history_text_id)->renderOrder = renderOrder + 1;
	engine->gfx.GetObjectPtr(input_box_id)->renderOrder = renderOrder + 2;
	engine->gfx.GetObjectPtr(input_text_id)->renderOrder = renderOrder + 3;

	isOpen = true;
}

void RMPG::Console::Close()
{
	engine->gfx.RemoveObject(bg_id);
	engine->gfx.RemoveObject(input_text_id);
	engine->gfx.RemoveObject(input_box_id);
	engine->gfx.RemoveObject(history_text_id);

	isOpen = false;
}

void RMPG::Console::Update(int renderOrder)
{
	if (isOpen)
	{
		while (!engine->keyboard.CharBufferIsEmpty())
		{
			unsigned char c = engine->keyboard.ReadChar();
			if (c >= 32 and c < 127)
			{
				inputRuns[1].text += c;
				historyPos = -1;
			}
		}

		static bool backflag = false;
		static bool returnflag = false;
		static bool upflag = false;
		static bool downflag = false;

		if (engine->keyboard.KeyIsPressed(VK_BACK))
		{
			if (backflag == false)
			{
				backflag = true;
				historyPos = -1;
				if (!inputRuns[1].text.empty()) inputRuns[1].text.pop_back();
			}
		}
		else
		{
			backflag = false;
		}

		if (engine->keyboard.KeyIsPressed(VK_RETURN))
		{
			if (returnflag == false)
			{
				returnflag = true;
				historyPos = -1;

				historyRuns.push_back({ L"\n> " + inputRuns[1].text, RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFFFFFFu });
				history.push_back(inputRuns[1].text);
				Args a = wstocom(inputRuns[1].text);

				if (a.size() == 0)
				{

				}
				else if (a.size() == 1)
				{
					if (a[0] == L"help")
					{
						ShowHelp();
					}
					else if (a[0] == L"clear")
					{
						Clear();
					}
					else if (a[0] == L"clear_history")
					{
						ClearHistory();
					}
					else if (commandList.contains(a[0]))
					{
						commandBuffer.push(a);
					}
					else
					{
						ShowError(L"Unknown command: " + a[0]);
					}
				}
				else if (a.size() == 2)
				{
					if (a[0] == L"help")
					{
						ShowHelp(a[1]);
					}
					else if (a[0] == L"clear")
					{
						ShowError(L"Invalid arguments");
					}
					else if (a[0] == L"clear_history")
					{
						ShowError(L"Invalid arguments");
					}
					else if (commandList.contains(a[0]))
					{
						commandBuffer.push(a);
					}
					else
					{
						ShowError(L"Unknown command: " + a[0]);
					}
				}
				else if (commandList.contains(a[0]))
				{
					if (a[0] == L"help")
					{
						ShowError(L"Invalid arguments");
					}
					else if (a[0] == L"clear")
					{
						ShowError(L"Invalid arguments");
					}
					else if (a[0] == L"clear_history")
					{
						ShowError(L"Invalid arguments");
					}
					else
					{
						commandBuffer.push(a);
					}
				}
				else
				{
					ShowError(L"Unknown command: " + a[0]);
				}

				inputRuns[1].text = L"";
			}
		}
		else
		{
			returnflag = false;
		}

		if (history.size() > 0)
		{
			if (engine->keyboard.KeyIsPressed(VK_UP))
			{
				if (upflag == false)
				{
					upflag = true;
					if (historyPos > 0)
					{
						historyPos--;
						inputRuns[1].text = history[historyPos];
					}
					else if (historyPos == -1)
					{
						historyPos = history.size() - 1;
						inputRuns[1].text = history[historyPos];
					}
				}
			}
			else
			{
				upflag = false;
			}

			if (engine->keyboard.KeyIsPressed(VK_DOWN))
			{
				if (downflag == false)
				{
					downflag = true;
					if (historyPos + 1 < history.size())
					{
						historyPos++;
						inputRuns[1].text = history[historyPos];
					}
				}
			}
			else
			{
				downflag = false;
			}
		}

		float bgrw = engine->gfx.GetObjectPtr(bg_id)->GetWidth();
		float bgrh = engine->gfx.GetObjectPtr(bg_id)->GetHeight();
		engine->gfx.GetObjectPtr(bg_id)->SetMatrix(XMMatrixTranslation(
			engine->gfx.GetBottomLeftWorldCoord().x + (bgrw / 2),
			engine->gfx.GetBottomLeftWorldCoord().y + (bgrh / 2),
			0.0001f));

		float ibrw = engine->gfx.GetObjectPtr(input_box_id)->GetWidth();
		float ibrh = engine->gfx.GetObjectPtr(input_box_id)->GetHeight();
		engine->gfx.GetObjectPtr(input_box_id)->SetMatrix(XMMatrixTranslation(
			engine->gfx.GetBottomLeftWorldCoord().x + (ibrw / 2),
			engine->gfx.GetBottomLeftWorldCoord().y + (ibrh / 2),
			-0.001f));

		engine->gfx.UpdateStyledTextObject(history_text_id, historyRuns);
		engine->gfx.UpdateStyledTextObject(input_text_id, inputRuns);

		float irw = engine->gfx.GetObjectPtr(input_text_id)->GetWidth();
		float irh = engine->gfx.GetObjectPtr(input_text_id)->GetHeight();
		engine->gfx.GetObjectPtr(input_text_id)->SetMatrix(XMMatrixTranslation(
			engine->gfx.GetBottomLeftWorldCoord().x + (irw / 2),
			engine->gfx.GetBottomLeftWorldCoord().y + (irh / 2),
			textZ));

		float hrw = engine->gfx.GetObjectPtr(history_text_id)->GetWidth();
		float hrh = engine->gfx.GetObjectPtr(history_text_id)->GetHeight();
		engine->gfx.GetObjectPtr(history_text_id)->SetMatrix(XMMatrixTranslation(
			engine->gfx.GetBottomLeftWorldCoord().x + (hrw / 2),
			engine->gfx.GetBottomLeftWorldCoord().y + (hrh / 2) + irh,
			textZ));

		engine->gfx.GetObjectPtr(bg_id)->renderOrder = renderOrder + 0;
		engine->gfx.GetObjectPtr(history_text_id)->renderOrder = renderOrder + 1;
		engine->gfx.GetObjectPtr(input_box_id)->renderOrder = renderOrder + 2;
		engine->gfx.GetObjectPtr(input_text_id)->renderOrder = renderOrder + 3;
	}
}

void RMPG::Console::Clear()
{
	historyRuns.clear();
	historyRuns.push_back({ L"", RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFFFF00u });
}

void RMPG::Console::ClearHistory()
{
	history.clear();
}

void RMPG::Console::ShowMessage(std::wstring msg)
{
	historyRuns.push_back({ L"\n"+msg, RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFFFF00u});
}

void RMPG::Console::ShowError(std::wstring err)
{
	historyRuns.push_back({ L"\n" + err, RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFF0000u });
}

void RMPG::Console::ShowHelp()
{
	for (const auto& [name, comTemplate] : commandList)
	{
		historyRuns.push_back({ L"\n" + name,
			RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFFFFFFu});
	}
}

void RMPG::Console::ShowHelp(std::wstring command)
{
	if (commandList.contains(command))
	{
		std::wstring outstr = L"\n";
		CommandTemplate com = commandList[command];
		outstr += com.name;
		for (int i = 0; i < com.args.size(); i++)
		{
			outstr += L" [";
			outstr += com.args[i];
			outstr += L"]";
		}
		outstr += L"\n";
		outstr += com.help;

		historyRuns.push_back({ outstr, RMPG::ttf(RMPG::FONTS::LORA_REGULAR), textSize, 0xFFFFFFFFu });
	}
	else
	{
		ShowError(L"Unknown command: " + command);
	}
}

RMPG::Args RMPG::Console::GetInput()
{
	if (this->commandBuffer.empty())
	{
		return Args();
	}
	else
	{
		Args a = this->commandBuffer.front();
		this->commandBuffer.pop();
		return a;
	}
}

bool RMPG::Console::IsOpen()
{
	return this->isOpen;
}

bool RMPG::Console::CommandBufferIsEmpty()
{
	return commandBuffer.empty();
}

std::vector<std::wstring> RMPG::Console::wstocom(std::wstring str)
{
	Args ret;
	std::wstring currentWord = L"";
	for (wchar_t c : str)
	{
		if (c == L' ')
		{
			if (!currentWord.empty())
			{
				ret.push_back(currentWord);
				currentWord = L"";
			}
		}
		else
		{
			currentWord += c;
		}
	}

	if (!currentWord.empty())
	{
		ret.push_back(currentWord);
	}

	return ret;
}

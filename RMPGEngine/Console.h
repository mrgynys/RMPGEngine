#pragma once
#include "Engine.h"

namespace RMPG
{
	using Args = std::vector<std::wstring>;

	struct CommandTemplate
	{
		std::wstring name;
		Args args;
		std::wstring help;
	};

	class Console
	{
	public:
		void Initialize(Engine *engine, RMPG::TextureID inputBoxTexture, RMPG::TextureID bgTexture, RMPG::FONTS font);
		~Console();

		void SetCommands(std::vector<CommandTemplate> commands);
		void AddCommand(CommandTemplate command);
		void DelCommand(std::wstring commandName);

		void Open(int renderOrder = 0);
		void Close();

		void Update(int renderOrder = 0);

		void Clear();
		void ClearHistory();

		void ShowMessage(std::wstring msg);
		void ShowError(std::wstring err);
		void ShowHelp();
		void ShowHelp(std::wstring command);

		Args GetInput();

		bool IsOpen();
		bool CommandBufferIsEmpty();

	private:
		std::vector<std::wstring> wstocom(std::wstring str);

		Engine* engine;

		bool isOpen;

		RMPG::FONTS font;

		std::vector<TextRun> inputRuns;
		std::vector<TextRun> historyRuns;

		// textures
		RMPG::TextureID input_box_tx;
		RMPG::TextureID bg_tx;

		// background objects
		RMPG::ObjectID input_box_id;
		RMPG::ObjectID bg_id;

		// text objects
		RMPG::ObjectID input_text_id;
		RMPG::ObjectID history_text_id;

		std::map<std::wstring, CommandTemplate> commandList;
		std::vector<std::wstring> history;
		std::queue<Args> commandBuffer;

		int historyPos = -1;

		float textZ = -0.001f;
		int textSize = 48;
	};
};

#pragma once
#include <string>

#define FONT_DIR L"Data\\Fonts"

namespace RMPG
{
	enum FONTS
	{
		LORA_REGULAR,
		LORA_ITALIC,
		LORA_BOLD,
		LORA_BOLD_ITALIC,
		LORA_MEDIUM,
		LORA_MEDIUM_ITALIC,
		LORA_SEMI_BOLD,
		LORA_SEMI_BOLD_ITALIC
	};

	static std::wstring ttf(int font = 0)
	{
		std::wstring filename = FONT_DIR;
		filename += L"\\";
		switch (font)
		{
		case LORA_REGULAR:
			filename += L"Lora-Regular";
			break;
		case LORA_ITALIC:
			filename += L"Lora-Italic";
			break;
		case LORA_BOLD:
			filename += L"Lora-Bold";
			break;
		case LORA_BOLD_ITALIC:
			filename += L"Lora-BoldItalic";
			break;
		case LORA_MEDIUM:
			filename += L"Lora-Medium";
			break;
		case LORA_MEDIUM_ITALIC:
			filename += L"Lora-MediumItalic";
			break;
		case LORA_SEMI_BOLD:
			filename += L"Lora-SemiBold";
			break;
		case LORA_SEMI_BOLD_ITALIC:
			filename += L"Lora-SemiBoldItalic";
			break;
		default:
			filename += L"Lora-Regular";
			break;
		}
		filename += L".ttf";
		return filename;
	}
};
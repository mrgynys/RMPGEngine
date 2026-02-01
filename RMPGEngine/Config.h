#pragma once
#include <Windows.h>
#include <string>

class Config
{
public:
	Config(const std::wstring& filename = L"config.ini");
	bool FileExists();

	std::wstring GetString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue = L"");
	int GetInt(const wchar_t* section, const wchar_t* key, int defaultValue = 0);
	void SetString(const wchar_t* section, const wchar_t* key, const wchar_t* value);
	void SetInt(const wchar_t* section, const wchar_t* key, int value);

private:
	std::wstring filePath;

};
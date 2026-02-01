#include "Config.h"

Config::Config(const std::wstring& filename)
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    this->filePath = std::wstring(buffer).substr(0, pos) + L"\\" + filename;
}

bool Config::FileExists()
{
    DWORD dwAttrib = GetFileAttributesW(this->filePath.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring Config::GetString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue)
{
    wchar_t buffer[256];
    GetPrivateProfileStringW(section, key, defaultValue, buffer, sizeof(buffer) / sizeof(wchar_t), this->filePath.c_str());
    return std::wstring(buffer);
}

int Config::GetInt(const wchar_t* section, const wchar_t* key, int defaultValue)
{
    return GetPrivateProfileIntW(section, key, defaultValue, this->filePath.c_str());
}

void Config::SetString(const wchar_t* section, const wchar_t* key, const wchar_t* value)
{
    WritePrivateProfileStringW(section, key, value, this->filePath.c_str());
}

void Config::SetInt(const wchar_t* section, const wchar_t* key, int value)
{
    SetString(section, key, std::to_wstring(value).c_str());
}

#include "Test.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR	lpCmdLine,
	_In_ int	nCmdShow)
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to call CoInitializeEx");
		return -1;
	}

	LPWSTR* arglist;
	int argsNum = 0;
	arglist = CommandLineToArgvW(GetCommandLineW(), &argsNum);
	if (arglist == NULL)
	{
		OutputDebugStringW(L"\nCommandLineToArgvW failed!\n");
		return 1;
	}

	OutputDebugStringW(L"\n");
	for (int i = 0; i < argsNum; i++)
	{
		OutputDebugStringW(arglist[i]);
		std::wstring ws = arglist[i];
		if (ws == L"-debug")
			debug_flag = true;
		OutputDebugStringW(L"\n");
	}
	OutputDebugStringW(L"\n");

	Test test;
	test.Initialize(hInstance, "RMPG", "TestClass", 800, 600);
	if (!test.GameInitialize())
		return -1;
	test.Run();
	
	LocalFree(arglist);

	return 0;
}

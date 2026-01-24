#include "Test.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR	lpCmdLine,
	_In_ int	nCmdShow)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to call CoInitialize");
		return -1;
	}

	Test test;
	test.Initialize(hInstance, "RMPG", "TestClass", 800, 600);
	if (!test.GameInitialize())
		return -1;
	test.Run();
	
	return 0;
}

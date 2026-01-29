# Начало работы с RMPGEngine
## Добавление RMPGE и DirectX11 в проект Visual Studio
*Важно: рекомендуется использовать Visual Studio 2026*
RMPGE является статической библиотекой C/C++, и процесс подключения этой библиотеки не отличается от подключения любой другой статической библиотеки.

### Компоненты Visual Studio
Перед началом рекомендуется установить компоненты Visual Studio:
- "Разработка классических приложений на C++",
- "Разработка игр на C++".

### Создание решения/проекта
Создайте пустой проект настольного приложения Windows:
- Откройте Visual Studio;
- Нажмите на кнопку *Create a new project*;
- Выберите шаблон *Windows Desktop Wizard*, введите название проекта, путь, имя решения, нажмите на кнопку *"Create"*;
- В появившемся окне выберите тип приложения *Desktop Application (.exe)*, поставьте галочку *Empty project*;
- Нажмите на кнопку *OK*.

Добавьте файл *Source.cpp* (*Project -> Add New Item...*) и вставьте в него следующий код:

``` cpp
#include <Windows.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR	lpCmdLine,
	_In_ int	nCmdShow)
{

	return 0;
}
```

Соберите и запустите проект. Убедитесь, что программа возвращает код 0. Чтобы это проверить, посмотрите на содержание последней строки консоли вывода:

```
The program 'your-project-name.exe' has exited with code 0 (0x0).
```

Если сборка или выполнение программы завершается с ошибкой, то удостоверьтесь, что сделали все правильно, и что у вас есть все необходимые компоненты для WinAPI и C/C++.
Если проект удалось собрать и запустить, значит проект видит все необходимые зависимости.
Соберите проект во всех конфигурациях, то есть **Debug** для платформ **Win32 (x86)** и **x64**, и **Release** для платформ **Win32 (x86)** и **x64**. Для этого можно зайти в *Build -> Batch Build ...*, поставить галочки на всех конфигурациях и нажать *Build*.

### Установка и подключение библиотек RMPGE
Скачайте прикрепленный к последнему релизу архив *Libs.zip*. Распакуйте в любую директорию.

Архив содержит следующие директории:
- *Includes*, заголовочные файлы для подключения библиотек;
- *Libs*, статические библиотеки для всех конфигураций;
- *Shaders*, скомпилированные объекты шейдеров для всех конфигураций.

Директории *Includes*, *Libs* и *Shaders* переместите в папку проекта.

Для подключения библиотек выполните следуюее:
- Откройте настройки проекта (*Project -> Properties*);
- Перейдите в *VC++ Directories*;
- Добавьте путь заголовков (*Include Directories*), убедитесь, что выбраны конфигурация *All Configurations* и платформа *All platforms*: *<Edit...> -> New Line -> ... -> Укажите путь к папке* ***Includes***;
- Добавьте пути библиотек (*Library Directories*), выбирая платформу и указывая соответствующую директорию:
  - конфигурация *Debug*, платформа *Win32*: */Libs/x86/Debug*;
  - конфигурация *Debug*, платформа *x64*: */Libs/x64/Debug*;
  - конфигурация *Release*, платформа *Win32*: */Libs/x86/Release*;
  - конфигурация *Release*, платформа *x64*: */Libs/x64/Release*;
- Верните *All Configurations* и *All platforms*, перейдите в *Linker -> Input*, в параметр *Additional Dependencies* добавьте **RMPGEngine.lib**.

## "Hello, world!" на RMPGE
Для проверки правильной работы движка вставьте в *Source.cpp* следующий код:

``` cpp
#include "Engine.h"

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

	Engine engine;
	engine.Initialize(hInstance, "Hello, world!", "MyWindowClass", 400, 300);
	while (engine.ProcessMessages())
	{
		engine.EngineUpdate();
		engine.RenderFrame();
	}

	return 0;
}
```

Соберите и запустите проект. На экране должно появиться окно 400х300 с серым фоном и именем окна *Hello, world!*.

Если на данном этапе нет никаких ошибок **CoInitialize**, движка, *DirectX*, *Windows* и *GDI+*, то вы все сделали правильно. В награду вы получили проект, готовый к работе с движком **RMPGEngine**.

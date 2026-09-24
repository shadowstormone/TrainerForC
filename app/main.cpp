#include "app/main.h"
#include "app/Application.h"
#include "platform/AudioService.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#ifdef _DEBUG
    if (__argc > 1 && wcscmp(__wargv[1], L"--run-tests") == 0)
    {
        return RunTests();
    }
#endif // _DEBUG

    // --no-sound: тихий режим, без щелчков на включение и выключение.
    for (int i = 1; i < __argc; ++i)
    {
        if (_wcsicmp(__wargv[i], L"--no-sound") == 0)
        {
            AudioService::Instance().SetEnabled(false);
        }
    }

    Application app;
    if (!app.Initialize(L"Tutorial-x86_64.exe"))
    {
        return 1;
    }

    return app.Run();
}

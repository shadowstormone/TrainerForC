#include "app/main.h"
#include "app/Application.h"

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

    Application app;
    if (!app.Initialize(L"Tutorial-x86_64.exe"))
    {
        return 1;
    }

    return app.Run();
}

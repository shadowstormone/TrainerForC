#include "ui/ConsoleWindow.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "platform/Logger.h"
#include "ui/ImGuiConsole.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    constexpr int DEFAULT_WIDTH = 760;
    constexpr int DEFAULT_HEIGHT = 420;
}

ConsoleWindow::~ConsoleWindow()
{
    Destroy();
}

void ConsoleWindow::WithContext(const std::function<void()>& action)
{
    if (!_imgui) return;

    ImGuiContext* previous = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(_imgui);

    action();

    ImGui::SetCurrentContext(previous);
}

bool ConsoleWindow::Create(Console& console, HINSTANCE instance,
                           const std::function<void(ImGuiIO&)>& configure)
{
    _console = &console;

    WindowDesc desc;
    desc.className = L"TrainerDebugConsoleWindow";
    desc.title = L"Debug Console";
    desc.width = DEFAULT_WIDTH;
    desc.height = DEFAULT_HEIGHT;
    desc.borderless = false;  // обычное окно: системный заголовок и рамка
    desc.resizable = true;

    if (instance)
    {
        desc.icon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPLICATION));
        desc.iconSmall = desc.icon;
    }

    if (!_window.Create(desc))
    {
        Log::Error("Не удалось создать окно консоли");
        return false;
    }

    if (!_d3d.Create(_window.Handle()))
    {
        Log::Error("Не удалось создать swapchain для окна консоли");
        _window.Destroy();
        return false;
    }

    ImGuiContext* previous = ImGui::GetCurrentContext();

    _imgui = ImGui::CreateContext();
    ImGui::SetCurrentContext(_imgui);

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (configure) configure(io);

    ImGui_ImplWin32_Init(_window.Handle());
    ImGui_ImplDX11_Init(_d3d.Device(), _d3d.Context());

    ImGui::SetCurrentContext(previous);

    // Сообщения этого окна должен разбирать ЕГО контекст, иначе ввод уедет
    // в панель трейнера.
    _window.SetMessageHandler(
        [this](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result) -> bool
        {
            bool handled = false;

            WithContext([&]()
            {
                if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                {
                    result = 1;
                    handled = true;
                }
            });

            if (handled) return true;

            switch (msg)
            {
            case WM_SIZE:
                if (_d3d.IsValid() && wParam != SIZE_MINIMIZED)
                {
                    _d3d.Resize(LOWORD(lParam), HIWORD(lParam));
                }
                break;

            case WM_CLOSE:
                // Крестик прячет консоль, а не закрывает трейнер.
                SetVisible(false);
                result = 0;
                return true;

            default:
                break;
            }

            return false;
        });

    return true;
}

void ConsoleWindow::Destroy()
{
    if (_imgui)
    {
        ImGuiContext* previous = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(_imgui);

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();

        // Восстанавливаем только если предыдущий контекст — не наш.
        ImGui::SetCurrentContext(previous != _imgui ? previous : nullptr);

        ImGui::DestroyContext(_imgui);
        _imgui = nullptr;
    }

    _d3d.Destroy();
    _window.Destroy();
    _console = nullptr;
}

void ConsoleWindow::SetVisible(bool visible)
{
    if (_visible == visible) return;

    _visible = visible;

    if (!_window.Handle()) return;

    ::ShowWindow(_window.Handle(), visible ? SW_SHOW : SW_HIDE);

    if (visible)
    {
        ::SetForegroundWindow(_window.Handle());
    }
}

void ConsoleWindow::Draw()
{
    if (!_visible || !_imgui || !_console || !_d3d.IsValid()) return;

    WithContext([&]()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Консоль занимает всё окно: отдельная плавающая рамка внутри
        // собственного окна выглядела бы окном в окне.
        _console->draw("Debug Console", nullptr, /*fillWindow=*/true);

        ImGui::EndFrame();
        ImGui::Render();

        constexpr float clear[4] = { 0.10f, 0.10f, 0.12f, 1.0f };
        _d3d.BeginFrame(clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        _d3d.Present(1);
    });
}

#pragma once
#include <Windows.h>
#include <tchar.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <functional>
#include "d3d9.h"
#include "d3d11.h"
#include "core/Cheat.h"
#include "ui/ImGuiConsole.h"

constexpr auto WIDTH = 500;
constexpr auto HEIGHT = 555;

// Состояние уведомлений живёт в Drawing.cpp (раньше было header-статиками,
// из-за чего каждый .cpp получал собственную копию).
extern bool showConsole;

struct FunctionOffset
{
    std::string buttonName;                 // Название кнопки
    std::vector<uintptr_t> offsets;         // Оффсеты
};

class Drawing
{
private:
    static LPCSTR lpWindowName;
    static ImVec2 vWindowSize;
    static ImGuiWindowFlags WindowFlags;
    static bool bDraw;
    static Cheat* _cheatProcGame;
    static std::function<void(const std::string&, const std::string&, bool, bool)> _toggleHandler;

    static std::vector<uintptr_t> Offsets;
    static std::unordered_map<std::string, FunctionOffset> OffsetFunctions; // Ассоциация кнопок и офсетов
    static std::map<std::string, int> inputValues;
    static std::map<std::string, bool> inputFieldFocused;  // Для отслеживания фокуса каждого поля

    // Рефакторинг приватных методов
    static void RenderToggles();
    static void RenderInputFields();
    static void RenderProcessInfo();
    static void HandleToggleInteraction(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState);
    static void HandlePopupsWithIcons(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);
    static void RenderAuthorLink(const char* text, const char* url, float offsetRight = 1.0f);

    // Своя полоса заголовка (системной у borderless-окна нет)
    static void RenderTitleBar();
public:
    // Высота полосы заголовка, которую рисует ImGui
    static constexpr float TITLE_BAR_HEIGHT = 32.0f;

    // Окно, которым управляют кнопки заголовка (свернуть/закрыть)
    static void SetWindowHandle(HWND hWnd);

    // true, если точка (в клиентских координатах) — перетаскиваемая часть
    // заголовка. Над кнопками возвращает false, иначе клики по ним уйдут
    // в перетаскивание окна.
    static bool IsCaptionPoint(POINT clientPoint);

    // Методы инициализации
    static void Initialize(Cheat* ClassCheatProcGame);
    static void Initialize(Cheat* ClassCheatProcGame, const std::vector<uintptr_t>& offsets);
    static void Initialize(Cheat* ClassCheatProcGame, const std::unordered_map<std::string, FunctionOffset>& offsets);
    static void Initialize(Cheat* ClassCheatProcGame, const std::unordered_map<std::string, FunctionOffset>& offsets, const std::vector<CheatOption*>& cheatOptions);

    // Установите обратный вызов обработчика переключения
    static void SetToggleHandler(std::function<void(const std::string&, const std::string&, bool, bool)> handler)
    {
        _toggleHandler = handler;
    }

    // Усправление статусом
    static void Active();
    static bool isActive();

    // Основной метод рисования
    static void Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);
};
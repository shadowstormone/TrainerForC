#pragma once
#include <Windows.h>
#include <tchar.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "d3d9.h"
#include "d3d11.h"
#include "Cheat.h"
#include "ImGuiConsole.h"

constexpr auto WIDTH = 500;
constexpr auto HEIGHT = 555;

// Переменные для управления уведомлениями
static std::string popupMessage = "";
static std::string popupType = ""; // "Error" или "Success"
extern Console console;
extern bool showConsole;

struct FunctionOffset {
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

    // Добавим карту для хранения состояний переключателей
    static std::unordered_map<std::string, bool> toggleStatesFunction;

    //static bool toggleState;
    static std::vector<uintptr_t> Offsets;
    static std::unordered_map<std::string, FunctionOffset> OffsetFunctions; // Ассоциация кнопок и офсетов
    static std::map<std::string, int> inputValues;
    static std::map<std::string, bool> inputFieldFocused;  // Для отслеживания фокуса каждого поля
    //static int intUserInput;
    //static float floatUserInput;
    //static double doubleUserInput;

    static std::string WStringToUtf8(const std::wstring& wstr);

    // Refactored private methods
    static void RenderToggles();
    static void RenderInputFields();
    static void RenderProcessInfo();
    static void HandleToggleInteraction(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState);
    static void DisplayPopup(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);

    static void HandlePopupsWithIcons(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);
public:
    // Initialization methods
    static void Initialize(Cheat* ClassCheatProcGame);
    static void Initialize(Cheat* ClassCheatProcGame, const std::vector<uintptr_t>& offsets);
    static void Initialize(Cheat* ClassCheatProcGame, const std::unordered_map<std::string, FunctionOffset>& offsets);
    static void Initialize(Cheat* ClassCheatProcGame, const std::unordered_map<std::string, FunctionOffset>& offsets, const std::vector<CheatOption*>& cheatOptions);

    // State management
    static void Active();
    static bool isActive();

    // Main drawing method
    static void Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);
};
#pragma once
#include <Windows.h>
#include <tchar.h>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "d3d9.h"
#include "d3d11.h"
#include "core/Cheat.h"
#include "ui/ImGuiConsole.h"

constexpr auto WIDTH = 500;
constexpr auto HEIGHT = 555;

extern bool showConsole;

struct ValueFieldDefinition;

// Подготовленное к отрисовке поле ввода: подпись уже в UTF-8, чтобы не
// перекодировать её каждый кадр.
struct InputFieldView
{
    std::string label;
    std::vector<uintptr_t> offsets;
    bool absolute = false;
    int defaultValue = 1;
};

// Содержимое главного окна: полоса заголовка, переключатели читов,
// поля ввода и строка состояния процесса.
//
// Раньше это был класс из одних static-полей (Drawing), поэтому состояние
// интерфейса было глобальным и существовало в единственном экземпляре.
// Теперь это обычный объект — им владеет тот, кто рисует окно.
class MainView
{
    const char* _windowName = "Test Trainer (+1)";
    bool _draw = true;

    Cheat* _process = nullptr;
    std::vector<CheatOption*> _options;
    std::vector<InputFieldView> _valueFields;

    std::function<void(const std::string&, const std::string&, bool, bool)> _toggleHandler;

    std::map<std::string, int> _inputValues;
    std::map<std::string, bool> _inputFieldFocused; // Для отслеживания фокуса каждого поля

    // Уведомления
    std::string _popupMessage;
    std::string _popupType; // "Error" или "Success"

    // Окно, которым управляют кнопки заголовка
    HWND _windowHandle = nullptr;

    // Левая граница блока кнопок заголовка (клиентские координаты).
    // До первого кадра — "бесконечность", чтобы окно таскалось сразу.
    float _titleButtonsMinX = 3.4e38f;

    void RenderTitleBar();
    // Считает X колонок таблицы: переключателей и названий.
    void ComputeColumns(float& outToggleX, float& outNameX) const;

    void RenderToggles();
    void RenderInputFields();
    void RenderProcessInfo();
    void HandleToggleInteraction(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState);
    void HandlePopupsWithIcons(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);
    void RenderAuthorLink(const char* text, const char* url, float offsetRight = 1.0f);

public:
    // Высота полосы заголовка, которую рисует ImGui
    static constexpr float TITLE_BAR_HEIGHT = 32.0f;

    MainView() = default;
    MainView(const MainView&) = delete;
    MainView& operator=(const MainView&) = delete;

    // Поля ввода приходят из ValueFieldRegistry, читы — из CheatRegistry:
    // всё содержимое панели описано в cheats/registry/, а не собирается
    // по кусочкам при старте программы.
    void Initialize(Cheat* process,
                    const std::vector<ValueFieldDefinition>& valueFields,
                    const std::vector<CheatOption*>& options);

    // Окно, которым управляют кнопки заголовка (свернуть/закрыть)
    void SetWindowHandle(HWND hWnd) { _windowHandle = hWnd; }

    // true, если точка (в клиентских координатах) — полоса заголовка.
    //
    // Перетаскивание за ОСТАЛЬНОЕ окно сделано не здесь: если объявить
    // заголовком всё подряд, Windows начнёт слать WM_NCMOUSEMOVE вместо
    // WM_MOUSEMOVE, ImGui перестанет видеть курсор, и по виджетам станет
    // невозможно кликнуть. Поэтому хиттест отвечает только за геометрию
    // заголовка, а тягу с пустого места инициирует Draw() (см. .cpp).
    bool IsCaptionPoint(POINT clientPoint) const;

    void SetToggleHandler(std::function<void(const std::string&, const std::string&, bool, bool)> handler)
    {
        _toggleHandler = std::move(handler);
    }

    void Active() { _draw = true; }
    bool isActive() const { return _draw; }

    void Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);
};

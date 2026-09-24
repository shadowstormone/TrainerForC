#pragma once
#include <Windows.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "d3d11.h"
#include "core/Cheat.h"
#include "patches/WriteAddressPatch.h" // PatchValue
#include "ui/ImGuiConsole.h"

constexpr auto WIDTH = 620;
constexpr auto HEIGHT = 690;      // стартовая высота, дальше подгоняется под содержимое

// Ниже этого окно не ужимается, даже когда читов один-два: совсем
// маленькая панель выглядит обрубком и в неё неудобно целиться мышью.
constexpr auto MIN_HEIGHT = 300;

extern bool showConsole;

class CheatOption;
struct ValueFieldDefinition;

// Подготовленное к отрисовке поле ввода: подпись уже в UTF-8, чтобы не
// перекодировать её каждый кадр.
struct InputFieldView
{
    std::string label;
    std::string hint;
    std::vector<uintptr_t> offsets;
    std::wstring module;
    bool absolute = false;
    PatchValue value;   // то, что сейчас в поле; тип задаёт тип записи
};

// Всплывающее уведомление внизу окна. Заменило модальные окна «Ошибка»/
// «Успех»: те требовали щелчка по OK посреди игры и перекрывали панель.
struct Toast
{
    enum class Kind { Info, Success, Error };

    Kind kind = Kind::Info;
    std::string title;
    std::string text;
    float age = 0.0f;       // секунд с появления
    float lifetime = 3.5f;  // сколько показывать
};

// Содержимое главного окна: полоса заголовка, состояние игры, таблица
// читов, поля ввода и уведомления.
class MainView
{
    std::string _title = "Test Trainer";
    bool _draw = true;

    Cheat* _process = nullptr;
    std::vector<CheatOption*> _options;
    std::vector<InputFieldView> _valueFields;

    std::function<void(CheatOption*, bool)> _toggleHandler;

    // Высота, под которую окно подогнали в прошлый раз. Нужна, чтобы не
    // дёргать SetWindowPos каждый кадр одним и тем же значением.
    int _fittedHeight = 0;

    // Высота содержимого таблицы, измеренная на прошлом кадре. Окно
    // подгоняется по ней, а не по формуле: формула расходилась с реальной
    // вёрсткой при каждой правке отступов.
    float _measuredListHeight = 0.0f;

    // Плавность перехода цвета строки состояния: 0 — игра не запущена, 1 — запущена.
    float _runningFade = 0.0f;
    DWORD _lastPid = 0;

    // Когда последний раз показывали ошибку каждой опции — чтобы одна
    // неудача давала одно уведомление, а не по одному на кадр.
    std::map<const CheatOption*, float> _seenFailure;

    std::vector<Toast> _toasts;

    // Окно, которым управляют кнопки заголовка
    HWND _windowHandle = nullptr;

    // Левая граница блока кнопок заголовка (клиентские координаты).
    // До первого кадра — "бесконечность", чтобы окно таскалось сразу.
    float _titleButtonsMinX = 3.4e38f;

    struct Columns
    {
        float key = 0.0f;     // X колонки клавиш
        float keyWidth = 0.0f;
        float toggle = 0.0f;  // X переключателя
        float name = 0.0f;    // X названия
        float right = 0.0f;   // правая граница содержимого
    };

    Columns ComputeColumns() const;

    void RenderTitleBar();
    void RenderStatusStrip();
    void RenderTableHeader(const Columns& columns);
    void RenderToggles(const Columns& columns);
    void RenderInputFields(const Columns& columns);
    void RenderFooter();
    void RenderToasts(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);

    void WriteValueField(InputFieldView& field);
    void CollectEvents();
    void FitWindowHeightToContent(float chromeHeight);
    void RenderAuthorLink(const char* text, const char* url);

public:
    // Высота полосы заголовка, которую рисует ImGui
    static constexpr float TITLE_BAR_HEIGHT = 34.0f;

    MainView() = default;
    MainView(const MainView&) = delete;
    MainView& operator=(const MainView&) = delete;

    // Поля ввода приходят из ValueFieldRegistry, читы — из CheatRegistry:
    // всё содержимое панели описано в cheats/registry/, а не собирается
    // по кусочкам при старте программы.
    void Initialize(Cheat* process,
                    const std::vector<ValueFieldDefinition>& valueFields,
                    const std::vector<CheatOption*>& options);

    // Название в заголовке. К нему дописывается число функций: «(+5)».
    void SetTitle(std::string title) { _title = std::move(title); }

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

    void SetToggleHandler(std::function<void(CheatOption*, bool)> handler)
    {
        _toggleHandler = std::move(handler);
    }

    // Показать уведомление.
    void Notify(Toast::Kind kind, std::string title, std::string text = {});

    void Active() { _draw = true; }
    bool isActive() const { return _draw; }

    void Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon);
};

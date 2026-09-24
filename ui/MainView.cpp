#include "ui/MainView.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <variant>

#include "cheats/CheatOption.h"
#include "cheats/ValueFieldRegistry.h"
#include "core/MemoryAccess.h"
#include "platform/AudioService.h"
#include "platform/KeyNames.h"
#include "platform/Logger.h"
#include "platform/Utils.h"
#include "platform/VKeys.h"
#include "ui/Layout.h"
#include "ui/UIControls.h"

// Флаг отладочной консоли остаётся глобальным: его читают и UI, и сама консоль.
bool showConsole = false;

namespace
{
    // Палитра панели. Акцент берётся из темы (CheckMark), остальное — здесь,
    // чтобы строки, статус и уведомления говорили одним языком цветов.
    const ImVec4 kMuted(0.56f, 0.59f, 0.64f, 1.0f);
    const ImVec4 kGood(0.30f, 0.82f, 0.46f, 1.0f);
    const ImVec4 kWarn(0.96f, 0.70f, 0.26f, 1.0f);
    const ImVec4 kBad(0.93f, 0.33f, 0.33f, 1.0f);

    // Отступ строки от краёв фона строки.
    constexpr float ROW_PAD_Y = 3.0f;
    constexpr float ROW_GAP = 2.0f;

    ImU32 U32(const ImVec4& c, float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alpha));
    }

    float Pulse(float period)
    {
        const float t = static_cast<float>(std::fmod(ImGui::GetTime(), static_cast<double>(period))) / period;
        return t;
    }

    // Подпись поля значения по его типу — чтобы было видно, что float
    // ждёт дробь, а не целое.
    ImGuiDataType DataTypeOf(const PatchValue& value)
    {
        if (std::holds_alternative<float>(value)) return ImGuiDataType_Float;
        if (std::holds_alternative<double>(value)) return ImGuiDataType_Double;
        if (std::holds_alternative<std::int64_t>(value)) return ImGuiDataType_S64;
        return ImGuiDataType_S32;
    }

    std::string ValueToString(const PatchValue& value)
    {
        return std::visit([](const auto& v)
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_floating_point_v<T>) return std::format("{:g}", v);
            else return std::to_string(static_cast<long long>(v));
        }, value);
    }
}

#ifdef _DEBUG
static bool isKeyHold = false;

static void ImGuiDebugConsoleActivation()
{
    // Клавиша ` открывает консоль, только когда окно трейнера активно:
    // иначе она срабатывала бы и посреди игры.
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) return;

    if (GetAsyncKeyState(VKeys::KEY_BACKTICK) & 0x8000)
    {
        if (!isKeyHold)
        {
            isKeyHold = true;
            showConsole = !showConsole;
        }
    }
    else
    {
        isKeyHold = false;
    }
}
#endif // _DEBUG

// ---------------- MainView ----------------

void MainView::Initialize(Cheat* process,
                          const std::vector<ValueFieldDefinition>& valueFields,
                          const std::vector<CheatOption*>& options)
{
    _process = process;
    _options = options;

    _valueFields.clear();
    _valueFields.reserve(valueFields.size());

    for (const ValueFieldDefinition& field : valueFields)
    {
        InputFieldView view;
        view.label = Utils::WStringToUtf8(field.name);
        view.hint = Utils::WStringToUtf8(field.hint);
        view.offsets = field.offsets;
        view.module = field.module;
        view.absolute = field.absoluteAddress;
        view.value = field.defaultValue;

        _valueFields.push_back(std::move(view));
    }
}

float MainView::TitleBarHeight()
{
    return Layout::Px(34.0f);
}

bool MainView::IsCaptionPoint(POINT clientPoint) const
{
    // Только полоса заголовка и только вне кнопок. Всё остальное окно —
    // HTCLIENT, иначе ImGui перестанет получать WM_MOUSEMOVE и виджеты
    // станут некликабельными.
    if (clientPoint.y < 0 || clientPoint.y >= static_cast<LONG>(_titleBarHeight)) return false;

    return static_cast<float>(clientPoint.x) < _titleButtonsMinX;
}

void MainView::Notify(Toast::Kind kind, std::string title, std::string text)
{
    // Одинаковое уведомление подряд не множим — продлеваем имеющееся.
    for (Toast& toast : _toasts)
    {
        if (toast.kind == kind && toast.title == title && toast.text == text)
        {
            toast.age = (std::min)(toast.age, 0.2f);
            return;
        }
    }

    Toast toast;
    toast.kind = kind;
    toast.title = std::move(title);
    toast.text = std::move(text);
    toast.lifetime = kind == Toast::Kind::Error ? 5.0f : 3.0f;
    _toasts.push_back(std::move(toast));

    // Не больше двух сразу: старые уступают место, список остаётся виден.
    if (_toasts.size() > 2) _toasts.erase(_toasts.begin());
}

// События, которые случаются вне UI-потока: горячая клавиша не включила
// чит, игра запустилась или закрылась. UI узнаёт о них, сравнивая
// состояние с прошлым кадром.
void MainView::CollectEvents()
{
    const DWORD pid = _process->GetProcessID();
    if (pid != _lastPid)
    {
        const std::string name = Utils::WStringToUtf8(_process->GetProcessName());

        if (pid != 0)
            Notify(Toast::Kind::Success, "Игра найдена", std::format("{} · PID {}", name, pid));
        else if (_lastPid != 0)
            Notify(Toast::Kind::Info, "Игра закрыта", "Функции выключены. Запустите игру снова.");

        _lastPid = pid;
    }

    for (const CheatOption* option : _options)
    {
        const float since = option->SecondsSinceFailure();
        float& previous = _seenFailure.try_emplace(option, -1.0f).first->second;

        // Новая неудача: отметка появилась или помолодела.
        if (since >= 0.0f && (previous < 0.0f || since < previous))
        {
            Notify(Toast::Kind::Error, Utils::WStringToUtf8(option->GetDescription()), option->LastError());
        }

        previous = since;
    }
}

void MainView::RenderAuthorLink(const char* text, const char* url)
{
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - textSize.x);

    const ImVec2 textPos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##author", textSize);
    const bool hovered = ImGui::IsItemHovered();

    const ImVec4 color = hovered ? ImVec4(0.55f, 0.80f, 1.0f, 1.0f) : ImVec4(0.67f, 0.62f, 0.82f, 1.0f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddText(textPos, U32(color), text);

    if (hovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        draw->AddLine(ImVec2(textPos.x, textPos.y + textSize.y), ImVec2(textPos.x + textSize.x, textPos.y + textSize.y),
                      U32(color), 1.0f);
    }

    if (ImGui::IsItemClicked())
    {
        ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

// Колонки таблицы. Считаются в одном месте, чтобы поля ввода стояли ровно
// под колонкой названий, а не сами по себе.
MainView::Columns MainView::ComputeColumns() const
{
    const ImGuiStyle& style = ImGui::GetStyle();
    Columns c;

    // Ширина колонки клавиш — по самой длинной подписи, чтобы тумблеры
    // стояли ровным столбцом независимо от того, какие клавиши назначены.
    float widest = ImGui::CalcTextSize("Клавиша").x;
    for (const CheatOption* option : _options)
    {
        const std::string hotkey = KeyNames::Hotkey(option->GetKeys());
        widest = (std::max)(widest, ImGui::CalcTextSize(hotkey.c_str()).x + 12.0f);
    }

    c.key = style.WindowPadding.x;
    c.keyWidth = widest;
    c.toggle = c.key + widest + style.ItemSpacing.x * 2.0f;
    c.name = c.toggle + Layout::ToggleWidth() + style.ItemSpacing.x * 2.0f;
    c.right = ImGui::GetWindowWidth() - style.WindowPadding.x;
    return c;
}

void MainView::RenderTitleBar()
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 winPos = ImGui::GetWindowPos();
    const float winWidth = ImGui::GetWindowWidth();
    const ImVec4 accent = ImGui::GetStyle().Colors[ImGuiCol_CheckMark];

    const float titleH = TitleBarHeight();
    _titleBarHeight = titleH;

    // Фон полосы во всю ширину (рисуем напрямую, минуя отступы окна)
    draw->AddRectFilled(winPos, ImVec2(winPos.x + winWidth, winPos.y + titleH), IM_COL32(24, 26, 30, 255));

    // Тонкая акцентная линия под заголовком: гаснет к краям.
    const float lineY = winPos.y + titleH - 1.0f;
    const float mid = winPos.x + winWidth * 0.5f;
    draw->AddRectFilledMultiColor(ImVec2(winPos.x, lineY), ImVec2(mid, lineY + 1.0f),
                                  U32(accent, 0.0f), U32(accent, 0.9f), U32(accent, 0.9f), U32(accent, 0.0f));
    draw->AddRectFilledMultiColor(ImVec2(mid, lineY), ImVec2(winPos.x + winWidth, lineY + 1.0f),
                                  U32(accent, 0.9f), U32(accent, 0.0f), U32(accent, 0.0f), U32(accent, 0.9f));

    // Значок: скруглённый квадрат с акцентом — узнаваемо и без картинки.
    const float lineH = ImGui::GetTextLineHeight();
    const float iconSize = lineH;
    const ImVec2 iconPos(winPos.x + Layout::Px(14.0f), winPos.y + (titleH - iconSize) * 0.5f);
    draw->AddRectFilled(iconPos, ImVec2(iconPos.x + iconSize, iconPos.y + iconSize), U32(accent), 4.0f);
    draw->AddRectFilled(ImVec2(iconPos.x + iconSize * 0.30f, iconPos.y + iconSize * 0.30f),
                        ImVec2(iconPos.x + iconSize * 0.70f, iconPos.y + iconSize * 0.70f),
                        IM_COL32(24, 26, 30, 255), 2.0f);

    // Название и число функций — как принято у трейнеров: «(+5)».
    const float textY = (titleH - lineH) * 0.5f;
    ImGui::SetCursorPos(ImVec2(Layout::Px(14.0f) + iconSize + Layout::Px(10.0f), textY));
    ImGui::TextUnformatted(_title.c_str());
    ImGui::SameLine(0.0f, 6.0f);
    ImGui::TextColored(kMuted, "+%d", static_cast<int>(_options.size() + _valueFields.size()));

    const float btnW = Layout::Px(46.0f);
    const float btnH = titleH - 1.0f;
    const float minimizeX = winWidth - btnW * 2.0f;
    const float closeX = winWidth - btnW;

    // Запоминаем для хиттеста: левее этой границы — перетаскивание окна
    _titleButtonsMinX = minimizeX;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 40));

    // Свернуть
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 25));
    ImGui::SetCursorPos(ImVec2(minimizeX, 0.0f));
    if (ImGui::Button("##minimize", ImVec2(btnW, btnH)) && _windowHandle)
    {
        ::ShowWindow(_windowHandle, SW_MINIMIZE);
    }
    ImGui::PopStyleColor();

    // Закрыть
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(196, 43, 28, 255));
    ImGui::SetCursorPos(ImVec2(closeX, 0.0f));
    if (ImGui::Button("##close", ImVec2(btnW, btnH)) && _windowHandle)
    {
        ::PostMessageW(_windowHandle, WM_CLOSE, 0, 0);
    }
    ImGui::PopStyleColor();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    // Значки поверх кнопок
    const ImU32 glyph = IM_COL32(220, 222, 226, 255);
    const float cy = winPos.y + btnH * 0.5f;

    const float g = Layout::Px(5.0f);
    const float mcx = winPos.x + minimizeX + btnW * 0.5f;
    draw->AddLine(ImVec2(mcx - g, cy), ImVec2(mcx + g, cy), glyph, Layout::Px(1.0f));

    const float ccx = winPos.x + closeX + btnW * 0.5f;
    draw->AddLine(ImVec2(ccx - g, cy - g), ImVec2(ccx + g, cy + g), glyph, Layout::Px(1.2f));
    draw->AddLine(ImVec2(ccx + g, cy - g), ImVec2(ccx - g, cy + g), glyph, Layout::Px(1.2f));
}

// Состояние игры — первое, что нужно знать, поэтому оно наверху, а не
// мелким текстом в подвале: ждём, подключились или нет доступа.
void MainView::RenderStatusStrip()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const bool running = _process->isProcessRunning();
    const bool denied = !running && _process->IsAccessDenied();
    const std::string exe = Utils::WStringToUtf8(_process->GetProcessName());

    _runningFade = UIControls::Approach(_runningFade, running ? 1.0f : 0.0f, 6.0f);

    const float height = ImGui::GetFrameHeight() + Layout::Px(6.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetWindowWidth() - style.WindowPadding.x * 2.0f;
    const ImVec2 max(p.x + width, p.y + height);

    const ImVec4 tone = denied ? kBad : UIControls::Mix(kWarn, kGood, _runningFade);

    draw->AddRectFilled(p, max, U32(tone, 0.08f), 6.0f);
    draw->AddRect(p, max, U32(tone, 0.22f), 6.0f);

    const float lineH = ImGui::GetTextLineHeight();
    const float textY = p.y + (height - lineH) * 0.5f;

    // Точка: пульсирует, пока ждём игру.
    UIControls::StatusDot(ImVec2(p.x + Layout::Px(14.0f), p.y + height * 0.5f), Layout::Px(4.0f), U32(tone),
                          running || denied ? 0.0f : Pulse(1.6f));

    const float textX = p.x + Layout::Px(28.0f);
    if (running)
    {
        draw->AddText(ImVec2(textX, textY), U32(style.Colors[ImGuiCol_Text]), exe.c_str());
        const float nameW = ImGui::CalcTextSize(exe.c_str()).x;
        draw->AddText(ImVec2(textX + nameW + Layout::Px(8.0f), textY), U32(kGood), "подключено");

        const std::string info = std::format("PID {}  ·  {}", _process->GetProcessID(),
                                             _process->IsTargetX64() ? "x64" : "x86");
        const float infoW = ImGui::CalcTextSize(info.c_str()).x;
        draw->AddText(ImVec2(max.x - infoW - Layout::Px(12.0f), textY), U32(kMuted), info.c_str());
    }
    else if (denied)
    {
        draw->AddText(ImVec2(textX, textY), U32(kBad),
                      "Нет доступа к игре — запустите трейнер от имени администратора");
    }
    else
    {
        const std::string text = "Ожидание игры  " + exe;
        draw->AddText(ImVec2(textX, textY), U32(style.Colors[ImGuiCol_Text], 0.85f), text.c_str());

        // Бегущее многоточие — видно, что трейнер жив и ищет.
        const int dots = static_cast<int>(ImGui::GetTime() * 2.0) % 4;
        const float textW = ImGui::CalcTextSize(text.c_str()).x;
        draw->AddText(ImVec2(textX + textW, textY), U32(kMuted), std::string(static_cast<size_t>(dots), '.').c_str());
    }

    ImGui::Dummy(ImVec2(width, height));
}

// Шапка таблицы. Рисуется ВНЕ прокручиваемой области, иначе уезжала бы
// вместе со списком.
void MainView::RenderTableHeader(const Columns& columns)
{
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::SetCursorPosX(columns.key);
    ImGui::TextUnformatted("Клавиша");
    ImGui::SameLine(columns.name);
    ImGui::TextUnformatted("Функция");
    ImGui::PopStyleColor();

    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y), ImVec2(ImGui::GetWindowPos().x + columns.right, p.y),
                                        ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
}

void MainView::RenderToggles(const Columns& columns)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec4 accent = style.Colors[ImGuiCol_CheckMark];
    const bool gameRunning = _process->isProcessRunning();

    const float itemH = Layout::RowHeight();
    const float rowH = itemH + Layout::Px(ROW_PAD_Y) * 2.0f;
    const float rowGap = Layout::Px(ROW_GAP);
    const float winX = ImGui::GetWindowPos().x;

    for (CheatOption* option : _options)
    {
        ImGui::PushID(option);

        const std::string name = Utils::WStringToUtf8(option->GetDescription());
        const std::string hotkey = KeyNames::Hotkey(option->GetKeys());
        const std::string error = option->LastError();
        const bool enabled = option->IsEnabled();
        const bool oneShot = option->IsOneShot();

        const ImVec2 rowPos = ImGui::GetCursorScreenPos();
        const ImVec2 rowMin(winX + columns.key - Layout::Px(6.0f), rowPos.y);
        const ImVec2 rowMax(winX + columns.right + Layout::Px(6.0f), rowPos.y + rowH);

        const bool rowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
                              && ImGui::IsMouseHoveringRect(rowMin, rowMax);

        // Анимация подсветки строки: включена / навели / только что упала.
        ImGuiStorage* storage = ImGui::GetStateStorage();
        const ImGuiID glowId = ImGui::GetID("##glow");
        const float glow = UIControls::Approach(storage->GetFloat(glowId, enabled ? 1.0f : 0.0f),
                                                enabled ? 1.0f : 0.0f, 10.0f);
        storage->SetFloat(glowId, glow);

        const float failAge = option->SecondsSinceFailure();
        const float failFlash = (failAge >= 0.0f && failAge < 0.9f) ? 1.0f - failAge / 0.9f : 0.0f;

        // --- фон строки ---
        if (rowHovered) draw->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 10), 6.0f);
        if (glow > 0.01f)
        {
            draw->AddRectFilled(rowMin, rowMax, U32(accent, 0.09f * glow), 6.0f);
            draw->AddRectFilled(ImVec2(rowMin.x, rowMin.y + Layout::Px(5.0f)), ImVec2(rowMin.x + Layout::Px(3.0f), rowMax.y - Layout::Px(5.0f)),
                                U32(accent, glow), 2.0f);
        }
        if (failFlash > 0.0f) draw->AddRectFilled(rowMin, rowMax, U32(kBad, 0.22f * failFlash), 6.0f);

        const float centerY = rowPos.y + rowH * 0.5f;

        // --- клавиша ---
        if (!hotkey.empty())
        {
            const float capH = ImGui::GetTextLineHeight() + 2.0f;
            ImGui::SetCursorScreenPos(ImVec2(winX + columns.key, centerY - capH * 0.5f));
            UIControls::KeyCap(hotkey.c_str());
        }

        // --- переключатель или кнопка действия ---
        //
        // Без игры они не заблокированы, а приглушены: щелчок всё равно
        // даёт понятный ответ («игра не запущена»), а не мёртвую кнопку.
        if (!gameRunning) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, style.Alpha * 0.55f);

        ImGui::SetCursorScreenPos(ImVec2(winX + columns.toggle, centerY - Layout::ToggleHeight() * 0.5f));
        bool requested = false;
        bool state = enabled;

        if (oneShot)
        {
            requested = UIControls::ActionPill("##action", "SET", enabled,
                                               ImVec2(Layout::ToggleWidth(), Layout::ToggleHeight()));
            state = true;
        }
        else
        {
            requested = UIControls::AnimatedToggleSwitch("##toggle", &state,
                                                         ImVec2(Layout::ToggleWidth(), Layout::ToggleHeight()));
        }

        if (!gameRunning) ImGui::PopStyleVar();

        // --- название: по нему тоже можно щёлкнуть ---
        const bool showError = !error.empty() && !enabled;
        const float badgeW = showError ? ImGui::GetTextLineHeight() + style.ItemSpacing.x : 0.0f;

        std::string tag;
        if (option->HasFreeze()) tag = "заморозка";
        const float tagW = tag.empty() ? 0.0f : ImGui::CalcTextSize(tag.c_str()).x + 12.0f + style.ItemSpacing.x;

        const float nameW = columns.right - columns.name - badgeW - tagW;
        const std::string shown = Layout::FitText(name, nameW);
        const ImVec2 shownSize = ImGui::CalcTextSize(shown.c_str());
        const float textY = centerY - ImGui::GetTextLineHeight() * 0.5f;

        ImGui::SetCursorScreenPos(ImVec2(winX + columns.name, rowPos.y));
        ImGui::InvisibleButton("##name", ImVec2((std::max)(1.0f, nameW), rowH));
        const bool nameHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
        {
            requested = true;
            state = oneShot ? true : !enabled;
        }

        const ImVec4 nameColor = enabled ? style.Colors[ImGuiCol_Text]
                                         : UIControls::Mix(style.Colors[ImGuiCol_Text], kMuted, 0.25f);
        draw->AddText(ImVec2(winX + columns.name, textY), U32(nameColor), shown.c_str());

        if (nameHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            const std::string hint = Utils::WStringToUtf8(option->GetHint());
            if (shown != name || !hint.empty())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
                if (shown != name) ImGui::TextUnformatted(name.c_str());
                if (!hint.empty()) ImGui::TextColored(kMuted, "%s", hint.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        // --- метка «заморозка» ---
        if (!tag.empty())
        {
            const ImVec2 tagSize = ImGui::CalcTextSize(tag.c_str());
            const ImVec2 tagMin(winX + columns.name + shownSize.x + style.ItemSpacing.x, centerY - tagSize.y * 0.5f - 1.0f);
            const ImVec2 tagMax(tagMin.x + tagSize.x + 12.0f, tagMin.y + tagSize.y + 2.0f);
            const ImVec4 tagColor(0.45f, 0.78f, 1.0f, 1.0f);
            draw->AddRectFilled(tagMin, tagMax, U32(tagColor, 0.12f), 4.0f);
            draw->AddText(ImVec2(tagMin.x + 6.0f, tagMin.y + 1.0f), U32(tagColor, 0.9f), tag.c_str());
        }

        // --- идёт включение / значок ошибки ---
        if (option->IsBusy())
        {
            const float size = ImGui::GetTextLineHeight();
            ImGui::SetCursorScreenPos(ImVec2(winX + columns.right - size, centerY - size * 0.5f));
            UIControls::Spinner("##busy", size, U32(accent));
        }
        else if (showError)
        {
            const float size = ImGui::GetTextLineHeight();
            ImGui::SetCursorScreenPos(ImVec2(winX + columns.right - size, centerY - size * 0.5f));
            UIControls::ErrorBadge("##error", error, failFlash);
        }

        if (requested && _toggleHandler)
        {
            _toggleHandler(option, state);
        }

        // Строка целиком — один элемент для раскладки: следующая встанет под ней.
        ImGui::SetCursorScreenPos(rowPos);
        ImGui::Dummy(ImVec2(columns.right - columns.key, rowH + rowGap));

        ImGui::PopID();
    }
}

void MainView::WriteValueField(InputFieldView& field)
{
    if (!_process->isProcessRunning())
    {
        Notify(Toast::Kind::Error, field.label, "Игра не запущена");
        AudioService::Instance().Play(Sound::CheatFailed);
        return;
    }

    // Пишем напрямую через MemoryAccess: отдельный патч ради
    // одноразовой записи из поля ввода не нужен.
    MemoryAccess mem(_process->GetProcessID());

    // Абсолютный адрес берём как есть, иначе идём цепочкой от базы
    // модуля — та же семантика, что у читов.
    const uintptr_t address =
        !mem.IsValid()   ? 0
        : field.absolute ? (field.offsets.empty() ? 0 : field.offsets.back())
                         : mem.ResolveChain(mem.ModuleOrMain(field.module).base, field.offsets);

    const bool ok = address != 0
        && std::visit([&](const auto& v) { return mem.WriteValue(address, v); }, field.value);

    const std::string shown = ValueToString(field.value);

    if (ok)
    {
        Log::Info(std::format("{}: записано {} по адресу 0x{:X}", field.label, shown, address));
        AudioService::Instance().Play(Sound::CheatEnabled);
        Notify(Toast::Kind::Success, field.label, "Записано: " + shown);
    }
    else
    {
        const std::string why = address == 0
            ? "Адрес не вычислился — нужный объект в игре ещё не загружен?"
            : "Не удалось записать значение в память";

        Log::Error(field.label + ": " + why);
        AudioService::Instance().Play(Sound::CheatFailed);
        Notify(Toast::Kind::Error, field.label, why);
    }
}

void MainView::RenderInputFields(const Columns& columns)
{
    if (_valueFields.empty()) return;

    const ImGuiStyle& style = ImGui::GetStyle();

    // Раздел «Значения» — продолжение той же таблицы, с подписью.
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::SetCursorPosX(columns.key);
    ImGui::TextColored(kMuted, "Значения");
    ImGui::SameLine();
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float y = p.y + ImGui::GetTextLineHeight() * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + 4.0f, y), ImVec2(ImGui::GetWindowPos().x + columns.right, y),
                                            ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
        ImGui::NewLine();
    }

    const float buttonWidth = ImGui::CalcTextSize("Записать").x + style.FramePadding.x * 2.0f;
    const float stepperWidth = Layout::InputWidth();

    const float buttonX = columns.right - buttonWidth;
    const float stepperX = buttonX - style.ItemSpacing.x - stepperWidth;

    const float itemH = Layout::RowHeight();
    const float rowH = itemH + Layout::Px(ROW_PAD_Y) * 2.0f;
    const float rowGap = Layout::Px(ROW_GAP);

    for (InputFieldView& field : _valueFields)
    {
        ImGui::PushID(&field);

        const ImVec2 rowPos = ImGui::GetCursorScreenPos();
        const float frameY = ImGui::GetCursorPosY() + (rowH - ImGui::GetFrameHeight()) * 0.5f;

        ImGui::SetCursorPos(ImVec2(columns.name, frameY));
        ImGui::AlignTextToFramePadding();
        Layout::RowLabel(field.label, stepperX - columns.name - style.ItemSpacing.x, style.Colors[ImGuiCol_Text]);
        if (!field.hint.empty() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", field.hint.c_str());

        // Поле показывает тип значения: у дробного — дробный шаг.
        const ImGuiDataType type = DataTypeOf(field.value);
        const double step = (type == ImGuiDataType_Float || type == ImGuiDataType_Double) ? 0.1 : 1.0;

        ImGui::SetCursorPos(ImVec2(stepperX, frameY));
        bool submit = false;
        std::visit([&](auto& v)
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::int32_t> || std::is_same_v<T, float>
                       || std::is_same_v<T, double> || std::is_same_v<T, std::int64_t>)
            {
                UIControls::ValueStepper("##stepper", type, &v, step, stepperWidth, &submit);
            }
            else
            {
                int proxy = static_cast<int>(v);
                if (UIControls::ValueStepper("##stepper", ImGuiDataType_S32, &proxy, 1.0, stepperWidth, &submit))
                    v = static_cast<T>(proxy);
            }
        }, field.value);

        ImGui::SetCursorPos(ImVec2(buttonX, frameY));

        // Enter в поле — то же, что кнопка.
        if (ImGui::Button("Записать", ImVec2(buttonWidth, 0.0f))) submit = true;
        if (submit) WriteValueField(field);

        ImGui::SetCursorScreenPos(rowPos);
        ImGui::Dummy(ImVec2(columns.right - columns.key, rowH + rowGap));

        ImGui::PopID();
    }
}

void MainView::RenderFooter()
{
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);

#ifdef _DEBUG
    ImGui::TextUnformatted("Горячие клавиши работают в игре  ·  ` — консоль");
#else
    ImGui::TextUnformatted("Горячие клавиши работают в игре");
#endif

    ImGui::PopStyleColor();
    ImGui::SameLine();
    RenderAuthorLink("By ShadowStormOne", "https://t.me/ShadowStormOne");
}

void MainView::RenderToasts(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
    if (_toasts.empty()) return;

    const float dt = ImGui::GetIO().DeltaTime;
    for (Toast& toast : _toasts) toast.age += dt;

    _toasts.erase(std::remove_if(_toasts.begin(), _toasts.end(),
                                 [](const Toast& t) { return t.age >= t.lifetime; }),
                  _toasts.end());

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 winSize = ImGui::GetWindowSize();

    const float width = (std::min)(Layout::Px(300.0f), winSize.x - Layout::Px(32.0f));
    const float padding = Layout::Px(8.0f);
    const float lineH = ImGui::GetTextLineHeight();
    const float iconSize = lineH;
    const float textWidth = width - padding * 2.0f - iconSize - 8.0f;

    // Снизу вверх, над подвалом.
    float bottom = winPos.y + winSize.y - ImGui::GetFrameHeightWithSpacing() * 1.6f;

    for (auto it = _toasts.rbegin(); it != _toasts.rend(); ++it)
    {
        const Toast& toast = *it;

        // Появление и исчезновение — прозрачностью и сдвигом.
        const float in = (std::min)(1.0f, toast.age / 0.18f);
        const float out = (std::min)(1.0f, (toast.lifetime - toast.age) / 0.35f);
        const float alpha = (std::max)(0.0f, (std::min)(in, out));
        const float slide = (1.0f - in) * 12.0f;

        const ImVec2 textSize = toast.text.empty()
            ? ImVec2(0.0f, 0.0f)
            : ImGui::CalcTextSize(toast.text.c_str(), nullptr, false, textWidth);
        const float height = padding * 2.0f + lineH + (toast.text.empty() ? 0.0f : textSize.y + 2.0f);

        const ImVec2 min(winPos.x + winSize.x - width - style.WindowPadding.x, bottom - height + slide);
        const ImVec2 max(min.x + width, min.y + height);

        const ImVec4 tone = toast.kind == Toast::Kind::Error   ? kBad
                          : toast.kind == Toast::Kind::Success ? kGood
                          : style.Colors[ImGuiCol_CheckMark];

        draw->AddRectFilled(ImVec2(min.x + 2.0f, min.y + 4.0f), ImVec2(max.x + 2.0f, max.y + 4.0f),
                            IM_COL32(0, 0, 0, static_cast<int>(90 * alpha)), 8.0f);
        draw->AddRectFilled(min, max, U32(ImVec4(0.13f, 0.14f, 0.16f, 1.0f), 0.97f * alpha), 8.0f);
        draw->AddRect(min, max, U32(tone, 0.45f * alpha), 8.0f);
        draw->AddRectFilled(ImVec2(min.x, min.y + 8.0f), ImVec2(min.x + 3.0f, max.y - 8.0f), U32(tone, alpha), 2.0f);

        const ImVec2 iconPos(min.x + padding + 2.0f, min.y + padding);
        ID3D11ShaderResourceView* icon = toast.kind == Toast::Kind::Error ? errorIcon
                                       : toast.kind == Toast::Kind::Success ? successIcon
                                       : nullptr;
        if (icon)
        {
            draw->AddImage(reinterpret_cast<ImTextureID>(icon), iconPos,
                           ImVec2(iconPos.x + iconSize, iconPos.y + iconSize),
                           ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)));
        }
        else
        {
            UIControls::StatusDot(ImVec2(iconPos.x + iconSize * 0.5f, iconPos.y + iconSize * 0.5f), 4.0f, U32(tone, alpha));
        }

        const float textX = iconPos.x + iconSize + 8.0f;
        draw->AddText(ImVec2(textX, min.y + padding), U32(style.Colors[ImGuiCol_Text], alpha), toast.title.c_str());

        if (!toast.text.empty())
        {
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(textX, min.y + padding + lineH + 2.0f),
                          U32(ImVec4(0.78f, 0.80f, 0.84f, 1.0f), alpha), toast.text.c_str(), nullptr, textWidth);
        }

        bottom = min.y - slide - 8.0f;
    }
}

// Подгоняет высоту окна под количество строк.
//
// Высота списка измеряется на прошлом кадре — ImGui к этому моменту уже
// знает реальную высоту каждой строки с учётом шрифта и масштаба экрана.
void MainView::FitWindowHeightToContent(float chromeHeight)
{
    if (!_windowHandle || _measuredListHeight <= 0.0f) return;

    const float content = chromeHeight + _measuredListHeight;

    // Потолок — чтобы окно не выросло во весь экран на полусотне читов:
    // дальше уже работает прокрутка.
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    const int maxHeight = static_cast<int>((work.bottom - work.top) * 0.85f);

    int desired = static_cast<int>(std::ceil(content));
    desired = Utils::Clamp(desired, static_cast<int>(MIN_HEIGHT * Layout::Scale()), maxHeight);

    if (desired == _fittedHeight) return;
    _fittedHeight = desired;

    RECT current{};
    if (!GetWindowRect(_windowHandle, &current)) return;

    // Окно растёт вниз от своей верхней кромки, поэтому при большом списке
    // оно уехало бы за нижний край экрана. Если не помещается — поднимаем.
    int top = current.top;
    if (top + desired > work.bottom) top = work.bottom - desired;
    if (top < work.top) top = work.top;

    const UINT flags = (top == current.top)
        ? (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)
        : (SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(_windowHandle, nullptr, current.left, top, current.right - current.left, desired, flags);
}

void MainView::Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
    if (!isActive() || !_process) return;

    CollectEvents();

    // Одно окно ImGui на весь клиент: размером владеет Win32.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##MainWindow", nullptr, flags);
    ImGui::PopStyleVar(2);

#ifdef _DEBUG
    ImGuiDebugConsoleActivation();
#endif // _DEBUG

    RenderTitleBar();

    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetCursorPos(ImVec2(style.WindowPadding.x, TitleBarHeight() + Layout::Px(10.0f)));

    RenderStatusStrip();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    const Columns columns = ComputeColumns();
    RenderTableHeader(columns);

    // Список читов и поля ввода — в прокручиваемой области. Подвал всегда
    // на своём месте, а не поверх строк списка.
    const float footerHeight = ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y + 4.0f;
    const float listTop = ImGui::GetCursorPosY();
    const float listHeight = ImGui::GetContentRegionAvail().y - footerHeight;

    // Нулевые отступы у дочерней области не случайны: так её содержимое
    // считает координаты от тех же краёв, что и шапка с подвалом, и
    // колонки не разъезжаются между ними.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
    ImGui::SetCursorPosX(0.0f);
    ImGui::BeginChild("##content", ImVec2(ImGui::GetWindowWidth(), (std::max)(listHeight, 1.0f)),
                      ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

    RenderToggles(columns);
    RenderInputFields(columns);

    _measuredListHeight = ImGui::GetCursorPosY() + Layout::Px(2.0f);

    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    // Подвал
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - footerHeight + 4.0f);
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(ImGui::GetWindowPos().x + style.WindowPadding.x, p.y - 4.0f),
                                            ImVec2(ImGui::GetWindowPos().x + columns.right, p.y - 4.0f),
                                            ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
    }
    ImGui::SetCursorPosX(style.WindowPadding.x);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
    RenderFooter();

    // Всё, что не список: заголовок, статус, шапка, подвал, отступы.
    FitWindowHeightToContent(listTop + footerHeight + style.WindowPadding.y);

    RenderToasts(successIcon, errorIcon);

    // Перетаскивание окна за любое пустое место.
    //
    // Делается здесь, а не в WM_NCHITTEST: ImGui знает, есть ли под
    // курсором виджет, только пока область остаётся HTCLIENT и он
    // получает обычные WM_MOUSEMOVE. Поэтому мы ловим нажатие по пустому
    // месту и просим саму систему начать перетаскивание — ровно так же,
    // как если бы нажали на заголовок.
    if (_windowHandle
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        && !ImGui::IsAnyItemHovered()
        && !ImGui::IsAnyItemActive()
        && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
    {
        // ImGui на нажатии захватывает мышь — отпускаем, иначе система
        // не начнёт свой цикл перетаскивания.
        ::ReleaseCapture();
        ::SendMessageW(_windowHandle, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    ImGui::End();
}

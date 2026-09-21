#include "ui/MainView.h"

#include "ui/Layout.h"
#include "cheats/ValueFieldRegistry.h"
#include "platform/KeyNames.h"

#include <climits>  // Для INT_MAX

#include "core/MemoryAccess.h"
#include <cmath>

#include "platform/AudioService.h"
#include "platform/Utils.h"
#include "platform/VKeys.h"
#include "resource.h"
#include "ui/UIControls.h"

// Флаг отладочной консоли остаётся глобальным: его читают и UI, и сама консоль.
bool showConsole = false;


namespace
{
    bool isKeyHold = false;
}

static void ImGuiDebugConsoleActivation()
{
    // Проверяем состояние клавиши VK_OEM_3
    if (GetAsyncKeyState(VKeys::KEY_BACKTICK) & 0x8000) // Клавиша нажата
    {
        if (!isKeyHold) // Если это первое нажатие
        {
            isKeyHold = true; // Устанавливаем флаг
            showConsole = !showConsole; // Переключаем состояние консоли
        }
    }
    else
    {
        isKeyHold = false; // Сбрасываем флаг при отпускании
    }
}

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
        view.offsets = field.offsets;
        view.absolute = field.absoluteAddress;
        view.defaultValue = field.defaultValue;

        _valueFields.push_back(std::move(view));
    }
}

bool MainView::IsCaptionPoint(POINT clientPoint) const
{
    // Только полоса заголовка и только вне кнопок. Всё остальное окно —
    // HTCLIENT, иначе ImGui перестанет получать WM_MOUSEMOVE и виджеты
    // станут некликабельными.
    if (clientPoint.y < 0 || clientPoint.y >= static_cast<LONG>(TITLE_BAR_HEIGHT)) return false;

    return static_cast<float>(clientPoint.x) < _titleButtonsMinX;
}

void MainView::HandlePopupsWithIcons(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
    // Высота строки текста
    float textHeight = ImGui::GetTextLineHeightWithSpacing();

    if (_popupType == "Error" && ImGui::BeginPopup("ErrorPopup"))
    {
        if (errorIcon)
        {
            float iconSize = 22.0f; // Размер иконки
            float iconYOffset = (textHeight - iconSize) * 0.10f; // Смещение для центровки

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + iconYOffset); // Смещаем иконку по Y
            ImGui::Image((void*)errorIcon, ImVec2(iconSize, iconSize));
            ImGui::SameLine();
        }

        // Рисуем текст рядом с иконкой
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Ошибка");
        ImGui::Separator();
        ImGui::Text("%s", _popupMessage.c_str());
        if (ImGui::Button("OK"))
        {
            ImGui::CloseCurrentPopup();
            _popupType = ""; // Сброс типа Popup
        }
        ImGui::EndPopup();
    }

    if (_popupType == "Success" && ImGui::BeginPopup("SuccessPopup"))
    {
        if (successIcon)
        {
            float iconSize = 22.0f; // Размер иконки
            float iconYOffset = (textHeight - iconSize) * 0.10f; // Смещение для центровки

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + iconYOffset); // Смещаем иконку по Y
            ImGui::Image((void*)successIcon, ImVec2(iconSize, iconSize));
            ImGui::SameLine();
        }

        // Рисуем текст рядом с иконкой
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Успех");
        ImGui::Separator();
        ImGui::Text("%s", _popupMessage.c_str());
        if (ImGui::Button("OK"))
        {
            ImGui::CloseCurrentPopup();
            _popupType = ""; // Сброс типа Popup
        }
        ImGui::EndPopup();
    }
}

void MainView::RenderAuthorLink(const char* text, const char* url, float offsetRight)
{
    float textWidth = ImGui::CalcTextSize(text).x;
    float windowWidth = ImGui::GetWindowContentRegionMax().x;
    ImGui::SetCursorPosX(windowWidth - textWidth - offsetRight);

    // Позиция текста
    ImVec2 textPos = ImGui::GetCursorScreenPos();
    ImGui::TextColored(ImVec4(171.0f / 255.0f, 157.0f / 255.0f, 209.0f / 255.0f, 1.0f), "%s", text);

    // Подчёркивание
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 lineStart(textPos.x, textPos.y + textSize.y);
    ImVec2 lineEnd(textPos.x + textSize.x, textPos.y + textSize.y);
    ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, IM_COL32(171, 157, 209, 255), 1.0f);

    // Эффект ссылки
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, IM_COL32(120, 210, 255, 255), 1.0f); // светлее
    }

    if (ImGui::IsItemClicked())
    {
        ShellExecuteA(0, "open", url, 0, 0, SW_SHOWNORMAL);
    }
}


// X колонок таблицы. Считается в одном месте, чтобы нижняя группа
// (поля ввода) стояла ровно под колонкой названий, а не сама по себе.
void MainView::ComputeColumns(float& outToggleX, float& outNameX) const
{
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    // Ширина колонки клавиш — по самой длинной подписи, чтобы тумблеры
    // стояли ровным столбцом независимо от того, какие клавиши назначены.
    float hotkeyWidth = ImGui::CalcTextSize("Hotkeys").x;
    for (CheatOption* option : _options)
    {
        const std::string hotkey = KeyNames::Hotkey(option->GetKeys());
        hotkeyWidth = (std::max)(hotkeyWidth, ImGui::CalcTextSize(hotkey.c_str()).x);
    }

    outToggleX = Layout::ContentLeft() + hotkeyWidth + spacing * 2.0f;
    outNameX   = outToggleX + Layout::TOGGLE_WIDTH + spacing * 2.0f;
}

// Шапка таблицы. Рисуется ВНЕ прокручиваемой области, иначе уезжала бы
// вместе со списком.
void MainView::RenderTableHeader()
{
    float toggleX = 0.0f;
    float nameX = 0.0f;
    ComputeColumns(toggleX, nameX);

    ImGui::TextDisabled("Hotkeys");
    ImGui::SameLine(nameX);
    ImGui::TextDisabled("Options");
    ImGui::Separator();
}

void MainView::RenderToggles()
{
    const bool isGameRunning = _process->GetProcessID() != 0;

    float toggleX = 0.0f;
    float nameX = 0.0f;
    ComputeColumns(toggleX, nameX);

    for (CheatOption* option : _options)
    {
        const std::string name = Utils::WStringToUtf8(option->GetDescription());
        const std::string toggleId = "##toggle_" + name;
        const std::string hotkey = KeyNames::Hotkey(option->GetKeys());


        // Клавиша — отдельная колонка, а не часть названия: подпись
        // выводится из реально назначенных кодов и не может с ними разойтись.
        ImGui::TextUnformatted(hotkey.c_str());

        ImGui::SameLine(toggleX);

        if (!isGameRunning)
        {
            ImGui::BeginDisabled();
        }

        // Источник истины — сама опция, а не отдельная карта в UI.
        // Раньше состояние дублировалось, из-за чего менеджеру приходилось
        // писать в карту UI (в том числе из другого потока).
        const bool previousState = option->IsEnabled();
        bool state = previousState;

        // Анимация переключателя хранится внутри виджета по его id,
        // поэтому локальной переменной здесь достаточно.
        if (UIControls::AnimatedToggleSwitch(toggleId.c_str(), &state))
        {
            HandleToggleInteraction(toggleId, name, state, previousState);
        }

        if (!isGameRunning)
        {
            ImGui::EndDisabled();
        }

        // Название идёт справа от тумблера и растёт вправо — длинным именам
        // больше не нужно тесниться, обрезка включается только если окно
        // реально узкое.
        ImGui::SameLine(nameX);
        Layout::RowLabel(name,
                         Layout::ContentRight() - nameX,
                         option->IsEnabled() ? ImVec4(0.35f, 0.85f, 0.35f, 1.0f)
                                             : ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
}


void MainView::RenderInputFields()
{
    // Колонки те же, что у читов: строки с полями должны читаться как
    // продолжение таблицы, а не как отдельная панель внизу.
    float toggleX = 0.0f;
    float nameX = 0.0f;
    ComputeColumns(toggleX, nameX);

    const ImGuiStyle& style = ImGui::GetStyle();
    const float spacing = style.ItemSpacing.x;
    const float buttonWidth = ImGui::CalcTextSize("Write").x + style.FramePadding.x * 2.0f;
    const float stepperWidth = Layout::INPUT_WIDTH;

    const float buttonX = Layout::ControlX(buttonWidth);
    const float stepperX = buttonX - spacing - stepperWidth;

    for (const InputFieldView& field : _valueFields)
    {
        const std::string& name = field.label;

        // try_emplace вместо find/вставки: значение заводится один раз и
        // дальше правится по ссылке.
        int& value = _inputValues.try_emplace(name, field.defaultValue).first->second;

        ImGui::SetCursorPosX(nameX);
        Layout::RowLabel(name, stepperX - nameX - spacing, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        ImGui::SameLine(stepperX);
        UIControls::ValueStepper(("##stepper_" + name).c_str(), &value, 1, 1, INT_MAX, stepperWidth);

        ImGui::SameLine(buttonX);

        if (!ImGui::Button(("Write##" + name).c_str())) continue;

        if (!_process->isProcessRunning())
        {
            _popupType = "Error";
            _popupMessage = "Процесс игры не запущен!";
            ImGui::OpenPopup("ErrorPopup");
            continue;
        }

        // Пишем напрямую через MemoryAccess: отдельный патч ради
        // одноразовой записи из поля ввода не нужен.
        MemoryAccess mem(_process->GetProcessID());
        int valueToWrite = value;
        SIZE_T written = 0;

        // Абсолютный адрес берём как есть, иначе идём цепочкой от базы
        // модуля — та же семантика, что у читов.
        const uintptr_t address =
            !mem.IsValid()   ? 0
            : field.absolute ? (field.offsets.empty() ? 0 : field.offsets.back())
                             : mem.ResolveChain(mem.ProcessBase(), field.offsets);

        const bool ok = address != 0
            && WriteProcessMemory(mem.Handle(), reinterpret_cast<LPVOID>(address),
                                  &valueToWrite, sizeof(valueToWrite), &written)
            && written == sizeof(valueToWrite);

        if (ok)
        {
            Log::Info(std::format("{}: записано {} по адресу 0x{:X}", name, valueToWrite, address));
            AudioService::Instance().Play(Sound::CheatEnabled);

            _popupType = "Success";
            _popupMessage = "Значение " + std::to_string(valueToWrite) + " записано в память";
            ImGui::OpenPopup("SuccessPopup");
        }
        else
        {
            Log::Error(name + ": не удалось записать значение");

            _popupType = "Error";
            _popupMessage = "Ошибка записи значения в память";
            ImGui::OpenPopup("ErrorPopup");
        }
    }
}

// Подгоняет высоту окна под количество строк.
//
// Считается ВНУТРИ кадра, когда ImGui уже знает реальную высоту строки и
// отступы: до создания контекста их пришлось бы угадывать, да ещё с
// поправкой на масштаб экрана.
void MainView::FitWindowHeightToContent()
{
    if (!_windowHandle) return;

    const ImGuiStyle& style = ImGui::GetStyle();

    const float rowPitch = Layout::RowHeight() + style.ItemSpacing.y;
    const float rows = static_cast<float>(_options.size() + _valueFields.size());

    // Шапка таблицы с разделителем, подвал в две строки и отступы сверху
    // и снизу — всё, что есть в окне помимо самих строк.
    const float header = ImGui::GetTextLineHeightWithSpacing() + style.ItemSpacing.y * 2.0f;
    const float footer = ImGui::GetTextLineHeightWithSpacing() * 2.0f + style.ItemSpacing.y * 3.0f;

    const float content = TITLE_BAR_HEIGHT + 14.0f + header + rowPitch * rows + footer
                        + style.WindowPadding.y * 2.0f;

    // Потолок — чтобы окно не выросло во весь экран на полусотне читов:
    // дальше уже работает прокрутка.
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    const int maxHeight = static_cast<int>((work.bottom - work.top) * 0.85f);

    int desired = static_cast<int>(content + 0.5f);
    desired = Utils::Clamp(desired, static_cast<int>(MIN_HEIGHT), maxHeight);

    if (desired == _fittedHeight) return;
    _fittedHeight = desired;

    RECT current{};
    if (!GetWindowRect(_windowHandle, &current)) return;

    // Окно растёт вниз от своей верхней кромки, поэтому при большом списке
    // оно уехало бы за нижний край экрана. Если не помещается — поднимаем.
    int top = current.top;
    if (top + desired > work.bottom)
    {
        top = work.bottom - desired;
    }
    if (top < work.top)
    {
        top = work.top;
    }

    const UINT flags = (top == current.top)
        ? (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)
        : (SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(_windowHandle, nullptr, current.left, top,
                 current.right - current.left, desired,
                 flags);
}

void MainView::RenderProcessInfo()
{
    std::wstring processName = _process->GetProcessName();
    size_t dotPos = processName.find_last_of(L'.');
    if (dotPos != std::wstring::npos)
    {
        processName = processName.substr(0, dotPos);
    }

    const bool isRunning = _process->isProcessRunning();

    // Цвет переходит плавно, а не прыгает: запуск и закрытие игры перестают
    // выглядеть как дёрганье строки. Сглаживание то же, что у тумблера, —
    // не зависящее от частоты кадров.
    const float target = isRunning ? 1.0f : 0.0f;
    const float t = 1.0f - std::exp(-6.0f * ImGui::GetIO().DeltaTime);
    _runningFade += (target - _runningFade) * t;

    const ImVec4 idle(0.55f, 0.55f, 0.58f, 1.0f);
    const ImVec4 live(0.20f, 0.80f, 0.35f, 1.0f);
    const ImVec4 statusColor(
        idle.x + (live.x - idle.x) * _runningFade,
        idle.y + (live.y - idle.y) * _runningFade,
        idle.z + (live.z - idle.z) * _runningFade,
        1.0f);

    ImGui::TextColored(statusColor, "%s %s",
        Utils::WStringToUtf8(processName).c_str(), isRunning ? "is running" : "is not running");

    // PID Информация
    ImGui::TextColored(ImVec4(0.05f, 0.7f, 0.8f, 1.0f), "Process ID: %s", isRunning ? std::to_string(_process->GetProcessID()).c_str() : "N/A");

    ImGui::SameLine();
    RenderAuthorLink("By ShadowStormOne", "https://t.me/ShadowStormOne");
}

void MainView::HandleToggleInteraction(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
{
    // Delegate to the handler if set
    if (_toggleHandler)
    {
        _toggleHandler(toggleId, optionName, currentState, previousState);
    }
}

void MainView::RenderTitleBar()
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 winPos = ImGui::GetWindowPos();
    const float winWidth = ImGui::GetWindowWidth();

    // Фон полосы во всю ширину (рисуем напрямую, минуя отступы окна)
    draw->AddRectFilled(winPos,
                        ImVec2(winPos.x + winWidth, winPos.y + TITLE_BAR_HEIGHT),
                        IM_COL32(32, 34, 38, 255));

    // Название чита
    ImGui::SetCursorPos(ImVec2(12.0f, (TITLE_BAR_HEIGHT - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::TextUnformatted(_windowName);

    const float btnW = 46.0f;
    const float btnH = TITLE_BAR_HEIGHT;
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
    const ImU32 glyph = IM_COL32(230, 230, 230, 255);
    const float cy = winPos.y + btnH * 0.5f;

    const float mcx = winPos.x + minimizeX + btnW * 0.5f;
    draw->AddLine(ImVec2(mcx - 5.0f, cy), ImVec2(mcx + 5.0f, cy), glyph, 1.0f);

    const float ccx = winPos.x + closeX + btnW * 0.5f;
    draw->AddLine(ImVec2(ccx - 5.0f, cy - 5.0f), ImVec2(ccx + 5.0f, cy + 5.0f), glyph, 1.2f);
    draw->AddLine(ImVec2(ccx + 5.0f, cy - 5.0f), ImVec2(ccx - 5.0f, cy + 5.0f), glyph, 1.2f);
}

void MainView::Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
#ifdef _DEBUG
    ImGuiDebugConsoleActivation();
#endif // _DEBUG

    if (isActive() && _process)
    {
        // Одно окно ImGui на весь клиент: размером владеет Win32, поэтому
        // ручной размер и флаг NoResize больше не нужны.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("##MainWindow", nullptr, flags);
        ImGui::PopStyleVar();

        RenderTitleBar();

        // Содержимое — под полосой заголовка, с запасом по вертикали,
        // чтобы первый переключатель не липнул к заголовку
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGui::SetCursorPos(ImVec2(style.WindowPadding.x, TITLE_BAR_HEIGHT + 14.0f));

        RenderTableHeader();

        // Список читов и поля ввода — в прокручиваемой области.
        //
        // Раньше всё рисовалось сплошняком, а подвал ставился абсолютно на
        // GetWindowHeight() - 65. При двух десятках читов список уходил за
        // нижний край окна, прокрутки не было, а строка состояния ложилась
        // ПОВЕРХ строк списка.
        //
        // Нулевые отступы у дочерней области не случайны: так её содержимое
        // считает координаты от тех же краёв, что и шапка с подвалом, и
        // колонки не разъезжаются между ними.
        const float footerHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f
                                 + style.ItemSpacing.y * 3.0f;
        const float listHeight = ImGui::GetContentRegionAvail().y - footerHeight;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::BeginChild("##content", ImVec2(0.0f, listHeight), ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoBackground);

        RenderToggles();
        RenderInputFields();

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::Separator();
        RenderProcessInfo();

        FitWindowHeightToContent();

        HandlePopupsWithIcons(successIcon, errorIcon);

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
            && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
            && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
        {
            // ImGui на нажатии захватывает мышь — отпускаем, иначе система
            // не начнёт свой цикл перетаскивания.
            ::ReleaseCapture();
            ::SendMessageW(_windowHandle, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        ImGui::End();
    }
}

/*
            ImGui::Text("Integer Input:");
            if (ImGui::InputInt("##IntInput", &intInput))
            {
                if (intInput < 0) intInput = 0; // Ограничение на отрицательные числа
            }

            ImGui::Text("Float Input:");
            if (ImGui::InputText("##FloatInput", inputBufferFloat, IM_ARRAYSIZE(inputBufferFloat), ImGuiInputTextFlags_CharsDecimal))
            {
                try
                {
                    floatInput = std::stof(inputBufferFloat);
                }
                catch (const std::exception&)
                {
                    floatInput = 0.0f; // Установить значение по умолчанию при ошибке
                }
            }

            ImGui::Text("Double Input:");
            if (ImGui::InputText("##DoubleInput", inputBufferDouble, IM_ARRAYSIZE(inputBufferDouble), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific))
            {
                try
                {
                    doubleInput = std::stod(inputBufferDouble);
                }
                catch (const std::exception&)
                {
                    doubleInput = 0.0; // Установить значение по умолчанию при ошибке
                }
            }
*/
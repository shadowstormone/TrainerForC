#include "ui/MainView.h"

#include <climits>  // Для INT_MAX

#include "core/MemoryAccess.h"
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
                          const std::unordered_map<std::string, FunctionOffset>& offsets,
                          const std::vector<CheatOption*>& options)
{
    _process = process;
    _offsetFunctions = offsets;
    _options = options;

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

namespace
{
    // Убирает с конца один символ UTF-8 целиком, а не байт: иначе
    // кириллица распадается на мусор.
    void PopUtf8Char(std::string& text)
    {
        if (text.empty()) return;

        size_t i = text.size() - 1;
        while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) --i;
        text.erase(i);
    }

    // Укорачивает подпись многоточием, чтобы она не залезала на элемент
    // справа. Раньше длинное имя чита просто рисовалось поверх тумблера.
    std::string FitText(const std::string& text, float maxWidth)
    {
        if (maxWidth <= 0.0f) return std::string();
        if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) return text;

        std::string shortened = text;
        while (!shortened.empty()
               && ImGui::CalcTextSize((shortened + "...").c_str()).x > maxWidth)
        {
            PopUtf8Char(shortened);
        }

        return shortened + "...";
    }

    // X, с которого начинается колонка управляющих элементов: она прижата
    // к правому краю окна, поэтому не зависит от длины подписей.
    float ControlColumnX(float controlWidth)
    {
        return ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - controlWidth;
    }
}

void MainView::RenderToggles()
{
    for (CheatOption* option : _options)
    {
        std::string name = Utils::WStringToUtf8(option->GetDescription());

        // Используем уникальный идентификатор для каждого переключателя
        std::string toggleId = "##toggle_" + name;

        // Проверяем, запущен ли процесс игры
        bool isGameRunning = _process->GetProcessID() != 0;

        // Колонка переключателей прижата к правому краю окна, а подпись
        // обрезается по оставшемуся месту: длинное имя чита больше не
        // наезжает на тумблер.
        const float toggleX = ControlColumnX(UIControls::Constants::TOGGLE_WIDTH);
        const std::string label = FitText(name, toggleX - ImGui::GetCursorPosX() - 10.0f);

        ImGui::TextColored(option->IsEnabled() ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", label.c_str());

        // Полное имя — в подсказке, если подпись пришлось укоротить.
        if (label != name && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", name.c_str());
        }

        ImGui::SameLine(toggleX);

        // Делаем элементы неактивными если игра не запущена
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
    }
}


void MainView::RenderInputFields()
{
    for (const auto& [buttonName, functionOffset] : _offsetFunctions)
    {
        // Инициализация значения, если его нет
        if (_inputValues.find(buttonName) == _inputValues.end())
        {
            _inputValues[buttonName] = 1;
        }

        // Кнопка Write стоит в той же правой колонке, что и переключатели,
        // а поле ввода — вплотную слева от неё. Раньше и поле, и кнопка
        // сидели на жёстких отступах и разъезжались с остальной панелью.
        const float buttonWidth = ImGui::CalcTextSize("Write").x
                                + ImGui::GetStyle().FramePadding.x * 2.0f;
        const float buttonX = ControlColumnX(buttonWidth);
        const float fieldX  = buttonX - ImGui::GetStyle().ItemSpacing.x
                            - static_cast<float>(UIControls::Constants::INPUT_WIDTH);

        ImGui::Text("%s", FitText(buttonName, fieldX - ImGui::GetCursorPosX() - 10.0f).c_str());
        ImGui::SameLine(fieldX);

        // Подготовка буфера ввода
        char inputBuffer[32];
        bool isFieldEmpty = _inputValues[buttonName] == 1 && !_inputFieldFocused[buttonName];

        if (isFieldEmpty)
        {
            strcpy_s(inputBuffer, "1"); // Плейсхолдер
        }
        else
        {
            sprintf_s(inputBuffer, "%d", _inputValues[buttonName]);
        }

        ImGui::SetNextItemWidth(static_cast<float>(UIControls::Constants::INPUT_WIDTH));

        // Серый цвет для плейсхолдера
        if (isFieldEmpty)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        // Перед InputText проверяем, нужно ли очистить поле
        if (ImGui::IsItemClicked() && isFieldEmpty)
        {
            inputBuffer[0] = '\0';
        }

        if (ImGui::InputText(("##ValueInput_" + buttonName).c_str(),
            inputBuffer,
            IM_ARRAYSIZE(inputBuffer),
            ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll))
        {
            if (inputBuffer == NULL || strlen(inputBuffer) == 0)
            {
                // Оставляем поле пустым при редактировании
                _inputValues[buttonName] = 1;
            }
            else
            {
                long long newValue = atoll(inputBuffer);
                if (newValue <= 0)
                {
                    _inputValues[buttonName] = 1;
                }
                else if (newValue > INT_MAX)
                {
                    _inputValues[buttonName] = INT_MAX;
                }
                else
                {
                    _inputValues[buttonName] = static_cast<int>(newValue);
                }
            }
        }

        // Обработка фокуса
        if (ImGui::IsItemActivated())
        {
            _inputFieldFocused[buttonName] = true;
            // Очищаем поле при первом клике
            if (isFieldEmpty)
            {
                memset(inputBuffer, 0, sizeof(inputBuffer));
                ImGui::SetKeyboardFocusHere(-1);
            }
        }

        if (ImGui::IsItemDeactivated())
        {
            _inputFieldFocused[buttonName] = false;
            // Если поле пустое при потере фокуса, возвращаем 1
            if (strlen(inputBuffer) == 0)
            {
                _inputValues[buttonName] = 1;
            }
        }

        if (isFieldEmpty)
        {
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        // Кнопка записи
        if (ImGui::Button(("Write##" + buttonName).c_str()))
        {
            if (!_process->isProcessRunning() == true)
            {
#ifdef _DEBUG
                _popupType = "Error";
                _popupMessage = "Процесс игры не запущен!";
                ImGui::OpenPopup("ErrorPopup");
#endif // _DEBUG
            }
            else
            {
                // Пишем напрямую через MemoryAccess: отдельный патч ради
                // одноразовой записи из поля ввода не нужен.
                MemoryAccess mem(_process->GetProcessID());
                int valueToWrite = _inputValues[buttonName];
                SIZE_T written = 0;

                const uintptr_t address = mem.IsValid()
                    ? mem.ResolveChain(mem.ProcessBase(), functionOffset.offsets)
                    : 0;

                const bool ok = address != 0
                    && WriteProcessMemory(mem.Handle(), reinterpret_cast<LPVOID>(address),
                                          &valueToWrite, sizeof(valueToWrite), &written)
                    && written == sizeof(valueToWrite);

                if (ok)
                {
                    AudioService::Instance().Play(Sound::CheatEnabled);
#ifdef _DEBUG
                    _popupType = "Success";
                    _popupMessage = "Значение " + std::to_string(_inputValues[buttonName]) + " успешно записанно в память!";
                    ImGui::OpenPopup("SuccessPopup");
#endif // _DEBUG
                }
                else
                {
#ifdef _DEBUG
                    _popupType = "Error";
                    _popupMessage = "Ошибка записи значения в память!";
                    ImGui::OpenPopup("ErrorPopup");
#endif // _DEBUG
                }
            }
        }
    }
}

void MainView::RenderProcessInfo()
{
    std::wstring processName = _process->GetProcessName();
    size_t dotPos = processName.find_last_of(L'.');
    if (dotPos != std::wstring::npos)
    {
        processName = processName.substr(0, dotPos);
    }

    bool isRunning = _process->isProcessRunning();
    ImGui::TextColored(isRunning ? ImVec4(0.1f, 0.7f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s %s",
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

        RenderToggles();
        ImGui::Separator();
        RenderInputFields();
        ImGui::Separator();

        // Переместить курсор в нижнюю часть окна
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65); // Чем больше цифра, тем выше от низа
        RenderProcessInfo();

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
#include <climits>  // Для INT_MAX
#include "ui/Drawing.h"
#include "patches/WriteAddressPatch.h"
#include "resource.h"
#include "platform/Utils.h"
#include "ui/UIControls.h"
#include "platform/VKeys.h"

// ---------------- Static Member Initialization ----------------
LPCSTR Drawing::lpWindowName = "Test Trainer (+1)";
ImVec2 Drawing::vWindowSize = { WIDTH, HEIGHT };
ImGuiWindowFlags Drawing::WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavInputs;
bool Drawing::bDraw = true;
Cheat* Drawing::_cheatProcGame = nullptr;

std::unordered_map<std::string, FunctionOffset> Drawing::OffsetFunctions = {};
std::vector<uintptr_t> Drawing::Offsets = {};
std::unordered_map<std::string, bool> Drawing::toggleStatesFunction = {};
std::map<std::string, int> Drawing::inputValues = {};
std::map<std::string, bool> Drawing::inputFieldFocused = {};

// ---------------- Global Variables ----------------
namespace
{
    // Состояние всплывающих уведомлений — только для этого файла.
    std::string popupMessage;
    std::string popupType; // "Error" или "Success"

    // Окно, которым управляют кнопки заголовка.
    HWND g_windowHandle = nullptr;

    // Левая граница блока кнопок заголовка (в клиентских координатах).
    // Обновляется каждый кадр в RenderTitleBar и используется хиттестом.
    float g_titleButtonsMinX = 0.0f;
}

void Drawing::SetWindowHandle(HWND hWnd)
{
    g_windowHandle = hWnd;
}

bool Drawing::IsCaptionPoint(POINT clientPoint)
{
    if (clientPoint.y < 0 || clientPoint.y >= static_cast<LONG>(TITLE_BAR_HEIGHT)) return false;

    // Над кнопками — не заголовок, иначе клики уйдут в перетаскивание.
    return static_cast<float>(clientPoint.x) < g_titleButtonsMinX;
}

bool showConsole = false;
bool isKeyHold = false;
std::vector<CheatOption*> existingVector;
std::vector<CheatOption*>& cheatOptionsFn = existingVector;
Cheat* procGameCheat = nullptr;
std::function<void(const std::string&, const std::string&, bool, bool)> Drawing::_toggleHandler = nullptr;

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

// ---------------- Drawing Class Methods ----------------
void Drawing::Initialize(Cheat* ClassCheatProcGame)
{
    _cheatProcGame = ClassCheatProcGame;
}

void Drawing::Initialize(Cheat* ClassCheatProcGame, const std::vector<uintptr_t>& offsets)
{
    _cheatProcGame = ClassCheatProcGame;
    Offsets = offsets;
}

void Drawing::Initialize(Cheat* ClassCheatProcGame, const std::unordered_map<std::string, FunctionOffset>& offsets)
{
    _cheatProcGame = ClassCheatProcGame;
    OffsetFunctions = offsets;
}

void Drawing::Initialize(Cheat* ClassCheatProcGame, const std::unordered_map<std::string, FunctionOffset>& offsets, const std::vector<CheatOption*>& cheatOptions)
{
    _cheatProcGame = ClassCheatProcGame;
    OffsetFunctions = offsets;
    cheatOptionsFn = cheatOptions;
    procGameCheat = ClassCheatProcGame;
}

void Drawing::Active()
{
    bDraw = true;
}

bool Drawing::isActive()
{
    return bDraw == true;
}

void Drawing::HandlePopupsWithIcons(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
    // Высота строки текста
    float textHeight = ImGui::GetTextLineHeightWithSpacing();

    if (popupType == "Error" && ImGui::BeginPopup("ErrorPopup"))
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
        ImGui::Text("%s", popupMessage.c_str());
        if (ImGui::Button("OK"))
        {
            ImGui::CloseCurrentPopup();
            popupType = ""; // Сброс типа Popup
        }
        ImGui::EndPopup();
    }

    if (popupType == "Success" && ImGui::BeginPopup("SuccessPopup"))
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
        ImGui::Text("%s", popupMessage.c_str());
        if (ImGui::Button("OK"))
        {
            ImGui::CloseCurrentPopup();
            popupType = ""; // Сброс типа Popup
        }
        ImGui::EndPopup();
    }
}

void Drawing::RenderAuthorLink(const char* text, const char* url, float offsetRight)
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

void Drawing::RenderToggles()
{
    for (CheatOption* option : cheatOptionsFn)
    {
        std::string name = Utils::WStringToUtf8(option->GetDescription());

        // Используем уникальный идентификатор для каждого переключателя
        std::string toggleId = "##toggle_" + name;

        // Проверяем, запущен ли процесс игры
        bool isGameRunning = _cheatProcGame->GetProcessID() != 0;

        // Отображение текста с цветом
        ImGui::TextColored(option->IsEnabled() ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", name.c_str());

        // Позиционирование переключателя
        ImGui::SetNextItemWidth(UIControls::Constants::TEXT_WIDTH);

        // Отображение переключателя на той же линии
        ImGui::SameLine(UIControls::Constants::TEXT_WIDTH + 10.0f);

        // Делаем элементы неактивными если игра не запущена
        if (!isGameRunning)
        {
            ImGui::BeginDisabled();
        }

        // Проверка на существование состояния в карте
        if (toggleStatesFunction.find(toggleId) == toggleStatesFunction.end())
        {
            toggleStatesFunction[toggleId] = option->IsEnabled(); // Инициализируем состояние переключателя
        }

        // Сохраняем предыдущее состояние переключателя
        bool previousState = toggleStatesFunction[toggleId];

        if (UIControls::AnimatedToggleSwitch(toggleId.c_str(), &toggleStatesFunction[toggleId]))
        {
            HandleToggleInteraction(toggleId, name, toggleStatesFunction[toggleId], previousState);
        }

        if (!isGameRunning)
        {
            ImGui::EndDisabled();
        }
    }
}

void Drawing::RenderInputFields()
{
    for (const auto& [buttonName, functionOffset] : OffsetFunctions)
    {
        // Инициализация значения, если его нет
        if (inputValues.find(buttonName) == inputValues.end())
        {
            inputValues[buttonName] = 1;
        }

        // Установка фиксированной ширины для названия
        ImGui::SetNextItemWidth(UIControls::Constants::LABEL_WIDTH);
        ImGui::Text("%s", buttonName.c_str());
        ImGui::SameLine(UIControls::Constants::LABEL_WIDTH + 10.0f);

        // Подготовка буфера ввода
        char inputBuffer[32];
        bool isFieldEmpty = inputValues[buttonName] == 1 && !inputFieldFocused[buttonName];

        if (isFieldEmpty)
        {
            strcpy_s(inputBuffer, "1"); // Плейсхолдер
        }
        else
        {
            sprintf_s(inputBuffer, "%d", inputValues[buttonName]);
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
                inputValues[buttonName] = 1;
            }
            else
            {
                long long newValue = atoll(inputBuffer);
                if (newValue <= 0)
                {
                    inputValues[buttonName] = 1;
                }
                else if (newValue > INT_MAX)
                {
                    inputValues[buttonName] = INT_MAX;
                }
                else
                {
                    inputValues[buttonName] = static_cast<int>(newValue);
                }
            }
        }

        // Обработка фокуса
        if (ImGui::IsItemActivated())
        {
            inputFieldFocused[buttonName] = true;
            // Очищаем поле при первом клике
            if (isFieldEmpty)
            {
                memset(inputBuffer, 0, sizeof(inputBuffer));
                ImGui::SetKeyboardFocusHere(-1);
            }
        }

        if (ImGui::IsItemDeactivated())
        {
            inputFieldFocused[buttonName] = false;
            // Если поле пустое при потере фокуса, возвращаем 1
            if (strlen(inputBuffer) == 0)
            {
                inputValues[buttonName] = 1;
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
            if (!_cheatProcGame->isProcessRunning() == true)
            {
#ifdef _DEBUG
                popupType = "Error";
                popupMessage = "Процесс игры не запущен!";
                ImGui::OpenPopup("ErrorPopup");
#endif // _DEBUG
            }
            else
            {
                WriteAddressPatch writer;
                LPCWSTR procName = _cheatProcGame->GetProcessName();
                if (writer.WriteValueMemory(procName, functionOffset.offsets, inputValues[buttonName]))
                {
                    std::thread([]() { PlaySound(MAKEINTRESOURCE(IDR_WAVE1), NULL, SND_RESOURCE | SND_ASYNC); }).detach();
#ifdef _DEBUG
                    popupType = "Success";
                    popupMessage = "Значение " + std::to_string(inputValues[buttonName]) + " успешно записанно в память!";
                    ImGui::OpenPopup("SuccessPopup");
#endif // _DEBUG
                }
                else
                {
#ifdef _DEBUG
                    popupType = "Error";
                    popupMessage = "Ошибка записи значения в память!";
                    ImGui::OpenPopup("ErrorPopup");
#endif // _DEBUG
                }
            }
        }
    }
}

void Drawing::RenderProcessInfo()
{
    std::wstring processName = _cheatProcGame->GetProcessName();
    size_t dotPos = processName.find_last_of(L'.');
    if (dotPos != std::wstring::npos)
    {
        processName = processName.substr(0, dotPos);
    }

    bool isRunning = _cheatProcGame->isProcessRunning();
    ImGui::TextColored(isRunning ? ImVec4(0.1f, 0.7f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s %s",
        Utils::WStringToUtf8(processName).c_str(), isRunning ? "is running" : "is not running");

    // PID Информация
    ImGui::TextColored(ImVec4(0.05f, 0.7f, 0.8f, 1.0f), "Process ID: %s", isRunning ? std::to_string(_cheatProcGame->GetProcessID()).c_str() : "N/A");

    ImGui::SameLine();
    RenderAuthorLink("By ShadowStormOne", "https://t.me/ShadowStormOne");
}

void Drawing::HandleToggleInteraction(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
{
    // Delegate to the handler if set
    if (_toggleHandler)
    {
        _toggleHandler(toggleId, optionName, currentState, previousState);
    }
}

void Drawing::RenderTitleBar()
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
    ImGui::TextUnformatted(lpWindowName);

    const float btnW = 46.0f;
    const float btnH = TITLE_BAR_HEIGHT;
    const float minimizeX = winWidth - btnW * 2.0f;
    const float closeX = winWidth - btnW;

    // Запоминаем для хиттеста: левее этой границы — перетаскивание окна
    g_titleButtonsMinX = minimizeX;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 40));

    // Свернуть
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 25));
    ImGui::SetCursorPos(ImVec2(minimizeX, 0.0f));
    if (ImGui::Button("##minimize", ImVec2(btnW, btnH)) && g_windowHandle)
    {
        ::ShowWindow(g_windowHandle, SW_MINIMIZE);
    }
    ImGui::PopStyleColor();

    // Закрыть
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(196, 43, 28, 255));
    ImGui::SetCursorPos(ImVec2(closeX, 0.0f));
    if (ImGui::Button("##close", ImVec2(btnW, btnH)) && g_windowHandle)
    {
        ::PostMessageW(g_windowHandle, WM_CLOSE, 0, 0);
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

void Drawing::Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
#ifdef _DEBUG
    ImGuiDebugConsoleActivation();
#endif // _DEBUG

    if (isActive() && _cheatProcGame)
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

        // Содержимое — под полосой заголовка, с обычными отступами окна
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGui::SetCursorPos(ImVec2(style.WindowPadding.x, TITLE_BAR_HEIGHT + style.WindowPadding.y));

        RenderToggles();
        ImGui::Separator();
        RenderInputFields();
        ImGui::Separator();

        // Переместить курсор в нижнюю часть окна
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65); // Чем больше цифра, тем выше от низа
        RenderProcessInfo();

        HandlePopupsWithIcons(successIcon, errorIcon);

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
#include <climits> // Для INT_MAX
#include "Drawing.h"
#include "WriteAddressPatch.h"
#include "resource.h"
#include "Utils.h"
#include "UIControls.h"

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

//int Drawing::intUserInput = 1;
//float Drawing::floatUserInput = 0.0f;
//double Drawing::doubleUserInput = 0.0;

// ---------------- Global Variables ----------------
Console console;
bool showConsole = false;
bool isKeyHold = false;
std::vector<CheatOption*> existingVector;
std::vector<CheatOption*>& cheatOptionsFn = existingVector;
Cheat* procGameCheat = nullptr;

static void ProcessInput()
{
    // Проверяем состояние клавиши VK_OEM_3
    if (GetAsyncKeyState(VK_OEM_3) & 0x8000) // Клавиша нажата
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

void Drawing::RenderToggles()
{
    for (const auto& pair : _cheatProcGame->GetCheatOptionState())
    {
        // Отображение текста с цветом
        ImGui::TextColored(pair.second ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", Utils::WStringToUtf8(pair.first).c_str());

        // Позиционирование переключателя
        ImGui::SetNextItemWidth(UIControls::Constants::TEXT_WIDTH); // Устанавливаем ширину для переключателя

        // Отображение переключателя на той же линии
        ImGui::SameLine(UIControls::Constants::TEXT_WIDTH + 10.0f); // Отступ между текстом и переключателем

        // Проверяем, запущен ли процесс игры
        bool isGameRunning = _cheatProcGame->GetProcessID() != 0; // или другая проверка

        // Делаем элементы неактивными если игра не запущена
        if (!isGameRunning)
        {
            ImGui::BeginDisabled();
        }

        // Используем уникальный идентификатор для каждого переключателя
        std::string toggleId = "##toggle_" + Utils::WStringToUtf8(pair.first);

        // Проверка на существование состояния в карте
        if (toggleStatesFunction.find(toggleId) == toggleStatesFunction.end())
        {
            toggleStatesFunction[toggleId] = pair.second; // Инициализируем состояние переключателя
        }

        // Сохраняем предыдущее состояние переключателя
        bool previousState = toggleStatesFunction[toggleId];

        if (UIControls::AnimatedToggleSwitch(toggleId.c_str(), &toggleStatesFunction[toggleId]))
        {
            HandleToggleInteraction(toggleId, Utils::WStringToUtf8(pair.first), toggleStatesFunction[toggleId], previousState);
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
            if (strlen(inputBuffer) == 0)
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
                    popupMessage = "Successfully write value " + std::to_string(inputValues[buttonName]) + " to memory!";
                    ImGui::OpenPopup("SuccessPopup");
#endif // _DEBUG
                }
                else
                {
#ifdef _DEBUG
                    popupType = "Error";
                    popupMessage = "Failed to write value to memory!";
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
}

void Drawing::HandleToggleInteraction(const std::string& toggleId, const std::string& optionName, bool currentState, bool previousState)
{
    auto it = cheatOptionsFn.begin();
    CheatOption* n1fn = it[0];
    CheatOption* n2fn = it[1];

    if (currentState && !previousState)
    {
        switch (Utils::hash(optionName.c_str()))
        {
        case Utils::hash("[Numpad 1] - Cheat Test"):
            console.addLog("INFO", "Переключатель Опция1 активирован");
            n1fn->pEnable(_cheatProcGame->GetProcessID());
            n1fn->IsEnabled(true);
            break;

        case Utils::hash("[Numpad 2] - Set 9999 HP"):
            console.addLog("INFO", "Переключатель Опция2 активирован");
            n2fn->pEnable(_cheatProcGame->GetProcessID());
            break;
        }
    }
    else if (!currentState && previousState)
    {
        switch (Utils::hash(optionName.c_str()))
        {
        case Utils::hash("[Numpad 1] - Cheat Test"):
            console.addLog("INFO", "Опция1 выключена");
            n1fn->pDisable(_cheatProcGame->GetProcessID());
            n1fn->IsEnabled(false);
            break;

        case Utils::hash("[Numpad 2] - Set 9999 HP"):
            console.addLog("INFO", "Опция2 выключена");
            break;
        }
    }
}

void Drawing::Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
#ifdef _DEBUG
    ProcessInput();
#endif // _DEBUG

    if (isActive() && _cheatProcGame)
    {
        ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
        ImGui::Begin(lpWindowName, &bDraw, WindowFlags);

        RenderToggles();
        ImGui::Separator();
        RenderInputFields();
        ImGui::Separator();
        // Переместить курсор в нижнюю часть окна
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65); // 53 - расстояние от низа окна(Чем больше цифра тем выше от низа)
        RenderProcessInfo();

        //DisplayPopup(successIcon, errorIcon);
        HandlePopupsWithIcons(successIcon, errorIcon);

        ImGui::End();
    }
}

//void Drawing::Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
//{
//    //static int inputWidth = 158;        // Длина поля ввода, можно менять
//    //static int minValue = 1;            // Минимальное значение, запрещаем ввод 0
//    //const float labelWidth = 150.0f;    // Фиксированная ширина для текста
//    //const float textWidth = 315.0f;     // Фиксированая ширена для названия функций из main в цикле for
//
//#ifdef _DEBUG
//    ProcessInput();
//#endif // _DEBUG
//
//    if (isActive() && _cheatProcGame)
//    {
//        ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
//        ImGui::Begin(lpWindowName, &bDraw, WindowFlags);
//        {
//            // Рисуем функции из main
//            for (const auto& pair : _cheatProcGame->GetCheatOptionState())
//            {
//                // Отображение текста с цветом
//                ImGui::TextColored(pair.second ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", WStringToUtf8(pair.first).c_str());
//
//                // Позиционирование переключателя
//                ImGui::SetNextItemWidth(::TEXT_WIDTH); // Устанавливаем ширину для переключателя
//
//                // Отображение переключателя на той же линии
//                ImGui::SameLine(::TEXT_WIDTH + 10.0f); // Отступ между текстом и переключателем
//
//                // Проверяем, запущен ли процесс игры
//                bool isGameRunning = _cheatProcGame->GetProcessID() != 0; // или другая проверка
//
//                // Делаем элементы неактивными если игра не запущена
//                if (!isGameRunning)
//                    ImGui::BeginDisabled();
//
//                // Используем уникальный идентификатор для каждого переключателя
//                std::string toggleId = "##toggle_" + WStringToUtf8(pair.first);
//
//                // Проверка на существование состояния в карте
//                if (toggleStatesFunction.find(toggleId) == toggleStatesFunction.end())
//                {
//                    toggleStatesFunction[toggleId] = pair.second; // Инициализируем состояние переключателя
//                }
//
//                // Сохраняем предыдущее состояние переключателя
//                bool previousState = toggleStatesFunction[toggleId];
//
//                if (AnimatedToggleSwitch(toggleId.c_str(), &toggleStatesFunction[toggleId]))
//                {
//                    std::string optionName = WStringToUtf8(pair.first);
//                    bool currentState = toggleStatesFunction[toggleId];
//                    std::vector<CheatOption*> foundOptions;
//
//                    auto it = cheatOptionsFn.begin();
//                    CheatOption* n1fn = it[0];
//                    CheatOption* n2fn = it[1];
//
//                    if (currentState && !previousState)
//                    {
//                        switch (hash(optionName.c_str()))
//                        {
//                        case hash("[Numpad 1] - Cheat Test"):
//                            ::console.addLog("INFO", "Переключатель Опция1 активирован");
//                            n1fn->pEnable(_cheatProcGame->GetProcessID());
//                            break;
//
//                        case hash("[Numpad 2] - Set 9999 HP"):
//                            ::console.addLog("INFO", "Переключатель Опция2 активирован");
//                            n2fn->pEnable(_cheatProcGame->GetProcessID());
//                            break;
//                        }
//                    }
//                    else if (!currentState && previousState)
//                    {
//                        switch (hash(optionName.c_str()))
//                        {
//                        case hash("[Numpad 1] - Cheat Test"):
//                            ::console.addLog("INFO", "Опция1 выключена");
//                            n1fn->pDisable(_cheatProcGame->GetProcessID());
//                            break;
//
//                        case hash("[Numpad 2] - Set 9999 HP"):
//                            ::console.addLog("INFO", "Опция2 выключена");
//                            break;
//                        }
//                    }
//                }
//
//                if (!isGameRunning)
//                    ImGui::EndDisabled();
//            }
//
//            ImGui::Separator();
//
//            // Пользовательский ввод и кнопки для ассоциации офсетов
//            for (const auto& [buttonName, functionOffset] : OffsetFunctions)
//            {
//                // Инициализация значения, если его нет
//                if (inputValues.find(buttonName) == inputValues.end()) {
//                    inputValues[buttonName] = 1;
//                }
//
//                // Установка фиксированной ширины для названия
//                ImGui::SetNextItemWidth(::LABEL_WIDTH);
//                ImGui::Text("%s", buttonName.c_str());
//                ImGui::SameLine(::LABEL_WIDTH + 10.0f);
//
//                // Подготовка буфера ввода
//                char inputBuffer[32];
//                bool isFieldEmpty = inputValues[buttonName] == 1 && !inputFieldFocused[buttonName];
//
//                if (isFieldEmpty) {
//                    strcpy_s(inputBuffer, "1"); // Плейсхолдер
//                }
//                else {
//                    sprintf_s(inputBuffer, "%d", inputValues[buttonName]);
//                }
//
//                ImGui::SetNextItemWidth(static_cast<float>(::INPUT_WIDTH));
//
//                // Серый цвет для плейсхолдера
//                if (isFieldEmpty) {
//                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
//                }
//
//                // Перед InputText проверяем, нужно ли очистить поле
//                if (ImGui::IsItemClicked() && isFieldEmpty) {
//                    inputBuffer[0] = '\0';
//                }
//
//                if (ImGui::InputText(("##ValueInput_" + buttonName).c_str(),
//                    inputBuffer,
//                    IM_ARRAYSIZE(inputBuffer),
//                    ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll))
//                {
//                    if (strlen(inputBuffer) == 0) {
//                        // Оставляем поле пустым при редактировании
//                        inputValues[buttonName] = 1;
//                    }
//                    else {
//                        long long newValue = atoll(inputBuffer);
//                        if (newValue <= 0) {
//                            inputValues[buttonName] = 1;
//                        }
//                        else if (newValue > INT_MAX) {
//                            inputValues[buttonName] = INT_MAX;
//                        }
//                        else {
//                            inputValues[buttonName] = static_cast<int>(newValue);
//                        }
//                    }
//                }
//
//                // Обработка фокуса
//                if (ImGui::IsItemActivated()) {
//                    inputFieldFocused[buttonName] = true;
//                    // Очищаем поле при первом клике
//                    if (isFieldEmpty) {
//                        memset(inputBuffer, 0, sizeof(inputBuffer));
//                        ImGui::SetKeyboardFocusHere(-1);
//                    }
//                }
//
//                if (ImGui::IsItemDeactivated()) {
//                    inputFieldFocused[buttonName] = false;
//                    // Если поле пустое при потере фокуса, возвращаем 1
//                    if (strlen(inputBuffer) == 0) {
//                        inputValues[buttonName] = 1;
//                    }
//                }
//
//                if (isFieldEmpty) {
//                    ImGui::PopStyleColor();
//                }
//
//                ImGui::SameLine();
//
//                // Кнопка записи
//                if (ImGui::Button(("Write##" + buttonName).c_str()))
//                {
//                    if (!_cheatProcGame->isProcessRunning() == true)
//                    {
//#ifdef _DEBUG
//                        popupType = "Error";
//                        popupMessage = "Процесс игры не запущен!";
//                        ImGui::OpenPopup("ErrorPopup");
//#endif // _DEBUG
//                    }
//                    else
//                    {
//                        WriteAddressPatch writer;
//                        LPCWSTR procName = _cheatProcGame->GetProcessName();
//                        if (writer.WriteValueMemory(procName, functionOffset.offsets, inputValues[buttonName]))
//                        {
//                            std::thread([]() { PlaySound(MAKEINTRESOURCE(IDR_WAVE1), NULL, SND_RESOURCE | SND_ASYNC); }).detach();
//#ifdef _DEBUG
//                            popupType = "Success";
//                            popupMessage = "Successfully write value " + std::to_string(inputValues[buttonName]) + " to memory!";
//                            ImGui::OpenPopup("SuccessPopup");
//#endif // _DEBUG
//                        }
//                        else
//                        {
//#ifdef _DEBUG
//                            popupType = "Error";
//                            popupMessage = "Failed to write value to memory!";
//                            ImGui::OpenPopup("ErrorPopup");
//#endif // _DEBUG
//                        }
//                    }
//                }
//            }
//            
//            ImGui::Separator();
//
//            // Переместить курсор в нижнюю часть окна
//            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65); // 53 - расстояние от низа окна(Чем больше цифра тем выше от низа)
//
//            // Информация о процессе
//            std::wstring processName = _cheatProcGame->GetProcessName();
//            size_t dotPos = processName.find_last_of(L'.');
//            if (dotPos != std::wstring::npos)
//            {
//                processName = processName.substr(0, dotPos);
//            }
//
//            bool isRunning = _cheatProcGame->isProcessRunning();
//            ImGui::TextColored(isRunning ? ImVec4(0.1f, 0.7f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s %s", 
//                WStringToUtf8(processName).c_str(), isRunning ? "is running" : "is not running");
//
//            // PID Информация
//            ImGui::TextColored(ImVec4(0.05f, 0.7f, 0.8f, 1.0f), "PID Process: %s", isRunning ? std::to_string(_cheatProcGame->GetProcessID()).c_str() : "N/A");
//        }
//
//        // Вызов централизованной обработки Popup
//        HandlePopupsWithIcons(successIcon, errorIcon);
//
//        ImGui::End();
//    }
//}

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
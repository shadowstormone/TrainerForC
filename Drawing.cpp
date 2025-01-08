#include <climits> // Для INT_MAX
#include "Drawing.h"
#include "WriteAdressNum.h"
#include "resource.h"

LPCSTR Drawing::lpWindowName = "Test Trainer (+1)";
ImVec2 Drawing::vWindowSize = { WIDTH, HEIGHT };
ImGuiWindowFlags Drawing::WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
bool Drawing::bDraw = true;
Cheat* Drawing::_cheatProcGame = nullptr;

std::unordered_map<std::string, FunctionOffset> Drawing::OffsetFunctions = {};
std::vector<uintptr_t> Drawing::Offsets = {};
int Drawing::intUserInput = 1;
float Drawing::floatUserInput = 0.0f;
double Drawing::doubleUserInput = 0.0;

std::string Drawing::WStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return {};

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        return {};
    }

    std::string utf8str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], size_needed, nullptr, nullptr);

    utf8str.pop_back();
    return utf8str;
}

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

void Drawing::Active()
{
    bDraw = true;
}

bool Drawing::isActive()
{
    return bDraw == true;
}

void Drawing::Draw(ID3D11ShaderResourceView* successIcon, ID3D11ShaderResourceView* errorIcon)
{
    static int inputWidth = 158;        // Длина поля ввода, можно менять
    static int minValue = 1;            // Минимальное значение, запрещаем ввод 0
    const float labelWidth = 150.0f;    // Фиксированная ширина для текста

    if (isActive() && _cheatProcGame)
    {
        ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
        ImGui::Begin(lpWindowName, &bDraw, WindowFlags);
        {
            // Рисуем функции из main
            for (const auto& pair : _cheatProcGame->GetCheatOptionState())
            {
                ImGui::TextColored(pair.second ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", WStringToUtf8(pair.first).c_str());
            }

            ImGui::Separator();

            // Пользовательский ввод и кнопки для ассоциации офсетов
            for (const auto& [buttonName, functionOffset] : OffsetFunctions)
            {
                // Установка фиксированной ширины для названия
                ImGui::SetNextItemWidth(labelWidth);
                ImGui::Text("%s", buttonName.c_str());
                ImGui::SameLine(labelWidth + 10.0f); // Отступ между текстом и полем ввода

                // Поле ввода значения
                char inputBuffer[32];
                sprintf_s(inputBuffer, "%d", intUserInput);

                ImGui::SetNextItemWidth(static_cast<float>(inputWidth));
                if (ImGui::InputText(("##ValueInput_" + buttonName).c_str(), inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_CharsDecimal))
                {
                    // Убедимся, что значение не превышает INT_MAX
                    long long newValue = atoll(inputBuffer); // Используем `long long` для безопасной проверки
                    if (newValue <= 0)
                    {
                        intUserInput = minValue; // Устанавливаем минимальное значение
                    }
                    else if (newValue > INT_MAX)
                    {
                        sprintf_s(inputBuffer, "%d", INT_MAX); // Ограничиваем строку значением INT_MAX
                        intUserInput = INT_MAX;
                    }
                    else
                    {
                        intUserInput = static_cast<int>(newValue); // Устанавливаем корректное значение
                    }
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
                        if (intUserInput <= 0) // Проверка перед записью
                        {
#ifdef _DEBUG
                            popupType = "Error";
                            popupMessage = "Cannot write value 0 to memory!";
                            ImGui::OpenPopup("ErrorPopup");
#endif // _DEBUG
                        }
                        else
                        {
                            WriteAddressPatch writer;
                            LPCWSTR procName = _cheatProcGame->GetProcessName();

                            if (writer.WriteValueMemory(procName, functionOffset.offsets, intUserInput))
                            {
                                std::thread([]() { PlaySound(MAKEINTRESOURCE(IDR_WAVE1), NULL, SND_RESOURCE | SND_ASYNC); }).detach();
#ifdef _DEBUG
                                popupType = "Success";
                                popupMessage = "Successfully write value " + std::to_string(intUserInput) + " to memory!";
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
            
            ImGui::Separator();

            // Переместить курсор в нижнюю часть окна
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65); // 53 - расстояние от низа окна(Чем больше цифра тем выше от низа)

            // Информация о процессе
            std::wstring processName = _cheatProcGame->GetProcessName();
            size_t dotPos = processName.find_last_of(L'.');
            if (dotPos != std::wstring::npos)
            {
                processName = processName.substr(0, dotPos);
            }

            bool isRunning = _cheatProcGame->isProcessRunning();
            ImGui::TextColored(isRunning ? ImVec4(0.1f, 0.7f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s %s", 
                WStringToUtf8(processName).c_str(), isRunning ? "is running" : "is not running");

            // PID Информация
            ImGui::TextColored(ImVec4(0.05f, 0.7f, 0.8f, 1.0f), "PID Process: %s", isRunning ? std::to_string(_cheatProcGame->GetProcessID()).c_str() : "N/A");
        }

        // Вызов централизованной обработки Popup
        HandlePopupsWithIcons(successIcon, errorIcon);

        ImGui::End();
    }
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
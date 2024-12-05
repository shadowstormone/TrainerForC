#include "Drawing.h"
#include <iostream>

LPCSTR Drawing::lpWindowName = "Test Trainer (+1)";
ImVec2 Drawing::vWindowSize = { WIDTH, HEIGHT };
ImGuiWindowFlags Drawing::WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
bool Drawing::bDraw = true;
Cheat* Drawing::_cheat = nullptr;

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

void Drawing::Initialize(Cheat* cheat)
{
    _cheat = cheat;
}

void Drawing::Active()
{
    bDraw = true;
}

bool Drawing::isActive()
{
    return bDraw == true;
}

void Drawing::Draw()
{
    if (isActive() && _cheat)
    {
        ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
        ImGui::Begin(lpWindowName, &bDraw, WindowFlags);
        {
            // Рисуем функции из main
            for (const auto& pair : _cheat->GetCheatOptionState())
            {
                ImGui::TextColored(pair.second ? ImVec4(0.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", WStringToUtf8(pair.first).c_str());
            }

            ImGui::Separator();

            // Переместить курсор в нижнюю часть окна
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 60); // 53 - расстояние от низа окна(Чем больше цифра тем выше от низа)

            // Информация о процессе
            std::wstring processName = _cheat->GetProcessName();
            size_t dotPos = processName.find_last_of(L'.');
            if (dotPos != std::wstring::npos)
            {
                processName = processName.substr(0, dotPos);
            }

            bool isRunning = _cheat->isProcessRunning();
            ImGui::TextColored(isRunning ? ImVec4(0.1f, 0.7f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s %s", 
                WStringToUtf8(processName).c_str(), isRunning ? "is running" : "is not running");

            // PID Информация
            ImGui::TextColored(ImVec4(0.05f, 0.7f, 0.8f, 1.0f), "PID Process: %d", isRunning ? _cheat->GetProcessID() : 0);
        }
        ImGui::End();
    }
}
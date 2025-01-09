#pragma once
#include "imgui.h"
#include <vector>
#include <string>

class Console
{
private:
    std::vector<std::string> items;
    char inputBuf[256];
    bool scrollToBottom;

public:
    Console()
    {
        inputBuf[0] = 0;
        scrollToBottom = true;
        items.push_back("Добро пожаловать в консоль разработчика");
    }

    void addLog(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        va_end(args);
        items.push_back(buf);
        scrollToBottom = true;
    }

    void draw(const char* title, bool* p_open = NULL)
    {
        ImGui::SetNextWindowSize(ImVec2(900, 800), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Кнопка очистки
        if (ImGui::Button("Clear"))
        {
            items.clear();
        }
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");

        ImGui::Separator();

        // Область вывода сообщений
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (copy)
        {
            ImGui::LogToClipboard();
        }

        for (const auto& item : items)
        {
            ImGui::TextUnformatted(item.c_str());
        }

        if (scrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom = false;
        }

        ImGui::EndChild();

        ImGui::Separator();

        // Поле ввода
        bool reclaimFocus = false;
        if (ImGui::InputText("Input", inputBuf, IM_ARRAYSIZE(inputBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (inputBuf[0] != 0)
            {
                addLog("> %s", inputBuf);
                // Здесь можно добавить обработку команд
                inputBuf[0] = 0;
                reclaimFocus = true;
            }
        }

        ImGui::SetItemDefaultFocus();
        if (reclaimFocus)
        {
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::End();
    }
};
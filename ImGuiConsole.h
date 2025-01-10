#pragma once
#include "imgui.h"
#include <vector>
#include <string>

extern Cheat* procGameCheat;

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
        items.push_back("[INFO] Добро пожаловать в консоль разработчика");
        items.push_back("Версия: 1.0 (Debug)");
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
                std::string command(inputBuf);
                addLog("> %s", inputBuf);

                // Обработка команд
                if (command == "!GetPID")
                {
                    DWORD pid = procGameCheat->GetProcessID();
                    if (pid != 0)
                    {
                        addLog("Process ID: %d", pid);
                    }
                    else
                    {
                        addLog("Процесс не запущен!");
                    }
                }
                else if (command == "!help" || command == "!Help")
                {
                    addLog("Доступные команды:");
                    addLog(" - !Help: Показать список команд.");
                    addLog(" - !clear: Отчистить консоль.");
                    addLog(" - !GetPID: Получить ID процесса.");
                }
                else if (command == "!clear"  || command == "!Сlear")
                {
                    items.clear();
                }
                else
                {
                    addLog("Команда не существует или не найдена!");
                }

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
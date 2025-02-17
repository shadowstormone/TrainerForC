#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>

extern Cheat* procGameCheat;

class Console
{
private:
    struct LogEntry
    {
        std::string timestamp;
        std::string type;
        std::string message;
        bool isUserInput;

        LogEntry() : timestamp(), type(), message(), isUserInput(false) {} // Инициализируем все поля
    };

    std::vector<LogEntry> items;
    std::vector<std::string> availableCommands;
    int historyPos;
    char inputBuf[256];
    bool scrollToBottom;

    std::string GetCurrentTimestamp()
    {
        std::time_t now = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        std::ostringstream oss;
        oss << std::put_time(&timeinfo, "%H:%M:%S");
        return oss.str();
    }

public:
    Console()
    {
        inputBuf[0] = 0;
        scrollToBottom = true;

        // Initialize available commands
        availableCommands = {
            "!help",
            "!Help",
            "!clear",
            "!Clear",
            "!GetPID"
        };

        addLog("INFO", "Добро пожаловать в консоль разработчика");
        addLog("INFO", "Версия: 1.0 (Debug)");
    }

    void addLog(const std::string& type, const std::string& message, bool isUserInput = false)
    {
        LogEntry entry;
        entry.timestamp = GetCurrentTimestamp();
        entry.type = type;
        entry.message = message;
        entry.isUserInput = isUserInput;
        items.push_back(entry);
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

        if (ImGui::Button("Clear")) items.clear();
        {
            ImGui::SameLine();
        }
        bool copy = ImGui::Button("Copy");

        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (copy)
        {
            ImGui::LogToClipboard();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        for (const auto& item : items)
        {
            if (item.isUserInput)
            {
                ImGui::TextUnformatted(item.message.c_str());
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s]", item.timestamp.c_str());
                ImGui::SameLine();

                ImVec4 typeColor;
                if (item.type == "INFO")
                {
                    typeColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                }
                else if (item.type == "ERROR")
                {
                    typeColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                }
                else if (item.type == "WARNING")
                {
                    typeColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                }
                else
                {
                    typeColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                }

                ImGui::TextColored(typeColor, "[%s]", item.type.c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(item.message.c_str());
            }
        }
        ImGui::PopStyleVar();

        if (scrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom = false;
        }
        ImGui::EndChild();
        ImGui::Separator();

        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;

        // Поле для ввода
        bool reclaimFocus = false;
        if (ImGui::InputText("##Input", inputBuf, IM_ARRAYSIZE(inputBuf), inputFlags))
        {
            if (inputBuf[0] != 0)
            {
                std::string command(inputBuf);
                addLog("USER", "> " + command, true);
                historyPos = -1;

                // Command processing
                if (command == "!GetPID")
                {
                    DWORD pid = ::procGameCheat->GetProcessID();
                    if (pid != 0)
                    {
                        addLog("INFO", "Process ID: " + std::to_string(pid));
                    }
                    else
                    {
                        addLog("ERROR", "Процесс не запущен!");
                    }
                }
                else if (command == "!help" || command == "!Help")
                {
                    addLog("INFO", "Доступные команды:");
                    addLog("INFO", " - !Help: Показать список команд.");
                    addLog("INFO", " - !clear: Отчистить консоль.");
                    addLog("INFO", " - !GetPID: Получить ID процесса.");
                }
                else if (command == "!clear" || command == "!Clear")
                {
                    items.clear();
                }
                else
                {
                    addLog("WARNING", "Команда не существует или не найдена!");
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
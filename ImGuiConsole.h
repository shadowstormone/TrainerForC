#pragma once
#include "imgui.h"
#include "Utils.h"
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <Windows.h>
#include <functional>
#include <format>

extern Cheat* procGameCheat;

/**
 * @class Console
 * @brief Встроенный отладочный консольный интерфейс на базе ImGui.
 *
 * Предназначен для ввода команд, просмотра логов и отладки в реальном времени.
 * Поддерживает: историю команд, автодополнение (Tab), цветные сообщения, прокрутку.
 */
class Console
{
private:
    /**
     * @brief Структура, представляющая одну запись в логе консоли.
     */
    struct LogEntry
    {
        std::string timestamp;      ///< Время записи в формате HH:MM:SS.
        std::string type;           ///< Тип сообщения (например, "INFO", "ERROR", "WARNING", "DEBUG").
        std::string message;        ///< Текст сообщения.
        bool isUserInput;           ///< Флаг: является ли запись вводом пользователя.

        /**
         * @brief Конструктор по умолчанию.
         */
        LogEntry() : timestamp(), type(), message(), isUserInput(false) {}
    };

    /**
     * @brief Структура, описывающая зарегистрированную команду.
     */
    struct Command
    {
        std::string name;           ///< Имя команды (без префикса '!').
        std::string description;    ///< Описание команды для справки.
        std::function<void(Console*, const std::vector<std::string>&)> callback; ///< Функция-обработчик команды.
        bool caseSensitive = false; ///< Флаг: учитывать ли регистр при сравнении имени команды.

        /**
         * @brief Конструктор по умолчанию.
         */
        Command() = default;

        /**
         * @brief Конструктор для инициализации команды.
         * @param n Имя команды.
         * @param desc Описание команды.
         * @param cb Функция-обработчик.
         * @param cs Флаг чувствительности к регистру (по умолчанию false).
         */
        Command(const std::string& n, const std::string& desc,
            const std::function<void(Console*, const std::vector<std::string>&)>& cb, bool cs = false)
            : name(n), description(desc), callback(cb), caseSensitive(cs) {}
    };

    /**
     * @brief Преобразует строку в нижний регистр.
     * @param str Входная строка.
     * @return Строка в нижнем регистре.
     */
    static std::string toLower(const std::string& str)
    {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    /**
     * @brief Добавляет новую команду в систему.
     * @param name Имя команды (без '!').
     * @param description Описание команды.
     * @param callback Функция, вызываемая при выполнении команды.
     * @param caseSensitive Флаг: учитывать ли регистр при вызове команды.
     */
    void AddCommand(const std::string& name, const std::string& description,
        const std::function<void(Console*, const std::vector<std::string>&)>& callback, bool caseSensitive = false)
    {
        std::string key = caseSensitive ? name : toLower(name);
        commandMap[key] = Command(name, description, callback, caseSensitive);
        availableCommands.push_back("!" + name); // для автодополнения
    }

    /**
     * @brief Разбирает строку ввода на аргументы, поддерживая кавычки для строк с пробелами.
     * @param input Входная строка.
     * @return Вектор аргументов.
     */
    static std::vector<std::string> ParseArgs(const std::string& input)
    {
        std::vector<std::string> args;
        std::stringstream ss(input);
        std::string part;
        while (ss >> std::quoted(part, '"'))  // поддержка "текст с пробелами"
        {
            args.push_back(part);
        }
        return args;
    }

    /**
     * @brief Возвращает текущее время в формате HH:MM:SS.
     * @return Строка с временной меткой.
     */
    std::string GetCurrentTimestamp()
    {
        std::time_t now = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        std::ostringstream oss;
        oss << std::put_time(&timeinfo, "%H:%M:%S");
        return oss.str();
    }

    /**
     * @brief Статическая заглушка для обратного вызова редактирования текста в ImGui.
     * @param data Данные ImGuiInputTextCallbackData.
     * @return Результат обратного вызова.
     */
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        Console* console = reinterpret_cast<Console*>(data->UserData);
        return console->TextEditCallback(data);
    }

    /**
     * @brief Обработчик обратного вызова для ввода текста (автодополнение и история).
     * @param data Данные ImGuiInputTextCallbackData.
     * @return 0 в случае успеха.
     */
    int TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        // Автодополнение по Tab
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
        {
            std::string prefix(data->Buf, data->BufTextLen);
            std::string match;
            int matches = 0;
            for (const auto& cmd : availableCommands)
            {
                if (cmd.rfind(prefix, 0) == 0) // начинается с prefix
                {
                    if (matches == 0)
                    {
                        match = cmd;
                    }
                    else
                    {
                        size_t i = 0;
                        while (i < match.size() && i < cmd.size() && match[i] == cmd[i])
                        {
                            ++i;
                        }
                        match = match.substr(0, i);
                    }
                    ++matches;
                }
            }
            if (matches == 1)
            {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, match.c_str());
            }
            else if (matches > 1 && !match.empty())
            {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, match.c_str());
            }
        }
        else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)    // История команд (↑ / ↓)
        {
            const int prevHistoryPos = historyPos;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (historyPos == -1)
                {
                    if (!commandHistory.empty())
                    {
                        historyPos = static_cast<int>(commandHistory.size()) - 1;
                    }
                }
                else if (historyPos > 0)
                {
                    historyPos--;
                }
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (historyPos != -1)
                {
                    if (++historyPos >= static_cast<int>(commandHistory.size()))
                    {
                        historyPos = -1;
                    }
                }
            }
            if (prevHistoryPos != historyPos)
            {
                const char* historyStr = (historyPos >= 0) ? commandHistory[historyPos].c_str() : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, historyStr);
            }
        }
        return 0;
    }

    /**
     * @brief Регистрирует встроенные команды консоли.
     */
    void RegisterBuildInCommands()
    {
        AddCommand("help", "Показать список всех команд", [](Console* console, const std::vector<std::string>& args)
            {
            console->addLog("INFO", "Доступные команды:");

            for (const auto& [key, cmd] : console->commandMap)
            {
                std::string line = " - !" + cmd.name + ": " + cmd.description;
                console->addLog("INFO", line);
            }
            });

        AddCommand("clear", "Очистить консоль", [](Console* console, const std::vector<std::string>& args)
            {
                console->items.clear();
            });

        AddCommand("GetPID", "Получить ID процесса", [](Console* console, const std::vector<std::string>& args)
            {
                DWORD pid = ::procGameCheat->GetProcessID();

                if (pid != 0)
                {
                    console->addLog("INFO", "Process ID: " + std::to_string(pid));
                }
                else
                {
                    console->addLog("ERROR", "Процесс не запущен!");
                }
            });

        AddCommand("status", "Показать состояние процесса", [](Console* console, const std::vector<std::string>& args)
            {
                LPCWSTR rawName = ::procGameCheat->GetProcessName();
                std::string procName = Utils::WStringToUtf8(rawName);
                bool running = ::procGameCheat->isProcessRunning();
                DWORD pid = ::procGameCheat->GetProcessID();

                console->addLog("INFO", "Имя процесса: " + procName);
                console->addLog("INFO", "Запущен: " + std::string(running ? "Да" : "Нет"));

                if (running && pid != 0)
                {
                    console->addLog("INFO", "Process ID: " + std::to_string(pid));
                }
                else
                {
                    console->addLog("WARNING", "Процесс не найден. Запустите игру.");
                }
            });

        AddCommand("echo", "Вывести текст", [](Console* console, const std::vector<std::string>& args)
            {
                if (args.size() < 2)
                {
                    console->addLog("INFO", "Использование: !echo <текст>");
                    return;
                }

                std::string text = args[1];

                for (size_t i = 2; i < args.size(); ++i)
                {
                    text += " " + args[i];
                }
                console->addLog("INFO", text);
            });

    }

    std::map<std::string, Command> commandMap;       ///< Карта команд: ключ -> команда (для быстрого поиска).
    std::vector<LogEntry> items;                     ///< Список всех записей в консоли.
    std::vector<std::string> availableCommands;      ///< Список команд для автодополнения (с префиксом '!').
    std::vector<std::string> commandHistory;         ///< История введённых команд.
    int historyPos;                                  ///< Текущая позиция в истории (для навигации стрелками).
    char inputBuf[256];                              ///< Буфер ввода команды.
    bool scrollToBottom;                             ///< Флаг: нужно ли прокручивать вниз при новом сообщении.
public:
    /**
     * @brief Конструктор класса Console.
     * Инициализирует буфер, флаги и регистрирует встроенные команды.
     */
    Console() : historyPos(-1)
    {
        inputBuf[0] = 0;
        scrollToBottom = true;

        RegisterBuildInCommands();

        addLog("INFO", "Добро пожаловать в консоль разработчика");
        addLog("INFO", "Версия: 1.0 (Debug)");
    }

    /**
     * @brief Добавляет новую запись в лог консоли.
     * @param type Тип сообщения ("INFO", "ERROR", "WARNING").
     * @param message Текст сообщения.
     * @param isUserInput Является ли сообщение вводом пользователя (не отображает тип и время).
     */
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

    /**
     * @brief Отрисовывает окно консоли с помощью ImGui.
     * @param title Заголовок окна.
     * @param p_open Указатель на булеву переменную, управляющую видимостью окна.
     */
    void draw(const char* title, bool* p_open = NULL)
    {
        ImGui::SetNextWindowSize(ImVec2(900, 800), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Clear"))
        {
            items.clear();
        }

        ImGui::SameLine();
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
                else if (item.type == "DEBUG")
                {
                    typeColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
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

        ImGuiInputTextFlags inputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackCompletion |
            ImGuiInputTextFlags_CallbackHistory;

        bool reclaimFocus = false;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##Input", inputBuf, IM_ARRAYSIZE(inputBuf), inputFlags, &Console::TextEditCallbackStub, (void*)this))
        {
            if (inputBuf[0] != 0)
            {
                std::string fullCommand(inputBuf);
                if (commandHistory.empty() || commandHistory.back() != fullCommand)
                {
                    commandHistory.push_back(fullCommand);
                }

                historyPos = -1;
                addLog("USER", "> " + fullCommand, true);

                auto args = ParseArgs(fullCommand);
                if (args.empty())
                {
                    inputBuf[0] = 0;
                    reclaimFocus = true;
                    ImGui::SetKeyboardFocusHere(-1);
                }
                else
                {
                    std::string cmdName = args[0];
                    if (cmdName.empty() || cmdName[0] != '!')
                    {
                        addLog("WARNING", "Команда должна начинаться с '!'");;
                    }
                    else
                    {
                        std::string baseName = cmdName.substr(1);
                        std::string key = toLower(baseName);
                        auto it = commandMap.find(key);
                        if (it != commandMap.end())
                        {
                            try
                            {
                                it->second.callback(this, args);
                            }
                            catch (const std::exception& e)
                            {
                                addLog("ERROR", std::string("Ошибка выполнения: ") + e.what());
                            }
                        }
                        else
                        {
                            addLog("WARNING", "Команда '" + baseName + "' не найдена. Введите !help для списка.");
                        }
                    }
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

/**
 * @brief Глобальный указатель на экземпляр консоли.
 */
inline Console* gConsole = nullptr;
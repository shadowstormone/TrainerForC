#pragma once
#include <string>

// Куда уходят сообщения. Реализуется отладочной консолью; домен об этом
// не знает и поэтому не тянет за собой ImGui.
class ILogger
{
public:
    virtual ~ILogger() = default;
    virtual void Log(const std::string& level, const std::string& message) = 0;
};

// Точка доступа к активному логгеру. Приёмник подставляет Application.
//
// Это по-прежнему одна точка на программу, но за интерфейсом: домен пишет
// в ILogger, а не в конкретную ImGui-консоль, приёмник подменяем (в тестах —
// на молчащий), и вызов без установленного приёмника просто ничего не делает,
// а не падает, как раньше делал gConsole->addLog() при null.
namespace Log
{
    // Заменяет все приёмники одним. nullptr — отключить логирование.
    void SetSink(ILogger* sink);

    // Добавляет ещё один приёмник: сообщения уходят во все сразу.
    // Так консоль и файл получают одно и то же.
    void AddSink(ILogger* sink);

    void Info(const std::string& message);
    void Error(const std::string& message);
    void Debug(const std::string& message);
    void Fatal(const std::string& message);
}

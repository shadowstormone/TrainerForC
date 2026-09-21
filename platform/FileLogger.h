#pragma once
#include <fstream>
#include <mutex>
#include <string>

#include "platform/Logger.h"

// Пишет лог в файл рядом с exe.
//
// Отладочная консоль по умолчанию скрыта, а сообщения об ошибках нужны
// именно тогда, когда что-то не сработало — и особенно после падения,
// когда смотреть уже некуда. Файл переживает и то, и другое.
//
// Каждая строка сбрасывается на диск сразу: потерять последнюю запись
// перед падением — ровно то, чего мы избегаем.
class FileLogger : public ILogger
{
    std::ofstream _file;
    std::mutex _mutex; // в лог пишут и UI-поток, и поток обработки читов

public:
    // Открывает <папка_exe>/trainer.log, переписывая прошлый запуск.
    explicit FileLogger(const std::wstring& fileName = L"trainer.log");
    ~FileLogger() override;

    bool IsOpen() const { return const_cast<std::ofstream&>(_file).good(); }

    void Log(const std::string& level, const std::string& message) override;
};

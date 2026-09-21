#include "platform/FileLogger.h"

#include <Windows.h>

#include <chrono>
#include <format>

namespace
{
    // Папка, где лежит exe: класть лог в текущий каталог нельзя — он
    // зависит от того, откуда программу запустили.
    std::wstring ExeDirectory()
    {
        wchar_t path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (length == 0) return L"";

        std::wstring full(path, length);
        const size_t slash = full.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"" : full.substr(0, slash + 1);
    }

    std::string Timestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const auto local = std::chrono::current_zone()->to_local(now);
        return std::format("{:%H:%M:%S}", std::chrono::floor<std::chrono::milliseconds>(local));
    }
}

FileLogger::FileLogger(const std::wstring& fileName)
{
    _file.open(ExeDirectory() + fileName, std::ios::out | std::ios::trunc);

    if (_file.is_open())
    {
        _file << "=== Запуск трейнера ===" << std::endl;
    }
}

FileLogger::~FileLogger()
{
    if (_file.is_open())
    {
        _file << "=== Завершение ===" << std::endl;
        _file.close();
    }
}

void FileLogger::Log(const std::string& level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_file.is_open()) return;

    // endl, а не '\n': сбрасываем сразу, иначе последние строки перед
    // падением не доедут до диска.
    _file << Timestamp() << "  [" << level << "] " << message << std::endl;
}

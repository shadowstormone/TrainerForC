#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "ui/MainView.h" // FunctionOffset, MainView

class Cheat;
class CheatOptionManager;
class Console;
class FileLogger;

// Владеет всем, что живёт столько же, сколько программа, и связывает части
// между собой: процесс-цель, менеджер читов, содержимое окна и консоль.
//
// Раньше эта сборка лежала прямо в wWinMain вперемешку с глобалами
// (offsets, статический Console). Теперь main остаётся тонким:
// создать -> Initialize -> Run.
class Application
{
    std::unique_ptr<Console> _console;
    std::unique_ptr<FileLogger> _fileLog;
    std::unique_ptr<Cheat> _process;
    std::unique_ptr<CheatOptionManager> _cheats;
    std::unique_ptr<MainView> _view;

    std::unordered_map<std::string, FunctionOffset> _offsets;

public:
    // И конструктор, и деструктор определены в .cpp: unique_ptr на неполные
    // типы требует полного типа там, где генерируется их уничтожение
    // (в том числе при откате незавершённого конструктора).
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize(const wchar_t* targetProcessName);
    int Run();
};

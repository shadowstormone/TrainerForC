#pragma once
#include <Windows.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class CheatOption;

// Процесс-цель и фоновый поток трейнера.
//
// Поток ищет игру, следит, жива ли она, опрашивает горячие клавиши и тикает
// опции (автовыключение, заморозка значений).
//
// Раньше поток был detached и продолжал работать после уничтожения
// объекта; теперь он принадлежит объекту и дожидается в Stop()/деструкторе.
class Cheat
{
	std::wstring _processName;
	std::vector<CheatOption*> _options;

	std::atomic<DWORD> _processId{ 0 };
	std::atomic<bool> _isX64{ false };
	std::atomic<bool> _running{ false };
	std::atomic<bool> _accessDenied{ false };
	std::thread _thread;

	std::mutex _tasksMutex;
	std::vector<std::function<void()>> _tasks;

	void Run();
	void RunPostedTasks();

public:
	explicit Cheat(std::wstring processName) : _processName(std::move(processName)) {}
	~Cheat() { Stop(); }

	Cheat(const Cheat&) = delete;
	Cheat& operator=(const Cheat&) = delete;

	// Опции подключаются до Start(): поток читает список без блокировки.
	void AddCheatOption(CheatOption* option) { _options.push_back(option); }

	void Start();
	void Stop();

	// Выполнить задачу в фоновом потоке. Так включение чита по щелчку
	// не держит окно: первый поиск сигнатуры в большой игре занимает
	// заметное время, и раньше интерфейс на это время замирал.
	// Если поток не запущен — задача выполняется сразу.
	void Post(std::function<void()> task);

	LPCWSTR GetProcessName() const { return _processName.c_str(); }
	DWORD GetProcessID() const { return _processId.load(); }
	bool isProcessRunning() const { return _processId.load() != 0; }

	// Разрядность найденной игры (для строки состояния).
	bool IsTargetX64() const { return _isX64.load(); }

	// Игра запущена, но открыть её не дали — обычно она запущена от
	// администратора, а трейнер нет.
	bool IsAccessDenied() const { return _accessDenied.load(); }
};

#include "core/Cheat.h"

#include <chrono>
#include <format>
#include <memory>

#include "cheats/CheatOption.h"
#include "core/MemoryAccess.h"
#include "core/Memory_Functions.h"
#include "platform/Logger.h"
#include "platform/Utils.h"

namespace
{
	constexpr auto TICK = std::chrono::milliseconds(16);

	// Список процессов — снимок всей системы, он дорогой. Пока игры нет,
	// ищем её раз в полсекунды, а не 60 раз в секунду, как раньше.
	constexpr auto SEARCH_INTERVAL = std::chrono::milliseconds(500);
}

void Cheat::Start()
{
	if (_running.exchange(true)) return;
	_thread = std::thread(&Cheat::Run, this);
}

void Cheat::Stop()
{
	_running = false;
	if (_thread.joinable()) _thread.join();
}

void Cheat::Run()
{
	std::unique_ptr<MemoryAccess> mem;
	DWORD deniedPid = 0; // процесс, который не удалось открыть: не спамим логом
	auto nextSearch = std::chrono::steady_clock::now();

	while (_running)
	{
		const auto now = std::chrono::steady_clock::now();

		// Игра закрылась. Опции забывают прежний процесс: иначе после
		// перезапуска игры переключатели показывали бы «включено», хотя
		// в новом процессе ничего не пропатчено.
		if (mem && !mem->IsAlive())
		{
			mem.reset();
			_processId = 0;

			for (CheatOption* option : _options) option->OnProcessLost();
			Log::Info("Игра закрыта — опции сброшены");
		}

		if (!mem && now >= nextSearch)
		{
			nextSearch = now + SEARCH_INTERVAL;

			const DWORD pid = GetProcessIdByProcessName(_processName.c_str());
			_accessDenied = (pid != 0 && pid == deniedPid);

			if (pid != 0 && pid != deniedPid)
			{
				auto opened = std::make_unique<MemoryAccess>(pid);
				if (!opened->IsValid())
				{
					deniedPid = pid;
					_accessDenied = true;
				}
				else
				{
					_isX64 = opened->IsTargetX64();
					mem = std::move(opened);
					_processId = pid;
					Log::Info(std::format("Найдена игра {}, PID {} ({})",
						Utils::WStringToUtf8(_processName), pid, _isX64 ? "x64" : "x86"));
				}
			}
		}

		const DWORD pid = _processId.load();
		for (CheatOption* option : _options)
		{
			option->Process(pid, mem.get());
		}

		std::this_thread::sleep_for(TICK);
	}
}

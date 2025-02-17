#include "Cheat.h"
#include "Memory_Functions.h"

void Cheat::ProcessorOptions()
{
	std::thread processorThread([&]()
		{
			while (isRunning)
			{
				processId = GetProcessIdByProcessName(_processName);
				if (processId)
				{
					for (CheatOption* option : options)
					{
						option->Process(processId);
						m_optionsState[option->GetDescription()] = option->IsEnabled();
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
		});
	processorThread.detach();
}

void Cheat::OpenConsole()
{
	AllocConsole();
	FILE* pCout;
	freopen_s(&pCout, "CONOUT$", "w", stdout);
}

void Cheat::ImGuiOpenConsole()
{
	// Создаем консоль
	if (AllocConsole())
	{
		// Перенаправляем стандартные потоки
		FILE* fDummy;
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);

		// Устанавливаем стандартный режим работы
		HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);
		SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
		SetConsoleMode(hConIn, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);

		// Устанавливаем заголовок консоли
		SetConsoleTitle(L"Debug Console");

		// Устанавливаем буферизацию для stdout
		std::ios::sync_with_stdio(true);
	}
}

int Cheat::AddCheatOption(CheatOption* option)
{
	options.push_back(option);
	m_optionsState.insert(std::make_pair(option->GetDescription(), option->IsEnabled()));
	return static_cast<int>(options.size()) - 1; // Явное преобразование size_t в int
}

void Cheat::RemoveCheatOption(int index)
{
	m_optionsState.erase(options[index]->GetDescription());
	options.erase(options.begin() + index);
}
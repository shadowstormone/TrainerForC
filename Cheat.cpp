#include "Cheat.h"
#include "Memory_Functions.h"

//void Cheat::ProcessorOptions()
//{
//	while (isRunning)
//	{
//		processId = GetProcessIdByProcessName(_processName);
//		if (processId)
//		{
//			for (CheatOption* option : options)
//			{
//				option->Process(processId);
//				m_optionsState[option->GetDescription()] = option->IsEnabled();
//			}
//		}
//		Sleep(16);
//	}
//}

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


int Cheat::AddCheatOption(CheatOption* option)
{
	options.push_back(option);
	m_optionsState.insert(std::make_pair(option->GetDescription(), option->IsEnabled()));
	return options.size() - 1;
}

void Cheat::RemoveCheatOption(int index)
{
	m_optionsState.erase(options[index]->GetDescription());
	options.begin() + index;
}

#include "Include.h"

void Cheat::ProcessorOptions()
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
		Sleep(50);
	}
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

#pragma once
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "patches/CavePatch.h"         // CaveMode
#include "patches/IPatch.h"
#include "patches/WriteAddressPatch.h" // PatchValue, Mode
#include "platform/Hotkey.h"

class MemoryAccess;

// Одна функция трейнера: набор патчей, горячая клавиша и состояние.
//
// Включают её с двух сторон — переключателем в окне (UI-поток) и горячей
// клавишей (фоновый поток). Раньше у каждой стороны была своя логика:
// хоткей не знал про автовыключение, UI — про разовые записи, и оба
// трогали патчи без синхронизации. Теперь вход один — SetEnabled, и он
// под мьютексом.
class CheatOption
{
public:
    using Clock = std::chrono::steady_clock;

private:
	std::wstring m_description;
	std::wstring m_moduleName;
	std::wstring m_hint;
	std::vector<int> m_keys;

	std::vector<std::unique_ptr<IPatch>> m_patches;

	// Состояние читают каждый кадр из UI-потока — поэтому atomic, а не
	// под мьютексом: отрисовка не должна ждать сканирования памяти.
	std::atomic<bool> m_enabled{ false };

	// Мьютекс на всё, что меняет патчи. Рекурсивный: автовыключение
	// вызывается изнутри Process, который уже держит его.
	mutable std::recursive_mutex m_mutex;

	int m_autoOffMs = 0;
	Clock::time_point m_autoOffAt{};
	bool m_autoOffPending = false;

	// Почему не включилось в последний раз — для подсказки в UI.
	// Под отдельным мьютексом: UI читает его каждый кадр и не должен ждать,
	// пока фоновый поток сканирует память под основным.
	std::string m_lastError;
	mutable std::mutex m_errorMutex;
	void SetLastError(std::string error);

	// Сколько запросов на переключение ждут фонового потока.
	std::atomic<int> m_pending{ 0 };

	// Когда последний раз не удалось включить: UI подсвечивает строку.
	std::atomic<long long> m_failedAtTicks{ 0 };

	// Своя комбинация клавиш с собственным состоянием антидребезга.
	Hotkey m_hotkey;

	// Применяет все патчи; при неудаче одного откатывает уже применённые.
	bool ApplyAll(MemoryAccess& mem);
	bool RestoreAll(MemoryAccess& mem);

public:
	CheatOption(std::wstring description, std::vector<int> keys = {}, std::wstring moduleName = {});
	~CheatOption();

	CheatOption(const CheatOption&) = delete;
	CheatOption& operator=(const CheatOption&) = delete;

	// --- Сборка ---

	void AddPatch(std::unique_ptr<IPatch> patch);

	// Nop: length 0 — одну инструкцию целиком.
	CheatOption* AddNopPatch(LPCWSTR signature, SIZE_T length, std::ptrdiff_t offset = 0);

	CheatOption* AddCavePatch(LPCWSTR signature, const BYTE* bytes, SIZE_T size,
	                          CaveMode mode = CaveMode::ReplaceOriginal,
	                          bool preserveRegisters = true, std::ptrdiff_t offset = 0);

	// Патч текстом ассемблера — собирается при применении, под разрядность цели.
	CheatOption* AddCavePatchAsm(LPCWSTR signature, std::string asmText,
	                             AsmSyntax syntax = AsmSyntax::Standard,
	                             CaveMode mode = CaveMode::ReplaceOriginal,
	                             bool preserveRegisters = true, std::ptrdiff_t offset = 0);

	CheatOption* AddWriteValuePatch(std::vector<uintptr_t> offsets, PatchValue value,
	                                bool absolute = false,
	                                WriteAddressPatch::Mode mode = WriteAddressPatch::Mode::OneShot,
	                                std::wstring module = {});

	void SetAutoOff(int milliseconds) { m_autoOffMs = milliseconds; }
	void SetHint(std::wstring hint) { m_hint = std::move(hint); }

	// --- Работа ---

	// Единая точка включения/выключения — и для UI, и для горячей клавиши.
	// true — опция в запрошенном состоянии.
	bool SetEnabled(bool enabled, DWORD pid);
	// Разовую запись горячая клавиша повторяет, а не выключает.
	bool Toggle(DWORD pid) { return SetEnabled(IsOneShot() || !IsEnabled(), pid); }

	// Тик фонового потока: горячая клавиша, автовыключение, заморозка.
	// mem — открытый процесс игры или nullptr, если игры нет.
	void Process(DWORD pid, MemoryAccess* mem);

	// Игра закрылась: забыть всё о прежнем процессе, память не трогать.
	void OnProcessLost();

	// Запрос на переключение отправлен в фоновый поток и ещё не выполнен.
	void BeginPending() { ++m_pending; }
	void EndPending() { --m_pending; }
	bool IsBusy() const { return m_pending.load() > 0; }

	// --- Сведения для UI ---

	bool IsEnabled() const { return m_enabled.load(); }

	// Опция из одних разовых записей — это действие, а не режим.
	bool IsOneShot() const;

	// Есть ли в опции заморозка значения.
	bool HasFreeze() const;

	std::string LastError() const;

	// Сколько секунд назад не удалось включить; < 0 — давно или никогда.
	float SecondsSinceFailure() const;

	const std::wstring& GetDescription() const { return m_description; }
	const std::wstring& GetModuleName() const { return m_moduleName; }
	const std::wstring& GetHint() const { return m_hint; }
	const std::vector<int>& GetKeys() const { return m_keys; }
	std::size_t PatchCount() const { return m_patches.size(); }
};

#pragma once
#include <Windows.h>

// Звуки приложения. Добавить новый = добавить элемент сюда и строку
// в таблицу ресурсов в AudioService.cpp.
enum class Sound
{
    CheatEnabled,
    CheatDisabled,

    // Чит не применился. Раньше отказ был беззвучным и выглядел как
    // "ничего не произошло".
    CheatFailed,
};

// Звуковой сервис поверх XAudio2.
//
// Зачем отдельный сервис: раньше на каждое переключение создавался
// detached-поток ради PlaySound(SND_ASYNC) — поток был лишним (SND_ASYNC
// и так асинхронный), громкостью управлять было нельзя, а два звука
// подряд обрывали друг друга. XAudio2 микширует их нормально.
//
// Движок спрятан за pimpl, поэтому этот заголовок не тянет за собой
// xaudio2.h — бэкенд можно поменять, не трогая вызывающий код.
class AudioService
{
public:
    static AudioService& Instance();

    // Поднимает движок и загружает звуки из ресурсов модуля.
    // Вызывается лениво из Play(), но можно и явно на старте.
    // Если звук недоступен, сервис молча ничего не играет.
    bool Initialize(HMODULE module = nullptr);
    void Shutdown();

    void Play(Sound sound);

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    void SetVolume(float volume); // 0.0 .. 1.0
    float GetVolume() const;

    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;

private:
    AudioService() = default;
    ~AudioService();

    struct Impl;
    Impl* _impl = nullptr;
    bool _enabled = true;
};

#include "platform/AudioService.h"

#include <xaudio2.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "resource.h"

#pragma comment(lib, "xaudio2.lib")

namespace
{
    // Какому звуку какой ресурс соответствует.
    struct SoundResource
    {
        Sound sound;
        int resourceId;
    };

    constexpr SoundResource kSoundResources[] = {
        { Sound::CheatEnabled,  IDR_WAVE1 },
        { Sound::CheatDisabled, IDR_WAVE2 },
        { Sound::CheatFailed,   IDR_WAVE3 },
    };

    // Разобранный WAV: формат + сырые сэмплы.
    struct WavData
    {
        WAVEFORMATEX format{};
        std::vector<BYTE> samples;
        bool valid = false;
    };

    constexpr uint32_t FourCC(const char (&tag)[5])
    {
        return static_cast<uint32_t>(static_cast<unsigned char>(tag[0]))
             | (static_cast<uint32_t>(static_cast<unsigned char>(tag[1])) << 8)
             | (static_cast<uint32_t>(static_cast<unsigned char>(tag[2])) << 16)
             | (static_cast<uint32_t>(static_cast<unsigned char>(tag[3])) << 24);
    }

    // Минимальный разбор RIFF/WAVE: ищем чанки "fmt " и "data".
    WavData ParseWav(const BYTE* data, size_t size)
    {
        WavData wav;
        if (!data || size < 12) return wav;

        if (std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0)
        {
            return wav;
        }

        size_t pos = 12;
        bool haveFormat = false;
        bool haveData = false;

        while (pos + 8 <= size)
        {
            uint32_t chunkId = 0;
            uint32_t chunkSize = 0;
            std::memcpy(&chunkId, data + pos, 4);
            std::memcpy(&chunkSize, data + pos + 4, 4);
            pos += 8;

            if (pos + chunkSize > size) break;

            if (chunkId == FourCC("fmt "))
            {
                const size_t copy = std::min<size_t>(chunkSize, sizeof(WAVEFORMATEX));
                std::memcpy(&wav.format, data + pos, copy);
                // Для PCM поле cbSize в файле может отсутствовать.
                if (chunkSize < sizeof(WAVEFORMATEX)) wav.format.cbSize = 0;
                haveFormat = true;
            }
            else if (chunkId == FourCC("data"))
            {
                wav.samples.assign(data + pos, data + pos + chunkSize);
                haveData = true;
            }

            pos += chunkSize;
            if (chunkSize & 1) ++pos; // чанки выровнены по 2 байта
        }

        wav.valid = haveFormat && haveData && !wav.samples.empty();
        return wav;
    }

    WavData LoadWavFromResource(HMODULE module, int resourceId)
    {
        HRSRC res = ::FindResourceW(module, MAKEINTRESOURCEW(resourceId), L"WAVE");
        if (!res) return {};

        HGLOBAL loaded = ::LoadResource(module, res);
        if (!loaded) return {};

        const auto* bytes = static_cast<const BYTE*>(::LockResource(loaded));
        const DWORD size = ::SizeofResource(module, res);
        if (!bytes || size == 0) return {};

        return ParseWav(bytes, size);
    }
}

struct AudioService::Impl
{
    IXAudio2* engine = nullptr;
    IXAudio2MasteringVoice* masterVoice = nullptr;
    std::unordered_map<int, WavData> sounds; // ключ — static_cast<int>(Sound)
    std::vector<IXAudio2SourceVoice*> activeVoices;

    bool enabled = true;
    float volume = 1.0f;
    bool comInitialized = false;

    // Удаляем голоса, которые доиграли.
    void ReapFinishedVoices()
    {
        auto it = activeVoices.begin();
        while (it != activeVoices.end())
        {
            XAUDIO2_VOICE_STATE state{};
            (*it)->GetState(&state);

            if (state.BuffersQueued == 0)
            {
                (*it)->Stop(0);
                (*it)->DestroyVoice();
                it = activeVoices.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void DestroyAllVoices()
    {
        for (auto* voice : activeVoices)
        {
            voice->Stop(0);
            voice->DestroyVoice();
        }
        activeVoices.clear();
    }
};

AudioService& AudioService::Instance()
{
    static AudioService instance;
    return instance;
}

AudioService::~AudioService()
{
    Shutdown();
}

bool AudioService::Initialize(HMODULE module)
{
    if (_impl) return true;

    auto* impl = new Impl();

    // XAudio2 требует инициализированный COM. Если поток уже COM-инициализирован
    // с другой моделью, получим RPC_E_CHANGED_MODE — это не ошибка для нас,
    // просто не будем вызывать CoUninitialize.
    const HRESULT coHr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    impl->comInitialized = SUCCEEDED(coHr);

    if (FAILED(::XAudio2Create(&impl->engine, 0, XAUDIO2_DEFAULT_PROCESSOR)))
    {
        if (impl->comInitialized) ::CoUninitialize();
        delete impl;
        return false;
    }

    if (FAILED(impl->engine->CreateMasteringVoice(&impl->masterVoice)))
    {
        impl->engine->Release();
        if (impl->comInitialized) ::CoUninitialize();
        delete impl;
        return false;
    }

    if (!module) module = ::GetModuleHandleW(nullptr);

    for (const auto& entry : kSoundResources)
    {
        WavData wav = LoadWavFromResource(module, entry.resourceId);
        if (wav.valid)
        {
            impl->sounds.emplace(static_cast<int>(entry.sound), std::move(wav));
        }
    }

    _impl = impl;
    return true;
}

void AudioService::Shutdown()
{
    if (!_impl) return;

    _impl->DestroyAllVoices();

    if (_impl->masterVoice)
    {
        _impl->masterVoice->DestroyVoice();
        _impl->masterVoice = nullptr;
    }
    if (_impl->engine)
    {
        _impl->engine->Release();
        _impl->engine = nullptr;
    }
    if (_impl->comInitialized)
    {
        ::CoUninitialize();
    }

    delete _impl;
    _impl = nullptr;
}

void AudioService::Play(Sound sound)
{
    if (!_impl && !Initialize()) return;
    if (!_impl->enabled) return;

    auto found = _impl->sounds.find(static_cast<int>(sound));
    if (found == _impl->sounds.end()) return;

    const WavData& wav = found->second;

    _impl->ReapFinishedVoices();

    IXAudio2SourceVoice* voice = nullptr;
    if (FAILED(_impl->engine->CreateSourceVoice(&voice, &wav.format)))
    {
        return;
    }

    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(wav.samples.size());
    buffer.pAudioData = wav.samples.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    if (FAILED(voice->SubmitSourceBuffer(&buffer)))
    {
        voice->DestroyVoice();
        return;
    }

    voice->SetVolume(_impl->volume);

    if (FAILED(voice->Start(0)))
    {
        voice->DestroyVoice();
        return;
    }

    _impl->activeVoices.push_back(voice);
}

void AudioService::SetEnabled(bool enabled)
{
    if (!_impl && !Initialize()) return;
    _impl->enabled = enabled;
}

bool AudioService::IsEnabled() const
{
    return _impl ? _impl->enabled : true;
}

void AudioService::SetVolume(float volume)
{
    if (!_impl && !Initialize()) return;
    _impl->volume = std::clamp(volume, 0.0f, 1.0f);
    if (_impl->masterVoice) _impl->masterVoice->SetVolume(_impl->volume);
}

float AudioService::GetVolume() const
{
    return _impl ? _impl->volume : 1.0f;
}

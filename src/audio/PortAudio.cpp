#include "audio/PortAudio.hpp"

#include "audio/Ingress.hpp"

#include "portaudio.h"
#include "../src/common/pa_ringbuffer.h"
#include "spdlog/spdlog.h"

#ifdef __linux__
#include "pa_jack.h"
#endif

namespace {
int portAudioCallback(const void* input, void* output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
    scin::audio::PortAudio* audio = static_cast<scin::audio::PortAudio*>(userData);
    if (input && audio->inputChannels()) {
        audio->ingress()->ingestSamples(static_cast<const float*>(input), frameCount);
    }

    return PaStreamCallbackResult::paContinue;
}
}

namespace scin { namespace audio {

PortAudio::PortAudio(int inputChannels, int outputChannels): m_inputChannels(inputChannels),
    m_outputChannels(outputChannels), m_init(false) {}

PortAudio::~PortAudio() { destroy(); }

bool PortAudio::create() {
    if (m_inputChannels <= 0 && m_outputChannels <= 0) {
        spdlog::info("No input or output audio channels selected, skipping PortAudio initialization");
        return true;
    }

    if (m_init) {
        spdlog::warn("Duplicate calls to PortAudio::create()");
        return false;
    }

    const PaVersionInfo* versionInfo = Pa_GetVersionInfo();
    spdlog::info("Initializing PortAudio subsystem: {}", versionInfo->versionText);

    PaError result;

#ifdef __linux__
    result = PaJack_SetClientName("ScintillatorSynth");
    spdlog::info("Setting PortAudio JACK client name to ScintillatorSynth");
    if (result != paNoError) {
        spdlog::error("Failed to set PortAudio JACK client name: {}", Pa_GetErrorText(result));
        return false;
    }
#endif

    spdlog::info("Initializing PortAudio subsystem.");
    result = Pa_Initialize();
    if (result != paNoError) {
        spdlog::error("Failed to initialize PortAudio subsystem: {}", Pa_GetErrorText(result));
        return false;
    }
    m_init = true;

#ifdef __linux__
    auto apiIndex = Pa_HostApiTypeIdToHostApiIndex(PaHostApiTypeId::paJACK);
    if (apiIndex < 0) {
        spdlog::error("PortAudio failed to find JACK, is it running? {}.", Pa_GetErrorText(apiIndex));
        return false;
    }
#elif (__APPLE__)
    auto apiIndex = Pa_HostApiTypeIdToHostApiIndex(PaHostApiTypeId::paCoreAudio);
    if (apiIndex < 0) {
        spdlog::error("PortAudio failed to find CoreAudio: {}.", Pa_GetErrorText(apiIndex));
        return false;
    }
#elif (WIN32)
    // TODO: settle on preferred Windows audio API. (ASIO?)
    auto apiIndex = 0;
#endif

    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(apiIndex);
    if (!apiInfo) {
        spdlog::error("Failed to get PortAudio API info.");
        return false;
    }

    double sampleRate = 0.0;

    PaStreamParameters inputStream;
    if (m_inputChannels > 0) {
        inputStream.channelCount = m_inputChannels;
        inputStream.sampleFormat = paFloat32;
        inputStream.hostApiSpecificStreamInfo = nullptr;

#ifdef __linux__
        auto deviceIndex = apiInfo->defaultInputDevice;
        if (deviceIndex == paNoDevice) {
            spdlog::error("No default input device configured for JACK, is JACK configured correctly?");
            return false;
        }
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
        if (!deviceInfo) {
            spdlog::error("Failed to retrieve info about default JACK input device.");
            return false;
        }
        spdlog::info("JACK audio input max channels: {}, low latency: {}s, high latency: {}s, default sample rate: {}",
            deviceInfo->maxInputChannels, deviceInfo->defaultLowInputLatency, deviceInfo->defaultHighInputLatency,
            deviceInfo->defaultSampleRate);
        inputStream.suggestedLatency = deviceInfo->defaultLowInputLatency;
        inputStream.device = deviceIndex;
        if (m_inputChannels > deviceInfo->maxInputChannels) {
            spdlog::error("Requested {} input channels, but JACK input configured to support maximum of {} channels.");
            return false;
        }
        m_ingress.reset(new Ingress(m_inputChannels, deviceInfo->defaultSampleRate));
        if (!m_ingress->create()) {
            spdlog::error("PortAudio failed to create Ingress object.");
            return false;
        }
        sampleRate = deviceInfo->defaultSampleRate;
#endif
    }

    PaStreamParameters outputStream;
    if (m_outputChannels > 0) {
        outputStream.channelCount = m_outputChannels;
        outputStream.sampleFormat = paFloat32;
        outputStream.hostApiSpecificStreamInfo = nullptr;

#ifdef __linux__
        auto deviceIndex = apiInfo->defaultOutputDevice;
        if (deviceIndex == paNoDevice) {
            spdlog::error("No default output device configured for JACK, is JACK configured correctly?");
            return false;
        }
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
        if (!deviceInfo) {
            spdlog::error("Failed to retrieve info about default JACK output device.");
            return false;
        }
        spdlog::info("JACK audio output max channels: {}, low latency: {}s, high latency: {}s, default sample rate: {}",
            deviceInfo->maxOutputChannels, deviceInfo->defaultLowInputLatency, deviceInfo->defaultHighInputLatency,
            deviceInfo->defaultSampleRate);
        inputStream.suggestedLatency = deviceInfo->defaultLowInputLatency;
        inputStream.device = deviceIndex;
        if (m_outputChannels > deviceInfo->maxOutputChannels) {
            spdlog::error("Requested {} output channels, but JACK output configured to support maximum of {} channels.");
            return false;
        }
        sampleRate = deviceInfo->defaultSampleRate;
#endif
    }

    PaStream* stream = nullptr;
    const PaStreamParameters* inputParams = m_inputChannels > 0 ? &inputStream : nullptr;
    const PaStreamParameters* outputParams = m_outputChannels > 0 ? &outputStream : nullptr;
    result = Pa_OpenStream(&stream, inputParams, outputParams, sampleRate, paFramesPerBufferUnspecified, paNoFlag,
        portAudioCallback, this);
    if (result != paNoError) {
        spdlog::error("PortAudio failed to open stream: {}", Pa_GetErrorText(result));
        return false;
    }

    result = Pa_StartStream(stream);
    if (result != paNoError) {
        spdlog::error("PortAudio failed to start stream: {}", Pa_GetErrorText(result));
        return false;
    }

    return true;
}

void PortAudio::destroy() {
    if (m_init) {
        spdlog::info("Terminating PortAudio subsystem.");
        Pa_Terminate();
        m_init = false;
    }
}

} // namespace audio
} // namespace scin

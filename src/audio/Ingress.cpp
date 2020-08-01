#include "audio/Ingress.hpp"

#include "../src/common/pa_ringbuffer.h"
#include "spdlog/spdlog.h"

namespace scin { namespace audio {

const int kSamplesPerChannel = 8192;

Ingress::Ingress(int channels, int sampleRate):
    m_channels(channels),
    m_sampleRate(sampleRate),
    m_ringBuffer(new PaUtilRingBuffer),
    m_buffer(new float[kSamplesPerChannel * channels]) {}

Ingress::~Ingress() {}

bool Ingress::create() {
    ring_buffer_size_t size =
        PaUtil_InitializeRingBuffer(m_ringBuffer.get(), 4, kSamplesPerChannel * m_channels, m_buffer.get());
    if (size < 0) {
        spdlog::error("Ingress failed to create ring buffer.");
        return false;
    }
    return true;
}

void Ingress::destroy() {}

void Ingress::ingestSamples(const float* input, unsigned long frameCount) {
    unsigned long elementCount = frameCount * m_channels;
    unsigned long writeAvailable = PaUtil_GetRingBufferWriteAvailable(m_ringBuffer.get());
    if (elementCount > writeAvailable) {
        // Clobber oldest elements if needed.
        dropSamples(elementCount - writeAvailable);
    }
    PaUtil_WriteRingBuffer(m_ringBuffer.get(), input, elementCount);
}

unsigned long Ingress::availableFrames() { return PaUtil_GetRingBufferReadAvailable(m_ringBuffer.get()) / m_channels; }

void Ingress::dropSamples(unsigned long frameCount) {
    PaUtil_AdvanceRingBufferReadIndex(m_ringBuffer.get(), frameCount * m_channels);
}

unsigned long Ingress::extractSamples(float* output, unsigned long frameCount) {
    auto readSamples = std::min(availableFrames(), frameCount) * m_channels;
    return PaUtil_ReadRingBuffer(m_ringBuffer.get(), output, readSamples);
}

} // namespace audio
} // namespace scin

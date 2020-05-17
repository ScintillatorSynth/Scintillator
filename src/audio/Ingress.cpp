#include "audio/Ingress.hpp"

#include "../src/common/pa_ringbuffer.h"
#include "spdlog/spdlog.h"

namespace scin { namespace audio {

const int kSamplesPerChannel = 8192;

Ingress::Ingress(int channels): m_channels(channels), m_ringBuffer(new PaUtilRingBuffer),
    m_buffer(new float[kSamplesPerChannel * channels]) {
}

Ingress::~Ingress() {

}

bool Ingress::create() {
    ring_buffer_size_t size = PaUtil_InitializeRingBuffer(m_ringBuffer.get(), 4, kSamplesPerChannel * m_channels,
        m_buffer.get());
    if (size < 0) {
        spdlog::error("Ingress failed to create ring buffer.");
        return false;
    }
    return true;
}

void Ingress::destroy() {

}

void Ingress::ingestSamples(const float* input, unsigned long frameCount) {
    unsigned long writeAvailable = PaUtil_GetRingBufferWriteAvailable(m_ringBuffer.get());
    // TODO: logging on buffer oflow, maybe atomically increment a counter?
    unsigned long elementCount = std::min(frameCount * m_channels, writeAvailable);
    unsigned long framesWritten = PaUtil_WriteRingBuffer(m_ringBuffer.get(), input, elementCount);
}

} // namespace audio
} // namespace scin

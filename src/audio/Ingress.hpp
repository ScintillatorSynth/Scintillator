#ifndef SRC_AUDIO_INGRESS_HPP_
#define SRC_AUDIO_INGRESS_HPP_

#include <memory>

struct PaUtilRingBuffer;

namespace scin { namespace audio {

// Represents an input of audio in to the Scintillator system. Uses a lockfree input ringbuffer that can be added to
// asynchronously anytime. Monitors that queue and can inform a variety of audio consumers within the Scintillator
// system when there's new data available and what that data might be.
class Ingress {
public:
    Ingress(int channels);
    ~Ingress();

    bool create();
    void destroy();

    // Called on realtime audio loop. Needs to always be called by same thread.
    void ingestSamples(const float* input, unsigned long frameCount);

private:
    int m_channels;

    std::unique_ptr<PaUtilRingBuffer> m_ringBuffer;
    std::unique_ptr<float[]> m_buffer;
};

} // namespace scin
} // namespace audio

#endif // SRC_AUDIO_INGRESS_HPP_

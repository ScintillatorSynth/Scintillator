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
    Ingress(int channels, int sampleRate);
    ~Ingress();

    bool create();
    void destroy();

    // Called on realtime audio loop. Needs to always be called by same thread.
    void ingestSamples(const float* input, unsigned long frameCount);

    // something to get samples available for reading count
    unsigned long availableFrames();

    // advance read pointer without copying (just drop)
    void dropSamples(unsigned long frameCount);

    // extract samples
    unsigned long extractSamples(float* output, unsigned long frameCount);

    int channels() const { return m_channels; }
    int sampleRate() const { return m_sampleRate; }

private:
    int m_channels;
    int m_sampleRate;

    std::unique_ptr<PaUtilRingBuffer> m_ringBuffer;
    std::unique_ptr<float[]> m_buffer;
};

} // namespace scin
} // namespace audio

#endif // SRC_AUDIO_INGRESS_HPP_

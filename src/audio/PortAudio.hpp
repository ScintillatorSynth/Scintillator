#ifndef SRC_AUDIO_PORT_AUDIO_H_
#define SRC_AUDIO_PORT_AUDIO_H_

#include <memory>
#include <string>

// A PortAudio forward declaration to avoid leading the PA header into the rest of Scintillator.
typedef void PaStream;

namespace scin { namespace audio {

class Ingress;

// Represents the common interface for sending audio to and receiving audio from the PortAudio abstraction layer.
class PortAudio {
public:
    PortAudio(int inputChannels, int outputChannels);
    ~PortAudio();

    // Initialize the PortAudio system and enumerate available devices.
    bool create();

    void destroy();

    std::shared_ptr<Ingress> ingress() { return m_ingress; }

    int inputChannels() const { return m_inputChannels; }
    int outputChannels() const { return m_outputChannels; }

private:
    // Platform-specific functions for PortAudio library configuration.
    // returns -1 if no suitable API found.
    int pickApiIndex();

    int m_inputChannels;
    int m_outputChannels;

    // True if we actually did initalize the PortAudio system, and therefore need to de-init it on destroy().
    bool m_init;

    PaStream* m_stream;

    std::shared_ptr<Ingress> m_ingress;
};


} // namespace audio
} // namespace scin

#endif // SRC_AUDIO_PORT_AUDIO_H_

#ifndef SRC_AUDIO_PORT_AUDIO_H_
#define SRC_AUDIO_PORT_AUDIO_H_

#include <string>

namespace scin { namespace audio {

// Represents the common interface for sending audio to and receiving audio from the PortAudio abstraction layer.
class PortAudio {
public:
    PortAudio(int inputChannels, int outputChannels);
    ~PortAudio();

    // Initialize the PortAudio system and enumerate available devices.
    bool create();

    void destroy();
private:
    int m_inputChannels;
    int m_outputChannels;

    // True if we actually did initalize the PortAudio system, and therefore need to de-init it on destroy().
    bool m_init;
};


} // namespace audio
} // namespace scin

#endif // SRC_AUDIO_PORT_AUDIO_H_

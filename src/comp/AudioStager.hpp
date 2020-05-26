#ifndef SRC_COMP_AUDIO_STAGER_HPP_
#define SRC_COMP_AUDIO_STAGER_HPP_

#include <memory>

namespace scin {

namespace audio {
class Ingress;
}

namespace vk {
class Device;
class DeviceImage;
class HostBuffer;
}

namespace comp {
class StageManager;

/*! Owned by Compositor, manages a host and device buffer and automates copy of data from Ingress to device.
 */
class AudioStager {
public:
    AudioStager(std::shared_ptr<audio::Ingress> ingress);
    ~AudioStager();

    bool createBuffers(std::shared_ptr<vk::Device> device);
    void destroy();

    void stageAudio(std::shared_ptr<StageManager> stageManager);

    std::shared_ptr<vk::DeviceImage> image() { return m_image; }

private:
    std::shared_ptr<audio::Ingress> m_ingress;
    int m_bufferFrameSize;
    std::shared_ptr<vk::HostBuffer> m_buffer;
    std::shared_ptr<vk::DeviceImage> m_image;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_AUDIO_STAGER_HPP_

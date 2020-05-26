#include "comp/AudioStager.hpp"

#include "audio/Ingress.hpp"
#include "comp/StageManager.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

AudioStager::AudioStager(std::shared_ptr<audio::Ingress> ingress): m_ingress(ingress) { }

AudioStager::~AudioStager() {}

bool AudioStager::createBuffers(std::shared_ptr<vk::Device> device) {
    // TODO: fix hard-coded assumptions about 60Hz sample rate and stereo channels.
    m_bufferFrameSize = m_ingress->sampleRate() / 60;
    m_buffer.reset(new vk::HostBuffer(device, vk::Buffer::Kind::kStaging, m_bufferFrameSize * 8));
    if (!m_buffer->create()) {
        spdlog::error("AudioStager failed to create HostBuffer of {} bytes", m_bufferFrameSize * 8);
        return false;
    }

    m_image.reset(new vk::DeviceImage(device, VK_FORMAT_R32G32_SFLOAT));
    if (!m_image->create(m_bufferFrameSize, 1)) {
        spdlog::error("AudioStager failed to create DeviceImage of {} pixels wide", m_bufferFrameSize);
        return false;
    }
    if (!m_image->createView()) {
        spdlog::error("AudioStager failed to create VkImageView");
        return false;
    }
    return true;
}

void AudioStager::destroy() {
    m_buffer.reset();
    m_image.reset();
}

void AudioStager::stageAudio(std::shared_ptr<StageManager> stageManager) {
    // Drop samples greater than two buffer sizes.
    auto framesAvailable = m_ingress->availableFrames();
    if (framesAvailable > 2 * m_bufferFrameSize) {
        auto dropSize = framesAvailable - (2 * m_bufferFrameSize);
        m_ingress->dropSamples(dropSize);
    }
    if (framesAvailable > m_bufferFrameSize) {
        m_ingress->extractSamples(static_cast<float*>(m_buffer->mappedAddress()), m_bufferFrameSize);
        stageManager->stageImage(m_buffer, m_image, []{});
    }
}

} // namespace comp
} // namespace scin

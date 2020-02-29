#include "comp/SamplerFactory.hpp"

#include "base/AbstractSampler.hpp"
#include "vulkan/Sampler.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace comp {

SamplerFactory::SamplerFactory(std::shared_ptr<vk::Device> device): m_device(device) {}

SamplerFactory::~SamplerFactory() {}

std::shared_ptr<vk::Sampler> SamplerFactory::getSampler(const base::AbstractSampler& abstractSampler) {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    std::shared_ptr<vk::Sampler> sampler;
    auto it = m_samplerMap.find(abstractSampler.key());
    if (it != m_samplerMap.end()) {
        ++(it->second.first);
        sampler = it->second.second;
    } else {
        std::shared_ptr<vk::Sampler> newSampler(new vk::Sampler(m_device, abstractSampler));
        // Sampler creation while holding mutex means we might block other threads for longer but also avoids
        // creation of duplicate samplers during race.
        if (!sampler->create()) {
            spdlog::error("SamplerFactory failed to create Sampler with key {:08x}, {} existing samplers.",
                          m_samplerMap.size(), abstractSampler.key());
            return sampler;
        }
        m_samplerMap[abstractSampler.key()] = std::make_pair(1, newSampler);
        sampler = newSampler;
    }
    return sampler;
}

void SamplerFactory::releaseSampler(std::shared_ptr<vk::Sampler> sampler) {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    auto it = m_samplerMap.find(sampler->abstractSampler().key());
    if (it == m_samplerMap.end()) {
        spdlog::error("SamplerFactory got release request for Sampler key {:08x} with no associated map entry.",
                      sampler->abstractSampler().key());
        return;
    }
    --(it->second.first);
    if (it->second.first == 0) {
        m_samplerMap.erase(it);
    }
}

} // namespace comp

} // namespace scin

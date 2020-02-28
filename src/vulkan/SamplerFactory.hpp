#ifndef SRC_VULKAN_SAMPLER_FACTORY_HPP_
#define SRC_VULKAN_SAMPLER_FACTORY_HPP_

#include <memory>
#include <mutex>
#include <unordered_map>

namespace scin {

namespace core {
class AbstractSampler;
}

namespace vk {

class Device;
class Sampler;

/*! Keeps a centralized repository of all allocated Sampler objects, and reclaims them when they are no longer needed.
 * Vulkan limits the number of allocated samplers
 */
class SamplerFactory {
public:
    SamplerFactory(std::shared_ptr<Device> device);
    ~SamplerFactory();

    std::shared_ptr<Sampler> getSampler(const core::AbstractSampler& abstractSampler);
    void releaseSampler(std::shared_ptr<Sampler> sampler);

private:
    std::shared_ptr<Device> m_device;
    std::mutex m_mapMutex;
    std::unordered_map<uint32_t, std::pair<size_t, std::shared_ptr<Sampler>>> m_samplerMap;
};

} // namespace vk
} // namespace scin

#endif // SRC_VULKAN_SAMPLER_FACTORY_HPP_

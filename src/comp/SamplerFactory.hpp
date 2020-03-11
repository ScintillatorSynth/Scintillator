#ifndef SRC_COMP_SAMPLER_FACTORY_HPP_
#define SRC_COMP_SAMPLER_FACTORY_HPP_

#include <memory>
#include <mutex>
#include <unordered_map>

namespace scin {

namespace base {
class AbstractSampler;
}

namespace vk {
class Device;
class Sampler;
}

namespace comp {

/*! Keeps a centralized repository of all allocated Sampler objects, and reclaims them when they are no longer needed.
 * Vulkan limits the number of allocated samplers
 */
class SamplerFactory {
public:
    SamplerFactory(std::shared_ptr<vk::Device> device);
    ~SamplerFactory();

    std::shared_ptr<vk::Sampler> getSampler(const base::AbstractSampler& abstractSampler);
    std::shared_ptr<vk::Sampler> getSampler(uint32_t samplerKey);
    void releaseSampler(std::shared_ptr<vk::Sampler> sampler);

private:
    std::shared_ptr<vk::Device> m_device;
    std::mutex m_mapMutex;
    std::unordered_map<uint32_t, std::pair<size_t, std::shared_ptr<vk::Sampler>>> m_samplerMap;
};

} // namespace comp
} // namespace scin

#endif // SRC_VULKAN_SAMPLER_FACTORY_HPP_

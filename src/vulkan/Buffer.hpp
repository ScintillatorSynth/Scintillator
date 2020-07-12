#ifndef SRC_VULKAN_BUFFER_HPP_
#define SRC_VULKAN_BUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include "vk_mem_alloc.h"

#include <memory>

namespace scin { namespace vk {

class Device;

/*! Abstract base class describing a Vulkan Buffer.
 */
class Buffer {
public:
    enum Kind { kIndex, kUniform, kVertex, kStaging, kStorage };
    Buffer(std::shared_ptr<Device> device, Kind kind, size_t size);
    virtual ~Buffer();

    virtual bool create() = 0;
    void destroy();

    size_t size() const { return m_size; }
    VkBuffer buffer() { return m_buffer; }

protected:
    bool createVulkanBuffer(bool hostAccessRequired);

    std::shared_ptr<Device> m_device;
    Kind m_kind;
    size_t m_size;
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    VmaAllocationInfo m_info;
};


/*! A Vulkan buffer accessible only from the GPU. GPU memory not accessible by the host may be faster depending on
 * system architecture. But it is not likely to be slower.
 */
class DeviceBuffer : public Buffer {
public:
    DeviceBuffer(std::shared_ptr<Device> device, Buffer::Kind kind, size_t size);
    virtual ~DeviceBuffer();

    bool create() override;
};

/*! A Vulkan buffer accessible by the CPU (host). If a kStaging buffer it will be marked as CPU-only, otherwise it is
 * assumed it will be read by both CPU and GPU (for a uniform buffer, for example).
 */
class HostBuffer : public Buffer {
public:
    HostBuffer(std::shared_ptr<Device> device, Buffer::Kind kind, size_t size);
    virtual ~HostBuffer();

    bool create() override;

    void* mappedAddress() { return m_mappedAddress; }

protected:
    void* m_mappedAddress;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_BUFFER_HPP_

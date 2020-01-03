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
    enum Kind { kIndex, kUniform, kVertex };
    Buffer(Kind kind, size_t size, std::shared_ptr<Device> device);
    virtual ~Buffer();

    virtual bool create() = 0;
    void destroy();

    size_t size() const { return m_size; }
    VkBuffer buffer() { return m_buffer; }

protected:
    bool createVulkanBuffer(bool hostAccessRequired);

    Kind m_kind;
    std::shared_ptr<Device> m_device;
    size_t m_size;
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
};


/*! A Vulkan buffer accessible only from the GPU. GPU memory not accessible by the host may be more plentiful, faster,
 *  both, or neither, depending on system architecture. But it is not very likely to be more scarce or slower.
 */
class DeviceBuffer : public Buffer {
    DeviceBuffer(Buffer::Kind kind, size_t size, std::shared_ptr<Device> device);
    virtual ~DeviceBuffer();

    bool create() override;
};

/*! A Vulkan buffer accessible by the CPU (host).
 */
class HostBuffer : public Buffer {
public:
    HostBuffer(Buffer::Kind kind, size_t size, std::shared_ptr<Device> device);
    virtual ~HostBuffer();

    bool create() override;

    /*! Maps the buffer into CPU memory, copies source to it, then unmaps the buffer.
     *
     * \param source A pointer to at least size() bytes of data to transfer.
     */
    void copyToGPU(const void* source);
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_BUFFER_HPP_

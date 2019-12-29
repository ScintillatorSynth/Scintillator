#ifndef SRC_VULKAN_BUFFER_HPP_
#define SRC_VULKAN_BUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include "vk_mem_alloc.h"

#include <memory>

namespace scin { namespace vk {

class Device;

class Buffer {
public:
    enum Kind { kIndex, kUniform, kVertex };
    Buffer(Kind kind, bool requiresHostAccess, std::shared_ptr<Device> device);
    ~Buffer();

    bool create(size_t size);
    void destroy();

    /*! Maps the buffer into CPU memory, copies source to it, then unmaps the buffer.
     *
     * \param source A pointer to at least size() bytes of data to transfer.
     */
    void copyToGPU(const void* source);

    size_t size() const { return m_size; }
    VkBuffer buffer() { return m_buffer; }

private:
    void mapMemory();
    void unmapMemory();

    Kind m_kind;
    std::shared_ptr<Device> m_device;
    size_t m_size;
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    void* m_mappedAddress;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_BUFFER_HPP_

#ifndef SRC_VULKAN_BUFFER_HPP_
#define SRC_VULKAN_BUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>

namespace scin { namespace vk {

class Device;

class Buffer {
public:
    enum Kind { kIndex, kUniform, kVertex };
    Buffer(Kind kind, std::shared_ptr<Device> device);
    ~Buffer();

    bool Create(size_t size);
    void Destroy();

    /*! Maps the buffer into CPU memory, copies source to it, then unmaps the buffer.
     *
     * \param source A pointer to at least size() bytes of data to transfer.
     */
    void copyToGPU(const void* source);


    void* mapped_address() { return mapped_address_; }
    size_t size() const { return size_; }
    VkBuffer buffer() { return buffer_; }

private:
    void mapMemory();
    void unmapMemory();

    Kind kind_;
    std::shared_ptr<Device> device_;
    size_t size_;
    VkBuffer buffer_;
    VkDeviceMemory device_memory_;
    void* mapped_address_;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_BUFFER_HPP_

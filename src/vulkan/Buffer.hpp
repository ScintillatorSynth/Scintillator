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

    void MapMemory();
    // TODO: void Flush();  // call after writing from CPU
    // TODO: void Invalidate();  // call before reading from CPU
    void UnmapMemory();

    void* mapped_address() { return mapped_address_; }
    size_t size() const { return size_; }
    VkBuffer buffer() { return buffer_; }

private:
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

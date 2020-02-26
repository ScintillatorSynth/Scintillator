#include "vulkan/Buffer.hpp"

#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Buffer::Buffer(Kind kind, size_t size, std::shared_ptr<Device> device):
    m_kind(kind),
    m_device(device),
    m_size(size),
    m_buffer(VK_NULL_HANDLE) {}

Buffer::~Buffer() { destroy(); }

void Buffer::destroy() {
    // Calls to vmaDestroyBuffer with VK_NULL_HANDLE as the buffer argument will cause assertions within the allocator.
    if (m_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_device->allocator(), m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
    }
}

bool Buffer::createVulkanBuffer(bool hostAccessRequired) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    switch (m_kind) {
    case kIndex:
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;

    case kUniform:
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;

    case kVertex:
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;

    default:
        spdlog::error("buffer created with unsupported buffer type.");
        return false;
    }

    VmaAllocationCreateInfo allocInfo = {};
    if (hostAccessRequired) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    } else {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (vmaCreateBuffer(m_device->allocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr)
        != VK_SUCCESS) {
        spdlog::error("failed to allocate buffer memory of {} bytes.", m_size);
    }

    return true;
}

DeviceBuffer::DeviceBuffer(Buffer::Kind kind, size_t size, std::shared_ptr<Device> device):
    Buffer(kind, size, device) {}

DeviceBuffer::~DeviceBuffer() {}

bool DeviceBuffer::create() { return createVulkanBuffer(false); }

HostBuffer::HostBuffer(Buffer::Kind kind, size_t size, std::shared_ptr<Device> device): Buffer(kind, size, device) {}

HostBuffer::~HostBuffer() {}

bool HostBuffer::create() { return createVulkanBuffer(true); }

void HostBuffer::copyToGPU(const void* source) {
    void* mappedAddress = nullptr;
    vmaMapMemory(m_device->allocator(), m_allocation, &mappedAddress);
    std::memcpy(mappedAddress, source, m_size);
    vmaUnmapMemory(m_device->allocator(), m_allocation);
}

} // namespace scin

} // namespace vk

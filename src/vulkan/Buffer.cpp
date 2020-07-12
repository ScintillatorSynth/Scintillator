#include "vulkan/Buffer.hpp"

#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Buffer::Buffer(std::shared_ptr<Device> device, Kind kind, size_t size):
    m_device(device),
    m_kind(kind),
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

    case kStaging:
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        break;

    case kStorage:
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;

    default:
        spdlog::error("buffer created with unsupported buffer type.");
        return false;
    }

    VmaAllocationCreateInfo allocInfo = {};
    if (hostAccessRequired) {
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        if (m_kind == kStaging) {
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        } else {
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        }
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (vmaCreateBuffer(m_device->allocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, &m_info)
        != VK_SUCCESS) {
        spdlog::error("failed to allocate buffer memory of {} bytes.", m_size);
    }

    return true;
}

DeviceBuffer::DeviceBuffer(std::shared_ptr<Device> device, Buffer::Kind kind, size_t size):
    Buffer(device, kind, size) {}

DeviceBuffer::~DeviceBuffer() {}

bool DeviceBuffer::create() { return createVulkanBuffer(false); }

HostBuffer::HostBuffer(std::shared_ptr<Device> device, Buffer::Kind kind, size_t size):
    Buffer(device, kind, size),
    m_mappedAddress(nullptr) {}

HostBuffer::~HostBuffer() {}

bool HostBuffer::create() {
    if (!createVulkanBuffer(true)) {
        return false;
    }
    m_mappedAddress = m_info.pMappedData;
    return true;
}


} // namespace scin

} // namespace vk

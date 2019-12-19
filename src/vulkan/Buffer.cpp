#include "vulkan/Buffer.hpp"

#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Buffer::Buffer(Kind kind, std::shared_ptr<Device> device):
    m_kind(kind),
    m_device(device),
    m_size(0),
    m_buffer(VK_NULL_HANDLE),
    m_mappedAddress(nullptr) {}

Buffer::~Buffer() { destroy(); }

bool Buffer::create(size_t size) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    switch (m_kind) {
    case kIndex:
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;

    case kUniform:
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;

    case kVertex:
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;

    default:
        spdlog::error("Buffer::Create called with unsupported buffer type.");
        return false;
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    if (vmaCreateBuffer(m_device->allocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr)
        != VK_SUCCESS) {
        spdlog::error("failed to allocate buffer memory of {} bytes.", size);
    }

    m_size = size;

    return true;
}

void Buffer::destroy() {
    unmapMemory();

    if (m_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_device->allocator(), m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
    }
}

void Buffer::copyToGPU(const void* source) {
    mapMemory();
    std::memcpy(m_mappedAddress, source, m_size);
    unmapMemory();
}

void Buffer::mapMemory() {
    if (m_mappedAddress == nullptr) {
        vmaMapMemory(m_device->allocator(), m_allocation, &m_mappedAddress);
    }
}

void Buffer::unmapMemory() {
    if (m_mappedAddress != nullptr) {
        vmaUnmapMemory(m_device->allocator(), m_allocation);
        m_mappedAddress = nullptr;
    }
}

} // namespace scin

} // namespace vk

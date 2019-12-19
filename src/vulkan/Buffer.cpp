#include "vulkan/Buffer.hpp"

#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

#include <iostream>

namespace scin { namespace vk {

Buffer::Buffer(Kind kind, std::shared_ptr<Device> device):
    kind_(kind),
    device_(device),
    size_(0),
    buffer_(VK_NULL_HANDLE),
    device_memory_(VK_NULL_HANDLE),
    mapped_address_(nullptr) {}

Buffer::~Buffer() { Destroy(); }

bool Buffer::Create(size_t size) {
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    switch (kind_) {
    case kIndex:
        buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;

    case kUniform:
        buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;

    case kVertex:
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;

    default:
        spdlog::error("unsupported buffer type.");
        return false;
    }

    if (vkCreateBuffer(device_->get(), &buffer_info, nullptr, &buffer_) != VK_SUCCESS) {
        std::cerr << "Vulkan create buffer call failed." << std::endl;
        return false;
    }

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device_->get(), buffer_, &requirements);

    // Actual size of buffer may exceed requested size due to alignment or other allocation constraints.
    size_ = requirements.size;

    // TODO: consider relocating to device, or centralized memory manager.
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(device_->get_physical(), &properties);

    uint32_t type_index;
    for (type_index = 0; type_index < properties.memoryTypeCount; ++type_index) {
        if ((requirements.memoryTypeBits & (1 << type_index))
            && ((properties.memoryTypes[type_index].propertyFlags &
                 // TODO: optimum flag choice based on access patterns.
                 (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            break;
        }
    }

    if (type_index >= properties.memoryTypeCount) {
        std::cerr << "couldn't find appropriate memory in create buffer." << std::endl;
        return false;
    }

    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = size_;
    info.memoryTypeIndex = type_index;

    if (vkAllocateMemory(device_->get(), &info, nullptr, &device_memory_) != VK_SUCCESS) {
        std::cerr << "Vulkan buffer memory allocation failed." << std::endl;
        return false;
    }

    vkBindBufferMemory(device_->get(), buffer_, device_memory_, 0);

    return true;
}

void Buffer::Destroy() {
    UnmapMemory();

    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_->get(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }

    if (device_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_->get(), device_memory_, nullptr);
        device_memory_ = VK_NULL_HANDLE;
    }
}

void Buffer::MapMemory() {
    if (mapped_address_ == nullptr) {
        vkMapMemory(device_->get(), device_memory_, 0, size_, 0, &mapped_address_);
    }
}

void Buffer::UnmapMemory() {
    if (mapped_address_ != nullptr) {
        vkUnmapMemory(device_->get(), device_memory_);
        mapped_address_ = nullptr;
    }
}

} // namespace scin

} // namespace vk

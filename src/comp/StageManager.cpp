#include "comp/StageManager.hpp"

#include "vulkan/CommandBuffer.hpp"
#include "vulkan/Device.hpp"
#include "vulkan/Image.hpp"

namespace scin { namespace comp {

StageManager::StageManager(std::shared_ptr<vk::Device> device): m_device(device) {}

StageManager::~StageManager() {}

} // namespace comp
} // namespace scin

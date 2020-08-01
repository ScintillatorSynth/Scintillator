#include "comp/ImageMap.hpp"

#include "vulkan/Image.hpp"

namespace scin { namespace comp {

void ImageMap::addImage(size_t imageID, std::shared_ptr<vk::DeviceImage> image) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_images[imageID] = image;
}

std::shared_ptr<vk::DeviceImage> ImageMap::getImage(size_t imageID) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::shared_ptr<vk::DeviceImage> image;
    auto it = m_images.find(imageID);
    if (it != m_images.end()) {
        image = it->second;
    }
    return image;
}

void ImageMap::removeImage(size_t imageID) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_images.erase(imageID);
}

} // namespace comp
} // namespace scin

#ifndef SRC_COMP_IMAGE_MAP_HPP_
#define SRC_COMP_IMAGE_MAP_HPP_

#include <memory>
#include <mutex>
#include <unordered_map>

namespace scin {

namespace vk {
class DeviceImage;
}

namespace comp {

class ImageMap {
public:
    ImageMap() = default;
    ~ImageMap() = default;

    // Will overwrite any existing image associated with imageID.
    void addImage(size_t imageID, std::shared_ptr<vk::DeviceImage> image);
    std::shared_ptr<vk::DeviceImage> getImage(size_t imageID);
    void removeImage(size_t imageID);

    // Returns a 1x1 transparent black image.
    std::shared_ptr<vk::DeviceImage> getEmptyImage() const { return m_emptyImage; }
    // Called by the Compositor after it has staged the empty image.
    void setEmptyImage(std::shared_ptr<vk::DeviceImage> emptyImage) { m_emptyImage = emptyImage; }

private:
    std::mutex m_mutex;
    std::unordered_map<size_t, std::shared_ptr<vk::DeviceImage>> m_images;
    std::shared_ptr<vk::DeviceImage> m_emptyImage;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_IMAGE_MAP_HPP_

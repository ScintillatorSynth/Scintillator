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
    void addImage(int imageID, std::shared_ptr<vk::DeviceImage> image);
    std::shared_ptr<vk::DeviceImage> getImage(int imageID);
    void removeImage(int imageID);

private:
    std::mutex m_mutex;
    std::unordered_map<int, std::shared_ptr<vk::DeviceImage>> m_images;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_IMAGE_MAP_HPP_

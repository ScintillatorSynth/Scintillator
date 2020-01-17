#ifndef SRC_AV_FRAME_HPP_
#define SRC_AV_FRAME_HPP_

namespace scin { namespace av {

class Frame {
public:

    size_t sizeInBytes();
    uint8_t* data();
private:
};

} // namespace av

} // namespace scin

#endif  // SRC_AV_FRAME_HPP_

#ifndef SRC_AV_FRAME_HPP_
#define SRC_AV_FRAME_HPP_

#include <memory>

namespace scin { namespace av {

class Buffer;

/*! RAII-style AVFrame wrapper.
 */
class Frame {
public:
    Frame(std::shared_ptr<Buffer> buffer);
    ~Frame();

    bool create(int width, int height);

    AVFrame* get() { return m_frame; }

private:
    std::shared_ptr<Buffer> m_buffer;
    AVFrame* m_frame;
};

} // namespace av

} // namespace scin

#endif  // SRC_AV_FRAME_HPP_

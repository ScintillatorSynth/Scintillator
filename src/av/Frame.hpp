#ifndef SRC_AV_FRAME_HPP_
#define SRC_AV_FRAME_HPP_

#include "av/AVIncludes.hpp"

#include <memory>

namespace scin { namespace av {

class Buffer;

/*! RAII-style AVFrame wrapper.
 */
class Frame {
public:
    Frame();
    ~Frame();

    bool createFromBuffer(std::shared_ptr<Buffer> buffer);
    bool createEmpty();
    void destroy();

    AVFrame* get() { return m_frame; }

private:
    std::shared_ptr<Buffer> m_buffer;
    AVFrame* m_frame;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_FRAME_HPP_

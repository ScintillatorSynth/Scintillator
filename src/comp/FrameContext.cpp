#include "comp/FrameContext.hpp"

namespace scin { namespace comp {

FrameContext::FrameContext(size_t imageIndex): m_imageIndex(imageIndex), m_frameTime(0) {}

void FrameContext::reset(double frameTime) {
    m_frameTime = frameTime;
    m_scinths.clear();
    m_computeCommands.clear();
    m_drawCommands.clear();
    m_images.clear();
    m_computePrimary = nullptr;
    m_drawPrimary = nullptr;
}

} // namespace comp
} // namespace scin

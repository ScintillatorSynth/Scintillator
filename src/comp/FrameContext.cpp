#include "comp/FrameContext.hpp"

namespace scin { namespace comp {

FrameContext::FrameContext(): m_imageIndex(0), m_frameTime(0) {}

void FrameContext::reset(size_t imageIndex, double frameTime) {
    m_imageIndex = imageIndex;
    m_frameTime = frameTime;
    m_nodes.clear();
    m_computeCommands.clear();
    m_drawCommands.clear();
    m_images.clear();
    m_computePrimary = nullptr;
    m_drawPrimary = nullptr;
}

} // namespace comp
} // namespace scin

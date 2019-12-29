#ifndef SRC_COMPOSITOR_HPP_
#define SRC_COMPOSITOR_HPP_

namespace scin {

/*! A Compositor keeps the ScinthDef instance dictionary as well as all running Scinths. It can render on command to a
 * supplied FrameBuffer, which is typically owned by either a Window/SwapChain combination or an Offscreen render pass
 * in the case of non-realtime rendering.
 */
class Compositor {
};

} // namespace scin

#endif // SRC_COMPOSITOR_HPP_

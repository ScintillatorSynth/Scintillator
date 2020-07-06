#ifndef SRC_BASE_RENDER_OPTIONS_HPP_
#define SRC_BASE_RENDER_OPTIONS_HPP_

namespace scin { namespace base {

class RenderOptions {
public:
    RenderOptions();
    ~RenderOptions() = default;
    RenderOptions(const RenderOptions&) = default;

    enum PolygonMode { kFill, kLine, kPoint };
    PolygonMode polygonMode() const { return m_polygonMode; }
    void setPolygonMode(PolygonMode mode) { m_polygonMode = mode; }

private:
    PolygonMode m_polygonMode;
};

} // namespace base
} // namespace scin

#endif // SRC_BASE_RENDER_OPTIONS_HPP_

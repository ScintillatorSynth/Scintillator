#ifndef SRC_CORE_SHAPE_HPP_
#define SRC_CORE_SHAPE_HPP_

#include "base/Manifest.hpp"

#include "glm/glm.hpp"

namespace scin { namespace base {

/*! Abstract base class representing a geometric shape used as the starting point for rendering.
 *
 * Note that additional shape support requires adding parsing in Archetypes.
 */
class Shape {
public:
    Shape();
    virtual ~Shape();

    virtual Manifest::ElementType elementType() const = 0;
    virtual uint32_t numberOfVertices() const = 0;
    virtual uint32_t numberOfIndices() const = 0;

    enum Topology { kTriangleStrip };
    virtual Topology topology() const = 0;

    /*! Populate the provided (likely memory-mapped) buffer with vertex data according to the provided manifest.
     *
     * \param vertexManifest The vertex manifest to follow when filling per-vertex data.
     * \param normPosSclae For any NormPos intrinsics we apply this additional scaling factor.
     * \param store A pointer to the buffer to populate. Needs to be at least numberOfVertices() * per-vertex manifest
     *        size in floats.
     *
     * \return true on success, false on failure.
     */
    virtual bool storeVertexData(const Manifest& vertexManifest, const glm::vec2& normPosScale, float* store) const = 0;


    /*! Populate the provided (likely memory-mapped) buffer with index data.
     *
     * \param store A pointer to the buffer to fill with index data. Should be at least numberOfIndices() shorts in
     *        size.
     *
     * \return true on success, false on failure.
     */
    virtual bool storeIndexData(uint16_t* store) const = 0;
};

/*! Quad class by default is a 2D shape spanning the entire clip space. Can be tessellated to arbitrary number of
 * vertices in width and height.
 *
 *  See reference at https://www.learnopengles.com/tag/triangle-strips/
 */
class Quad : public Shape {
public:
    Quad(int widthEdges, int heightEdges);
    virtual ~Quad();

    Manifest::ElementType elementType() const override;
    uint32_t numberOfVertices() const override;
    uint32_t numberOfIndices() const override;
    Shape::Topology topology() const override;

    bool storeVertexData(const Manifest& vertexManifest, const glm::vec2& normPosScale, float* store) const override;
    bool storeIndexData(uint16_t* store) const override;

private:
    int m_widthEdges;
    int m_heightEdges;
};

} // namespace base

} // namespace scin

#endif // SRC_CORE_SHAPE_HPP_

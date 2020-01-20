#ifndef SRC_CORE_SHAPE_HPP_
#define SRC_CORE_SHAPE_HPP_

#include "core/Manifest.hpp"

namespace scin { namespace core {

/*! Abstract base class representing a geometric shape used as the starting point for rendering.
 */
class Shape {
public:
    Shape();
    virtual ~Shape();

    enum Topology { kTriangleStrip };

    virtual Manifest::ElementType elementType() const = 0;
    virtual uint32_t numberOfVertices() const = 0;
    virtual uint32_t numberOfIndices() const = 0;
    virtual Topology topology() const = 0;

    /*! Copy the canonical vertex data for this shape into the provided output buffer.
     *
     * \param index Which index in the overall vertex array, from [0, numberOfVertices - 1].
     * \param store Where to copy the position data to.
     * \return The number of floats copied to store.
     */
    virtual size_t storeVertexAtIndex(uint32_t index, float* store) const = 0;

    /*! Index buffers are not normally interleaved with other data, so we can provide direct access to a read-only
     * copy of the indices.
     *
     * \return A pointer to the index data, always assumed to be uint16_t for now, and with numberOfIndices() elements.
     */
    virtual const uint16_t* getIndices() const = 0;
};

class Quad : public Shape {
public:
    Quad();
    virtual ~Quad();

    Manifest::ElementType elementType() const override;
    uint32_t numberOfVertices() const override;
    uint32_t numberOfIndices() const override;
    Shape::Topology topology() const override;
    size_t storeVertexAtIndex(uint32_t index, float* store) const override;
    const uint16_t* getIndices() const override;
};

} // namespace core

} // namespace scin

#endif // SRC_CORE_SHAPE_HPP_

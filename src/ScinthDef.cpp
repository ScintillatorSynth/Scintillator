#include "ScinthDef.hpp"

#include "Scinth.hpp"
#include "core/AbstractScinthDef.hpp"
#include "core/Intrinsic.hpp"
#include "core/Shape.hpp"
#include "core/VGen.hpp"
#include "vulkan/Buffer.hpp"
#include "vulkan/Canvas.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/ShaderCompiler.hpp"
#include "vulkan/UniformLayout.hpp"

#include "glm/glm.hpp"
#include "spdlog/spdlog.h"

#include <array>

namespace scin {

ScinthDef::ScinthDef(std::shared_ptr<vk::Device> device, std::shared_ptr<vk::Canvas> canvas,
                     std::shared_ptr<vk::CommandPool> commandPool,
                     std::shared_ptr<const core::AbstractScinthDef> abstractScinthDef):
    m_device(device),
    m_canvas(canvas),
    m_commandPool(commandPool),
    m_abstractScinthDef(abstractScinthDef) {}

ScinthDef::~ScinthDef() { spdlog::debug("ScinthDef destructor"); }

bool ScinthDef::build(vk::ShaderCompiler* compiler) {
    // Build the vertex data. Because Intrinsics can add data payloads to the vertex data, each ScinthDef shares a
    // vertex buffer and index buffer across all Scinth instances, allowing for the potential unique combination
    // between Shape data and payloads.
    if (!buildVertexData()) {
        spdlog::error("error building vertex data for ScinthDef {}.", m_abstractScinthDef->name());
        return false;
    }

    m_vertexShader = compiler->compile(m_device, m_abstractScinthDef->vertexShader(),
                                       m_abstractScinthDef->prefix() + "_vertexShader", "main", vk::Shader::kVertex);
    if (!m_vertexShader) {
        spdlog::error("error compiling vertex shader for ScinthDef {}.", m_abstractScinthDef->name());
        return false;
    }

    m_fragmentShader =
        compiler->compile(m_device, m_abstractScinthDef->fragmentShader(),
                          m_abstractScinthDef->prefix() + "_fragmentShader", "main", vk::Shader::kFragment);
    if (!m_fragmentShader) {
        spdlog::error("error compiling fragment shader for ScinthDef {}", m_abstractScinthDef->name());
        return false;
    }

    if (m_abstractScinthDef->uniformManifest().sizeInBytes()) {
        m_uniformLayout.reset(new vk::UniformLayout(m_device));
        if (!m_uniformLayout->create()) {
            spdlog::error("failed creating uniform layout for ScinthDef {}", m_abstractScinthDef->name());
            return false;
        }
    }

    m_pipeline.reset(new vk::Pipeline(m_device));
    if (!m_pipeline->create(m_abstractScinthDef->vertexManifest(), m_abstractScinthDef->shape(), m_canvas.get(),
                            m_vertexShader, m_fragmentShader, m_uniformLayout)) {
        spdlog::error("error creating pipeline for ScinthDef {}", m_abstractScinthDef->name());
        return false;
    }

    return true;
}

// static
std::shared_ptr<Scinth> ScinthDef::play(std::shared_ptr<ScinthDef> scinthDef, int nodeID, const TimePoint& startTime) {
    std::shared_ptr<Scinth> scinth(new Scinth(scinthDef->m_device, nodeID, scinthDef));
    if (!scinth->create(startTime, scinthDef->m_uniformLayout.get(), scinthDef->m_canvas->numberOfImages())) {
        spdlog::error("failed to create Scinth {} from ScinthDef {}", nodeID, scinthDef->m_abstractScinthDef->name());
        return nullptr;
    }
    if (!scinth->buildBuffers(scinthDef->m_commandPool.get(), scinthDef->m_canvas.get(), scinthDef->m_vertexBuffer,
                              scinthDef->m_indexBuffer, scinthDef->m_pipeline)) {
        spdlog::error("failed to build command buffers on Scinth {} from ScinthDef {}", nodeID,
                      scinthDef->m_abstractScinthDef->name());
        return nullptr;
    }
    return scinth;
}

bool ScinthDef::buildVertexData() {
    // The kNormPos intrinsic applies a scale to the input vertices, but only makes sense for 2D verts.
    glm::vec2 normPosScale;
    if (m_abstractScinthDef->intrinsics().count(core::Intrinsic::kNormPos)) {
        if (m_abstractScinthDef->shape()->elementType() != core::Manifest::ElementType::kVec2) {
            spdlog::error("normpos intrinsic only supported for 2D vertices in ScinthDef {}.",
                          m_abstractScinthDef->name());
            return false;
        }
        if (m_canvas->width() > m_canvas->height()) {
            normPosScale.x = static_cast<float>(m_canvas->width()) / static_cast<float>(m_canvas->height());
            normPosScale.y = 1.0f;
        } else {
            normPosScale.x = 1.0f;
            normPosScale.y = static_cast<float>(m_canvas->height()) / static_cast<float>(m_canvas->width());
        }
    }

    // Build the vertex data based on the manifest and the shape.
    size_t numberOfFloats = m_abstractScinthDef->shape()->numberOfVertices()
        * (m_abstractScinthDef->vertexManifest().sizeInBytes() / sizeof(float));
    // TODO: memalign or std::align?
    std::unique_ptr<float[]> vertexData(new float[numberOfFloats]);
    float* vertex = vertexData.get();
    for (auto i = 0; i < m_abstractScinthDef->shape()->numberOfVertices(); ++i) {
        for (auto j = 0; j < m_abstractScinthDef->vertexManifest().numberOfElements(); ++j) {
            // If this is the Shape position vertex data get from the Shape, otherwise build from Intrinsics.
            if (m_abstractScinthDef->vertexManifest().nameForElement(j)
                == m_abstractScinthDef->vertexPositionElementName()) {
                m_abstractScinthDef->shape()->storeVertexAtIndex(i, vertex);
            } else {
                switch (m_abstractScinthDef->vertexManifest().intrinsicForElement(j)) {
                case core::Intrinsic::kNormPos: {
                    // TODO: would it be faster/easier to just provide normPosScale in UBO and do this on
                    // the vertex shader?
                    std::array<float, 2> verts;
                    m_abstractScinthDef->shape()->storeVertexAtIndex(i, verts.data());
                    vertex[0] = verts[0] * normPosScale.x;
                    vertex[1] = verts[1] * normPosScale.y;
                } break;

                case core::Intrinsic::kPi:
                    spdlog::error("pi is not a valid vertex intrinsic for ScinthDef {}", m_abstractScinthDef->name());
                    return false;

                case core::Intrinsic::kTime:
                    spdlog::error("time is not a valid vertex intrinsic for ScinthDef {}", m_abstractScinthDef->name());
                    return false;

                case core::Intrinsic::kNotFound:
                    spdlog::error("unknown intrinsic in vertex manifest for ScinthDef {}", m_abstractScinthDef->name());
                    return false;
                }
            }

            // Advance vertex pointer to next element.
            vertex += (m_abstractScinthDef->vertexManifest().strideForElement(j) / sizeof(float));
        }
    }

    // Vertex data now populated in host memory, copy to a host-accessible buffer.
    m_vertexBuffer.reset(new vk::HostBuffer(vk::Buffer::Kind::kVertex, numberOfFloats * sizeof(float), m_device));
    if (!m_vertexBuffer->create()) {
        spdlog::error("error creating vertex buffer for ScinthDef {}", m_abstractScinthDef->name());
        return false;
    }
    spdlog::info("copying {} bytes of vertex data to GPU for ScinthDef {}", m_vertexBuffer->size(),
                 m_abstractScinthDef->name());
    m_vertexBuffer->copyToGPU(vertexData.get());

    // Lastly, copy the index buffer as well.
    m_indexBuffer.reset(new vk::HostBuffer(
        vk::Buffer::Kind::kIndex, m_abstractScinthDef->shape()->numberOfIndices() * sizeof(uint16_t), m_device));
    if (!m_indexBuffer->create()) {
        spdlog::error("error creating index buffer for ScinthDef {}", m_abstractScinthDef->name());
        return false;
    }
    spdlog::info("copying {} bytes of index data to GPU for ScinthDef {}", m_indexBuffer->size(),
                 m_abstractScinthDef->name());
    m_indexBuffer->copyToGPU(m_abstractScinthDef->shape()->getIndices());
    // TODO: investigate if device-only copies of these buffers are faster?
    return true;
}

} // namespace scin

#include "comp/RootNode.hpp"

#include "comp/Canvas.hpp"
#include "vulkan/Device.hpp"

#include <spdlog/spdlog.h>

namespace scin { namespace comp {

RootNode::RootNode(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas):
    Node(device, 0),
    m_canvas(canvas),
    m_shaderCompiler(new ShaderCompiler()),
    m_computeCommandPool(new vk::CommandPool(device)),
    m_drawCommandPool(new vk::CommandPool(device)),
    m_stageManager(new StageManager(device)),
    m_samplerFactory(new SamplerFactory(device)),
    m_imageMap(new ImageMap()),
    m_commandBufferDirty(true),
    m_nodeSerial(-2) {
    m_computeCommands.resize(m_canvas->numberOfImages());
    m_drawCommands.resize(m_canvas->numberOfImages());
}

bool RootNode::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::error("Compositor unable to load shader compiler.");
        return false;
    }

    if (!m_computeCommandPool->create()) {
        spdlog::error("Compositor failed creating compute command pool.");
    }

    if (!m_drawCommandPool->create()) {
        spdlog::error("Compositor failed creating draw command pool.");
        return false;
    }

    if (!m_stageManager->create(m_canvas->numberOfImages())) {
        spdlog::error("Compositor failed to create stage manager.");
        return false;
    }

    // Create the empty image and stage.
    std::shared_ptr<vk::HostBuffer> emptyImageBuffer(new vk::HostBuffer(m_device, vk::Buffer::Kind::kStaging, 4));
    if (!emptyImageBuffer->create()) {
        spdlog::error("Compositor failed to create empty staging image.");
        return false;
    }
    std::memset(emptyImageBuffer->mappedAddress(), 0, 4);

    std::shared_ptr<vk::DeviceImage> emptyImage(new vk::DeviceImage(m_device, VK_FORMAT_R8G8B8A8_UNORM));
    if (!emptyImage->create(1, 1)) {
        spdlog::error("Compositor failed to create empty device image.");
        return false;
    }

    if (!m_stageManager->stageImage(emptyImageBuffer, emptyImage, [this, emptyImage] {
            if (!emptyImage->createView()) {
                spdlog::error("Compositor failed to create ImageView for empty image");
                return;
            }
            m_imageMap->setEmptyImage(emptyImage);
            spdlog::info("Compositor finished staging the empty image");
        })) {
        spdlog::error("Compositor failed to stage the empty image.");
        return false;
    }

    rebuildCommandBuffer();
    return true;
}

void RootNode::destroy() {
    // Delete all command buffers outstanding first, which means emptying the Scinth map and list.
    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        m_scinthMap.clear();
        for (auto scinth : m_scinths) {
            scinth->destroy();
        }
        m_scinths.clear();
        m_audioStagers.clear();
    }
    m_computePrimary.reset();
    m_computeSecondary.clear();
    m_computeCommands.clear();

    m_drawPrimary.reset();
    m_drawSecondary.clear();
    m_drawCommands.clear();

    // We leave the command pools undestroyed as there may be outstanding commandbuffers pipelined. The shared_ptr
    // system should collect them before all is done. But we must remove our references to them or we keep them alive
    // until our own destructor.
    m_computeCommandPool = nullptr;
    m_drawCommandPool = nullptr;

    m_stageManager->destroy();
    m_imageMap = nullptr;
    m_samplerFactory = nullptr;

    // Now delete all of the ScinthDefs, which hold shared graphics resources.
    {
        std::lock_guard<std::mutex> lock(m_scinthDefMutex);
        m_scinthDefs.clear();
    }
}

bool RootNode::prepareFrame(uint32_t imageIndex, double frameTime) {
    m_computeSecondary.clear();
    m_drawSecondary.clear();

    {
        std::lock_guard<std::mutex> lock(m_scinthMutex);
        for (auto stager : m_audioStagers) {
            stager->stageAudio(m_stageManager);
        }

        for (auto scinth : m_scinths) {
            if (scinth->running()) {
                scinth->prepareFrame(imageIndex, frameTime);
                std::shared_ptr<vk::CommandBuffer> computeCommands = scinth->computeCommands();
                if (computeCommands) {
                    computeCommands->associateScinth(scinth);
                    m_computeSecondary.emplace_back(computeCommands);
                }
                std::shared_ptr<vk::CommandBuffer> drawCommands = scinth->drawCommands();
                drawCommands->associateScinth(scinth);
                m_drawSecondary.emplace_back(drawCommands);
            }
        }
    }

    m_computeCommands[imageIndex] = m_computeSecondary;
    m_drawCommands[imageIndex] = m_drawSecondary;

    if (m_commandBufferDirty) {
        rebuildCommandBuffer();
    }

    return m_drawPrimary != nullptr;
}


} // namespace comp
} // namespace scin

#include "Application.hpp"

#include "Core/Camera.hpp"
#include "Core/Input.hpp"
#include "Core/Util/FileUtils.hpp"
#include "Map/BuildingData.hpp"
#include "Map/HighwayData.hpp"
#include "Map/TileData.hpp"
#include "Map/OSMTileDataSource.hpp"
#include "Util/GeometryUtils.hpp"
#include "Vertex.hpp"
#include "Core/Vulkan/VulkanGraphicsPipelineBuilder.hpp"
#include "Core/Vulkan/VulkanContext.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"

#include <GLFW/glfw3.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>

const uint32_t SHADOW_MAP_WIDTH = 1024;
const uint32_t SHADOW_MAP_HEIGHT = 1024;

/**
 * @brief Constructor
 */
Application::Application()
    : m_isRunning(false)
    , m_camera()
    , m_workerThreadRunning(true)
    , m_retrieveTileJobs()
    , m_retrieveTileJobsMutex()
    , m_tilesUpdateMutex()
    , m_tilesUpdated(false)
{
}

/**
 * @brief Destructor
 */
Application::~Application()
{
}

/**
 * @brief Runs the application.
 */
void Application::Run()
{
    // If the application is somehow running when this
    // function is called, don't allow to "re-run".
    if (m_isRunning)
    {
        return;
    }
    m_isRunning = true;

    if (!Init())
    {
        std::cout << "[Application] Failed to initialize application!" << std::endl;
        return;
    }

    const uint32_t MAX_VERTEX_COUNT = 1000000;
    if (!m_testVertexBuffer.Create(sizeof(Vertex) * MAX_VERTEX_COUNT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        std::cerr << "Failed to create vertex buffer!" << std::endl;
    }

    const int ZOOM_LEVEL = 16;
    glm::ivec2 tileIndex = GeometryUtils::LonLatToTileIndex(139.75, 35.6, 16);
    UpdateCurrentTile(tileIndex);

    double prevTime = glfwGetTime();

    m_camera.SetFieldOfView(60.0f);
    m_camera.SetAspectRatio(m_window.GetWidth() * 1.0f / m_window.GetHeight());
    m_camera.SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
    m_camera.SetWorldUpVector(glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 dirLightDirection = glm::vec3(0.0f, -1.0f, 1.0f);

    m_workerThreadRunning = true;
    std::thread workerThread1(std::bind(&Application::WorkerThreadFunc, this, true)); // Worker thread that downloads data if needed
    std::thread workerThread2(std::bind(&Application::WorkerThreadFunc, this, false)); // Worker thread that only focuses on cached tiles

    uint32_t currentFrame = 0;

    // Game loop
    while (!m_window.IsClosed())
    {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - prevTime);
        prevTime = currentTime;

        // --- Camera input ---
        glm::vec3 cameraMovement(0.0f);
        if (Input::IsKeyDown(Input::Key::W))
        {
            cameraMovement.z =  1.0f;
        }
        if (Input::IsKeyDown(Input::Key::S))
        {
            cameraMovement.z = -1.0f;
        }
        if (Input::IsKeyDown(Input::Key::A))
        {
            cameraMovement.x = -1.0f;
        }
        if (Input::IsKeyDown(Input::Key::D))
        {
            cameraMovement.x =  1.0f;
        }
        if (glm::dot(cameraMovement, cameraMovement) > 0.0f)
        {
            cameraMovement = glm::normalize(cameraMovement);
        }
        cameraMovement *= 10.0f * deltaTime;

        float yaw = m_camera.GetYaw() + Input::GetMouseDeltaX() * 0.25f;
        float pitch = m_camera.GetPitch() - Input::GetMouseDeltaY() * 0.25f;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
        m_camera.SetYaw(yaw);
        m_camera.SetPitch(pitch);

        m_camera.SetPosition
        (
            m_camera.GetPosition() + cameraMovement.x * m_camera.GetRightVector() + cameraMovement.z * m_camera.GetForwardVector()
        );

        glm::dvec2 playerWorldPosition = { m_camera.GetPosition().x / SCALE, m_camera.GetPosition().z / SCALE };
        playerWorldPosition += GeometryUtils::LonLatToXY(m_origin);
        glm::dvec2 playerLonLat = GeometryUtils::XYToLonLat(playerWorldPosition.x, playerWorldPosition.y);
        glm::ivec2 newTileIndex = GeometryUtils::LonLatToTileIndex(playerLonLat.x, playerLonLat.y, ZOOM_LEVEL);
        if (newTileIndex != m_currentTileIndex)
        {
            UpdateCurrentTile(newTileIndex);

            // Readjust player position to the new origin
            glm::dvec2 originXY = GeometryUtils::LonLatToXY(m_origin);
            playerWorldPosition -= originXY;
            playerWorldPosition *= SCALE;
            m_camera.SetPosition(glm::vec3(playerWorldPosition.x, m_camera.GetPosition().y, playerWorldPosition.y));
        }

        if (m_tilesUpdateMutex.try_lock())
        {
            if (m_tilesUpdated)
            {
                std::vector<Vertex> vertices;

                const uint32_t MAX_VERTEX_COUNT = 1000000;
                Vertex *data = reinterpret_cast<Vertex*>(m_testVertexBuffer.MapMemory(0, MAX_VERTEX_COUNT * sizeof(Vertex)));

                m_numVertices = 0;
                for (size_t i = 0; i < m_activeTiles.size(); ++i)
                {
                    vertices.clear();
                    AppendTileGeometryVertices(m_activeTiles[i], m_origin, vertices);
                    memcpy(data + m_numVertices, vertices.data(), sizeof(Vertex) * vertices.size());
                    m_numVertices += vertices.size();
                }

                m_testVertexBuffer.UnmapMemory();

                m_tilesUpdated = false;
            }

            m_tilesUpdateMutex.unlock();
        }

        // --- Draw frame start ---

        // Wait for the current frame to be done rendering
        vkWaitForFences
        (
            VulkanContext::GetLogicalDevice(),
            1,
            &m_frameDataList[currentFrame].renderDoneFence,
            VK_TRUE,
            UINT64_MAX
        );
        vkResetFences
        (
            VulkanContext::GetLogicalDevice(), 
            1, 
            &m_frameDataList[currentFrame].renderDoneFence
        );

        uint32_t nextImageIndex;
        VkResult acquireImageResult = vkAcquireNextImageKHR
        (
            VulkanContext::GetLogicalDevice(), 
            m_vkSwapchain, 
            UINT64_MAX, 
            m_frameDataList[currentFrame].imageAvailableSemaphore, 
            VK_NULL_HANDLE, 
            &nextImageIndex
        );
        if ((acquireImageResult != VK_SUCCESS) && (acquireImageResult != VK_SUBOPTIMAL_KHR))
        {
            std::cerr << "Failed to acquire next image!" << std::endl;
            glfwSetWindowShouldClose(m_window.GetHandle(), GLFW_TRUE);
            continue;
        }

        // --- Build render commands ---
        
        // Start command buffer recording
        VkCommandBuffer &commandBuffer = m_frameDataList[currentFrame].commandBuffer;
        vkResetCommandBuffer(commandBuffer, 0);
        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        {
            std::cout << "Failed to begin recording command buffer!" << std::endl;
            glfwSetWindowShouldClose(m_window.GetHandle(), GLFW_TRUE);
            continue;
        }

        glm::mat4 lightProj = glm::orthoRH_ZO(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
        lightProj[1][1] *= -1.0f;
        glm::mat4 lightView = glm::lookAt(m_camera.GetPosition() - dirLightDirection * 5.0f, m_camera.GetPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightMatrix = lightProj * lightView;
        {
            // Begin shadow pass
            VkRenderPassBeginInfo shadowPassBeginInfo {};
            shadowPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            shadowPassBeginInfo.renderPass = m_shadowRenderPass;
            shadowPassBeginInfo.framebuffer = m_frameDataList[currentFrame].shadowMapFramebuffer;
            shadowPassBeginInfo.renderArea.offset = { 0, 0 };
            shadowPassBeginInfo.renderArea.extent = { SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT };
            std::array<VkClearValue, 1> shadowClearValues;
            shadowClearValues[0].depthStencil = { 1.0f, 0 };
            shadowPassBeginInfo.clearValueCount = static_cast<uint32_t>(shadowClearValues.size());
            shadowPassBeginInfo.pClearValues = shadowClearValues.data();
            vkCmdBeginRenderPass(commandBuffer, &shadowPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Viewport and scissors are dynamic, so we set here as a command
            float viewportHeight = static_cast<float>(SHADOW_MAP_HEIGHT);
            VkViewport viewport {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(SHADOW_MAP_WIDTH);
            viewport.height = viewportHeight;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor {};
            scissor.offset = { 0, 0 };
            scissor.extent = { SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT } ;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline);

            VkDeviceSize offset = 0;
            VkBuffer vertexBuffers[] = { m_testVertexBuffer.GetHandle() };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &offset);
            
            // Push constants
            PushConstant pushConstant;
            pushConstant.projView = lightMatrix;
            vkCmdPushConstants(commandBuffer, m_shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &pushConstant);

            vkCmdDraw(commandBuffer, m_numVertices, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);
        }

        // Begin render pass
        VkRenderPassBeginInfo renderPassBeginInfo {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = m_vkRenderPass;
        renderPassBeginInfo.framebuffer = m_frameDataList[currentFrame].framebuffer;
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = m_vkSwapchainImageExtent;
        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Viewport and scissors are dynamic, so we set here as a command
        float viewportHeight = static_cast<float>(m_vkSwapchainImageExtent.height);
        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_vkSwapchainImageExtent.width);
        viewport.height = viewportHeight; // Supported from Vulkan 1.1 to switch from Y=down convention
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor {};
        scissor.offset = { 0, 0 };
        scissor.extent = m_vkSwapchainImageExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline);

        VkDeviceSize offset = 0;
        VkBuffer vertexBuffers[] = { m_testVertexBuffer.GetHandle() };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &offset);

        // Update camera UBO
        CameraData *cameraDataUBO = reinterpret_cast<CameraData*>(m_frameDataList[currentFrame].cameraDataUniformBuffer.MapMemory(0, sizeof(CameraData)));
        cameraDataUBO->position = m_camera.GetPosition();
        m_frameDataList[currentFrame].cameraDataUniformBuffer.UnmapMemory();

        // Update LightData UBO
        LightData *lightDataUBO = reinterpret_cast<LightData*>(m_frameDataList[currentFrame].lightDataUniformBuffer.MapMemory(0, sizeof(LightData)));
        lightDataUBO->lightPosition = glm::vec4(dirLightDirection, 0.0f);
        lightDataUBO->ambient = { 0.1f, 0.1f, 0.1f };
        lightDataUBO->diffuse = { 1.0f, 1.0f, 1.0f };
        lightDataUBO->specular = { 1.0f, 1.0f, 1.0 };
        m_frameDataList[currentFrame].lightDataUniformBuffer.UnmapMemory();

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout, 0, 1, &m_frameDataList[currentFrame].descriptorSet, 0, nullptr);

        glm::mat4 projView = m_camera.GetProjectionMatrix() * m_camera.GetViewMatrix();
        // Push constants
        PushConstant pushConstant;
        pushConstant.lightProjView = lightMatrix;
        pushConstant.projView = projView;
        vkCmdPushConstants(commandBuffer, m_vkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &pushConstant);
        vkCmdDraw(commandBuffer, m_numVertices, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            std::cerr << "Failed to end recording of command buffer!" << std::endl;
            glfwSetWindowShouldClose(m_window.GetHandle(), GLFW_TRUE);
            continue;
        }

        // --- Submit ---
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_frameDataList[currentFrame].imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = { m_frameDataList[currentFrame].renderDoneSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitResult = vkQueueSubmit
        (
            VulkanContext::GetGraphicsQueue(),
            1,
            &submitInfo,
            m_frameDataList[currentFrame].renderDoneFence
        );
        if (submitResult != VK_SUCCESS)
        {
            std::cerr << "Failed to submit!" << std::endl;
            glfwSetWindowShouldClose(m_window.GetHandle(), GLFW_TRUE);
            continue;
        }

        // --- Present ---
        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { m_vkSwapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &nextImageIndex;

        currentFrame = (currentFrame + 1) % m_maxFramesInFlight;

        VkResult presentResult = vkQueuePresentKHR
        (
            VulkanContext::GetPresentQueue(),
            &presentInfo
        );
        if (presentResult != VK_SUCCESS)
        {
            std::cerr << "Failed to present!" << std::endl;
            glfwSetWindowShouldClose(m_window.GetHandle(), GLFW_TRUE);
            continue;
        }

        // --- Draw frame end ---

        //m_sceneManager.GetActiveScene()->Draw();

        Input::Prepare();
        m_window.PollEvents();
    }

    vkDeviceWaitIdle(VulkanContext::GetLogicalDevice());

    m_workerThreadRunning = false;
    workerThread1.join();
    workerThread2.join();

    Cleanup();
}

/**
 * @brief Initializes the application.
 * @return Returns true if the initialization was successful.
 * Returns false otherwise.
 */
bool Application::Init()
{
    if (glfwInit() == GLFW_FALSE)
    {
        std::cout << "[Application] Failed to initialize GLFW!" << std::endl;
        return false;
    }

    if (!m_window.Init(800, 600, "Map Viewer 3D"))
    {
        std::cout << "[Application] Failed to create GLFW window!" << std::endl;
        return false;
    }

    // --- Register callbacks ---
    GLFWwindow* windowHandle = m_window.GetHandle();

    if (!VulkanContext::Initialize(windowHandle))
    {
        std::cerr << "[Application] Failed to initialize Vulkan context!" << std::endl;
    }
    if (!InitSwapchain())
    {
        std::cerr << "[Application] Failed to initialize Vulkan swapchain!" << std::endl;
    }
    if (!InitShadowPass())
    {
        std::cerr << "[Application] Failed to initialize resources for shadow pass!" << std::endl;
    }
    if (!InitRenderPass())
    {
        std::cerr << "[Application] Failed to initialize Vulkan renderpass!" << std::endl;
    }
    if (!InitDepthStencilImage())
    {
        std::cerr << "[Application] Failed to initialize Vulkan depth/stencil image!" << std::endl;
    }
    if (!InitFramebuffers())
    {
        std::cerr << "[Application] Failed to initialize Vulkan framebuffers!" << std::endl;
    }
    if (!InitCommandPool())
    {
        std::cerr << "[Application] Failed to initialize Vulkan command pool" << std::endl;
    }
    if (!InitCommandBuffers())
    {
        std::cerr << "[Application] Failed to initialize Vulkan command buffers!" << std::endl;
    }
    if (!InitDescriptors())
    {
        std::cerr << "[Application] Failed to initialize descriptor sets!" << std::endl;
    }
    if (!InitGraphicsPipeline())
    {
        std::cerr << "[Application] Failed to initialize graphics pipeline!" << std::endl;
    }
    if (!InitSynchronizationTools())
    {
        std::cerr << "[Application] Failed to initialize synchronization tools!" << std::endl;
    }

    // Set the callback function for when the framebuffer size changed
	//glfwSetFramebufferSizeCallback(windowHandle, Application::FramebufferSizeChangedCallback);

    // Disable the cursor.
    // TODO: Maybe have a function inside the window class for this
    glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the callback function for when a key was pressed
	glfwSetKeyCallback(windowHandle, Input::KeyCallback);

	// Register callback function for when a mouse button was pressed
	glfwSetMouseButtonCallback(windowHandle, Input::MouseButtonCallback);

    // Register callback function for when the mouse scroll wheel was scrolled
    glfwSetScrollCallback(windowHandle, Input::MouseScrollCallback);

	// Register callback function for when the mouse cursor's position changed
	glfwSetCursorPosCallback(windowHandle, Input::CursorCallback);

	// Register callback function for when the mouse cursor entered/left the window
	glfwSetCursorEnterCallback(windowHandle, Input::CursorEnterCallback);

    return true;
}

/**
 * @brief Cleans up the application.
 */
void Application::Cleanup()
{
    m_testVertexBuffer.Cleanup();

    for (size_t i = 0; i < m_frameDataList.size(); i++)
    {
        vkDestroyFence(VulkanContext::GetLogicalDevice(), m_frameDataList[i].renderDoneFence, nullptr);
        vkDestroySemaphore(VulkanContext::GetLogicalDevice(), m_frameDataList[i].imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(VulkanContext::GetLogicalDevice(), m_frameDataList[i].renderDoneSemaphore, nullptr);
    }

    vkDestroyPipeline(VulkanContext::GetLogicalDevice(), m_shadowPipeline, nullptr);
    m_shadowPipeline = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), m_shadowPipelineLayout, nullptr);
    m_shadowPipelineLayout = VK_NULL_HANDLE;
    vkDestroyPipeline(VulkanContext::GetLogicalDevice(), m_vkPipeline, nullptr);
    m_vkPipeline = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), m_vkPipelineLayout, nullptr);
    m_vkPipelineLayout = VK_NULL_HANDLE;

    for (size_t i = 0; i < m_frameDataList.size(); i++)
    {
        m_frameDataList[i].cameraDataUniformBuffer.Cleanup();
        m_frameDataList[i].lightDataUniformBuffer.Cleanup();
    }

    vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), m_vkDescriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), m_vkDescriptorSetLayout, nullptr);
    m_vkDescriptorSetLayout = VK_NULL_HANDLE;

    vkDestroySampler(VulkanContext::GetLogicalDevice(), m_shadowMapSampler, nullptr);
    m_shadowMapSampler = VK_NULL_HANDLE;

    m_vkDepthBufferImageView.Cleanup();
    m_vkDepthBufferImage.Cleanup();

    for (size_t i = 0; i < m_frameDataList.size(); i++)
    {
        vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), m_frameDataList[i].shadowMapFramebuffer, nullptr);
        m_frameDataList[i].shadowMapImageView.Cleanup();
        m_frameDataList[i].shadowMapImage.Cleanup();
        vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), m_frameDataList[i].framebuffer, nullptr);
        m_frameDataList[i].imageView.Cleanup();
        vkFreeCommandBuffers(VulkanContext::GetLogicalDevice(), m_vkCommandPool, 1, &m_frameDataList[i].commandBuffer);
    }
    m_frameDataList.clear();

    vkDestroyCommandPool(VulkanContext::GetLogicalDevice(), m_vkCommandPool, nullptr);
    m_vkCommandPool = VK_NULL_HANDLE;

    vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), m_vkRenderPass, nullptr);
    m_vkRenderPass = VK_NULL_HANDLE;
    vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), m_shadowRenderPass, nullptr);
    m_shadowRenderPass = VK_NULL_HANDLE;

    vkDestroySwapchainKHR(VulkanContext::GetLogicalDevice(), m_vkSwapchain, nullptr);
    m_vkSwapchain = VK_NULL_HANDLE;

    VulkanContext::Cleanup();

    m_window.Cleanup();
    glfwTerminate();
}

/**
 * @brief Initializes the Vulkan render pass.
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitSwapchain()
{
    // --- Preparations for swapchain creation ---

    // Query surface capabilities (can also do this as a criteria when selecting a physical device)
    // For now, we'll just do a delayed check.
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanContext::GetPhysicalDevice(), VulkanContext::GetVulkanSurface(), &surfaceCapabilities);

    // Query supported formats
    uint32_t numAvailableSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanContext::GetPhysicalDevice(), VulkanContext::GetVulkanSurface(), &numAvailableSurfaceFormats, nullptr);
    if (numAvailableSurfaceFormats == 0)
    {
        std::cout << "Selected physical device has no supported surface formats!" << std::endl;
        return false;
    }
    std::vector<VkSurfaceFormatKHR> availableSurfaceFormats(numAvailableSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanContext::GetPhysicalDevice(), VulkanContext::GetVulkanSurface(), &numAvailableSurfaceFormats, availableSurfaceFormats.data());

    // Query presentation modes
    uint32_t numAvailablePresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanContext::GetPhysicalDevice(), VulkanContext::GetVulkanSurface(), &numAvailablePresentModes, nullptr);
    if (numAvailablePresentModes == 0)
    {
        std::cout << "Selected physical device has no supported present modes!" << std::endl;
        return false;
    }
    std::vector<VkPresentModeKHR> availablePresentModes(numAvailablePresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanContext::GetPhysicalDevice(), VulkanContext::GetVulkanSurface(), &numAvailablePresentModes, availablePresentModes.data());

    // Select the preferred surface format.
    // Ideally, a format that supports BGRA with SRGB colorspace, but if we can't find such a format, just use the first one
    VkSurfaceFormatKHR selectedSurfaceFormat = availableSurfaceFormats[0];
    for (size_t i = 1; i < availableSurfaceFormats.size(); ++i)
    {
        if ((availableSurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && (availableSurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
        {
            selectedSurfaceFormat = availableSurfaceFormats[i];
            break;
        }
    }

    // Select preferred present mode.
    // By default, GPUs should support VK_PRESENT_MODE_FIFO_KHR at the bare minimum, but
    // if we can find VK_PRESENT_MODE_MAILBOX_KHR, then we use that
    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < availablePresentModes.size(); ++i)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            selectedPresentMode = availablePresentModes[i];
            break;
        }
    }

    // Calculate swapchain extent
    VkExtent2D swapchainImageExtent;
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        // Use the extent provided to us by Vulkan
        swapchainImageExtent = surfaceCapabilities.currentExtent;
    }
    else
    {
        // Use the framebuffer size provided by GLFW as the extent.
        // However, we make sure that the extent is within the capabilities of the surface
        int width, height;
        glfwGetFramebufferSize(m_window.GetHandle(), &width, &height);

        swapchainImageExtent.width = std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapchainImageExtent.height = std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

    // --- Create swapchain ---

    // Prepare swapchain create struct
    uint32_t numSwapchainImages = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0)
    {
        numSwapchainImages = std::min(numSwapchainImages, surfaceCapabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = VulkanContext::GetVulkanSurface();
    swapchainCreateInfo.minImageCount = numSwapchainImages;
    swapchainCreateInfo.imageFormat = selectedSurfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
    swapchainCreateInfo.presentMode = selectedPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE; // Clip pixels that are obscured, e.g., by other windows
    swapchainCreateInfo.imageExtent = swapchainImageExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Can use VK_IMAGE_USAGE_TRANSFER_DST_BIT for rendering to an offscreen buffer (e.g., post-processing)
    if (VulkanContext::GetGraphicsQueueIndex() != VulkanContext::GetPresentQueueIndex())
    {
        // Graphics queue and present queue are different (in some GPUs, they can be the same).
        // In this case, we use VK_SHARING_MODE_CONCURRENT, which means swapchain images can be owned by
        // multiple queue famillies without the need for transfer of ownership.
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

        // We also need to specify which queue families concurrently own a swapchain image 
        uint32_t indices[] = { VulkanContext::GetGraphicsQueueIndex(), VulkanContext::GetPresentQueueIndex() };
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = indices;
    }
    else
    {
        // If graphics queue and present queue are the same, set to exclusive mode since
        // we don't need to transfer ownership normally. This also offers the best performance.
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    // In case we want to apply a transformation to be applied to all swapchain images.
    // If not, we just use the current transformation of the surface capabilities struct.
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    // Blending with other windows. (We almost always ignore)
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // In case the previous swap chain becomes invalid. In that case, we need to supply the old swapchain
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Actually create the swapchain
    if (vkCreateSwapchainKHR(VulkanContext::GetLogicalDevice(), &swapchainCreateInfo, nullptr, &m_vkSwapchain) != VK_SUCCESS)
    {
        std::cout << "Failed to create swapchain!" << std::endl;
        return false;
    }

    // Retrieve swapchain images, and store the image format and extent
    vkGetSwapchainImagesKHR(VulkanContext::GetLogicalDevice(), m_vkSwapchain, &numSwapchainImages, nullptr);
    std::vector<VkImage> swapchainImages;
    swapchainImages.resize(numSwapchainImages);
    vkGetSwapchainImagesKHR(VulkanContext::GetLogicalDevice(), m_vkSwapchain, &numSwapchainImages, swapchainImages.data());
    m_vkSwapchainImageFormat = selectedSurfaceFormat.format;
    m_vkSwapchainImageExtent = swapchainImageExtent;

    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        m_frameDataList.emplace_back();

        m_frameDataList.back().image = swapchainImages[i];
        if (!m_frameDataList.back().imageView.Create(swapchainImages[i], m_vkSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT))
        {
            std::cout << "Failed to create swapchain image views!" << std::endl;
            return false;
        }
    }

    m_maxFramesInFlight = static_cast<uint32_t>(swapchainImages.size());

    return true;
}

/**
 * @brief Initializes all things needed for a shadow pass
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitShadowPass()
{
    // --- Render pass ---

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 0;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 0;
    subpassDescription.pColorAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

    VkSubpassDependency depthDependency = {};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstSubpass = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 1> attachments = { depthAttachment };
    std::array<VkSubpassDependency, 1> dependencies = { depthDependency };
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassCreateInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassCreateInfo, nullptr, &m_shadowRenderPass) != VK_SUCCESS)
    {
        std::cout << "Failed to create render pass for the shadow map!" << std::endl;
        return false;
    }
    
    // --- Image and image view
    for (size_t i = 0; i < m_frameDataList.size(); ++i)
    {
        if (!m_frameDataList[i].shadowMapImage.Create(
            SHADOW_MAP_WIDTH, 
            SHADOW_MAP_HEIGHT,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        ))
        {
            std::cout << "Failed to create image for the shadow map!" << std::endl;
            return false;
        }

        if (!m_frameDataList[i].shadowMapImageView.Create(
            m_frameDataList[i].shadowMapImage.GetHandle(),
            VK_FORMAT_D32_SFLOAT, 
            VK_IMAGE_ASPECT_DEPTH_BIT))
        {
            std::cout << "Failed to create image view for the shadow map!" << std::endl;
            return false;
        }
    }

    // --- Framebuffer ---
    for (size_t i = 0; i < m_frameDataList.size(); ++i)
    {
        std::array<VkImageView, 1> attachments =
        {
            m_frameDataList[i].shadowMapImageView.GetHandle(),
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_shadowRenderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = SHADOW_MAP_WIDTH;
        framebufferCreateInfo.height = SHADOW_MAP_HEIGHT;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferCreateInfo, nullptr, &m_frameDataList[i].shadowMapFramebuffer) != VK_SUCCESS)
        {
            std::cout << "Failed to create a framebuffer for the shadow map!" << std::endl;
            return false;
        }
    }

    // --- Pipeline ---
    VulkanGraphicsPipelineBuilder builder {};

    // --- Vertex input ---
    std::vector<VkVertexInputBindingDescription> bindings = Vertex::GetBindingDescriptions();
    std::vector<VkVertexInputAttributeDescription> attributes = Vertex::GetAttributeDescriptions();
    builder
        .SetVertexBindingDescriptions(bindings)
        .SetVertexAttributeDescriptions(attributes);

    // --- Input assembly ---
    builder.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // --- Viewport and scissors ---
    // Both are meant to be dynamic, so we only set the number of viewports and scissors, but not the actual data yet
    builder.SetViewportCount(1);
    builder.SetScissorCount(1);

    // --- Rasterizer ---
    builder
        .SetPolygonMode(VK_POLYGON_MODE_FILL)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_CLOCKWISE);

    // --- Multisampling ---

    // --- Depth & stencil ---
    builder
        .SetDepthTestEnabled(VK_TRUE)
        .SetDepthWriteEnabled(VK_TRUE)
        .SetDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);

    // --- Color blending ---

    // --- Dynamic state ---
    // (Attributes specified here will have to be specified at drawing time)
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    builder.SetDynamicStates(dynamicStates);

    // --- Pipeline layout ---
    std::vector<VkPushConstantRange> pushConstantRanges;
    pushConstantRanges.emplace_back();
    pushConstantRanges.back().offset = 0;
    pushConstantRanges.back().size = static_cast<uint32_t>(sizeof(PushConstant));
    pushConstantRanges.back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    builder.SetPushConstantRanges(pushConstantRanges);
    
    // --- Shaders ---
    builder
        .SetVertexShaderFilePath("Resources/Shaders/shadow_vert.spv")
        .SetFragmentShaderFilePath("Resources/Shaders/shadow_frag.spv");

    builder.SetRenderPass(m_shadowRenderPass);

    if (!builder.Build())
    {
        std::cout << "Failed to build pipeline for shadow pass!" << std::endl;
        return false;
    }

    m_shadowPipelineLayout = builder.GetPipelineLayout();
    m_shadowPipeline = builder.GetPipeline();

    return true;
}

/**
 * @brief Initializes the Vulkan render pass.
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitRenderPass()
{
    // Setup color attachment for the framebuffer
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_vkSwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear framebuffer upon being loaded
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Keep rendered contents in memory after
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't use stencil testing, so we don't care for now
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // We don't use stencil testing, so we don't care for now
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0; // This is reflected in the "layout(location = 0) out color" in the fragment shader
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Setup depth attachment for the framebuffer
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT; // TODO: Create a routine for finding the suitable depth format
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Rendering subpass (not compute subpass)
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstSubpass = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDependency = {};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstSubpass = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    std::array<VkSubpassDependency, 2> dependencies = { subpassDependency, depthDependency };
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassCreateInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassCreateInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
    {
        std::cout << "Failed to create render pass!" << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Initializes the resources needed for the depth/stencil buffer attachment.
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitDepthStencilImage()
{
    // TODO: We can use the VK_FORMAT_D32_SFLOAT format for the depth buffer, but we can add more
    // flexibility by querying which format is supported.
    // If somehow we will need the stencil buffer in the future, we'll have to use another format
    // VK_FORMAT_D32_SFLOAT_S8_UINT or VK_FORMAT_D24_UNORM_S8_UINT.
    if (!m_vkDepthBufferImage.Create(
            m_vkSwapchainImageExtent.width, 
            m_vkSwapchainImageExtent.height, 
            VK_FORMAT_D32_SFLOAT, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        std::cout << "Failed to create Vulkan image for the depth buffer!" << std::endl;
        return false;
    }

    if (!m_vkDepthBufferImageView.Create(m_vkDepthBufferImage.GetHandle(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT))
    {
        std::cout << "Failed to create Vulkan image view for the depth buffer!" << std::endl;
        return false;
    }

    // TODO: Can transition image layout of the depth buffer image if we so choose,
    // but we'll already handle this in the render pass.

    return true;
}

/**
 * @brief Initializes the Vulkan framebuffers.
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitFramebuffers()
{
    for (size_t i = 0; i < m_frameDataList.size(); ++i)
    {
        std::array<VkImageView, 2> attachments =
        {
            m_frameDataList[i].imageView.GetHandle(),
            m_vkDepthBufferImageView.GetHandle()
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_vkRenderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = m_vkSwapchainImageExtent.width;
        framebufferCreateInfo.height = m_vkSwapchainImageExtent.height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferCreateInfo, nullptr, &m_frameDataList[i].framebuffer) != VK_SUCCESS)
        {
            std::cout << " FAILED!" << std::endl;
            return false;
        }
    }
    return true;
}

/**
 * @brief Initializes the Vulkan command pool.
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitCommandPool()
{
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.queueFamilyIndex = VulkanContext::GetGraphicsQueueIndex();
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(VulkanContext::GetLogicalDevice(), &commandPoolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
    {
        std::cout << "Failed to create command pool!" << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Initializes the Vulkan command buffer.
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitCommandBuffers()
{
    VkCommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandPool = m_vkCommandPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = m_maxFramesInFlight;

    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.resize(m_maxFramesInFlight);
    if (vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &commandBufferInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        std::cout << "Failed to create command buffers!" << std::endl;
        return false;
    }

    for (size_t i = 0; i < m_frameDataList.size(); i++)
    {
        m_frameDataList[i].commandBuffer = commandBuffers[i];
    }

    return true;
}

/**
 * @brief Initializes descriptors
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitDescriptors()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(VulkanContext::GetLogicalDevice(), &samplerInfo, nullptr, &m_shadowMapSampler) != VK_SUCCESS)
    {
        std::cerr << "[Application] Failed to create sampler for the shadow map!" << std::endl;
        return false;
    }

    VkDescriptorSetLayoutBinding cameraUBOBinding = {};
    cameraUBOBinding.binding = 0;
    cameraUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraUBOBinding.descriptorCount = 1;
    cameraUBOBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding lightUBOBinding = {};
    lightUBOBinding.binding = 1;
    lightUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightUBOBinding.descriptorCount = 1;
    lightUBOBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding shadowMapBinding = {};
    shadowMapBinding.binding = 2;
    shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMapBinding.descriptorCount = 1;
    shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = { cameraUBOBinding, lightUBOBinding, shadowMapBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
    {
        std::cerr << "[Application] Failed to create descriptor layout for the UBO!" << std::endl;
        return false;
    }

    VkDeviceSize cameraUBOBufferSize = sizeof(CameraData) * m_frameDataList.size();
    VkDeviceSize lightUBOBufferSize = sizeof(LightData) * m_frameDataList.size();
    for (size_t i = 0; i < m_frameDataList.size(); i++)
    {
        if (!m_frameDataList[i].cameraDataUniformBuffer.Create(cameraUBOBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            std::cerr << "[Application] Failed to create uniform buffer for camera data!" << std::endl;
            return false;
        }
        if (!m_frameDataList[i].lightDataUniformBuffer.Create(lightUBOBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            std::cerr << "[Application] Failed to create uniform buffer for light data!" << std::endl;
            return false;
        }
    }

    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = m_maxFramesInFlight;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = m_maxFramesInFlight;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = m_maxFramesInFlight * poolSizes.size();

    if (vkCreateDescriptorPool(VulkanContext::GetLogicalDevice(), &poolInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
    {
        std::cerr << "[Application] Failed to create descriptor pool!" << std::endl;
        return false;
    }

    std::vector<VkDescriptorSetLayout> layouts(m_maxFramesInFlight, m_vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorsAllocInfo = {};
    descriptorsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorsAllocInfo.descriptorPool = m_vkDescriptorPool;
    descriptorsAllocInfo.descriptorSetCount = m_maxFramesInFlight;
    descriptorsAllocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets(m_maxFramesInFlight);
    VkResult r = vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &descriptorsAllocInfo, descriptorSets.data());
    if (r != VK_SUCCESS)
    {
        std::cerr << "[Application] Failed to allocate descriptor sets!" << std::endl;
        if (r == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            std::cerr << "[Application] Out of host memory!" << std::endl;
        }
        else if (r == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            std::cerr << "[Application] Out of device memory!" << std::endl;
        }
        else if (r == VK_ERROR_FRAGMENTED_POOL)
        {
            std::cerr << "[Application] Fragmented pool!" << std::endl;
        }
        else if (r == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            std::cerr << "[Application] Out of pool memory!" << std::endl;
        }
        return false;
    }

    for (size_t i = 0; i < m_frameDataList.size(); i++)
    {
        m_frameDataList[i].descriptorSet = descriptorSets[i];

        // Camera UBO
        VkDescriptorBufferInfo cameraUBO = {};
        cameraUBO.buffer = m_frameDataList[i].cameraDataUniformBuffer.GetHandle();
        cameraUBO.offset = 0;
        cameraUBO.range = sizeof(CameraData); // or VK_WHOLE_SIZE

        VkWriteDescriptorSet cameraUBOWrite = {};
        cameraUBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cameraUBOWrite.dstSet = m_frameDataList[i].descriptorSet;
        cameraUBOWrite.dstBinding = 0;
        cameraUBOWrite.dstArrayElement = 0;
        cameraUBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraUBOWrite.descriptorCount = 1;
        cameraUBOWrite.pBufferInfo = &cameraUBO;

        VkDescriptorBufferInfo lightUBO = {};
        lightUBO.buffer = m_frameDataList[i].lightDataUniformBuffer.GetHandle();
        lightUBO.offset = 0;
        lightUBO.range = sizeof(LightData);

        VkWriteDescriptorSet lightUBOWrite = {};
        lightUBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightUBOWrite.dstSet = m_frameDataList[i].descriptorSet;
        lightUBOWrite.dstBinding = 1;
        lightUBOWrite.dstArrayElement = 0;
        lightUBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightUBOWrite.descriptorCount = 1;
        lightUBOWrite.pBufferInfo = &lightUBO;

        VkDescriptorImageInfo shadowMap = {};
        shadowMap.sampler = m_shadowMapSampler;
        shadowMap.imageView = m_frameDataList[i].shadowMapImageView.GetHandle();
        shadowMap.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet shadowMapWrite = {};
        shadowMapWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        shadowMapWrite.dstSet = m_frameDataList[i].descriptorSet;
        shadowMapWrite.dstBinding = 2;
        shadowMapWrite.dstArrayElement = 0;
        shadowMapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapWrite.descriptorCount = 1;
        shadowMapWrite.pImageInfo = &shadowMap;

        std::array<VkWriteDescriptorSet, 3> writes = { cameraUBOWrite, lightUBOWrite, shadowMapWrite };
        vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), writes.size(), writes.data(), 0, nullptr);
    }

    return true;
}

/**
 * @brief Create Vulkan graphics pipeline
 * @return Returns true if the creation was successful. Returns false otherwise.
 */
bool Application::InitGraphicsPipeline()
{
    VulkanGraphicsPipelineBuilder builder {};

    // --- Vertex input ---
    std::vector<VkVertexInputBindingDescription> bindings = Vertex::GetBindingDescriptions();
    std::vector<VkVertexInputAttributeDescription> attributes = Vertex::GetAttributeDescriptions();
    builder
        .SetVertexBindingDescriptions(bindings)
        .SetVertexAttributeDescriptions(attributes);

    // --- Input assembly ---
    builder.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // --- Viewport and scissors ---
    // Both are meant to be dynamic, so we only set the number of viewports and scissors, but not the actual data yet
    builder.SetViewportCount(1);
    builder.SetScissorCount(1);

    // --- Rasterizer ---
    builder
        .SetPolygonMode(VK_POLYGON_MODE_FILL)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_CLOCKWISE);

    // --- Multisampling ---

    // --- Depth & stencil ---
    builder
        .SetDepthTestEnabled(VK_TRUE)
        .SetDepthWriteEnabled(VK_TRUE)
        .SetDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);

    // --- Color blending ---

    // --- Dynamic state ---
    // (Attributes specified here will have to be specified at drawing time)
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    builder.SetDynamicStates(dynamicStates);

    // --- Pipeline layout ---
    std::vector<VkPushConstantRange> pushConstantRanges;
    pushConstantRanges.emplace_back();
    pushConstantRanges.back().offset = 0;
    pushConstantRanges.back().size = static_cast<uint32_t>(sizeof(PushConstant));
    pushConstantRanges.back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    builder.SetPushConstantRanges(pushConstantRanges);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { m_vkDescriptorSetLayout };
    builder.SetDescriptorSetLayouts(descriptorSetLayouts);
    
    // --- Shaders ---
    builder
        .SetVertexShaderFilePath("Resources/Shaders/basic_vert.spv")
        .SetFragmentShaderFilePath("Resources/Shaders/basic_frag.spv");

    builder.SetRenderPass(m_vkRenderPass);

    if (!builder.Build())
    {
        return false;
    }

    m_vkPipelineLayout = builder.GetPipelineLayout();
    m_vkPipeline = builder.GetPipeline();

    return true;
}

/**
 * @brief Initializes the Vulkan synchronization tools.
 * @return Returns true if the initialization was successful. Returns false otherwise.
 */
bool Application::InitSynchronizationTools()
{
    // --- Create semaphores that we use for rendering ---
    for (uint32_t i = 0; i < m_maxFramesInFlight; ++i)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Set the flag to be already signalled at the start

        if ((vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_frameDataList[i].imageAvailableSemaphore) != VK_SUCCESS) 
                || (vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_frameDataList[i].renderDoneSemaphore) != VK_SUCCESS)
                || (vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceCreateInfo, nullptr, &m_frameDataList[i].renderDoneFence) != VK_SUCCESS))
        {
            std::cout << "Failed to create synchronization tools!" << std::endl;
            return false;
        }
    }

    return true;
}

/**
 * @brief Appends geometry vertices of a tile into a destination buffer
 * @param[in] tileData Tile whose geometry vertices to append
 * @param[in] origin Origin that the vertices are relative to (lon/lat)
 * @param[in] dest Destination buffer to append the vertices to
 * @return Number of vertices appended
 */
uint32_t Application::AppendTileGeometryVertices(const TileData &tileData, const glm::dvec2 &origin, std::vector<Vertex> &dest)
{
    uint32_t numVerticesAdded = static_cast<uint32_t>(dest.size());

    glm::dvec2 tileCenter = GeometryUtils::LonLatToXY(origin);

    const std::vector<BuildingData> &buildings = tileData.buildings;

    glm::vec3 sideColor(0.65f);
    glm::vec3 topColor(0.9f);
    glm::vec3 bottomColor(0.35f);

    std::vector<glm::dvec2> pointsInTriangulation;

    for (size_t i = 0; i < buildings.size(); i++)
    {
        const BuildingData &building = buildings.at(i);

        float buildingHeight = building.heightInMeters * SCALE;
        float buildingYOffset = building.heightFromGround * SCALE;

        std::vector<glm::dvec2> points;
        for (size_t j = 0; j < building.outline.size(); ++j)
        {
            points.push_back(GeometryUtils::LonLatToXY(building.outline[j]));
        }

        for (size_t j = 0; j < points.size(); j++)
        {
            glm::dvec2 &a = points[j];
            glm::dvec2 &b = points[(j + 1) % points.size()];
            glm::dvec2 &c = points[(j + 2) % points.size()];

            if (GeometryUtils::IsCollinear(a, b, c))
            {
                points.erase(points.begin() + ((j + 1) % points.size()));
                --j;
            }
        }

        if (!GeometryUtils::IsPolygonCCW(points))
        {
            std::reverse(points.begin(), points.end());
        }
        // Double check if polygon is now indeed CCW
        if (!GeometryUtils::IsPolygonCCW(points))
        {
            std::cout << "Polygon is still not CCW!" << std::endl;
        }

        // Top
        GeometryUtils::PolygonTriangulation(points, pointsInTriangulation);
        for (size_t j = 0; j < pointsInTriangulation.size(); j++)
        {
            glm::dvec2 point = (pointsInTriangulation[j] - tileCenter) * SCALE;
            dest.emplace_back();
            dest.back().position.x = point.x;
            dest.back().position.y = buildingYOffset + buildingHeight;
            dest.back().position.z = point.y;
            dest.back().color = topColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
        }
        // Bottom
        for (size_t j = pointsInTriangulation.size(); j > 0; j--)
        {
            glm::dvec2 point = (pointsInTriangulation[j - 1] - tileCenter) * SCALE;
            dest.emplace_back();
            dest.back().position.x = point.x;
            dest.back().position.y = buildingYOffset;
            dest.back().position.z = point.y;
            dest.back().color = bottomColor;
            dest.back().normal = { 0.0f, -1.0f, 0.0f };
        }

        // Extrude
        for (size_t j = 0; j < points.size(); j++)
        {
            const glm::vec2 &p0 = (points[j] - tileCenter) * SCALE;
            const glm::vec2 &p1 = (points[(j + 1) % points.size()] - tileCenter) * SCALE;

            dest.emplace_back();
            dest.back().position = { p0.x, buildingYOffset, p0.y };
            dest.back().color = sideColor;
            dest.emplace_back();
            dest.back().position = { p1.x, buildingYOffset, p1.y };
            dest.back().color = sideColor;
            dest.emplace_back();
            dest.back().position = { p1.x, buildingYOffset + buildingHeight, p1.y };
            dest.back().color = sideColor;
            dest.emplace_back();
            dest.back().position = { p1.x, buildingYOffset + buildingHeight, p1.y };
            dest.back().color = sideColor;
            dest.emplace_back();
            dest.back().position = { p0.x, buildingYOffset + buildingHeight, p0.y };
            dest.back().color = sideColor;
            dest.emplace_back();
            dest.back().position = { p0.x, buildingYOffset, p0.y };
            dest.back().color = sideColor;

            for (size_t k = 0; k < 2; ++k)
            {
                size_t offset = k * 3;
                const glm::vec3 a = dest[dest.size() - 1 - (offset + 2)].position;
                const glm::vec3 b = dest[dest.size() - 1 - (offset + 1)].position;
                const glm::vec3 c = dest[dest.size() - 1 - (offset + 0)].position;

                glm::vec3 normal = glm::normalize(glm::cross(c - a, b - a));
                dest[dest.size() - 1 - (offset + 2)].normal = normal;
                dest[dest.size() - 1 - (offset + 1)].normal = normal;
                dest[dest.size() - 1 - (offset + 0)].normal = normal;
            }
        }
    }

    // Road vertices
    const glm::vec3 roadColor(0.0f, 0.5f, 0.5f);
    const std::vector<HighwayData> &highways = tileData.highways;
    for (size_t i = 0; i < highways.size(); i++)
    {
        const HighwayData &highway = highways.at(i);

        double roadHeight = 0.0;
        double width = highway.roadWidth * SCALE;
        for (size_t j = 1; j < highway.points.size(); j++)
        {
            const glm::dvec2 &a = (GeometryUtils::LonLatToXY(highway.points[j - 1]) - tileCenter) * SCALE;
            const glm::dvec2 &b = (GeometryUtils::LonLatToXY(highway.points[j]) - tileCenter) * SCALE;

            glm::dvec2 dir = b - a;
            glm::dvec2 normal(-dir.y, dir.x);
            normal = glm::normalize(normal);

            glm::dvec2 p0 = a + normal * width / 2.0;
            glm::dvec2 p1 = a - normal * width / 2.0;
            glm::dvec2 p2 = b - normal * width / 2.0;
            glm::dvec2 p3 = b + normal * width / 2.0;

            dest.emplace_back();
            dest.back().position = { p0.x, roadHeight, p0.y };
            dest.back().color = roadColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
            dest.emplace_back();
            dest.back().position = { p1.x, roadHeight, p1.y };
            dest.back().color = roadColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
            dest.emplace_back();
            dest.back().position = { p2.x, roadHeight, p2.y };
            dest.back().color = roadColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
            dest.emplace_back();
            dest.back().position = { p2.x, roadHeight, p2.y };
            dest.back().color = roadColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
            dest.emplace_back();
            dest.back().position = { p3.x, roadHeight, p3.y };
            dest.back().color = roadColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
            dest.emplace_back();
            dest.back().position = { p0.x, roadHeight, p0.y };
            dest.back().color = roadColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
        }
    }

    // Water vertices
    glm::vec3 waterColor { 0.8314f, 0.9451f, 0.9765f };
    const std::vector<WaterFeatureData> &waters = tileData.waterFeatures;
    for (size_t i = 0; i < waters.size(); ++i)
    {
        const WaterFeatureData &water = waters[i];

        pointsInTriangulation.clear();
        
        std::vector<glm::dvec2> points;
        for (size_t j = 0; j < water.outline.size(); ++j)
        {
            points.push_back(GeometryUtils::LonLatToXY(water.outline[j]));
        }

        for (size_t j = 0; j < points.size(); j++)
        {
            glm::dvec2 &a = points[j];
            glm::dvec2 &b = points[(j + 1) % points.size()];
            glm::dvec2 &c = points[(j + 2) % points.size()];

            if (GeometryUtils::IsCollinear(a, b, c))
            {
                points.erase(points.begin() + ((j + 1) % points.size()));
                --j;
            }
        }

        if (!GeometryUtils::IsPolygonCCW(points))
        {
            std::reverse(points.begin(), points.end());
        }

        GeometryUtils::PolygonTriangulation(points, pointsInTriangulation);

        for (size_t j = 0; j < pointsInTriangulation.size(); j++)
        {
            glm::dvec2 point = (pointsInTriangulation[j] - tileCenter) * SCALE;
            dest.emplace_back();
            dest.back().position.x = point.x;
            dest.back().position.y = 0.0f;
            dest.back().position.z = point.y;
            dest.back().color = waterColor;
            dest.back().normal = { 0.0f, 1.0f, 0.0f };
        }
    }

    numVerticesAdded = static_cast<uint32_t>(dest.size()) - numVerticesAdded;
    return numVerticesAdded;
}

/**
 * @brief Performs the necessary setup to change to a new current tile.
 * @param[in] newCurrentTileIndex Tile index of the new tile
 */
void Application::UpdateCurrentTile(const glm::ivec2 &newCurrentTileIndex)
{
    m_currentTileIndex = newCurrentTileIndex;

    const int zoomLevel = 16;
    RectD tileBounds = GeometryUtils::GetLonLatBoundsFromTile(newCurrentTileIndex.x, newCurrentTileIndex.y, zoomLevel);
    m_origin = tileBounds.min;

    const int viewDist = 1;
    RectI oldViewArea = m_currentViewArea;
    RectI newViewArea = {};
    newViewArea.min = newCurrentTileIndex - viewDist;
    newViewArea.max = newCurrentTileIndex + viewDist;
    m_currentViewArea = newViewArea;

    // Remove tiles that are not part of the new view area
    for (size_t i = m_activeTiles.size(); i > 0; --i)
    {
        size_t idx = i - 1;
        if (!RectI::IsPointInsideRect(newViewArea, m_activeTiles[idx].index))
        {
            m_activeTiles.erase(m_activeTiles.begin() + idx);
        }
    }

    {
        std::lock_guard tileUpdateLock(m_tilesUpdateMutex);
        m_tilesUpdated = true;
    }

    std::lock_guard lock(m_retrieveTileJobsMutex);
    int prefetchDistance = 1;
    // Go through tiles in the new view area, and if they are also part
    // of the old view area, skip since its data should already be in the active tiles list
    OSMTileDataSource dataSource = {};
    for (int dy = -viewDist - prefetchDistance; dy <= viewDist + prefetchDistance; ++dy)
    {
        for (int dx = -viewDist - prefetchDistance; dx <= viewDist + prefetchDistance; ++dx)
        {
            glm::ivec2 index = newCurrentTileIndex;
            index.x += dx;
            index.y += dy;

            if (RectI::IsPointInsideRect(oldViewArea, index))
            {
                continue;
            }

            m_retrieveTileJobs.emplace_back();
            m_retrieveTileJobs.back().tileIndex = index;
            m_retrieveTileJobs.back().zoomLevel = zoomLevel;
            m_retrieveTileJobs.back().addImmediately = RectI::IsPointInsideRect(newViewArea, index);
        }
    }
}

/**
 * @brief Function run by the worker thread where tiles are downloaded
 * in the background.
 * @param[in] downloadIfNeeded Flag indicating whether the thread is to download data if needed
 */
void Application::WorkerThreadFunc(bool downloadIfNeeded)
{
    OSMTileDataSource dataSource = {};

    while (m_workerThreadRunning)
    {
        std::vector<RetrieveTileJob> jobs;
        if (m_retrieveTileJobsMutex.try_lock())
        {
            // Try to prioritize tiles that already have cached data
            for (size_t i = m_retrieveTileJobs.size(); i > 0; --i)
            {
                size_t index = i - 1;
                RetrieveTileJob &job = m_retrieveTileJobs[index];
                if (dataSource.IsTileCacheAvailable(job.tileIndex, job.zoomLevel))
                {
                    jobs.push_back(job);
                    m_retrieveTileJobs.erase(m_retrieveTileJobs.begin() + index);
                }
            }

            if (downloadIfNeeded)
            {
                if (!m_retrieveTileJobs.empty())
                {
                    jobs.push_back(m_retrieveTileJobs.front());
                    m_retrieveTileJobs.erase(m_retrieveTileJobs.begin());
                }
            }

            m_retrieveTileJobsMutex.unlock();
        }
        else
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1000ms);
            continue;
        }

        std::vector<TileData> tilesRetrieved;
        for (size_t i = 0; i < jobs.size(); ++i)
        {
            RetrieveTileJob &job = jobs[i];
            if (job.addImmediately)
            {
                tilesRetrieved.emplace_back();
                if (!dataSource.Retrieve(job.tileIndex, job.zoomLevel, tilesRetrieved.back()))
                {
                    tilesRetrieved.pop_back();
                }
            }
            else
            {
                dataSource.Prefetch(job.tileIndex, job.zoomLevel);
            }
        }

        if (!tilesRetrieved.empty())
        {
            std::lock_guard lock2(m_tilesUpdateMutex);
            m_activeTiles.insert(m_activeTiles.end(), tilesRetrieved.begin(), tilesRetrieved.end());
            m_tilesUpdated = true;
        }
    }
}

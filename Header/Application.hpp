#ifndef APPLICATION_HEADER
#define APPLICATION_HEADER

#include "Core/Window.hpp"
#include "Vertex.hpp"

#include "Core/Vulkan/VulkanBuffer.hpp"
#include "Core/Vulkan/VulkanImage.hpp"
#include "Core/Vulkan/VulkanImageView.hpp"

#include <vulkan/vulkan.hpp>

class Application
{
private:
    /**
     * Struct containing data needed for a frame
     */
    struct FrameData
    {
        /**
         * Frame image
         */
        VkImage image;

        /**
         * Frame image view
         */
        VulkanImageView imageView;

        /**
         * Framebuffer
         */
        VkFramebuffer framebuffer;

        /**
         * Frame commandbuffer
         */
        VkCommandBuffer commandBuffer;

        // --- Synchronization ---

        /**
         * Semaphore that is signalled when the next image to use
         * is available and ready for rendering
         */
        VkSemaphore imageAvailableSemaphore;

        /**
         * Semaphore that is signalled when the rendering has been done
         * and is render for presenting (used for waiting before presenting)
         */
        VkSemaphore renderDoneSemaphore;

        /**
         * Fence signalled when the rendering for this frame is done
         */
        VkFence renderDoneFence;
    };

private:
    /**
     * Flag indicating whether the application is running
     */
    bool m_isRunning;

    /**
     * Window
     */
    Window m_window;

    /**
     * Swapchain
     */
    VkSwapchainKHR m_vkSwapchain;

    /**
     * Swapchain image format
     */
    VkFormat m_vkSwapchainImageFormat;

    /**
     * Swapchain image extent
     */
    VkExtent2D m_vkSwapchainImageExtent;

    /**
     * Maximum number of frames in flight
     */
    uint32_t m_maxFramesInFlight;

    /**
     * Render pass
     */
    VkRenderPass m_vkRenderPass;

    /**
     * Pipeline layout
     */
    VkPipelineLayout m_vkPipelineLayout;

    /**
     * Pipeline
     */
    VkPipeline m_vkPipeline;

    /**
     * Command pool
     */
    VkCommandPool m_vkCommandPool;

    /**
     * List containing data for each frame
     */
    std::vector<FrameData> m_frameDataList;

    /**
     * Test vertex buffer
     */
    VulkanBuffer m_testVertexBuffer;

    /**
     * Image for the depth buffer
     */
    VulkanImage m_vkDepthBufferImage;

    /**
     * Image view for the depth buffer image
     */
    VulkanImageView m_vkDepthBufferImageView;

public:
    /**
     * @brief Constructor
     */
    Application();

    /**
     * @brief Destructor
     */
    ~Application();

    /**
     * @brief Runs the application.
     */
    void Run();

private:
    /**
     * @brief Initializes the application.
     * @return Returns true if the initialization was successful.
     * Returns false otherwise.
     */
    bool Init();

    /**
     * @brief Cleans up the application.
     */
    void Cleanup();

    /**
     * @brief Initializes the Vulkan render pass.
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitSwapchain();

    /**
     * @brief Initializes the Vulkan render pass.
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitRenderPass();

    /**
     * @brief Initializes the resources needed for the depth/stencil buffer attachment.
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitDepthStencilImage();

    /**
     * @brief Initializes the Vulkan framebuffers.
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitFramebuffers();

    /**
     * @brief Initializes the Vulkan command pool.
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitCommandPool();

    /**
     * @brief Initializes the Vulkan command buffer.
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitCommandBuffers();

    /**
     * @brief Create Vulkan graphics pipeline
     * @return Returns true if the creation was successful. Returns false otherwise.
     */
    bool InitGraphicsPipeline();

    /**
     * @brief Initializes the Vulkan synchronization tools.
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitSynchronizationTools();
};

#endif // APPLICATION_HEADER

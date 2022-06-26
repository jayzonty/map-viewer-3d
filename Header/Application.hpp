#ifndef APPLICATION_HEADER
#define APPLICATION_HEADER

#include "Core/Camera.hpp"
#include "Core/Window.hpp"
#include "Map/TileData.hpp"
#include "Vertex.hpp"

#include "Core/Vulkan/VulkanBuffer.hpp"
#include "Core/Vulkan/VulkanImage.hpp"
#include "Core/Vulkan/VulkanImageView.hpp"
#include "glm/fwd.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include <condition_variable>
#include <cstdalign>
#include <mutex>
#include <queue>
#include <vector>

class Application
{
private:
    // Push constant data
    struct PushConstant
    {
        glm::mat4 lightProjView;
        glm::mat4 projView; // Combined projection-view matrix
    };

    // Uniform buffer for camera data
    struct CameraData
    {
        alignas(16) glm::vec3 position; // Camera position
    };

    // Uniform buffer for light data 
    struct LightData
    {
        alignas(16) glm::vec4 lightPosition;    // Light position

        alignas(16) glm::vec3 ambient;          // Ambient intensity
        alignas(16) glm::vec3 diffuse;          // Diffuse intensity
        alignas(16) glm::vec3 specular;         // Specular intensity
    };

    // Struct containing data needed for a frame
    struct FrameData
    {
        VulkanImage shadowMapImage;         // Image for the shadow map
        VulkanImageView shadowMapImageView; // Image view for the shadow map image
        VkFramebuffer shadowMapFramebuffer; // Framebuffer for the shadow map

        VkImage image;                      // Frame image
        VulkanImageView imageView;          // Image view for the frame image
        VkFramebuffer framebuffer;          // Framebuffer
        VkCommandBuffer commandBuffer;      // Command buffer

        // --- Synchronization ---
        VkSemaphore imageAvailableSemaphore;    // Semaphore signalled when the next image is available and ready for rendering
        VkSemaphore renderDoneSemaphore;        // Semaphore signalled when rendering is done and is ready for presenting
        VkFence renderDoneFence;                // Fence signalled when the rendering for this frame is done

        // --- Descriptor sets ---
        VkDescriptorSet descriptorSet;          // Descriptor set for this frame
        VulkanBuffer lightDataUniformBuffer;    // Uniform buffer for the light data
        VulkanBuffer cameraDataUniformBuffer;   // Uniform buffer for the camera data
    };

    // Struct containing information about a retrieve tile job
    struct RetrieveTileJob
    {
        glm::ivec2 tileIndex;                   // Index of the tile to retrieve
        int zoomLevel;                          // Zoom level
        bool addImmediately;                    // Flag indicating whether to prefetch or to add to active tiles immediately
    };

    const double SCALE = 0.05;                  // World scale

private:
    bool m_isRunning;   // Flag indicating whether the application is running

    Window m_window;    // Window

    VkSwapchainKHR m_vkSwapchain;           // Swapchain
    VkFormat m_vkSwapchainImageFormat;      // Swapchain image format
    VkExtent2D m_vkSwapchainImageExtent;    // Swapchain image extent

    uint32_t m_maxFramesInFlight;           // Maximum number of frames in flight

    VkRenderPass m_shadowRenderPass;        // Render pass for the shadow pass
    VkRenderPass m_vkRenderPass;            // Render pass
    VkPipelineLayout m_shadowPipelineLayout;    // Pipeline layout for the shadow pass
    VkPipeline m_shadowPipeline;                // Pipeline object for the shadow pass
    VkPipelineLayout m_vkPipelineLayout;    // Pipeline layout
    VkPipeline m_vkPipeline;                // Pipeline

    VkCommandPool m_vkCommandPool;          // Command pool

    std::vector<FrameData> m_frameDataList; // List containing data for each frame

    VulkanBuffer m_testVertexBuffer;        // Test vertex buffer

    VulkanImage m_vkDepthBufferImage;           // Image for the depth buffer
    VulkanImageView m_vkDepthBufferImageView;   // Image view for the depth buffer image

    VkDescriptorSetLayout m_vkDescriptorSetLayout;  // Descriptor set layout
    VkDescriptorPool m_vkDescriptorPool;            // Descriptor pool

    VkSampler m_shadowMapSampler;           // sampler for the shadow map

    Camera m_camera;    // Camera

    glm::dvec2 m_origin;                    // Current global origin offset
    glm::ivec2 m_currentTileIndex;          // Current tile index
    std::vector<TileData> m_activeTiles;    // List of active tiles

    RectI m_currentViewArea;                // Current view area (in tiles)
    uint32_t m_numVertices;                 // Number of vertices to render

    bool m_workerThreadRunning;             // Flag indicating whether the worker thread is running

    std::vector<RetrieveTileJob> m_retrieveTileJobs;    // List of retrieve tile jobs
    std::mutex m_retrieveTileJobsMutex;                 // Mutex for the retrieve tile jobs list

    std::mutex m_tilesUpdateMutex;                      // Mutex for a tile update routine
    bool m_tilesUpdated;                                // Flag indicating whether the tiles have recently been updated

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
     * @brief Initializes all things needed for a shadow pass
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitShadowPass();

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
     * @brief Initializes descriptors
     * @return Returns true if the initialization was successful. Returns false otherwise.
     */
    bool InitDescriptors();

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

private:
    /**
     * @brief Appends geometry vertices of a tile into a destination buffer
     * @param[in] tileData Tile whose geometry vertices to append
     * @param[in] origin Origin that the vertices are relative to (lon/lat)
     * @param[in] dest Destination buffer to append the vertices to
     * @return Number of vertices appended
     */
    uint32_t AppendTileGeometryVertices(const TileData &tileData, const glm::dvec2 &origin, std::vector<Vertex> &dest);

    /**
     * @brief Performs the necessary setup to change to a new current tile.
     * @param[in] newCurrentTileIndex Tile index of the new tile
     */
    void UpdateCurrentTile(const glm::ivec2 &newCurrentTileIndex);

    /**
     * @brief Function run by the worker thread where tiles are downloaded
     * in the background.
     * @param[in] downloadIfNeeded Flag indicating whether the thread is to download data if needed
     */
    void WorkerThreadFunc(bool downloadIfNeeded);
};

#endif // APPLICATION_HEADER

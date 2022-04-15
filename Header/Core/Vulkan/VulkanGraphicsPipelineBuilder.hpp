#ifndef VULKAN_GRAPHICS_PIPELINE_BUILDER_HEADER
#define VULKAN_GRAPHICS_PIPELINE_BUILDER_HEADER

#include <vulkan/vulkan_core.h>

#include <string>
#include <vector>

/**
 * Builder class for a Vulkan graphics pipeline
 */
class VulkanGraphicsPipelineBuilder
{
private:
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;

    // Create info structs for each pipeline stage
    VkPipelineVertexInputStateCreateInfo m_vertexInputCreateInfo;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyCreateInfo;
    VkPipelineViewportStateCreateInfo m_viewportCreateInfo;
    VkPipelineRasterizationStateCreateInfo m_rasterizationCreateInfo;
    VkPipelineMultisampleStateCreateInfo m_multisampleCreateInfo;
    VkPipelineDepthStencilStateCreateInfo m_depthStencilCreateInfo;
    VkPipelineColorBlendStateCreateInfo m_colorBlendCreateInfo;
    VkPipelineDynamicStateCreateInfo m_dynamicStateCreateInfo;
    VkPipelineLayoutCreateInfo m_pipelineLayoutCreateInfo;
    VkPipelineShaderStageCreateInfo m_vertexShaderCreateInfo;
    VkPipelineShaderStageCreateInfo m_fragmentShaderCreateInfo;

    VkPipelineColorBlendAttachmentState m_colorBlendAttachment;

    std::string m_vertexShaderFilePath;
    std::string m_fragmentShaderFilePath;

    VkRenderPass m_renderPass;

public:
    VulkanGraphicsPipelineBuilder();
    ~VulkanGraphicsPipelineBuilder();

    // --- Vertex input stage ---
    VulkanGraphicsPipelineBuilder& SetVertexBindingDescriptions(const std::vector<VkVertexInputBindingDescription> &descriptions);
    VulkanGraphicsPipelineBuilder& SetVertexAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription> &descriptions);

    // --- Input assembly stage ---
    VulkanGraphicsPipelineBuilder& SetTopology(const VkPrimitiveTopology &topology);

    // --- Viewport & scissors
    VulkanGraphicsPipelineBuilder& SetViewportCount(const uint32_t &viewportCount);
    VulkanGraphicsPipelineBuilder& SetViewports(const std::vector<VkViewport> &viewports);
    VulkanGraphicsPipelineBuilder& SetScissorCount(const uint32_t &scissorCount);
    VulkanGraphicsPipelineBuilder& SetScissors(const std::vector<VkRect2D> &scissors);

    // --- Rasterizer ---
    VulkanGraphicsPipelineBuilder& SetPolygonMode(const VkPolygonMode &polygonMode);
    VulkanGraphicsPipelineBuilder& SetCullMode(const VkCullModeFlags &cullMode);
    VulkanGraphicsPipelineBuilder& SetFrontFace(const VkFrontFace &frontFace);

    // --- Multisample ---

    // --- Depth & stencil ---
    VulkanGraphicsPipelineBuilder& SetDepthTestEnabled(const VkBool32 &flag);
    VulkanGraphicsPipelineBuilder& SetDepthWriteEnabled(const VkBool32 &flag);
    VulkanGraphicsPipelineBuilder& SetDepthCompareOp(const VkCompareOp &compareOp);

    // --- Color blending ---

    // --- Dynamic state ---
    VulkanGraphicsPipelineBuilder& SetDynamicStates(const std::vector<VkDynamicState> &dynamicStates);

    // --- Pipeline layout ---
    VulkanGraphicsPipelineBuilder& SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &layouts);
    VulkanGraphicsPipelineBuilder& SetPushConstantRanges(const std::vector<VkPushConstantRange> &ranges);

    // --- Shaders ---
    VulkanGraphicsPipelineBuilder& SetVertexShaderFilePath(const std::string &vertexShaderFilePath);
    VulkanGraphicsPipelineBuilder& SetFragmentShaderFilePath(const std::string &fragmentShaderFilePath);

    // --- Render pass ---
    VulkanGraphicsPipelineBuilder& SetRenderPass(const VkRenderPass &renderPass);

    bool Build();

    VkPipelineLayout GetPipelineLayout();
    VkPipeline GetPipeline();

private:
    /**
     * @brief Create a shader module from the provided shader file path.
     * @param[in] shaderFilePath Shader file path
     * @param[in] device Logical device
     * @param[out] outShaderModule Shader module
     * @return Returns true if the shader module creation was successful. Returns false otherwise.
     */
    bool CreateShaderModule(const std::string& shaderFilePath, VkDevice device, VkShaderModule& outShaderModule);
};

#endif // VULKAN_GRAPHICS_PIPELINE_BUILDER_HEADER

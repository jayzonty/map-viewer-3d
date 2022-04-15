#include "Core/Vulkan/VulkanGraphicsPipelineBuilder.hpp"

#include "Core/Util/FileUtils.hpp"
#include "Core/Vulkan/VulkanContext.hpp"

#include <vulkan/vulkan_core.h>

VulkanGraphicsPipelineBuilder::VulkanGraphicsPipelineBuilder()
    : m_pipelineLayout(VK_NULL_HANDLE)
    , m_pipeline(VK_NULL_HANDLE)
    , m_vertexInputCreateInfo()
    , m_inputAssemblyCreateInfo()
    , m_viewportCreateInfo()
    , m_rasterizationCreateInfo()
    , m_multisampleCreateInfo()
    , m_depthStencilCreateInfo()
    , m_colorBlendCreateInfo()
    , m_dynamicStateCreateInfo()
    , m_pipelineLayoutCreateInfo()
    , m_vertexShaderCreateInfo()
    , m_fragmentShaderCreateInfo()
    , m_colorBlendAttachment()
    , m_vertexShaderFilePath()
    , m_fragmentShaderFilePath()
    , m_renderPass(VK_NULL_HANDLE)
{
    // Technically all create info structs will be initialized, but putting all of the possible
    // options here to see potential options easily

    // --- Vertex input ---
    m_vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    m_vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    m_vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    m_vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
    m_vertexInputCreateInfo.flags = 0;
    m_vertexInputCreateInfo.pNext = nullptr;

    // --- Input assembly ---
    m_inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
    m_inputAssemblyCreateInfo.flags = 0;
    m_inputAssemblyCreateInfo.pNext = nullptr;

    // --- Viewports & scisscors ---
    m_viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_viewportCreateInfo.viewportCount = 0;
    m_viewportCreateInfo.pViewports = nullptr;
    m_viewportCreateInfo.scissorCount = 0;
    m_viewportCreateInfo.pScissors = nullptr;
    m_viewportCreateInfo.flags = 0;
    m_viewportCreateInfo.pNext = nullptr;

    // --- Rasterization ---
    m_rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    m_rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    m_rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    m_rasterizationCreateInfo.lineWidth = 1.0f;
    m_rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    m_rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    m_rasterizationCreateInfo.depthBiasConstantFactor = 0.0f; // Optional
    m_rasterizationCreateInfo.depthBiasClamp = 0.0f; // Optional
    m_rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f; // Optional

    // --- Multisampling ---
    m_multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    m_multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_multisampleCreateInfo.minSampleShading = 1.0f; // Optional
    m_multisampleCreateInfo.pSampleMask = nullptr; // Optional
    m_multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    m_multisampleCreateInfo.alphaToOneEnable = VK_FALSE; // Optional

    // --- Depth & stencil ---
    m_depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_depthStencilCreateInfo.depthTestEnable = VK_FALSE;
    m_depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
    m_depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    m_depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    m_depthStencilCreateInfo.minDepthBounds = 0.0f;
    m_depthStencilCreateInfo.maxDepthBounds = 1.0f;
    m_depthStencilCreateInfo.stencilTestEnable = VK_FALSE; 
    m_depthStencilCreateInfo.front = {};
    m_depthStencilCreateInfo.back = {};

    // --- Color blending ---
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_FALSE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    m_colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    m_colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    m_colorBlendCreateInfo.attachmentCount = 1;
    m_colorBlendCreateInfo.pAttachments = &m_colorBlendAttachment;
    m_colorBlendCreateInfo.blendConstants[0] = 0.0f; // Optional
    m_colorBlendCreateInfo.blendConstants[1] = 0.0f; // Optional
    m_colorBlendCreateInfo.blendConstants[2] = 0.0f; // Optional
    m_colorBlendCreateInfo.blendConstants[3] = 0.0f; // Optional

    // --- Dynamic state ---
    m_dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_dynamicStateCreateInfo.dynamicStateCount = 0;
    m_dynamicStateCreateInfo.pDynamicStates = nullptr;

    // --- Pipeline layout ---
    m_pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    m_pipelineLayoutCreateInfo.setLayoutCount = 0;
    m_pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    m_pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
    m_pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // Optional

    // --- Shaders ---
    m_vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    m_vertexShaderCreateInfo.module = VK_NULL_HANDLE;
    m_vertexShaderCreateInfo.pName = "main";

    m_fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    m_fragmentShaderCreateInfo.module = VK_NULL_HANDLE;
    m_fragmentShaderCreateInfo.pName = "main";
}

VulkanGraphicsPipelineBuilder::~VulkanGraphicsPipelineBuilder()
{
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexBindingDescriptions(const std::vector<VkVertexInputBindingDescription> &descriptions)
{
    m_vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(descriptions.size());
    m_vertexInputCreateInfo.pVertexBindingDescriptions = descriptions.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription> &descriptions)
{
    m_vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(descriptions.size());
    m_vertexInputCreateInfo.pVertexAttributeDescriptions = descriptions.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetTopology(const VkPrimitiveTopology &topology)
{
    m_inputAssemblyCreateInfo.topology = topology;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetViewportCount(const uint32_t &viewportCount)
{
    m_viewportCreateInfo.viewportCount = viewportCount;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetViewports(const std::vector<VkViewport> &viewports)
{
    SetViewportCount(static_cast<uint32_t>(viewports.size()));
    m_viewportCreateInfo.pViewports = viewports.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetScissorCount(const uint32_t &scissorCount)
{
    m_viewportCreateInfo.scissorCount = scissorCount;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetScissors(const std::vector<VkRect2D> &scissors)
{
    SetScissorCount(static_cast<uint32_t>(scissors.size()));
    m_viewportCreateInfo.pScissors = scissors.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetPolygonMode(const VkPolygonMode &polygonMode)
{
    m_rasterizationCreateInfo.polygonMode = polygonMode;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetCullMode(const VkCullModeFlags &cullMode)
{
    m_rasterizationCreateInfo.cullMode = cullMode;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetFrontFace(const VkFrontFace &frontFace)
{
    m_rasterizationCreateInfo.frontFace = frontFace;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDepthTestEnabled(const VkBool32 &flag)
{
    m_depthStencilCreateInfo.depthTestEnable = flag;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDepthWriteEnabled(const VkBool32 &flag)
{
    m_depthStencilCreateInfo.depthWriteEnable = flag;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDepthCompareOp(const VkCompareOp &compareOp)
{
    m_depthStencilCreateInfo.depthCompareOp = compareOp;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDynamicStates(const std::vector<VkDynamicState> &dynamicStates)
{
    m_dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    m_dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &layouts)
{
    m_pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    m_pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetPushConstantRanges(const std::vector<VkPushConstantRange> &ranges)
{
    m_pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(ranges.size());
    m_pipelineLayoutCreateInfo.pPushConstantRanges = ranges.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexShaderFilePath(const std::string &filePath)
{
    m_vertexShaderFilePath = filePath;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetFragmentShaderFilePath(const std::string &filePath)
{
    m_fragmentShaderFilePath = filePath;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetRenderPass(const VkRenderPass &renderPass)
{
    m_renderPass = renderPass;
    return *this;
}

bool VulkanGraphicsPipelineBuilder::Build()
{
    // Create shader modules for the vertex and fragment shaders
    VkShaderModule vertexShaderModule;
    if (!CreateShaderModule(m_vertexShaderFilePath, VulkanContext::GetLogicalDevice(), vertexShaderModule))
    {
        return false;
    }

    VkShaderModule fragmentShaderModule;
    if (!CreateShaderModule(m_fragmentShaderFilePath, VulkanContext::GetLogicalDevice(), fragmentShaderModule))
    {
        vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), vertexShaderModule, nullptr);
        return false;
    }

    m_vertexShaderCreateInfo.module = vertexShaderModule;
    m_fragmentShaderCreateInfo.module = fragmentShaderModule;

    VkPipelineShaderStageCreateInfo shaderStages[] = { m_vertexShaderCreateInfo, m_fragmentShaderCreateInfo };

    if (vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &m_pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), fragmentShaderModule, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &m_vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &m_viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &m_rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &m_multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &m_depthStencilCreateInfo;
    pipelineCreateInfo.pColorBlendState = &m_colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &m_dynamicStateCreateInfo;
    pipelineCreateInfo.layout = m_pipelineLayout;
    pipelineCreateInfo.renderPass = m_renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // In case we inherit from an old pipeline
    pipelineCreateInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), fragmentShaderModule, nullptr);
        vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), m_pipelineLayout, nullptr);
        return false;
    }

    vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), vertexShaderModule, nullptr);
    vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), fragmentShaderModule, nullptr);

    return true;
}

VkPipelineLayout VulkanGraphicsPipelineBuilder::GetPipelineLayout()
{
    return m_pipelineLayout;
}

VkPipeline VulkanGraphicsPipelineBuilder::GetPipeline()
{
    return m_pipeline;
}

/**
 * @brief Create a shader module from the provided shader file path.
 * @param[in] shaderFilePath Shader file path
 * @param[in] device Logical device
 * @param[out] outShaderModule Shader module
 * @return Returns true if the shader module creation was successful. Returns false otherwise.
 */
bool VulkanGraphicsPipelineBuilder::CreateShaderModule(const std::string& shaderFilePath, VkDevice device, VkShaderModule& outShaderModule)
{
    std::vector<char> shaderData;
    if (!FileUtils::ReadFileAsBinary(shaderFilePath, shaderData))
    {
        return false;
    }

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = static_cast<uint32_t>(shaderData.size());
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderData.data());

    if (vkCreateShaderModule(device, &createInfo, nullptr, &outShaderModule) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

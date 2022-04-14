#pragma once

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include <array>

/**
 * Struct containing data about a vertex
 */
struct Vertex
{
    /**
	 * Position
     */
    glm::vec3 position;

    /**
	 * Color
     */
    glm::vec3 color;

    /**
	 * Texture coordinates
     */
    glm::vec2 uv;

    /**
	 * Normal
     */
    glm::vec3 normal;

    /**
     * @brief Gets the list of binding descriptions for this vertex
     * @return List of binding descriptions for this vertex
     */
    static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions()
    {
        std::vector<VkVertexInputBindingDescription> ret = {};

        ret.emplace_back();
        ret.back().binding = 0;
        ret.back().stride = sizeof(Vertex);
        ret.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return ret;
    }

    /**
     * @brief Gets the list of attribute descriptions for this vertex
     * @return List of attribute descriptions for this vertex
     */
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> ret = {};

        // Position
        ret.emplace_back();
        ret.back().binding = 0;
        ret.back().location = 0;
        ret.back().offset = offsetof(Vertex, position);
        ret.back().format = VK_FORMAT_R32G32B32_SFLOAT; // Three 32-bit signed floats

        // Color
        ret.emplace_back();
        ret.back().binding = 0;
        ret.back().location = 1;
        ret.back().offset = offsetof(Vertex, color);
        ret.back().format = VK_FORMAT_R32G32B32_SFLOAT; // Three 32-bit signed floats

        // UV
        ret.emplace_back();
        ret.back().binding = 0;
        ret.back().location = 2;
        ret.back().offset = offsetof(Vertex, uv);
        ret.back().format = VK_FORMAT_R32G32_SFLOAT; // Two 32-bit signed floats

        return ret;
    }
};

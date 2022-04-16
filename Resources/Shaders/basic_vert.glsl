#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragColor;
//layout(location = 1) out vec2 fragUV;

layout (push_constant) uniform PushConstants
{
    mat4 projView;
    uint uboIndex;
} pushConstants;

struct UniformBufferObject
{
    mat4 model;
};

layout (set = 0, binding = 0) uniform UniformBufferObjects
{
    UniformBufferObject buffers[1000];
} uboArray;

void main()
{
    //gl_Position = pushConstants.projView * uboArray.buffers[pushConstants.uboIndex].model * vec4(position, 1.0);
    gl_Position = pushConstants.projView * vec4(position, 1.0);

    fragColor = color;
    //fragUV = uv;
}

#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 normal;

layout (location = 0) out vec3 fragPosition;
layout (location = 1) out vec3 fragColor;
layout (location = 2) out vec3 fragNormal;

layout (push_constant) uniform PushConstants
{
    mat4 projView;
} pushConstants;

void main()
{
    gl_Position = pushConstants.projView * vec4(position, 1.0);

    fragPosition = position;
    fragColor = color;
    fragNormal = normal;
}

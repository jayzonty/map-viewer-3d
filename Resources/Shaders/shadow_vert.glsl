#version 460
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 position;

layout (push_constant) uniform PushConstants
{
    mat4 lightProjView; // Technically not needed, but for the sake of reusing the PushContants struct in the C++ app
    mat4 projView;
} pushConstants;

void main()
{
    gl_Position = pushConstants.projView * vec4(position, 1.0);
}
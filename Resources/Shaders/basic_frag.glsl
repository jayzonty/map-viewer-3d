#version 460
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 fragPosition;
layout (location = 1) in vec3 fragColor;
layout (location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 finalFragColor;

layout (set = 0, binding = 0) uniform CameraData
{
    vec3 position;
} cameraData;

layout (set = 0, binding = 1) uniform LightData
{
    vec4 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} lightData;

void main()
{
    vec3 finalColor = vec3(0.0);

    vec3 dirToLight = vec3(lightData.position);
    if (lightData.position.w > 0.0)
    {
    }
    dirToLight = -normalize(dirToLight);

    // Ambient
    vec3 ambient = lightData.ambient;

    // Diffuse
    float diff = max(dot(dirToLight, fragNormal), 0.0);
    vec3 diffuse = diff * lightData.diffuse;

    // Specular
    float shininess = 8.0;
    vec3 dirToEye = normalize(cameraData.position - fragPosition);
    vec3 r = reflect(-dirToLight, fragNormal);
    float spec = pow(max(dot(dirToEye, r), 0.0), shininess);
    vec3 specular = spec * lightData.specular;

    finalColor = (ambient + diffuse + specular) * fragColor;

    finalFragColor = vec4(finalColor, 1.0);
}

#version 460
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 fragPosition;
layout (location = 1) in vec3 fragColor;
layout (location = 2) in vec3 fragNormal;
layout (location = 3) in vec4 fragLightSpacePosition;

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

layout (set = 0, binding = 2) uniform sampler2D shadowMapSampler;

// Returns 1 if in shadow, 0 if not
float ShadowSample(vec3 fragLightSpaceNDC)
{
    float bias = 0.005;
    float sampledDepth = texture(shadowMapSampler, fragLightSpaceNDC.xy).r + bias;
    return sampledDepth < fragLightSpaceNDC.z ? 1.0 : 0.0;
}

// Returns percentage in shadow (0 ~ 1, 0 = 0% shadow, 1 = 100% shadow)
float PCF(vec3 fragLightSpaceNDC)
{
    float ret = 0.0;

    float offsetSize = 0.001;
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            vec3 offset = vec3(dx * offsetSize, dy * offsetSize, 0.0);
            ret += ShadowSample(fragLightSpaceNDC + offset);
        }
    }
    ret /= 9.0;

    return ret;
}

void main()
{
    vec3 fragLightSpaceNDC = fragLightSpacePosition.xyz / fragLightSpacePosition.w;
    fragLightSpaceNDC.xy = (fragLightSpaceNDC.xy + 1.0) / 2.0;

    float shadow = PCF(fragLightSpaceNDC);

    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    // Ambient
    ambient = lightData.ambient;

    vec3 dirToLight = vec3(lightData.position);
    if (lightData.position.w > 0.0)
    {
        dirToLight = fragPosition - lightData.position.xyz;
    }
    dirToLight = -normalize(dirToLight);

    // Diffuse
    float diff = max(dot(dirToLight, fragNormal), 0.0);
    diffuse = diff * lightData.diffuse;

    // Specular
    float shininess = 8.0;
    vec3 dirToEye = normalize(cameraData.position - fragPosition);
    vec3 r = reflect(-dirToLight, fragNormal);
    float spec = pow(max(dot(dirToEye, r), 0.0), shininess);
    specular = spec * lightData.specular;

    vec3 finalColor = (ambient + (1.0 - shadow) * (diffuse + specular)) * fragColor;
    finalFragColor = vec4(finalColor, 1.0);
}

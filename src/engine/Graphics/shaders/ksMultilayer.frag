#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragViewDir;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
    float nearPlane;
    float farPlane;
};

layout(push_constant) uniform PushConstants {
    int layerCount;
    float fresnelPower;
    float pad0;
    float pad1;
} pc;

layout(set = 0, binding = 1) uniform sampler2D layer0Texture;
layout(set = 0, binding = 2) uniform sampler2D layer0Normal;
layout(set = 0, binding = 3) uniform sampler2D layer1Texture;
layout(set = 0, binding = 4) uniform sampler2D layer1Normal;

layout(location = 0) out vec4 outColor;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(fragViewDir);
    float NdotV = max(dot(N, V), 0.0);

    vec3 baseColor0 = texture(layer0Texture, fragTexCoord).rgb;
    vec3 normal0 = texture(layer0Normal, fragTexCoord).rgb * 2.0 - 1.0;
    vec3 color0 = baseColor0;

    vec3 baseColor1 = texture(layer1Texture, fragTexCoord).rgb;
    vec3 normal1 = texture(layer1Normal, fragTexCoord).rgb * 2.0 - 1.0;
    vec3 color1 = baseColor1;

    float fresnel = pow(1.0 - NdotV, pc.fresnelPower > 0.0 ? pc.fresnelPower : 3.0);

    vec3 color = mix(color0, color1, fresnel);
    color = mix(color, color1 * 1.2, fresnel * 0.3);

    outColor = vec4(color, 1.0);
}

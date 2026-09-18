#version 450

layout(push_constant) uniform TerrainPC {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 fogColor;
    float fogDensity;
    float maxHeight;
    float time;
    uint layerCount;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out float fragHeight;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(pc.model) * inNormal);
    fragUV = inUV;
    fragHeight = inPosition.y / pc.maxHeight;
    gl_Position = pc.proj * pc.view * worldPos;
}

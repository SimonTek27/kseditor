#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(push_constant) uniform PC {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightDirection;
    float ambientIntensity;
    float coreTemp;
    float surfaceTemp;
    float wearLevel;
} pc;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out float v_coreTemp;
layout(location = 4) out float v_surfaceTemp;
layout(location = 5) out float v_wearLevel;

void main() {
    gl_Position = pc.projection * pc.view * pc.model * vec4(position, 1.0);
    v_normal = normalize(mat3(pc.model) * normal);
    v_worldPos = (pc.model * vec4(position, 1.0)).xyz;
    v_coreTemp = pc.coreTemp;
    v_surfaceTemp = pc.surfaceTemp;
    v_wearLevel = pc.wearLevel;
    v_uv = uv;
}

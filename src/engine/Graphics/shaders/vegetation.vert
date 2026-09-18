#version 450

layout(push_constant) uniform VegetationPC {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 windDir;
    float windStrength;
    float windPhase;
    float time;
    float windAmplitude;
    float windFrequency;
    float windResponse;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inInstanceData;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

void main() {
    vec3 pos = inPosition;
    float heightFactor = max(0.0, pos.y);
    vec3 windOffset = pc.windDir.xyz * pc.windStrength * pc.windAmplitude;
    float wave = sin(pc.time * pc.windFrequency + pos.x * 0.5 + pos.z * 0.3) * 0.5 + 0.5;
    float windEffect = heightFactor * pc.windResponse * wave;
    pos += windOffset * windEffect;
    vec4 worldPos = pc.model * vec4(pos, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(pc.model) * inNormal);
    fragUV = inUV;
    gl_Position = pc.proj * pc.view * worldPos;
}

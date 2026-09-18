#version 450

layout(push_constant) uniform WaterPC {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 cameraPos;
    vec4 deepColor;
    vec4 shallowColor;
    vec4 foamColor;
    float time;
    float seaLevel;
    float fresnelPower;
    float normalStrength;
    float texScale;
    float opacity;
} pc;

struct WaveLayer {
    float amplitude;
    float wavelength;
    float speed;
    vec2 direction;
    float steepness;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 fragViewDir;

void main() {
    vec3 pos = inPosition;
    pos.y = pc.seaLevel;
    float dx = 0.0;
    float dz = 0.0;
    vec2 dir1 = normalize(vec2(1.0, 0.3));
    float k1 = 6.28318 / 15.0;
    float w1 = 1.2 * k1;
    float phase1 = k1 * dot(dir1, pos.xz) - w1 * pc.time;
    pos.y += 0.8 * cos(phase1);
    dx += 0.8 * k1 * dir1.x * sin(phase1);
    dz += 0.8 * k1 * dir1.y * sin(phase1);
    vec2 dir2 = normalize(vec2(-0.3, 1.0));
    float k2 = 6.28318 / 8.0;
    float w2 = 1.5 * k2;
    float phase2 = k2 * dot(dir2, pos.xz) - w2 * pc.time;
    pos.y += 0.4 * cos(phase2);
    dx += 0.4 * k2 * dir2.x * sin(phase2);
    dz += 0.4 * k2 * dir2.y * sin(phase2);
    vec3 calcNormal = normalize(vec3(-dx, 1.0, -dz));
    fragWorldPos = (pc.model * vec4(pos, 1.0)).xyz;
    fragNormal = normalize(mat3(pc.model) * calcNormal);
    fragUV = pos.xz * pc.texScale;
    fragViewDir = normalize(pc.cameraPos.xyz - fragWorldPos);
    gl_Position = pc.proj * pc.view * vec4(fragWorldPos, 1.0);
}

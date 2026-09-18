#version 450

layout(push_constant) uniform ParticlePC {
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 cameraUp;
    vec4 cameraRight;
    float time;
    float globalWind;
} pc;

struct Particle {
    vec4 position;
    vec4 velocity;
    vec4 color;
    float size;
    float life;
    float maxLife;
    float rotation;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float inSize;
layout(location = 4) in float inLife;
layout(location = 5) in float inMaxLife;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragLife;

void main() {
    float lifeRatio = 1.0 - (inLife / max(inMaxLife, 0.001));
    float alpha = inColor.a * (1.0 - lifeRatio);
    alpha *= step(0.001, inLife);
    vec3 right = pc.cameraRight.xyz * inSize;
    vec3 up = pc.cameraUp.xyz * inSize;
    vec3 worldPos = inPosition + right * inPosition.x + up * inPosition.y;
    gl_Position = pc.proj * pc.view * vec4(worldPos, 1.0);
    fragUV = inUV;
    fragColor = vec4(inColor.rgb, alpha);
    fragLife = lifeRatio;
}

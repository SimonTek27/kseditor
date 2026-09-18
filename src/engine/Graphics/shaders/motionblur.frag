#version 450

layout(push_constant) uniform MotionBlurPC {
    mat4 viewProj;
    mat4 prevViewProj;
    vec2 screenSize;
    float strength;
    int sampleCount;
    float maxBlurLength;
} pc;

layout(set = 1, binding = 0) uniform sampler2D colorBuffer;
layout(set = 1, binding = 1) uniform sampler2D velocityBuffer;
layout(set = 1, binding = 2) uniform sampler2D depthBuffer;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 velocity = texture(velocityBuffer, fragUV).rg;
    float speed = length(velocity);
    float blurAmount = min(speed * pc.strength, pc.maxBlurLength);
    vec4 color = texture(colorBuffer, fragUV);
    vec2 blurDir = normalize(velocity + 0.0001) * blurAmount;
    vec4 result = color;
    float totalWeight = 1.0;
    for (int i = 1; i <= pc.sampleCount; ++i) {
        float t = float(i) / float(pc.sampleCount);
        vec2 offset = blurDir * t;
        vec4 s = texture(colorBuffer, fragUV + offset);
        float weight = 1.0 - t * 0.5;
        result += s * weight;
        totalWeight += weight;
        s = texture(colorBuffer, fragUV - offset);
        result += s * weight;
        totalWeight += weight;
    }
    result /= totalWeight;
    outColor = result;
}

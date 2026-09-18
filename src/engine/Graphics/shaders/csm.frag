#version 450

layout(push_constant) uniform CSMPC {
    mat4 cascadeViewProj[4];
    vec4 cascadeSplits;
    vec4 lightDir;
    vec4 lightColor;
    float shadowBias;
    float normalBias;
    int cascadeCount;
    int pcfSamples;
    float filterRadius;
} pc;

layout(set = 1, binding = 0) uniform sampler2DArrayShadow shadowMaps;
layout(set = 1, binding = 1) uniform sampler2D gDepth;
layout(set = 1, binding = 2) uniform sampler2D gNormal;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out float outShadow;

float getCSMSplitIndex(float viewDepth, out float blendFactor) {
    for (int i = 0; i < pc.cascadeCount; ++i) {
        if (viewDepth < pc.cascadeSplits[i]) {
            blendFactor = 0.0;
            return float(i);
        }
    }
    blendFactor = 1.0;
    return float(pc.cascadeCount - 1);
}

float sampleShadowMap(int cascadeIdx, vec3 worldPos, float viewDepth) {
    vec4 shadowCoord = pc.cascadeViewProj[cascadeIdx] * vec4(worldPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    shadowCoord.y = 1.0 - shadowCoord.y;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps, 0).xy);
    for (int x = -pc.pcfSamples; x <= pc.pcfSamples; ++x) {
        for (int y = -pc.pcfSamples; y <= pc.pcfSamples; ++y) {
            vec2 offset = vec2(x, y) * texelSize * pc.filterRadius;
            vec4 coord = vec4(shadowCoord.xy + offset, float(cascadeIdx), shadowCoord.z - pc.shadowBias);
            shadow += texture(shadowMaps, coord);
        }
    }
    float samples = float((2 * pc.pcfSamples + 1) * (2 * pc.pcfSamples + 1));
    return shadow / samples;
}

void main() {
    float depth = texture(gDepth, fragUV).r;
    vec3 normal = texture(gNormal, fragUV).rgb * 2.0 - 1.0;
    float viewDepth = depth * 1000.0;
    float blendFactor;
    int cascadeIdx = int(getCSMSplitIndex(viewDepth, blendFactor));
    cascadeIdx = clamp(cascadeIdx, 0, pc.cascadeCount - 1);
    float shadow = sampleShadowMap(cascadeIdx, vec3(fragUV, 0.0), viewDepth);
    if (cascadeIdx < pc.cascadeCount - 1 && blendFactor > 0.0) {
        float nextShadow = sampleShadowMap(cascadeIdx + 1, vec3(fragUV, 0.0), viewDepth);
        shadow = mix(shadow, nextShadow, blendFactor);
    }
    outShadow = shadow;
}

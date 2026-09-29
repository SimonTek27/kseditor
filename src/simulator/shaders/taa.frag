#version 450

// Temporal resolve. Sits between the deferred lighting pass (which now
// renders into an offscreen HDR target) and the swapchain, and writes its
// result twice: location 0 to the swapchain for display, location 1 to the
// history target that the next frame samples.
//
// Reprojection uses the *unjittered* view/projection pair from the UBO, so
// the motion vector is jitter-free; the actual sample point is then the
// jittered pixel UV plus that motion. A 3x3 neighbourhood clamp on the
// history kills most of the ghosting that reprojection alone would leave
// behind (moving objects, camera cuts), and taaParams.x drops to 0 on the
// first frame after a resize so undefined history never shows.

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outSwapchain;
layout(location = 1) out vec4 outHistory;

layout(set = 0, binding = 0) uniform FrameData {
    vec4 sunDirection;
    vec4 sunColor;
    mat4 cascadeViewProj[3];
    vec4 cascadeSplits;
    vec4 cameraPos;
    mat4 viewProj;         // unjittered, current frame
    mat4 prevViewProj;     // unjittered, previous frame
    vec4 taaParams;        // x = history feedback (0 disables TAA)
} frame;

layout(set = 0, binding = 1) uniform sampler2D gbufWorldPos;
layout(set = 0, binding = 2) uniform sampler2D currentHDR;
layout(set = 0, binding = 3) uniform sampler2D history;

void main() {
    vec3 cur = texture(currentHDR, vUV).rgb;
    float feedback = frame.taaParams.x;

    vec4 coverage = texture(gbufWorldPos, vUV);
    vec2 histUV = vUV;
    if (coverage.w > 0.5 && feedback > 0.0) {
        vec4 curClip = frame.viewProj * vec4(coverage.xyz, 1.0);
        vec4 prevClip = frame.prevViewProj * vec4(coverage.xyz, 1.0);
        vec2 curUV = curClip.xy / curClip.w * 0.5 + 0.5;
        vec2 prevUV = prevClip.xy / prevClip.w * 0.5 + 0.5;
        histUV = vUV + (prevUV - curUV);
    } else {
        feedback = 0.0;
    }

    if (histUV.x < 0.0 || histUV.x > 1.0 || histUV.y < 0.0 || histUV.y > 1.0) feedback = 0.0;

    vec3 result = cur;
    if (feedback > 0.0) {
        vec2 texel = 1.0 / vec2(textureSize(currentHDR, 0));
        vec3 nMin = vec3(1e20);
        vec3 nMax = vec3(-1e20);
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                vec3 s = texture(currentHDR, vUV + vec2(float(x), float(y)) * texel).rgb;
                nMin = min(nMin, s);
                nMax = max(nMax, s);
            }
        }
        vec3 hist = clamp(texture(history, histUV).rgb, nMin, nMax);
        result = mix(cur, hist, feedback);
    }

    outSwapchain = vec4(result, 1.0);
    outHistory = vec4(result, 1.0);
}

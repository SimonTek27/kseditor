#version 450

layout(push_constant) uniform FXAAPC {
    vec2 screenSize;
    float subpixelQuality;
    float edgeThreshold;
    float edgeThresholdMin;
} pc;

layout(set = 1, binding = 0) uniform sampler2D colorBuffer;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec2 texelSize = 1.0 / pc.screenSize;
    vec3 rgbNW = texture(colorBuffer, fragUV + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = texture(colorBuffer, fragUV + vec2(1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = texture(colorBuffer, fragUV + vec2(-1.0, 1.0) * texelSize).rgb;
    vec3 rgbSE = texture(colorBuffer, fragUV + vec2(1.0, 1.0) * texelSize).rgb;
    vec3 rgbM = texture(colorBuffer, fragUV).rgb;
    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);
    float lumaM = luma(rgbM);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * pc.subpixelQuality, 0.001);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(8.0), max(vec2(-8.0), dir * rcpDirMin)) * texelSize;
    vec3 rgbA = 0.5 * (texture(colorBuffer, fragUV + dir * (1.0 / 3.0 - 0.5)).rgb +
                         texture(colorBuffer, fragUV + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(colorBuffer, fragUV + dir * -0.5).rgb +
                                        texture(colorBuffer, fragUV + dir * 0.5).rgb);
    float lumaB = luma(rgbB);
    if (lumaB < lumaMin || lumaB > lumaMax) {
        outColor = vec4(rgbA, 1.0);
    } else {
        outColor = vec4(rgbB, 1.0);
    }
}

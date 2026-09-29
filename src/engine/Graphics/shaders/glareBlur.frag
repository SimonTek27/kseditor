#version 450

// Separable 9-tap Gaussian blur, one direction per invocation (horizontal
// then vertical, ping-ponged from the C++ side). Run N times back-to-back
// (N driven by PPFilterPreset::Glare::quality) for a wider, softer glare.

layout(location = 0) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(push_constant) uniform PushConstants {
    float texelSizeX;
    float texelSizeY;
    float direction; // 0 = horizontal, 1 = vertical
    float pad0;
} pc;

layout(location = 0) out vec4 outColor;

const float WEIGHTS[5] = float[5](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main() {
    vec2 texel = (pc.direction > 0.5) ? vec2(0.0, pc.texelSizeY) : vec2(pc.texelSizeX, 0.0);

    vec3 result = texture(sourceTexture, fragTexCoord).rgb * WEIGHTS[0];
    for (int i = 1; i < 5; ++i) {
        vec2 offset = texel * float(i);
        result += texture(sourceTexture, fragTexCoord + offset).rgb * WEIGHTS[i];
        result += texture(sourceTexture, fragTexCoord - offset).rgb * WEIGHTS[i];
    }

    outColor = vec4(result, 1.0);
}

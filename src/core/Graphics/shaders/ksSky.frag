#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragViewDir;

layout(set = 0, binding = 1) uniform samplerCube skyTexture;

layout(push_constant) uniform PushConstants {
    vec4 horizonColor;
    vec4 zenithColor;
    vec4 groundColor;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 dir = normalize(fragViewDir);
    vec4 skyColor = texture(skyTexture, dir);

    float y = dir.y * 0.5 + 0.5;

    vec3 gradientColor;
    if (dir.y > 0.0) {
        gradientColor = mix(pc.horizonColor.rgb, pc.zenithColor.rgb, pow(y, 1.5));
    } else {
        gradientColor = mix(pc.horizonColor.rgb, pc.groundColor.rgb, pow(1.0 - y, 1.5));
    }

    vec3 finalColor = mix(skyColor.rgb, gradientColor, 0.5);
    outColor = vec4(finalColor, 1.0);
}

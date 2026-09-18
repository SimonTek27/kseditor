#version 450

layout(set = 1, binding = 0) uniform sampler2D particleTexture;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float fragLife;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 centered = fragUV * 2.0 - 1.0;
    float dist = dot(centered, centered);
    if (dist > 1.0) discard;
    vec4 texColor = texture(particleTexture, fragUV);
    float alpha = fragColor.a * texColor.a;
    alpha *= 1.0 - fragLife * 0.5;
    vec3 color = fragColor.rgb * texColor.rgb;
    outColor = vec4(color, alpha);
}

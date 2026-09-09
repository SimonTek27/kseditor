#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(push_constant) uniform PushConstants {
    float exposure;
    float contrast;
    float saturation;
    float vignetteIntensity;
    float chromaticAberration;
    float gamma;
    float brightness;
    float pad0;
} pc;

layout(location = 0) out vec4 outColor;

vec3 tonemapACES(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 color;

    // Chromatic aberration
    if (pc.chromaticAberration > 0.001) {
        vec2 dir = uv - 0.5;
        float r = texture(inputTexture, uv + dir * pc.chromaticAberration).r;
        float g = texture(inputTexture, uv).g;
        float b = texture(inputTexture, uv - dir * pc.chromaticAberration).b;
        color = vec3(r, g, b);
    } else {
        color = texture(inputTexture, uv).rgb;
    }

    // Exposure
    color *= pc.exposure;

    // Tone mapping
    color = tonemapACES(color);

    // Brightness
    color *= pc.brightness;

    // Contrast
    color = (color - 0.5) * pc.contrast + 0.5;

    // Saturation
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luma), color, pc.saturation);

    // Vignette
    if (pc.vignetteIntensity > 0.001) {
        float dist = distance(uv, vec2(0.5));
        float vig = smoothstep(0.8, 0.2, dist);
        color *= mix(1.0, vig, pc.vignetteIntensity);
    }

    // Gamma
    color = pow(color, vec3(1.0 / max(pc.gamma, 0.1)));

    outColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}

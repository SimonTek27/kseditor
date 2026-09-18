#version 450

layout(push_constant) uniform ToneMapPC {
    mat4 proj;
    float exposure;
    float gamma;
    float whitePoint;
    float saturation;
    float contrast;
    float shadows;
    float highlights;
    vec3 colorFilter;
    float temperature;
    float tint;
    int mode;
} pc;

layout(set = 1, binding = 0) uniform sampler2D hdrBuffer;
layout(set = 1, binding = 1) uniform sampler2D bloomBuffer;
layout(set = 1, binding = 2) uniform sampler2D aoBuffer;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 Reinhard(vec3 x) {
    return x / (1.0 + x);
}

vec3 Uncharted2Partial(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2(vec3 x) {
    float exposureBias = pc.exposure;
    vec3 curr = Uncharted2Partial(x * exposureBias);
    vec3 whiteScale = 1.0 / Uncharted2Partial(vec3(pc.whitePoint));
    return curr * whiteScale;
}

vec3 Filmic(vec3 x) {
    vec3 X = max(vec3(0.0), x - 0.004);
    vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return pow(result, vec3(2.2));
}

void main() {
    vec3 color = texture(hdrBuffer, fragUV).rgb;
    vec3 bloom = texture(bloomBuffer, fragUV).rgb;
    float ao = texture(aoBuffer, fragUV).r;
    color += bloom * 0.3;
    color *= ao;
    color *= pc.exposure;
    switch (pc.mode) {
        case 0: color = color; break;
        case 1: color = Reinhard(color); break;
        case 2: color = ACESFilm(color); break;
        case 3: color = Uncharted2(color); break;
        case 4: color = Filmic(color); break;
    }
    color = pow(color, vec3(1.0 / pc.gamma));
    float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(lum), color, pc.saturation);
    color = (color - 0.5) * pc.contrast + 0.5;
    color *= pc.colorFilter;
    color.r += pc.temperature * 0.05;
    color.g += pc.tint * 0.02;
    color = clamp(color, 0.0, 1.0);
    outColor = vec4(color, 1.0);
}

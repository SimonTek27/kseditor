#version 450

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in float v_coreTemp;
layout(location = 4) in float v_surfaceTemp;
layout(location = 5) in float v_wearLevel;

layout(push_constant) uniform PC {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightDirection;
    float ambientIntensity;
    float coreTemp;
    float surfaceTemp;
    float wearLevel;
} pc;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform ThermalColors {
    vec3 coldColor;
    vec3 normalColor;
    vec3 hotColor;
    vec3 overheatedColor;
    vec3 wearColor;
    float coldThresh;
    float normalThresh;
    float overheatedThresh;
} thermal;

void main() {
    vec3 N = normalize(v_normal);
    float temp = clamp(v_surfaceTemp, -50.0, 160.0);

    float tNorm;
    if (temp <= thermal.coldThresh) {
        tNorm = 0.0;
    } else if (temp >= thermal.overheatedThresh) {
        tNorm = 1.0;
    } else {
        tNorm = (temp - thermal.coldThresh) / (thermal.overheatedThresh - thermal.coldThresh);
        tNorm = clamp(tNorm, 0.0, 1.0);
    }

    vec3 baseColor = mix(thermal.normalColor, thermal.overheatedColor, tNorm);

    if (v_wearLevel > 0.0) {
        float gray = dot(baseColor, vec3(0.3, 0.59, 0.11));
        baseColor = mix(baseColor, vec3(gray), v_wearLevel * 0.7);
    }

    float coreInfluence = clamp(v_coreTemp / 120.0, 0.0, 1.0);
    baseColor = mix(baseColor, baseColor + vec3(0.1, -0.05, -0.1), coreInfluence * 0.2);

    float diffuse = max(dot(N, normalize(-pc.lightDirection)), 0.1);
    vec3 finalColor = baseColor * diffuse * pc.ambientIntensity + vec3(0.1, 0.1, 0.1);

    outColor = vec4(finalColor, 1.0);
}

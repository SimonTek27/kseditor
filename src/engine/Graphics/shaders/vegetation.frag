#version 450

layout(push_constant) uniform VegetationPC {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 windDir;
    float windStrength;
    float windPhase;
    float time;
    float windAmplitude;
    float windFrequency;
    float windResponse;
} pc;

layout(set = 1, binding = 0) uniform sampler2D diffuseTex;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(diffuseTex, fragUV);
    if (texColor.a < 0.5) discard;
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.sunDir.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec3 ambient = texColor.rgb * 0.2;
    vec3 diffuse = texColor.rgb * pc.sunColor.rgb * NdotL;
    vec3 color = ambient + diffuse;
    outColor = vec4(color, texColor.a);
}

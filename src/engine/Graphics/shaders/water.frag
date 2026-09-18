#version 450

layout(push_constant) uniform WaterPC {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 cameraPos;
    vec4 deepColor;
    vec4 shallowColor;
    vec4 foamColor;
    float time;
    float seaLevel;
    float fresnelPower;
    float normalStrength;
    float texScale;
    float opacity;
} pc;

layout(set = 1, binding = 0) uniform sampler2D normalMap1;
layout(set = 1, binding = 1) uniform sampler2D normalMap2;
layout(set = 1, binding = 2) uniform sampler2D foamMap;
layout(set = 1, binding = 3) uniform samplerCube reflectionMap;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragViewDir;

layout(location = 0) out vec4 outColor;

float fresnel(vec3 viewDir, vec3 normal, float power) {
    return pow(1.0 - max(dot(viewDir, normal), 0.0), power);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(fragViewDir);
    vec2 uv1 = fragUV + vec2(pc.time * 0.02, pc.time * 0.01);
    vec2 uv2 = fragUV * 1.3 + vec2(-pc.time * 0.015, pc.time * 0.025);
    vec3 normal1 = texture(normalMap1, uv1).rgb * 2.0 - 1.0;
    vec3 normal2 = texture(normalMap2, uv2).rgb * 2.0 - 1.0;
    vec3 blendedNormal = normalize(N + (normal1 + normal2) * pc.normalStrength * 0.5);
    vec3 L = normalize(-pc.sunDir.xyz);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(blendedNormal, L), 0.0);
    float NdotH = max(dot(blendedNormal, H), 0.0);
    float specular = pow(NdotH, 256.0) * 2.0;
    float fresnelFactor = fresnel(V, blendedNormal, pc.fresnelPower);
    vec3 waterColor = mix(pc.deepColor.rgb, pc.shallowColor.rgb, fresnelFactor);
    vec3 R = reflect(-V, blendedNormal);
    vec3 reflection = texture(reflectionMap, R).rgb;
    waterColor = mix(waterColor, reflection, fresnelFactor * 0.8);
    waterColor += pc.sunColor.rgb * specular * 0.5;
    float foam = texture(foamMap, fragUV * 5.0 + vec2(pc.time * 0.1)).r;
    foam *= smoothstep(0.6, 1.0, 1.0 - fresnelFactor);
    waterColor = mix(waterColor, pc.foamColor.rgb, foam * 0.6);
    vec3 sunContrib = pc.sunColor.rgb * NdotL * 0.3;
    waterColor += sunContrib;
    float edgeFade = smoothstep(0.0, 0.3, V.y);
    waterColor *= edgeFade;
    outColor = vec4(waterColor, pc.opacity * edgeFade);
}

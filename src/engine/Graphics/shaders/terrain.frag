#version 450

layout(push_constant) uniform TerrainPC {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 fogColor;
    float fogDensity;
    float maxHeight;
    float time;
    uint layerCount;
} pc;

layout(set = 1, binding = 0) uniform sampler2D splatmap;
layout(set = 1, binding = 1) uniform sampler2D layerDiffuse[8];
layout(set = 1, binding = 2) uniform sampler2D layerNormal[8];

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in float fragHeight;

layout(location = 0) out vec4 outColor;

vec3 perturbNormal(vec3 N, vec3 V, vec2 texcoord) {
    vec3 map = texture(layerNormal[0], texcoord * 10.0).rgb * 2.0 - 1.0;
    map.xy = normalize(map.xy);
    return normalize(N + map * 0.3);
}

void main() {
    vec4 splat = texture(splatmap, fragUV);
    float weights[4] = float[4](splat.r, splat.g, splat.b, splat.a);
    float totalWeight = 0.0;
    vec3 diffuse = vec3(0.0);
    vec2 tiledUV = fragUV * 40.0;
    for (int i = 0; i < min(pc.layerCount, 4); i++) {
        if (weights[i] > 0.01) {
            diffuse += texture(layerDiffuse[i], tiledUV).rgb * weights[i];
            totalWeight += weights[i];
        }
    }
    if (totalWeight > 0.0) diffuse /= totalWeight;
    else diffuse = vec3(0.4, 0.35, 0.3);

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.sunDir.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec3 ambient = diffuse * 0.15;
    vec3 lighting = ambient + diffuse * pc.sunColor.rgb * NdotL;

    float heightFog = exp(-fragWorldPos.y * pc.fogDensity * 2.0);
    lighting = mix(lighting, pc.fogColor.rgb, heightFog * 0.5);

    outColor = vec4(lighting, 1.0);
}

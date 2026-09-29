#version 450

// Deferred lighting + volumetric atmosphere.
//
// Reads the GBuffer, re-runs the same directional-light + cascaded-shadow
// model native_forward.frag used (so the base image is recognisably the same
// scene), then layers two things the forward path never had:
//
//   1. analytic exponential height fog integrated along the view ray, and
//   2. a shadow-map raymarch (FOG_STEPS taps) modulated by a Henyey-
//      Greenstein phase function — i.e. real light shafts / volumetric
//      shadows, not a flat fog colour.
//
// Coverage test: geometry writes w = 1.0 into RT2, cleared pixels keep 0,
// so background pixels take the early-out and reproduce the forward
// path's clear colour exactly.

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform FrameData {
    vec4 sunDirection;      // xyz, w unused
    vec4 sunColor;          // rgb, a = intensity
    mat4 cascadeViewProj[3];
    vec4 cascadeSplits;     // view-space far distance of each cascade (xyz)
    vec4 cameraPos;         // xyz, w unused
} frame;

layout(set = 0, binding = 1) uniform sampler2DArray shadowCascades;
layout(set = 0, binding = 2) uniform sampler2D gbufAlbedo;
layout(set = 0, binding = 3) uniform sampler2D gbufNormal;
layout(set = 0, binding = 4) uniform sampler2D gbufWorldPos;

const vec3 SKY_COLOR = vec3(0.35, 0.55, 0.75);
const float PI = 3.14159265;

const float FOG_DENSITY = 0.0028;
const float FOG_HEIGHT_FALLOFF = 0.018;
const float FOG_ANISOTROPY = 0.55;
const float FOG_MAX = 0.9;
const float VOL_STRENGTH = 4.0;
const int FOG_STEPS = 12;

int cascadeFor(float viewDist) {
    if (viewDist < frame.cascadeSplits.x) return 0;
    if (viewDist < frame.cascadeSplits.y) return 1;
    return 2;
}

// Same contract as native_forward.frag's sampleShadow(): 1.0 = lit,
// 0.35 = shadowed (never fully black, avoids acne-looking terminators),
// 1.0 = outside the light frustum.
float sampleShadow(vec3 worldPos, int cascadeIndex) {
    vec4 lightClip = frame.cascadeViewProj[cascadeIndex] * vec4(worldPos, 1.0);
    vec3 ndc = lightClip.xyz / lightClip.w;
    vec2 uv = ndc.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 1.0;
    float storedDepth = texture(shadowCascades, vec3(uv, float(cascadeIndex))).r;
    float bias = 0.0015;
    return (ndc.z - bias > storedDepth) ? 0.35 : 1.0;
}

// Analytic integral of density * exp(-height * falloff) along the segment
// camPos -> worldPos. Degenerates to the uniform-fog case as dy -> 0.
float heightFogAmount(vec3 camPos, vec3 worldPos) {
    vec3 d = worldPos - camPos;
    float dist = length(d);
    float dy = d.y;
    float k = FOG_DENSITY * exp(-camPos.y * FOG_HEIGHT_FALLOFF);
    if (abs(dy) < 1e-3) return k * dist;
    return k * dist * (1.0 - exp(-FOG_HEIGHT_FALLOFF * dy)) / (FOG_HEIGHT_FALLOFF * dy);
}

// Transmittance of sunlight through the shadow cascades averaged along the
// camera ray — this is what turns the fog into shafts instead of soup.
vec3 volumetricInscatter(vec3 camPos, vec3 worldPos, vec3 sunRad) {
    vec3 delta = worldPos - camPos;
    float dist = length(delta);
    if (dist < 1e-3) return vec3(0.0);

    vec3 rayDir = delta / dist;
    vec3 L = normalize(-frame.sunDirection.xyz);
    float cosT = dot(rayDir, L);
    float g = FOG_ANISOTROPY;
    float denom = 1.0 + g * g - 2.0 * g * cosT;
    float phase = (1.0 - g * g) / (4.0 * PI * pow(max(denom, 1e-4), 1.5));

    float stepLen = dist / float(FOG_STEPS);
    float lit = 0.0;
    for (int i = 0; i < FOG_STEPS; ++i) {
        float t = stepLen * (float(i) + 0.5);
        vec3 p = camPos + rayDir * t;
        lit += sampleShadow(p, cascadeFor(t));
    }
    lit /= float(FOG_STEPS);

    return sunRad * phase * lit * VOL_STRENGTH;
}

void main() {
    vec4 coverage = texture(gbufWorldPos, vUV);
    if (coverage.w < 0.5) {
        outColor = vec4(SKY_COLOR, 1.0);
        return;
    }

    vec3 worldPos = coverage.xyz;
    vec4 albedoAO = texture(gbufAlbedo, vUV);
    vec4 normalRough = texture(gbufNormal, vUV);

    vec3 albedo = albedoAO.rgb;
    float ao = albedoAO.a;
    vec3 N = normalize(normalRough.xyz);
    float roughness = clamp(normalRough.w, 0.05, 1.0);

    vec3 camPos = frame.cameraPos.xyz;
    float viewDist = length(worldPos - camPos);
    vec3 V = normalize(camPos - worldPos);
    vec3 L = normalize(-frame.sunDirection.xyz);
    vec3 sunRad = frame.sunColor.rgb * frame.sunColor.a;

    float NdotL = max(dot(N, L), 0.0);
    float shadow = sampleShadow(worldPos, cascadeFor(viewDist));

    vec3 ambient = albedo * 0.25 * ao;
    vec3 diffuse = albedo * sunRad * NdotL * shadow;

    float a = roughness * roughness;
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    float dd = NdotH * NdotH * (a * a - 1.0) + 1.0;
    float D = (a * a) / (PI * dd * dd);
    float k = a * 0.5;
    float G = (NdotL / (NdotL * (1.0 - k) + k)) * (NdotV / (NdotV * (1.0 - k) + k));
    vec3 F0 = vec3(0.04);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
    vec3 specular = ((D * G * F) / (4.0 * NdotL * NdotV + 1e-4)) * NdotL * shadow;

    vec3 lit = ambient + diffuse + specular;

    float fogT = clamp(1.0 - exp(-heightFogAmount(camPos, worldPos)), 0.0, FOG_MAX);
    vec3 inscatter = volumetricInscatter(camPos, worldPos, sunRad);
    vec3 fogColor = SKY_COLOR + inscatter;

    outColor = vec4(mix(lit, fogColor, fogT), 1.0);
}

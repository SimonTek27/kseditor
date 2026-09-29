#version 450

// GBuffer MRT. Three render targets, all cleared to zero, with coverage
// encoded in RT0.a / RT2.w (geometry always writes 1.0 there) so the
// deferred lighting pass can tell "no geometry here" from a real surface.
//
//   RT0  R8G8B8A8_UNORM    rgb = albedo, a = ambient occlusion
//   RT1  R16G16B16A16_SF   xyz = world normal, w = roughness
//   RT2  R16G16B16A16_SF   xyz = world position, w = coverage (1 = drawn)

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;

layout(location = 0) out vec4 outAlbedoAO;
layout(location = 1) out vec4 outNormalRoughness;
layout(location = 2) out vec4 outWorldPosCoverage;

void main() {
    vec3 N = normalize(fragNormal);
    if (!gl_FrontFacing) N = -N;

    outAlbedoAO = vec4(fragColor.rgb, 1.0);
    outNormalRoughness = vec4(N, 0.75);
    outWorldPosCoverage = vec4(fragWorldPos, 1.0);
}

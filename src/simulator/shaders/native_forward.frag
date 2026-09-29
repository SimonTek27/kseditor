#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform FrameData {
    vec4 sunDirection;      // xyz, w unused
    vec4 sunColor;          // rgb, a = intensity
    mat4 cascadeViewProj[3];
    vec4 cascadeSplits;     // view-space far distance of each cascade (xyz), w unused
    vec4 cameraPos;         // xyz, w unused
} frame;

layout(set = 0, binding = 1) uniform sampler2DArray shadowCascades;

float sampleShadow(vec3 worldPos, int cascadeIndex) {
    vec4 lightClip = frame.cascadeViewProj[cascadeIndex] * vec4(worldPos, 1.0);
    vec3 ndc = lightClip.xyz / lightClip.w;
    vec2 uv = ndc.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 1.0;

    // ksShadow.frag writes linear NDC depth (gl_FragCoord.z) as the stored
    // value, so compare against that same space rather than a separate
    // depth attachment.
    float storedDepth = texture(shadowCascades, vec3(uv, float(cascadeIndex))).r;
    float bias = 0.0015;
    return (ndc.z - bias > storedDepth) ? 0.35 : 1.0; // 0.35 = not fully black, avoids unlit-looking shadow acne on large casters
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-frame.sunDirection.xyz);
    float NdotL = max(dot(N, L), 0.0);

    // Pick the nearest cascade whose split still covers this fragment's
    // view-space depth (cameraPos to fragment distance is used as a cheap
    // stand-in for true view-space Z, which is fine for a single forward
    // draw with no wide-FOV distortion concerns here).
    float viewDist = length(fragWorldPos - frame.cameraPos.xyz);
    int cascadeIndex = 2;
    if (viewDist < frame.cascadeSplits.x) cascadeIndex = 0;
    else if (viewDist < frame.cascadeSplits.y) cascadeIndex = 1;

    float shadow = sampleShadow(fragWorldPos, cascadeIndex);

    vec3 ambient = fragColor.rgb * 0.25;
    vec3 diffuse = fragColor.rgb * frame.sunColor.rgb * frame.sunColor.a * NdotL * shadow;

    outColor = vec4(ambient + diffuse, fragColor.a);
}

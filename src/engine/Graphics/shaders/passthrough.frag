#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec4 fragVertexColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 baseColor;
    vec4 sunDirection;
    vec4 sunColor;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.sunDirection.xyz);
    float NdotL = max(dot(N, L), 0.0);

    vec3 ambient = pc.baseColor.rgb * 0.3;
    vec3 diffuse = pc.sunColor.rgb * pc.baseColor.rgb * NdotL;
    vec3 color = ambient + diffuse;

    outColor = vec4(color, pc.baseColor.a);
}

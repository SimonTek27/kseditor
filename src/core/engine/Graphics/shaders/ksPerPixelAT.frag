#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
    float nearPlane;
    float farPlane;
};

layout(push_constant) uniform PushConstants {
    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
    vec4 emissiveColor;
    vec3 lightDirection;
    float specularEXP;
    vec4 lightColor;
    float alpha;
    float doubleSided;
    float alphaRef;
    float pad1;
} pc;

layout(set = 0, binding = 1) uniform sampler2D diffuseMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D specularMap;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(diffuseMap, fragTexCoord);
    if (texColor.a < pc.alphaRef) discard;

    vec3 N = normalize(fragNormal);
    if (pc.doubleSided > 0.5 && !gl_FrontFacing) N = -N;

    vec3 L = normalize(-pc.lightDirection);
    vec3 V = normalize(pc.cameraPosition.xyz - fragWorldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 baseColor = (pc.diffuseColor * texColor).rgb;
    vec3 ambient = pc.ambientColor.rgb * baseColor;
    vec3 diffuse = pc.lightColor.rgb * baseColor * NdotL;

    float specPow = max(pc.specularEXP, 1.0);
    vec3 specColor = pc.specularColor.rgb * texture(specularMap, fragTexCoord).rgb;
    vec3 specular = pc.lightColor.rgb * specColor * pow(NdotH, specPow);

    vec3 color = ambient + diffuse + specular + pc.emissiveColor.rgb * pc.emissiveColor.a;
    outColor = vec4(color, texColor.a * pc.alpha);
}

#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec4 inColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
    float nearPlane;
    float farPlane;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragViewDir;
layout(location = 4) out vec3 fragTangent;
layout(location = 5) out vec3 fragBitangent;

void main() {
    vec4 worldPos = push.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = camera.viewProjection * worldPos;

    fragWorldPos = worldPos.xyz;
    fragViewDir = normalize(camera.cameraPosition.xyz - fragWorldPos);
    
    mat3 nMat = mat3(push.normalMatrix);
    fragNormal = normalize(nMat * inNormal);
    fragTexCoord = inTexCoord;
    
    // Tangent space for normal mapping
    vec3 T = normalize(nMat * inTangent.xyz);
    vec3 B = normalize(cross(fragNormal, T) * inTangent.w);
    fragTangent = T;
    fragBitangent = B;
}
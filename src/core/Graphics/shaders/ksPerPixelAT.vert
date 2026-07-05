#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
    float nearPlane;
    float farPlane;
};

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 normalMatrix;
} pc;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;

void main() {
    vec4 worldPos = pc.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = viewProjection * worldPos;

    fragNormal = mat3(pc.normalMatrix) * inNormal;
    fragTexCoord = inTexCoord;
    fragWorldPos = worldPos.xyz;
}

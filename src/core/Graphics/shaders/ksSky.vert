#version 450

layout(location = 0) in vec3 inPosition;

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
} pc;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragViewDir;

void main() {
    vec4 worldPos = pc.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = (viewProjection * vec4(worldPos.xyz, 1.0)).xyww;

    fragWorldPos = worldPos.xyz;
    fragViewDir = normalize(inPosition);
}

#version 450

// Tire Thermal Mesh Vertex Shader
// Passes thermal data to fragment shader for color computation

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

// Thermal data from physics simulator
// These would come from a buffer or vertex attribute
// For now, we'll use instance or push constant data
out vec3 v_worldPos;
out vec3 v_normal;
out vec2 v_uv;
out float v_coreTemp;
out float v_surfaceTemp;
out float v_wearLevel;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPosition;

// Lighting (minimal for thermal viz)
uniform vec3 lightDirection;
uniform float ambientIntensity;

void main() {
    // Transform position to clip space
    gl_Position = projection * view * model * vec4(position, 1.0);

    // Transform normal and position to world space
    v_normal = normalize(mat3(model) * normal);
    v_worldPos = (model * vec4(position, 1.0)).xyz;

    // Pass thermal data through (these would come from a buffer)
    // In a real implementation, these would be stored in a vertex buffer or SSBO
    v_coreTemp = 80.0;  // placeholder
    v_surfaceTemp = 100.0;  // placeholder
    v_wearLevel = 0.3;  // placeholder

    // Basic UV pass
    v_uv = uv;
}
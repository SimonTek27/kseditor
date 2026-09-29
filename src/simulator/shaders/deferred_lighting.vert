#version 450

// Fullscreen triangle generated from gl_VertexIndex, so the lighting pass
// needs no vertex buffer. Vulkan's NDC (-1,-1) is the top-left corner and
// image texel (0,0) is also top-left, so uv maps straight through with no
// Y flip.

layout(location = 0) out vec2 vUV;

void main() {
    vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}

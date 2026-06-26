#version 450

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
} scene;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 color;
} pcs;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = scene.proj * scene.view * pcs.model * vec4(inPosition, 1.0);
    fragColor = pcs.color;
}

#version 450

struct SceneLight {
    vec4 position;
    vec4 color;
    float intensity;
    float pad0, pad1, pad2;
};

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec4 camera_pos;
    SceneLight lights[8];
    int light_count;
} scene;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(scene.camera_pos.xyz - fragWorldPos);
    vec4 texColor = texture(texSampler, fragTexCoord);

    vec3 result = 0.1 * texColor.rgb;

    for (int i = 0; i < scene.light_count; ++i) {
        vec3 L = normalize(scene.lights[i].position.xyz - fragWorldPos);
        vec3 H = normalize(L + V);

        float diff = max(dot(N, L), 0.0);

        float dist = length(scene.lights[i].position.xyz - fragWorldPos);
        float attenuation = scene.lights[i].intensity / (dist * dist + 1.0);

        vec3 light_color = scene.lights[i].color.rgb;
        result += diff * texColor.rgb * light_color * attenuation;
    }

    outColor = vec4(result, texColor.a);
}

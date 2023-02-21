


#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 varyRGB;
layout(location = 1) in vec2 varyST;

layout(location = 0) out vec4 outColor;

// New: We pass in a color uniform (and a matrix used by the vertex shader).
layout(binding = 0) uniform SceneUniforms {
    vec4 color;
    mat4 camera;
} scene;

void main() {
    outColor = vec4(varyRGB, 1.0) + vec4(varyST, 1.0, 1.0);
    // New: We modulate the old output color by the scene uniform color.
    outColor = 0.5 * outColor * scene.color;
}



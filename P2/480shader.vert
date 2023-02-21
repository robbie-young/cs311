


#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 attrXYZ;
layout(location = 1) in vec3 attrRGB;
layout(location = 2) in vec2 attrST;

layout(location = 0) out vec3 varyRGB;
layout(location = 1) out vec2 varyST;

// New: Instead of hard-coding the camera matrix, we pass it in as a uniform.
// (We also pass in a color, which is used by the fragment shader.)
layout(binding = 0) uniform SceneUniforms {
    vec4 color;
    mat4 camera;
} scene;

mat4 modeling = mat4(
    0.707107, -0.707107, 0.000000, 0.000000,    // column 0, not row 0
    0.707107, 0.707107, 0.000000, 0.000000,     // column 1
    0.000000, 0.000000, 1.000000, 0.000000,     // column 2
    0.000000, 0.000000, 0.000000, 1.000000);    // column 3

void main() {
    // New: We use the camera matrix from the scene uniforms.
    gl_Position = scene.camera * modeling * vec4(attrXYZ, 1.0);
    varyRGB = attrRGB;
    varyST = attrST;
}






#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 attrXYZ;
layout(location = 1) in vec3 attrRGB;
layout(location = 2) in vec2 attrST;

layout(location = 0) out vec3 varyRGB;
layout(location = 1) out vec2 varyST;

layout(binding = 0) uniform SceneUniforms {
    vec4 color;
    mat4 camera;
} scene;

layout(binding = 1) uniform BodyUniforms {
    mat4 modeling;
    // New: Four unsigned integers, which are indices into the texture array, 
    // even though we don't use them here in the vertex shader.
    uvec4 texIndices;
} body;

void main() {
    gl_Position = scene.camera * body.modeling * vec4(attrXYZ, 1.0);
    varyRGB = attrRGB;
    varyST = attrST;
}



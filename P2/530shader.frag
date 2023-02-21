


#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform SceneUniforms {
    mat4 camera;
} scene;
layout(binding = 1) uniform BodyUniforms {
    mat4 modeling;
    uvec4 texIndices;
} body;
layout(binding = 2) uniform sampler2D samplers[3];

layout(location = 0) in vec2 st;
layout(location = 1) in vec3 nop;
layout(location = 2) in vec4 xyzwWorld;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 rgbaTex = texture(samplers[body.texIndices[0]], st);
    float intensity = nop[2] / length(nop);
    float stripe = 0.75 + 0.25 * (xyzwWorld[2] - floor(xyzwWorld[2]));
    outColor = rgbaTex * stripe;
}



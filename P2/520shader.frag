


#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 varyRGB;
layout(location = 1) in vec2 varyST;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform SceneUniforms {
    vec4 color;
    mat4 camera;
} scene;

// New: The body uniforms must be given to the fragment shader, because it needs 
// the texture indices therein.
layout(binding = 1) uniform BodyUniforms {
    mat4 modeling;
    uvec4 texIndices;
} body;

// New: An array of three textures, each already connected to a sampler.
layout(binding = 2) uniform sampler2D samplers[3];

void main() {
    // New: Sample from two textures. We triple the texture coordinates, just to 
    // reveal the difference between the two samplers.
    vec4 texColorA = texture(samplers[body.texIndices[0]], 3.0 * varyST);
    vec4 texColorB = texture(samplers[body.texIndices[1]], 3.0 * varyST);
    // New: Modulate those colors together. Ignore the varying RGB, just to make 
    // the final image easier to understand.
    outColor = texColorA * texColorB;
}



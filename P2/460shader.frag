


#version 450
#extension GL_ARB_separate_shader_objects : enable

// Varyings come into the fragment shader from the rasterizer-interpolator.
layout(location = 0) in vec3 varyRGB;
layout(location = 1) in vec2 varyST;

// This lone output must be the fragment color. Usually outColor[3] is 1.0.
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(varyRGB, 1.0) + vec4(varyST, 1.0, 1.0);
    outColor = 0.5 * outColor;
}



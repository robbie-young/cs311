


// This line of code indicates GLSL version number 4.5.0.
#version 450
#extension GL_ARB_separate_shader_objects : enable

// Attributes come into the vertex shader from the mesh.
layout(location = 0) in vec3 attrXYZ;
layout(location = 1) in vec2 attrST;
layout(location = 2) in vec3 attrNOP;

// Varyings go out to the rasterizer-interpolator.
layout(location = 0) out vec3 varyRGB;
layout(location = 1) out vec2 varyST;

// Matrices in GLSL are laid out column-by-column, not row-by-row.
mat4 camera = mat4(
    3.700123, -0.344453, 0.093228, 0.092296,    // column 0, not row 0
    -0.487130, -2.616382, 0.708139, 0.701057,   // column 1
    0.000000, -2.638959, -0.714249, -0.707107,  // column 2
    0.000000, 0.000004, 9.090910, 10.000000);   // column 3

mat4 modeling = mat4(
    0.707107, -0.707107, 0.000000, 0.000000,    // column 0, not row 0
    0.707107, 0.707107, 0.000000, 0.000000,     // column 1
    0.000000, 0.000000, 1.000000, 0.000000,     // column 2
    0.000000, 0.000000, 0.000000, 1.000000);    // column 3

void main() {
    // There is always a hidden varying, vec4 gl_Position, that you must write.
    gl_Position = camera * modeling * vec4(attrXYZ, 1.0);
    varyRGB = vec3(1.0, 0.5, 0.0);
    varyST = attrST;
}



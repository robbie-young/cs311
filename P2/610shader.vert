


#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform SceneUniforms {
    mat4 camera;
    vec4 uLight;
    vec4 cLight;
    vec4 cLightPositional;
    vec4 pLight;
    vec4 cAmbient;
    vec4 pCamera;
    vec4 attenK;
} scene;
layout(binding = 1) uniform BodyUniforms {
    mat4 modeling;
    uvec4 texIndices;
    vec4 cSpecular;
} body;

layout(location = 0) in vec3 attrXYZ;
layout(location = 1) in vec2 attrST;
layout(location = 2) in vec3 attrNOP;

layout(location = 0) out vec3 pFrag;
layout(location = 1) out vec2 st;
layout(location = 2) out vec3 uNormal;
layout(location = 3) out vec3 uLightPositional;
layout(location = 4) out vec3 uCamera;

void main() {
    vec4 pWorld = body.modeling * vec4(attrXYZ, 1.0);
    gl_Position = scene.camera * pWorld;

    pFrag = vec3(pWorld);
    st = attrST;
    uNormal = vec3(body.modeling * vec4(attrNOP, 0.0));
    uLightPositional = normalize(vec3(scene.pLight) - pFrag);
    uCamera = normalize(vec3(scene.pCamera) - pFrag);
}



#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform SceneUniforms {
    mat4 camera;
    vec4 uLight;
    vec4 cLight;
    vec4 cLightPositional;
    vec4 pLight;
} scene;
layout(binding = 1) uniform BodyUniforms {
    mat4 modeling;
    uvec4 texIndices;
} body;
layout(binding = 2) uniform sampler2D samplers[3];

layout(location = 0) in vec2 st;
layout(location = 1) in vec3 dNormal;
layout(location = 2) in vec3 dLightPositional;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 rgbaTex = texture(samplers[body.texIndices[0]], st);

    vec3 uNormal = normalize(dNormal);
    float iDiffuse = max(0.0, dot(scene.uLight, vec4(uNormal, 0.0)));

    vec3 uLightPositional = normalize(dLightPositional);
    float iDiffusePositional = max(0.0, dot(uLightPositional, uNormal));
    
    outColor = iDiffuse * scene.cLight * rgbaTex + iDiffusePositional * scene.cLightPositional * rgbaTex;
}
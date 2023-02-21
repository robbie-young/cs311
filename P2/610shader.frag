


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
layout(binding = 2) uniform sampler2D samplers[3];

layout(location = 0) in vec3 pFrag;
layout(location = 1) in vec2 st;
layout(location = 2) in vec3 dNormal;
layout(location = 3) in vec3 dLightPositional;
layout(location = 4) in vec3 dCamera;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 rgbaTex = texture(samplers[body.texIndices[0]], st);

    vec3 uNormal = normalize(dNormal);
    float iDiffuse = max(0.0, dot(vec3(scene.uLight), uNormal));

    vec3 uLightPositional = normalize(dLightPositional);
    float iDiffusePositional = max(0.0, dot(uLightPositional, uNormal));

    vec3 uCamera = normalize(dCamera);
    float iSpec = 0.0;
    if (iDiffuse != 0.0) {
        vec3 uRefl = 2 * iDiffuse * uNormal - vec3(scene.uLight);
        iSpec = max(0.0, dot(uRefl, uCamera));
    }
    float iSpecPositional = 0.0;
    if (iDiffusePositional != 0.0) {
        vec3 uReflPositional = 2 * iDiffusePositional * uNormal - uLightPositional;
        iSpecPositional = max(0.0, dot(uReflPositional, uCamera));
    }

    float pLightDistanceSq = pow(distance(pFrag, vec3(scene.pLight)), 2.0);
    float attenuation = 1 / (1 + scene.attenK.x * pLightDistanceSq);

    float shininess = 64.0;

    vec4 cDiff = iDiffuse * scene.cLight * rgbaTex;
    vec4 cDiffPositional = iDiffusePositional * (attenuation * scene.cLightPositional) * rgbaTex;
    vec4 cSpec = pow(iSpec, shininess) * scene.cLight * body.cSpecular;
    vec4 cSpecPositional = pow(iSpecPositional, shininess) * (attenuation * scene.cLightPositional) * body.cSpecular;
    vec4 cAmb = scene.cAmbient * rgbaTex;

    outColor = cDiff + cDiffPositional + cSpec + cSpecPositional + cAmb;
}
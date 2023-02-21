/*
    590mainDiffuse.c
    Implements diffuse reflection with a directional, positional, and ambient light.
    Designed by Josh Davis for Carleton College's CS311 - Computer Graphics.
	Edited by Cole Weinstein and Robbie Young.
*/


/*

Code that is new, compared to the final Vulkan tutorial, is marked 'New'.

On macOS, make sure that NUMDEVICEEXT below is 2, and then compile with 
    clang 590mainDiffuse.c -lglfw -lvulkan
You might also need to compile the shaders with 
    glslc 590shader.vert -o 590vert.spv
    glslc 590shader.frag -o 590frag.spv
Then run the program with 
    ./a.out

On Linux, make sure that NUMDEVICEEXT below is 1, and then compile with 
    clang 590mainDiffuse.c -lglfw -lvulkan -lm
You might also need to compile the shaders with 
    /mnt/c/VulkanSDK/1.3.216.0/Bin/glslc.exe 590shader.vert -o 590vert.spv
    /mnt/c/VulkanSDK/1.3.216.0/Bin/glslc.exe 590shader.frag -o 590frag.spv
(You might have to change the SDK version number to match your installation.) 
Then run the program with 
    ./a.out
If you see errors, then try changing ANISOTROPY to 0 and/or MAXFRAMESINFLIGHT to 
1.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <sys/time.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



/*** CONFIGURATION ************************************************************/

/* Informational messages should (1) or shouldn't (0) be printed to stderr. */
#define VERBOSE 1

/* Anisotropic texture filtering. If you get an error that there are no suitable 
Vulkan devices, then try changing ANISOTROPY from 1 to 0. */
#define ANISOTROPY 1

/* A bound on the number of frames under construction at any given time. Leave 
it at 2 unless you have a good reason. If Vulkan throws an error about 
simultaneous use of a command buffer, then try changing it to 1. */
#define MAXFRAMESINFLIGHT 2

/* To disable validation, set NUMVALLAYERS to 0. Otherwise, the first 
NUMVALLAYERS layers specified below will be used. The same goes for 
NUMINSTANCEEXT and NUMDEVICEEXT. I think that NUMDEVICEEXT should be 1 on Linux 
and 2 on macOS. */
#define NUMVALLAYERS 1
#define NUMINSTANCEEXT 1
#define NUMDEVICEEXT 2

/* Here are the validation layers and extensions, that you might have just 
chosen to activate. Don't change these unless you have a good reason. */
#define MAXVALLAYERS 1
const char* valLayers[MAXVALLAYERS] = {"VK_LAYER_KHRONOS_validation"};
#define MAXINSTANCEEXT 1
const char* instanceExtensions[MAXINSTANCEEXT] = {
    "VK_KHR_get_physical_device_properties2"};
#define MAXDEVICEEXT 2
const char* deviceExtensions[MAXDEVICEEXT] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};



/*** INFRASTRUCTURE ***********************************************************/

/* Remember to read from globals as you like but write to globals only through 
their accessor functions. */
#include "440gui.c"
guiGUI gui;
#include "440vulkan.c"
vulVulkan vul;
#include "450buffer.c"
#include "450image.c"
#include "450swap.c"
swapChain swap;
#include "460shader.c"
#include "460mesh.c"
#include "480uniform.c"
#include "480description.c"
#include "520texture.c"
#include "470vector.c"
#include "490matrix.c"
#include "490isometry.c"
#include "490camera.c"
#include "470mesh.c"
#include "470mesh2D.c"
#include "470mesh3D.c"
#include "470vesh.c"
#include "530landscape.c"

typedef struct BodyUniforms BodyUniforms;
struct BodyUniforms {
    float modelingT[4][4];
    uint32_t texIndices[4];
};

#include "550body.c"


/*** ARTWORK ******************************************************************/

/* Three veshes using the attribute style XYZ, ST, NOP. */
veshStyle style;
veshVesh landVesh, waterVesh, heroTorsoVesh, heroHeadVesh, heroLeftEyeVesh, heroRightEyeVesh, heroLeftIrisVesh, heroRightIrisVesh;

/* Elevation data and functions to set them. Keep in mind that each of our 
veshes is limited to 65,536 triangles. And the landscape and water will each use 
2 LANDSIZE^2 triangles. So don't set LANDSIZE to be more than about 180. */
#define LANDSIZE 100
float landData[LANDSIZE * LANDSIZE];
float waterData[LANDSIZE * LANDSIZE];

void setLand() {
    landFlat(LANDSIZE, landData, 0.0);
    time_t t;
	srand((unsigned)time(&t));
    for (int i = 0; i < 32; i += 1)
		landFaultRandomly(LANDSIZE, landData, 1.5 - i * 0.04);
	for (int i = 0; i < 4; i += 1)
		landBlur(LANDSIZE, landData);
	for (int i = 0; i < 16; i += 1)
		landBump(
		    LANDSIZE, landData, landInt(0, LANDSIZE - 1), 
		    landInt(0, LANDSIZE - 1), 5.0, 2.0);
}

void setWater() {
    float landMin, landMean, landMax;
    landStatistics(LANDSIZE, landData, &landMin, &landMean, &landMax);
    landFlat(LANDSIZE, waterData, landMean);
    for (int i = 0; i < LANDSIZE; i += 1)
        for (int j = 0; j < LANDSIZE; j += 1)
            waterData[i * LANDSIZE + j] += 0.1 * sin(i * M_PI / 5.0);
}

/* Our artwork initialization is big enough that we break it up. */
int initializeVeshes() {
    meshMesh mesh;
    /* Randomly generate the landscape. */
    setLand();
    setWater();
    /* Make the hero veshes. */
    /* First is the torso. */
    if (mesh3DInitializeCapsule(&mesh, 0.5, 2.0, 16, 32) != 0) {
        return 8;
    }
    if (veshInitializeMesh(&heroTorsoVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        return 7;
    }
    meshFinalize(&mesh);
    /* Next is the head. */
    if (mesh3DInitializeSphere(&mesh, 1.0, 20.0, 20.0) != 0) {
        veshFinalize(&heroTorsoVesh);
        return 6;
    }
    if (veshInitializeMesh(&heroHeadVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        veshFinalize(&heroTorsoVesh);
        return 5;
    }
    meshFinalize(&mesh);
    /* Then the left eye. */
    if (mesh3DInitializeSphere(&mesh, 0.25, 20.0, 20.0) != 0) {
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        return 6;
    }
    if (veshInitializeMesh(&heroLeftEyeVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        return 5;
    }
    meshFinalize(&mesh);
    /* And the right eye. */
    if (mesh3DInitializeSphere(&mesh, 0.25, 20.0, 20.0) != 0) {
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        return 6;
    }
    if (veshInitializeMesh(&heroRightEyeVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        return 5;
    }
    /* Then the left iris. */
    meshFinalize(&mesh);
    if (mesh3DInitializeSphere(&mesh, 0.125, 20.0, 20.0) != 0) {
        return 6;
    }
    if (veshInitializeMesh(&heroLeftIrisVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        veshFinalize(&heroRightEyeVesh);
        return 5;
    }
    /* And the right iris. */
    meshFinalize(&mesh);
    if (mesh3DInitializeSphere(&mesh, 0.125, 20.0, 20.0) != 0) {
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        veshFinalize(&heroRightEyeVesh);
        veshFinalize(&heroLeftIrisVesh);
        return 6;
    }
    if (veshInitializeMesh(&heroRightIrisVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        veshFinalize(&heroRightEyeVesh);
        veshFinalize(&heroLeftIrisVesh);
        return 5;
    }
    meshFinalize(&mesh);
    /* Make the land vesh. */
    if (mesh3DInitializeLandscape(&mesh, LANDSIZE, 1.0, landData) != 0) {
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        veshFinalize(&heroRightEyeVesh);
        veshFinalize(&heroLeftIrisVesh);
        veshFinalize(&heroRightIrisVesh);
        return 4;
    }
    if (veshInitializeMesh(&landVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        veshFinalize(&heroRightEyeVesh);
        veshFinalize(&heroLeftIrisVesh);
        veshFinalize(&heroRightIrisVesh);
        return 3;
    }
    meshFinalize(&mesh);
    /* Make the water vesh. */
    if (mesh3DInitializeLandscape(&mesh, LANDSIZE, 1.0, waterData) != 0) {
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        veshFinalize(&heroRightEyeVesh);
        veshFinalize(&heroLeftIrisVesh);
        veshFinalize(&heroRightIrisVesh);
        veshFinalize(&landVesh);
        return 2;
    }
    if (veshInitializeMesh(&waterVesh, &mesh) != 0) {
        meshFinalize(&mesh);
        veshFinalize(&heroTorsoVesh);
        veshFinalize(&heroHeadVesh);
        veshFinalize(&heroLeftEyeVesh);
        veshFinalize(&heroRightEyeVesh);
        veshFinalize(&heroLeftIrisVesh);
        veshFinalize(&heroRightIrisVesh);
        veshFinalize(&landVesh);
        return 1;
    }
    meshFinalize(&mesh);
    return 0;
}

/* Finalize the veshes. */
void finalizeVeshes() {
    veshFinalize(&waterVesh);
    veshFinalize(&landVesh);
    veshFinalize(&heroTorsoVesh);
    veshFinalize(&heroHeadVesh);
    veshFinalize(&heroLeftEyeVesh);
    veshFinalize(&heroRightEyeVesh);
    veshFinalize(&heroLeftIrisVesh);
    veshFinalize(&heroRightIrisVesh);
}

/* Textures. */
#define TEXNUM 3
VkSampler texSampRepeat, texSampClamp;
VkSampler texSamps[TEXNUM];
VkImage texIms[TEXNUM];
VkDeviceMemory texImMems[TEXNUM];
VkImageView texImViews[TEXNUM];

/* Initialize textures and samplers. */
int initializeTextures() {
    /* Initialize two samplers. */
    if (texInitializeSampler(
            &texSampRepeat, VK_SAMPLER_ADDRESS_MODE_REPEAT, 
            VK_SAMPLER_ADDRESS_MODE_REPEAT) != 0) {
        return 5;
    }
    if (texInitializeSampler(
            &texSampClamp, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) != 0) {
        texFinalizeSampler(&texSampRepeat);
        return 4;
    }
    /* The first sampler acts on the first two textures. The second sampler acts 
    on the third texture. */
    texSamps[0] = texSampRepeat;
    texSamps[1] = texSampRepeat;
    texSamps[2] = texSampClamp;
    /* Initialize three textures. */
    if (texInitializeFile(
            &texIms[0], &texImMems[0], &texImViews[0], "grayish.png") != 0) {
        texFinalizeSampler(&texSampClamp);
        texFinalizeSampler(&texSampRepeat);
        return 3;
    }
    if (texInitializeFile(
            &texIms[1], &texImMems[1], &texImViews[1], "bluish.png") != 0) {
        texFinalize(&texIms[0], &texImMems[0], &texImViews[0]);
        texFinalizeSampler(&texSampClamp);
        texFinalizeSampler(&texSampRepeat);
        return 2;
    }
    if (texInitializeFile(
            &texIms[2], &texImMems[2], &texImViews[2], "reddish.png") != 0) {
        texFinalize(&texIms[1], &texImMems[1], &texImViews[1]);
        texFinalize(&texIms[0], &texImMems[0], &texImViews[0]);
        texFinalizeSampler(&texSampClamp);
        texFinalizeSampler(&texSampRepeat);
        return 1;
    }
    return 0;
}

/* Finalize textures and samplers. */
void finalizeTextures() {
    texFinalize(&texIms[2], &texImMems[2], &texImViews[2]);
    texFinalize(&texIms[1], &texImMems[1], &texImViews[1]);
    texFinalize(&texIms[0], &texImMems[0], &texImViews[0]);
    texFinalizeSampler(&texSampClamp);
    texFinalizeSampler(&texSampRepeat);
}

/* Camera and hero data. */
camCamera camera;
float cameraRho = 10.0, cameraPhi = M_PI / 4.0, cameraTheta = M_PI / 4.0;
float heroPos[3] = {0.5 * LANDSIZE, 0.5 * LANDSIZE, 0.0};
float heroHeading = 0.0;
int heroWDown = 0, heroSDown = 0, heroADown = 0, heroDDown = 0;

/* Called by setBodyUniforms. */
void setHero() {
    float changeInTime = gui.currentTime - gui.lastTime;
    if (heroADown)
        heroHeading += M_PI * changeInTime;
    if (heroDDown)
        heroHeading -= M_PI * changeInTime;
    if (heroWDown) {
        heroPos[0] += 2.0 * changeInTime * cos(heroHeading);
        heroPos[1] += 2.0 * changeInTime * sin(heroHeading);
    }
    if (heroSDown) {
        heroPos[0] -= 2.0 * changeInTime * cos(heroHeading);
        heroPos[1] -= 2.0 * changeInTime * sin(heroHeading);
    }
    if (0.0 <= heroPos[0] && heroPos[0] <= LANDSIZE - 1.0 && 
            0.0 <= heroPos[1] && heroPos[1] <= LANDSIZE - 1.0) {
        /* Where have we seen this kind of calculation before now? */
        int flX = (int)floor(heroPos[0]);
        int ceX = (int)ceil(heroPos[0]);
        int flY = (int)floor(heroPos[1]);
        int ceY = (int)ceil(heroPos[1]);
        float frX = heroPos[0] - flX;
        float frY = heroPos[1] - flY;
        heroPos[2] = (1 - frX) * (1 - frY) * landData[flX * LANDSIZE + flY]
            + (1 - frX) * (frY) * landData[flX * LANDSIZE + ceY]
            + (frX) * (1 - frY) * landData[ceX * LANDSIZE + flY]
            + (frX) * (frY) * landData[ceX * LANDSIZE + ceY];
        heroPos[2] += 1.0;
    }
}

/* Called by setSceneUniforms. */
void setCamera() {
    camSetFrustum(
        &camera, M_PI / 6.0, cameraRho, 10.0, swap.extent.width, 
        swap.extent.height);
    camLookAt(&camera, heroPos, cameraRho, cameraPhi, cameraTheta);
}

/* We start to build a scene with three bodies. */
int bodyNum = 8;
bodyBody landscapeBody, waterBody, heroTorsoBody, heroHeadBody, heroLeftEyeBody, heroRightEyeBody, heroLeftIrisBody, heroRightIrisBody;

/* Initializes elements of the scene. The camera and the hero are part of the 
scene, but they get updated on each time step automatically. */
int initializeScene() {
    camSetProjectionType(&camera, camPERSPECTIVE);
    /* Initializes each of the bodies defined above */

    /* Hero torso has hero head as its only child and the water and landscape as its siblings. */
    bodyConfigure(&heroTorsoBody, &heroTorsoVesh, &heroHeadBody, &waterBody);
    /* Hero head has the two eyes as its children and no siblings. */
    bodyConfigure(&heroHeadBody, &heroHeadVesh, &heroLeftEyeBody, NULL);
    /* Hero left eye has the left iris as its child and the right eye as its sibling. */
    bodyConfigure(&heroLeftEyeBody, &heroLeftEyeVesh, &heroLeftIrisBody, &heroRightEyeBody);
    /* Hero right eye has the right iris as its child and is the sibling of the left eye. */
    bodyConfigure(&heroRightEyeBody, &heroRightEyeVesh, &heroRightIrisBody, NULL);
    /* Hero left iris has no children or siblings. */
    bodyConfigure(&heroLeftIrisBody, &heroLeftIrisVesh, NULL, NULL);
    /* Hero right iris has no children or siblings. */
    bodyConfigure(&heroRightIrisBody, &heroRightIrisVesh, NULL, NULL);
    /* The water has no children, is the sibling of the hero torso, and has the landscape as its sibling. */
    bodyConfigure(&waterBody, &waterVesh, NULL, &landscapeBody);
    /* The landscape has no children and is the sibling of the hero torso and water. */
    bodyConfigure(&landscapeBody, &landVesh, NULL, NULL);

    /* White landscape. */
    landscapeBody.uniforms.texIndices[0] = 0;
    /* Blue water. */
    waterBody.uniforms.texIndices[0] = 1;
    /* Red torso and body. */
    heroTorsoBody.uniforms.texIndices[0] = 2;
    heroHeadBody.uniforms.texIndices[0] = 2;
    /* White eyes. */
    heroLeftEyeBody.uniforms.texIndices[0] = 0;
    heroRightEyeBody.uniforms.texIndices[0] = 0;
    /* Blue irises. */
    heroLeftIrisBody.uniforms.texIndices[0] = 1;
    heroRightIrisBody.uniforms.texIndices[0] = 1;

    return 0;
}

/* Finalize the scene. (Currently does nothing.) */
void finalizeScene() {
    return;
}

/* Here's the variable to hold the shader program. */
shaProgram shaProg;

/* Initializes the artwork. Upon success (return code 0), don't forget to 
finalizeArtwork later. */
int initializeArtwork() {
    /* New shaders and new helper functions. */
    if (shaInitialize(&shaProg, "590vert.spv", "590frag.spv") != 0) {
        return 5;
    }
    int attrDims[3] = {3, 2, 3};
    if (veshInitializeStyle(&style, 3, attrDims) != 0) {
        shaFinalize(&shaProg);
        return 4;
    }
    if (initializeVeshes() != 0) {
        veshFinalizeStyle(&style);
        shaFinalize(&shaProg);
        return 3;
    }
    if (initializeTextures() != 0) {
        finalizeVeshes();
        veshFinalizeStyle(&style);
        shaFinalize(&shaProg);
        return 2;
    }
    if (initializeScene() != 0) {
        finalizeTextures();
        finalizeVeshes();
        veshFinalizeStyle(&style);
        shaFinalize(&shaProg);
        return 1;
    }
    return 0;
}

/* Releases the artwork resources. */
void finalizeArtwork() {
    finalizeScene();
    finalizeTextures();
    finalizeVeshes();
    veshFinalizeStyle(&style);
    shaFinalize(&shaProg);
}



/*** UNIFORM PART OF CONNECTION BETWEEN SWAP CHAIN AND SCENE ******************/

/* I've removed the color, because it was a mostly useless example. */
typedef struct SceneUniforms SceneUniforms;
struct SceneUniforms {
    float cameraT[4][4];
    float uLight[4];
    float cLight[4];
    float cLightPositional[4];
    float pLight[4];
    float cAmbient[4];
};

VkBuffer *sceneUniformBuffers;
VkDeviceMemory *sceneUniformBuffersMemory;

/* Configures the scene uniforms for a single frame. */
void setSceneUniforms(uint32_t imageIndex) {
    SceneUniforms sceneUnifs;
    /* Update the camera. */
    setCamera();
    float cam[4][4];
    camGetProjectionInverseIsometry(&camera, cam);
    mat44Transpose(cam, sceneUnifs.cameraT);

    /* Sets the color and direction of the directional light. */
    float uLight[4] = {0.0, sqrt(2), sqrt(2), 0.0};
    vecCopy(4, uLight, sceneUnifs.uLight);
    float cLight[4] = {0.0, 1.0, 1.0, 0.0};
    vecCopy(4, cLight, sceneUnifs.cLight);
    
    /* Sets the color and position of the positional light. */
    float cLightPositional[4] = {1.0, 0.0, 0.0, 0.0};
    vecCopy(4, cLightPositional, sceneUnifs.cLightPositional);
    float zPos = fmax(landData[0], waterData[0]) + 1;
    float pLight[4] = {0.0, 0.0, zPos, 0.0};
    vecCopy(4, pLight, sceneUnifs.pLight);

    /* Sets the color of the ambient light */
    float cAmbient[4] = {0.0, 1.0, 0.0, 0.0};
    vecCopy(4, cAmbient, sceneUnifs.cAmbient);

    /* Copy the bits. */
	void *data;
	vkMapMemory(
	    vul.device, sceneUniformBuffersMemory[imageIndex], 0, 
	    sizeof(SceneUniforms), 0, &data);
	memcpy(data, &sceneUnifs, sizeof(SceneUniforms));
	vkUnmapMemory(vul.device, sceneUniformBuffersMemory[imageIndex]);
}

VkBuffer *bodyUniformBuffers;
VkDeviceMemory *bodyUniformBuffersMemory;
unifAligned aligned;

/* Configures the body uniforms for a single frame. */
void setBodyUniforms(uint32_t imageIndex) {
    float identity[4][4] = {
        {1.0, 0.0, 0.0, 0.0},                           // row 0, not column 0
        {0.0, 1.0, 0.0, 0.0},                           // row 1
        {0.0, 0.0, 1.0, 0.0},                           // row 2
        {0.0, 0.0, 0.0, 1.0}};                          // row 3
    /* The hero has a heading and a location. */
    setHero();
    float axis[3] = {0.0, 0.0, 1.0};
    float rot[3][3];
    mat33AngleAxisRotation(heroHeading, axis, rot);
    isoSetRotation(&heroTorsoBody.isometry, rot);
    isoSetTranslation(&heroTorsoBody.isometry, heroPos);
    /* Set the head's position to rest on top of the torso. */
    float heroHeadPos[3] = {0, 0, 1.5};
    isoSetTranslation(&heroHeadBody.isometry, heroHeadPos);
    /* Set the eyes to sit somewhat outside of the head. */
    float heroLeftEyePos[3] = {0.98, 0.4, 0.05};
    isoSetTranslation(&heroLeftEyeBody.isometry, heroLeftEyePos);
    float heroRightEyePos[3] = {0.98, -0.4, 0.05};
    isoSetTranslation(&heroRightEyeBody.isometry, heroRightEyePos);
    /* Set the irises to barely poke out of the eyes. */
    float heroLeftIrisPos[3] = {0.15, 0.0, 0.0};
    isoSetTranslation(&heroLeftIrisBody.isometry, heroLeftIrisPos);
    float heroRightIrisPos[3] = {0.15, 0.0, 0.0};
    isoSetTranslation(&heroRightIrisBody.isometry, heroRightIrisPos);

    /* Set all of the uniforms recursively. */
    bodySetUniformsRecursively(&heroTorsoBody, identity, &aligned, 0);

    /* Copy the body UBO bits from the CPU to the GPU. */
    void *data;
    int amount = aligned.uboNum * aligned.alignedSize;
	vkMapMemory(
	    vul.device, bodyUniformBuffersMemory[imageIndex], 0, amount, 0, &data);
	memcpy(data, aligned.data, amount);
	vkUnmapMemory(vul.device, bodyUniformBuffersMemory[imageIndex]);
}

#define UNIFSCENE 0
#define UNIFBODY 1
#define UNIFTEX 2
#define UNIFNUM 3
int descriptorCounts[UNIFNUM] = {1, 1, 3};
VkDescriptorType descriptorTypes[UNIFNUM] = {
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
VkShaderStageFlags descriptorStageFlagss[UNIFNUM] = {
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
    VK_SHADER_STAGE_FRAGMENT_BIT};
int descriptorBindings[UNIFNUM] = {0, 1, 2};

descDescription desc;

/* Helper function for descInitialize. Provides the parts of the customization 
that are difficult to abstract. The i argument specifies which element of the 
swap chain we're operating on. */
void setDescriptorSet(descDescription *desc, int i) {
    /* Prepare to update the descriptor for the scene UBO. */
    VkDescriptorBufferInfo sceneUBOInfo = {0};
    sceneUBOInfo.buffer = sceneUniformBuffers[i];
    sceneUBOInfo.offset = 0;
    sceneUBOInfo.range = sizeof(SceneUniforms);
    VkDescriptorBufferInfo sceneUBODescBufInfos[] = {sceneUBOInfo};
    VkWriteDescriptorSet sceneUBOWrite = {0};
    sceneUBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sceneUBOWrite.dstSet = desc->descriptorSets[i];
    sceneUBOWrite.dstBinding = descriptorBindings[UNIFSCENE];
    sceneUBOWrite.dstArrayElement = 0;
    sceneUBOWrite.descriptorType = descriptorTypes[UNIFSCENE];
    sceneUBOWrite.descriptorCount = descriptorCounts[UNIFSCENE];
    sceneUBOWrite.pBufferInfo = sceneUBODescBufInfos;
    /* Prepare to update the descriptor for the body UBO. */
    VkDescriptorBufferInfo bodyUBOInfo = {0};
    bodyUBOInfo.buffer = bodyUniformBuffers[i];
    bodyUBOInfo.offset = 0;
    bodyUBOInfo.range = unifAlignment(sizeof(BodyUniforms));
    VkDescriptorBufferInfo bodyUBODescBufInfos[] = {bodyUBOInfo};
    VkWriteDescriptorSet bodyUBOWrite = {0};
    bodyUBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bodyUBOWrite.dstSet = desc->descriptorSets[i];
    bodyUBOWrite.dstBinding = descriptorBindings[UNIFBODY];
    bodyUBOWrite.dstArrayElement = 0;
    bodyUBOWrite.descriptorCount = descriptorCounts[UNIFBODY];
    bodyUBOWrite.descriptorType = descriptorTypes[UNIFBODY];
    bodyUBOWrite.pBufferInfo = bodyUBODescBufInfos;
    /* Prepare to update texNum descriptors for the texture array. */
    VkDescriptorImageInfo descriptorImageInfos[TEXNUM];
    for (int i = 0; i < TEXNUM; i += 1) {
        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texImViews[i];
        imageInfo.sampler = texSamps[i];
        descriptorImageInfos[i] = imageInfo;
    }
    VkWriteDescriptorSet samplerWrite = {0};
    samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    samplerWrite.dstSet = desc->descriptorSets[i];
    samplerWrite.dstBinding = descriptorBindings[UNIFTEX];
    samplerWrite.dstArrayElement = 0;
    samplerWrite.descriptorType = descriptorTypes[UNIFTEX];
    samplerWrite.descriptorCount = descriptorCounts[UNIFTEX];
    samplerWrite.pImageInfo = descriptorImageInfos;
    /* Update the three descriptors. */
    VkWriteDescriptorSet descWrites[] = {
        sceneUBOWrite, bodyUBOWrite, samplerWrite};
    vkUpdateDescriptorSets(vul.device, 3, descWrites, 0, NULL);
}

/* Initializes all of the machinery for communicating uniforms to shaders. 
Returns an error code (0 on success). On success, don't forget to 
finalizeUniforms when you're done. */
int initializeUniforms() {
    if (unifInitializeBuffers(
            &sceneUniformBuffers, &sceneUniformBuffersMemory, 
            sizeof(SceneUniforms)) != 0)
        return 4;
    if (unifInitializeBuffers(
            &bodyUniformBuffers, &bodyUniformBuffersMemory, 
            bodyNum * unifAlignment(sizeof(BodyUniforms))) != 0) {
        unifFinalizeBuffers(&sceneUniformBuffers, &sceneUniformBuffersMemory);
        return 3;
    }
    if (unifInitializeAligned(&aligned, bodyNum, sizeof(BodyUniforms)) != 0) {
        unifFinalizeBuffers(&bodyUniformBuffers, &bodyUniformBuffersMemory);
        unifFinalizeBuffers(&sceneUniformBuffers, &sceneUniformBuffersMemory);
        return 2;
    }
    if (descInitialize(
            &desc, UNIFNUM, descriptorCounts, descriptorTypes, 
            descriptorStageFlagss, descriptorBindings, setDescriptorSet) != 0) {
        unifFinalizeAligned(&aligned);
        unifFinalizeBuffers(&bodyUniformBuffers, &bodyUniformBuffersMemory);
        unifFinalizeBuffers(&sceneUniformBuffers, &sceneUniformBuffersMemory);
        return 1;
    }
    return 0;
}

/* Releases the resources backing all of the uniform machinery. */
void finalizeUniforms() {
    descFinalize(&desc);
    unifFinalizeAligned(&aligned);
    unifFinalizeBuffers(&bodyUniformBuffers, &bodyUniformBuffersMemory);
    unifFinalizeBuffers(&sceneUniformBuffers, &sceneUniformBuffersMemory);
}



/*** CONNECTION BETWEEN SWAP CHAIN AND SCENE **********************************/

VkPipelineLayout connPipelineLayout;
VkPipeline connGraphicsPipeline;
VkCommandBuffer *connCommandBuffers;

/* Helper function for initializePipeline. Configures viewport and scissor. (We 
don't use the scissor in this course.) */
void getViewportState(
        VkViewport *v, VkRect2D *s, VkPipelineViewportStateCreateInfo *vs) {
    VkViewport viewport = {0};
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = (float)swap.extent.width;
    viewport.height = (float)swap.extent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;
    *v = viewport;
    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swap.extent;
    *s = scissor;
    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = v;
    viewportState.scissorCount = 1;
    viewportState.pScissors = s;
    *vs = viewportState;
}

/* Helper function for initializePipeline. Common rasterization settings. */
void getRasterizerState(VkPipelineRasterizationStateCreateInfo *r) {
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0;
    rasterizer.depthBiasClamp = 0.0;
    rasterizer.depthBiasSlopeFactor = 0.0;
    *r = rasterizer;
}

/* Helper function for initializePipeline. Configures multisampling (an 
anti-aliasing technique, which we don't use here). */
void getMultisampleState(VkPipelineMultisampleStateCreateInfo *m) {
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    *m = multisampling;
}

/* Helper function for initializePipeline. Configures blending (very useful, but 
we don't use it.) */
void getBlendingState(
        VkPipelineColorBlendAttachmentState *cba, 
        VkPipelineColorBlendStateCreateInfo *cb) {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    *cba = colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = cba;
    colorBlending.blendConstants[0] = 0.0;
    colorBlending.blendConstants[1] = 0.0;
    colorBlending.blendConstants[2] = 0.0;
    colorBlending.blendConstants[3] = 0.0;
    *cb = colorBlending;
}

/* Helper function for initializePipeline. Configures stencil (which we don't 
use in this course). */
void getDepthStencilState(VkPipelineDepthStencilStateCreateInfo *d) {
    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType = 
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    *d = depthStencil;
}

/* The pipeline records a bunch of rendering options: viewport, backface 
culling, depth test, etc. This initializer returns an error code (0 on success). 
On success, don't forget to finalizePipeline when you're done. */
int initializePipeline(
        shaProgram *shaProg, 
        VkPipelineVertexInputStateCreateInfo *vertexInputInfo, 
        VkPipelineInputAssemblyStateCreateInfo *inputAssembly) {
    /* Get some rendering options from helper functions. */
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineViewportStateCreateInfo viewportState;
    getViewportState(&viewport, &scissor, &viewportState);
    VkPipelineRasterizationStateCreateInfo rasterizer;
    getRasterizerState(&rasterizer);
    VkPipelineMultisampleStateCreateInfo multisampling;
    getMultisampleState(&multisampling);
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlending;
    getBlendingState(&colorBlendAttachment, &colorBlending);
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    getDepthStencilState(&depthStencil);
    /* Pipeline layout and pipeline. */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &(desc.descriptorSetLayout);
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;
    if (vkCreatePipelineLayout(
            vul.device, &pipelineLayoutInfo, NULL, 
            &connPipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "error: initializePipeline: ");
        fprintf(stderr, "vkCreatePipelineLayout failed\n");
        return 2;
    }
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = NULL;
    pipelineInfo.layout = connPipelineLayout;
    pipelineInfo.renderPass = swap.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    /* Here the arguments about mesh style and shader program get used. */
    pipelineInfo.pStages = shaProg->shaderStages;
    pipelineInfo.pVertexInputState = vertexInputInfo;
    pipelineInfo.pInputAssemblyState = inputAssembly;
    if (vkCreateGraphicsPipelines(
            vul.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, 
            &connGraphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "error: initializePipeline: ");
        fprintf(stderr, "vkCreateGraphicsPipelines failed\n");
        return 1;
    }
    return 0;
}

/* Releases the resources backing the pipeline. */
void finalizePipeline() {
    vkDestroyPipeline(vul.device, connGraphicsPipeline, NULL);
    vkDestroyPipelineLayout(vul.device, connPipelineLayout, NULL);
}

/* A command buffer is a sequence of Vulkan commands that render a scene. This 
initializer returns an error code (0 on success). On success, don't forget to 
finalizeCommandBuffers when you're done. */
int initializeCommandBuffers() {
    connCommandBuffers = malloc(swap.numImages * sizeof(VkCommandBuffer));
    if (connCommandBuffers == NULL) {
        fprintf(stderr, "error: initializeCommandBuffers: malloc failed\n");
        return 4;
    }
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vul.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)swap.numImages;
    if (vkAllocateCommandBuffers(
            vul.device, &allocInfo, connCommandBuffers) != VK_SUCCESS) {
        fprintf(stderr, "error: initializeCommandBuffers: ");
        fprintf(stderr, "vkAllocateCommandBuffers failed\n");
        free(connCommandBuffers);
        return 3;
    }
    for (size_t i = 0; i < swap.numImages; i += 1) {
        VkCommandBufferBeginInfo beginInfo = {0};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = NULL;
        if (vkBeginCommandBuffer(
                connCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
            fprintf(stderr, "error: initializeCommandBuffers: ");
            fprintf(stderr, "vkBeginCommandBuffer failed\n");
            free(connCommandBuffers);
            return 2;
        }
        /* Render pass begin info. */
        VkRenderPassBeginInfo renderPassInfo = {0};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swap.renderPass;
        renderPassInfo.framebuffer = swap.framebuffers[i];
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent = swap.extent;
        /* Clear color and depth. */
        VkClearValue clearColor = {0.0, 0.0, 0.0, 1.0};
        VkClearValue clearDepth = {1.0, 0.0};
        VkClearValue clearValues[2] = {clearColor, clearDepth};
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;
        /* Begin render pass. */
        vkCmdBeginRenderPass(
            connCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(
            connCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, 
            connGraphicsPipeline);
            
        /* Render the three bodies. */
        bodyRenderRecursively(&heroTorsoBody, &connCommandBuffers[i], &connPipelineLayout, &(desc.descriptorSets[i]), &aligned, 0);
        vkCmdEndRenderPass(connCommandBuffers[i]);
        if (vkEndCommandBuffer(connCommandBuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "error: initializeCommandBuffers: ");
            fprintf(stderr, "vkEndCommandBuffer failed\n");
            free(connCommandBuffers);
            return 1;
        }
    }
    return 0;
}

/* Releases the resources backing the command buffers. */
void finalizeCommandBuffers() {
    vkFreeCommandBuffers(
        vul.device, vul.commandPool, (uint32_t)swap.numImages, 
        connCommandBuffers);
    free(connCommandBuffers);
}

/* Initializes the machinery that connects the swap chain to the scene. Returns 
an error code (0 on success). On success, don't forget to finalizeConnection 
when you're done. */
int initializeConnection() {
    if (initializeUniforms() != 0)
        return 3;
    /* Use the mesh style. */
    if (initializePipeline(
            &shaProg, &(style.vertexInputInfo), &(style.inputAssembly)) != 0) {
        finalizeUniforms();
        return 2;
    }
    if (initializeCommandBuffers() != 0) {
        finalizePipeline();
        finalizeUniforms();
        return 1;
    }
    return 0;
}

/* Releases the connection between the swap chain and the scene. */
void finalizeConnection() {
    finalizeCommandBuffers();
    finalizePipeline();
    finalizeUniforms();
}



/*** MAIN *********************************************************************/

/* Called by presentFrame. Returns an error code (0 on success). On success, 
remember to call the appropriate finalizers when you're done. */
int reinitializeSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(gui.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(gui.window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vul.device);
    finalizeConnection();
    swapFinalize(&swap);
    if (swapInitialize(&swap) != 0)
        return 2;
    if (initializeConnection() != 0) {
        swapFinalize(&swap);
        return 1;
    }
    return 0;
}

/* Called by guiRun. Presents one frame to the window. */
int presentFrame() {
    /* Synchronization. */
    vkWaitForFences(
        vul.device, 1, &swap.inFlightFences[swap.curFrame], VK_TRUE, 
        UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        vul.device, swap.swapChain, UINT64_MAX, 
        swap.imageAvailSems[swap.curFrame], VK_NULL_HANDLE, &imageIndex);
    /* Is something strange happening at the moment? */
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        int error = reinitializeSwapChain();
        return 5;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "error: presentFrame: ");
        fprintf(stderr, "vkAcquireNextImageKHR weird return value\n");
        return 4;
    }
    /* Synchronization. */
    swap.imagesInFlight[imageIndex] = swap.inFlightFences[swap.curFrame];
    if (swap.imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(
            vul.device, 1, &swap.imagesInFlight[imageIndex], VK_TRUE, 
            UINT64_MAX);
    /* Send data to the scene and body UBOs in the shaders. */
    setSceneUniforms(imageIndex);
    setBodyUniforms(imageIndex);
    /* Prepare to submit a request to render the new frame. */
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {swap.imageAvailSems[swap.curFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &connCommandBuffers[imageIndex];
    /* Synchronization. */
    VkSemaphore signalSemaphores[] = {swap.renderDoneSems[swap.curFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    vkResetFences(vul.device, 1, &swap.inFlightFences[swap.curFrame]);
    /* Submit the request. */
    if (vkQueueSubmit(
            vul.graphicsQueue, 1, &submitInfo, 
            swap.inFlightFences[swap.curFrame]) != VK_SUCCESS) {
        fprintf(stderr, "error: presentFrame: vkQueueSubmit failed\n");
        return 3;
    }
    /* Prepare to present a frame to the user. */
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swap.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL;
    result = vkQueuePresentKHR(vul.presentQueue, &presentInfo);
    /* Is something strange happening at the moment? */
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || 
            gui.framebufferResized) {
        guiSetFramebufferResized(&gui, 0);
        if (reinitializeSwapChain() != 0)
            return 2;
    } else if (result != VK_SUCCESS) {
        fprintf(stderr, "error: presentFrame: ");
        fprintf(stderr, "vkQueuePresentKHR weird return value\n");
        return 1;
    }
    /* We're finally done with this frame. */
    swapIncrementFrame(&swap);
    return 0;
}

/* Handles keyboard input for movement of the hero and camera. */
void handleKey(
        GLFWwindow *window, int key, int scancode, int action, int mods) {
    /* Detect which modifier keys are down. */
    int shiftIsDown, controlIsDown, altOptionIsDown, superCommandIsDown;
    shiftIsDown = mods & GLFW_MOD_SHIFT;
    controlIsDown = mods & GLFW_MOD_CONTROL;
    altOptionIsDown = mods & GLFW_MOD_ALT;
    superCommandIsDown = mods & GLFW_MOD_SUPER;
    /* Handle the camera. */
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (camera.projectionType == camORTHOGRAPHIC)
            camSetProjectionType(&camera, camPERSPECTIVE);
        else
            camSetProjectionType(&camera, camORTHOGRAPHIC);
    } else if (key == GLFW_KEY_J)
        cameraTheta -= M_PI / 36.0;
    else if (key == GLFW_KEY_L)
        cameraTheta += M_PI / 36.0;
    else if (key == GLFW_KEY_I)
        cameraPhi -= M_PI / 36.0;
    else if (key == GLFW_KEY_K)
        cameraPhi += M_PI / 36.0;
    else if (key == GLFW_KEY_O)
        cameraRho *= 0.95;
    else if (key == GLFW_KEY_U)
        cameraRho *= 1.05;
    /* Update which hero keys are down. They affect the hero automatically on 
    each time step. */
    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS)
            heroWDown = 1;
        else if (action == GLFW_RELEASE)
            heroWDown = 0;
    } if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS)
            heroSDown = 1;
        else if (action == GLFW_RELEASE)
            heroSDown = 0;
    } if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS)
            heroADown = 1;
        else if (action == GLFW_RELEASE)
            heroADown = 0;
    } if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS)
            heroDDown = 1;
        else if (action == GLFW_RELEASE)
            heroDDown = 0;
    }
}

int main() {
    if (guiInitialize(&gui, 512, 512, "Vulkan") != 0)
        return 5;
    if (vulInitialize(&vul) != 0) {
        guiFinalize(&gui);
        return 4;
    }
    if (swapInitialize(&swap) != 0) {
        vulFinalize(&vul);
        guiFinalize(&gui);
        return 3;
    }
    if (initializeArtwork() != 0) {
        swapFinalize(&swap);
        vulFinalize(&vul);
        guiFinalize(&gui);
        return 2;
    }
    if (initializeConnection() != 0) {
        finalizeArtwork();
        swapFinalize(&swap);
        vulFinalize(&vul);
        guiFinalize(&gui);
        return 1;
    }
    guiSetFramePresenter(&gui, presentFrame);
    /* Register the keyboard handler. */
    glfwSetKeyCallback(gui.window, handleKey);
    guiRun(&gui);
    vkDeviceWaitIdle(vul.device);
    finalizeConnection();
    finalizeArtwork();
    swapFinalize(&swap);
    vulFinalize(&vul);
    guiFinalize(&gui);
    return 0;
}



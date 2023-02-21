/*
    480mainUniforms.c
    Demos new implementation of camera as a uniform over the scene.
    Written by Josh Davis for Carleton College's CS311 - Computer Graphics.
*/


/*

This file is the fourth step in my Vulkan tutorials, which are based on the 2020 
April version of Alexander Overvoorde's Vulkan tutorial at 
    https://www.vulkan-tutorial.com/
in consultation with the Vulkan Programming Guide by Graham Sellers and other 
sources. In this step, we introduce scene-wide uniforms, which in practice could 
include time, camera, and lights. We must write a substantial amount of 
'descriptor' code, which describes how uniform data on the CPU side corresponds 
to uniform data in the shaders. The resulting program revolves the camera around 
the meshes. Code that is new, compared to the preceding tutorial, is marked 
'New' in the comments.

On macOS, make sure that NUMDEVICEEXT below is 2, and then compile with 
    clang 480mainUniforms.c -lglfw -lvulkan
You might also need to compile the shaders with 
    glslc 480shader.vert -o 480vert.spv
    glslc 480shader.frag -o 480frag.spv
Then run the program with 
    ./a.out

On Linux, make sure that NUMDEVICEEXT below is 1, and then compile with 
    clang 480mainUniforms.c -lglfw -lvulkan -lm
You might also need to compile the shaders with 
    /mnt/c/VulkanSDK/1.3.216.0/Bin/glslc.exe 480shader.vert -o 480vert.spv
    /mnt/c/VulkanSDK/1.3.216.0/Bin/glslc.exe 480shader.frag -o 480frag.spv
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

/* Remember (from the previous tutorials) to read from globals as you like but 
write to globals only through their accessor functions. */
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

/* New: A file to manage uniform buffers. A file to describe how uniforms on the 
CPU side connect to uniforms in the shaders. */
#include "480uniform.c"
#include "480description.c"



/*** ARTWORK ******************************************************************/

/* Here's the variable to hold the shader program. */
shaProgram shaProg;

/* Our meshes are assumed to use a single attribute style: three attributes of 
dimensions 3 (xyz), 3 (rgb), 2 (st). */
#define MESHNUMATTRS 3
int meshAttrDims[MESHNUMATTRS] = {3, 3, 2};
/* Several variables are needed to pass that mesh style to Vulkan. */
int meshTotalAttrDim;
VkPipelineVertexInputStateCreateInfo vertexInputInfo;
VkPipelineInputAssemblyStateCreateInfo inputAssembly;
VkVertexInputBindingDescription meshBindingDesc;
VkVertexInputAttributeDescription meshAttrDescs[MESHNUMATTRS];

/* Here is a specific mesh with eight vertices and four triangles. */
int meshNumVertsA = 8;
const float meshVertsA[] = {
    -0.5, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 
    0.5, -0.5, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 
    0.5, 0.5, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 
    -0.5, 0.5, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 
    -0.5, -0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 
    0.5, -0.5, 0.5, 0.0, 1.0, 0.0, 1.0, 0.0, 
    0.5, 0.5, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0, 
    -0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.0, 1.0
};
int meshNumTrisA = 4;
const uint16_t meshTrisA[] = {
    0, 1, 2, 
    2, 3, 0, 
    4, 5, 6, 
    6, 7, 4, 
};
VkBuffer meshVertBufA;
VkDeviceMemory meshVertBufMemA;
VkBuffer meshTriBufA;
VkDeviceMemory meshTriBufMemA;

/* Here is another specific mesh with four vertices and four triangles. */
int meshNumVertsB = 4;
const float meshVertsB[] = {
    1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 
    2.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 
    1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 
    1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0
};
int meshNumTrisB = 4;
const uint16_t meshTrisB[] = {
    0, 2, 1, 
    0, 1, 3, 
    0, 3, 2, 
    3, 1, 2, 
};
VkBuffer meshVertBufB;
VkDeviceMemory meshVertBufMemB;
VkBuffer meshTriBufB;
VkDeviceMemory meshTriBufMemB;

/* Initializes the artwork. Upon success (return code 0), don't forget to 
finalizeArtwork later. */
int initializeArtwork() {
    /* New: Load the updated vertex and fragment shaders. */
    if (shaInitialize(&shaProg, "480vert.spv", "480frag.spv") != 0) {
        return 7;
    }
    meshGetStyle(
        MESHNUMATTRS, meshAttrDims, &meshTotalAttrDim, &meshBindingDesc, 
        meshAttrDescs, &vertexInputInfo, &inputAssembly);
    if (meshInitializeVertexBuffer(
            &meshVertBufA, &meshVertBufMemA, meshTotalAttrDim, meshNumVertsA, 
            meshVertsA) != 0) {
        shaFinalize(&shaProg);
        return 6;
    }
    if (meshInitializeIndexBuffer(
            &meshTriBufA, &meshTriBufMemA, meshNumTrisA, meshTrisA) != 0) {
        meshFinalizeVertexBuffer(&meshVertBufA, &meshVertBufMemA);
        shaFinalize(&shaProg);
        return 5;
    }
    if (meshInitializeVertexBuffer(
            &meshVertBufB, &meshVertBufMemB, meshTotalAttrDim, meshNumVertsB, 
            meshVertsB) != 0) {
        meshFinalizeIndexBuffer(&meshTriBufA, &meshTriBufMemA);
        meshFinalizeVertexBuffer(&meshVertBufA, &meshVertBufMemA);
        shaFinalize(&shaProg);
        return 4;
    }
    if (meshInitializeIndexBuffer(
            &meshTriBufB, &meshTriBufMemB, meshNumTrisB, meshTrisB) != 0) {
        meshFinalizeVertexBuffer(&meshVertBufB, &meshVertBufMemB);
        meshFinalizeIndexBuffer(&meshTriBufA, &meshTriBufMemA);
        meshFinalizeVertexBuffer(&meshVertBufA, &meshVertBufMemA);
        shaFinalize(&shaProg);
        return 3;
    }
    return 0;
}

/* Releases the artwork resources. */
void finalizeArtwork() {
    meshFinalizeIndexBuffer(&meshTriBufB, &meshTriBufMemB);
    meshFinalizeVertexBuffer(&meshVertBufB, &meshVertBufMemB);
    meshFinalizeIndexBuffer(&meshTriBufA, &meshTriBufMemA);
    meshFinalizeVertexBuffer(&meshVertBufA, &meshVertBufMemA);
    shaFinalize(&shaProg);
}



/*** UNIFORM PART OF CONNECTION BETWEEN SWAP CHAIN AND SCENE ******************/

/* New: This entire section. It's really part of the connection section below, 
but we separate it out so that we can focus on what's new. What's new is the 
uniforms and the description of how uniforms are passed into the shaders.

The following data structure holds our scene-wide uniforms. It's called a 
uniform buffer object (UBO). We fill it with data on the CPU side, then copy it 
over to the GPU, and it shows up in the shaders. The member types declared here 
must exactly match those in the corresponding struct declared in the shaders, in 
the same order. And there are two annoying wrinkles, as follows.

First, GLSL stores matrices column-by-column, while C stores matrices row-by-
row. So, when we put a C matrix into the camera field of this struct, we should 
not simply copy it there but rather transpose it there. That's why I've named 
the camera field 'cameraT' rather than 'camera'.

Second, your fields don't have to be 4-dimensional vectors and 4x4 matrices. For 
example, you can have 3-dimensional vectors, isolated scalars, and 2x2 matices. 
But then you run into issues of how the members are 'aligned' in memory. The 
simplest solution is: Use only 4-dimensional vectors and 4x4 matrices, where the 
underlying type is float, int32_t, or uint32_t. */
typedef struct SceneUniforms SceneUniforms;
struct SceneUniforms {
    float color[4];
    float cameraT[4][4];
};

/* These dynamically allocated arrays have length swap.numImages. They hold the 
uniform values on the GPU side. */
VkBuffer *sceneUniformBuffers;
VkDeviceMemory *sceneUniformBuffersMemory;

/* New: Called by presentFrame. Copies a UBO from the CPU side to the GPU side, 
so that the shaders can access its contents. */
void setSceneUniforms(uint32_t imageIndex) {
    float soFarTime = gui.currentTime - gui.startTime;
    SceneUniforms sceneUnifs;
    /* Set the color member of the scene uniforms. */
    sceneUnifs.color[0] = 1.0;
    sceneUnifs.color[1] = 0.5;
    sceneUnifs.color[2] = 0.0;
    sceneUnifs.color[3] = 1.0;
    /* Here's the camera matrix from our previous vertex shader. */  
    float previous[4][4] = {
        {3.700123, -0.487130, 0.000000, 0.000000},      // row 0, not column 0
        {-0.344453, -2.616382, -2.638959, 0.000004},    // row 1
        {0.093228, 0.708139, -0.714249, 9.090910},      // row 2
        {0.092296, 0.701057, -0.707107, 10.000000}};    // row 3
    /* Here's a rotation that we can animate, to revolve the camera about its 
    target. */
    float rotation[4][4] = {
        {cos(soFarTime), -sin(soFarTime), 0.0, 0.0},    // row 0, not column 0
        {sin(soFarTime), cos(soFarTime), 0.0, 0.0},     // row 1
        {0.0, 0.0, 1.0, 0.0},                           // row 2
        {0.0, 0.0, 0.0, 1.0}};                          // row 3
    /* The overall camera matrix is the matrix product of those two. */
    float camera[4][4];
    for (int i = 0; i < 4; i += 1)
        for (int j = 0; j < 4; j += 1) {
            camera[i][j] = 0.0;
            for (int k = 0; k < 4; k += 1)
                camera[i][j] += previous[i][k] * rotation[k][j];
        }
    /* Transpose that camera matrix into the scene UBO. */
    for (int i = 0; i < 4; i += 1)
        for (int j = 0; j < 4; j += 1)
            sceneUnifs.cameraT[i][j] = camera[j][i];
	/* Copy the scene UBO bits from the CPU to the GPU. */
	void *data;
	vkMapMemory(
	    vul.device, sceneUniformBuffersMemory[imageIndex], 0, 
	    sizeof(sceneUnifs), 0, &data);
	memcpy(data, &sceneUnifs, sizeof(sceneUnifs));
	vkUnmapMemory(vul.device, sceneUniformBuffersMemory[imageIndex]);
}

/* Configure the description of how uniform data are passed into the shaders. 
Right now we have only one uniform to describe: the scene-wide UBO. */
#define UNIFSCENE 0
#define UNIFNUM 1
/* Some kinds of descriptors need multiple copies, but not this one. */
int descriptorCounts[UNIFNUM] = {1};
/* There are various kinds of descriptors, but this one is for a simple UBO. */
VkDescriptorType descriptorTypes[UNIFNUM] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
/* We make it available to both the vertex shader and the fragment shader. 
Alternatively, we could have picked just one of those shader stages. */
VkShaderStageFlags descriptorStageFlagss[UNIFNUM] = {
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT};
/* We bind it to 'binding = 0' in the shaders. */
int descriptorBindings[UNIFNUM] = {0};

/* This data structure presents all of the configuration information to Vulkan 
in the rather complicated format that it likes. */
descDescription desc;

/* Helper function for descInitialize. Provides the parts of the customization 
that are difficult to abstract. The i argument specifies which element of the 
swap chain we're operating on. */
void setDescriptorSet(descDescription *desc, int i) {
    /* Configure the descriptor for the scene UBO. Essentially we're telling it 
    what GPU memory to use and how big that memory is. */
    VkDescriptorBufferInfo sceneUBOInfo = {0};
    sceneUBOInfo.buffer = sceneUniformBuffers[i];
    sceneUBOInfo.offset = 0;
    sceneUBOInfo.range = sizeof(SceneUniforms);
    VkDescriptorBufferInfo sceneUBODescBufInfos[] = {sceneUBOInfo};
    /* Configure the data structure that controls how that configuration is 
    transmitted to Vulkan. */
    VkWriteDescriptorSet sceneUBOWrite = {0};
    sceneUBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sceneUBOWrite.dstSet = desc->descriptorSets[i];
    sceneUBOWrite.dstBinding = descriptorBindings[UNIFSCENE];
    sceneUBOWrite.dstArrayElement = 0;
    sceneUBOWrite.descriptorType = descriptorTypes[UNIFSCENE];
    sceneUBOWrite.descriptorCount = descriptorCounts[UNIFSCENE];
    sceneUBOWrite.pBufferInfo = sceneUBODescBufInfos;
    /* Transmit that one configuration to Vulkan. */
    VkWriteDescriptorSet descWrites[] = {sceneUBOWrite};
    vkUpdateDescriptorSets(vul.device, 1, descWrites, 0, NULL);
}

/* Initializes all of the machinery for communicating uniforms to shaders. 
Returns an error code (0 on success). On success, don't forget to 
finalizeUniforms when you're done. */
int initializeUniforms() {
    if (unifInitializeBuffers(
            &sceneUniformBuffers, &sceneUniformBuffersMemory, 
            sizeof(SceneUniforms)) != 0)
        return 4;
    if (descInitialize(
            &desc, UNIFNUM, descriptorCounts, descriptorTypes, 
            descriptorStageFlagss, descriptorBindings, setDescriptorSet) != 0) {
        unifFinalizeBuffers(&sceneUniformBuffers, &sceneUniformBuffersMemory);
        return 1;
    }
    return 0;
}

/* Releases the resources backing all of the uniform machinery. */
void finalizeUniforms() {
    descFinalize(&desc);
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
    /* New: These two lines of code specifying the descriptor set layout. */
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
        /* New: Connect the uniform descriptor sets to the command buffer. The 
        first three arguments are boilerplate. The next three arguments say: In 
        the ith descriptor set array, connect 1 descriptor set starting at index 
        0. The last two arguments exist to pass an array of offsets when we use 
        dynamic descriptors. We're not using them right now. */
        vkCmdBindDescriptorSets(
            connCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, 
            connPipelineLayout, 0, 1, &(desc.descriptorSets[i]), 0, NULL);
        /* Render a mesh. */
        VkDeviceSize offsets[] = {0};
        VkBuffer vertexBuffers[] = {meshVertBufA};
        vkCmdBindVertexBuffers(
            connCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(
            connCommandBuffers[i], meshTriBufA, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(
            connCommandBuffers[i], (uint32_t)(meshNumTrisA * 3), 1, 0, 0, 0);
        /* Render another mesh. */
        vertexBuffers[0] = meshVertBufB;
        vkCmdBindVertexBuffers(
            connCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(
            connCommandBuffers[i], meshTriBufB, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(
            connCommandBuffers[i], (uint32_t)(meshNumTrisB * 3), 1, 0, 0, 0);
        /* End render pass and command buffer. */
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
    /* New: Initialize the uniform descriptor machinery before the pipeline (and 
    handle errors). */
    if (initializeUniforms() != 0)
        return 3;
    if (initializePipeline(&shaProg, &vertexInputInfo, &inputAssembly) != 0) {
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
    /* New: Finalize the uniform descriptor machinery after the pipeline. */
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
    /* New: Send data to the scene UBO in the shaders. */
    setSceneUniforms(imageIndex);
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
    guiRun(&gui);
    vkDeviceWaitIdle(vul.device);
    finalizeConnection();
    finalizeArtwork();
    swapFinalize(&swap);
    vulFinalize(&vul);
    guiFinalize(&gui);
    return 0;
}



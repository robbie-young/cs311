


/*

This file is the third step in my Vulkan tutorials, which are based on the 2020 
April version of Alexander Overvoorde's Vulkan tutorial at 
    https://www.vulkan-tutorial.com/
in consultation with the Vulkan Programming Guide by Graham Sellers and other 
sources. In this step, we draw two meshes in a simple way and without any 
animation. To get there, we have to build a scene using a shader program and 
meshes, where each mesh consists of a vertex buffer and an index buffer. 
Moreover, we have to write a substantial amount of code to 'connect' the swap 
chain to this scene. Code that is new, compared to the preceding tutorial, is 
marked 'New' in the comments.

On macOS, make sure that NUMDEVICEEXT below is 2, and then compile with 
    clang 460mainMeshes.c -lglfw -lvulkan
You might also need to compile the shaders with 
    glslc 460shader.vert -o 460vert.spv
    glslc 460shader.frag -o 460frag.spv
Then run the program with 
    ./a.out

On Linux, make sure that NUMDEVICEEXT below is 1, and then compile with 
    clang 460mainMeshes.c -lglfw -lvulkan -lm
You might also need to compile the shaders with 
    /mnt/c/VulkanSDK/1.3.216.0/Bin/glslc.exe 460shader.vert -o 460vert.spv
    /mnt/c/VulkanSDK/1.3.216.0/Bin/glslc.exe 460shader.frag -o 460frag.spv
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

/* New: Two files. */
#include "460shader.c"
#include "460mesh.c"



/*** ARTWORK ******************************************************************/

/* New: This entire section, in which we load our artwork. Here's the variable 
to hold the shader program. */
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

/* Here is a specific mesh with eight vertices and four triangles. The vertices 
are packed into the vertex data array one after another. The 0th row is the 
first vertex, the 1th row is the second, and so on. Similarly, the triangles are 
packed into the triangle data array one after another. Each triangle consists of 
three indices into the vertex array. */
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
    if (shaInitialize(&shaProg, "460vert.spv", "460frag.spv") != 0) {
        return 6;
    }
    meshGetStyle(
        MESHNUMATTRS, meshAttrDims, &meshTotalAttrDim, &meshBindingDesc, 
        meshAttrDescs, &vertexInputInfo, &inputAssembly);
    if (meshInitializeVertexBuffer(
            &meshVertBufA, &meshVertBufMemA, meshTotalAttrDim, meshNumVertsA, 
            meshVertsA) != 0) {
        shaFinalize(&shaProg);
        return 5;
    }
    if (meshInitializeIndexBuffer(
            &meshTriBufA, &meshTriBufMemA, meshNumTrisA, meshTrisA) != 0) {
        meshFinalizeVertexBuffer(&meshVertBufA, &meshVertBufMemA);
        shaFinalize(&shaProg);
        return 4;
    }
    if (meshInitializeVertexBuffer(
            &meshVertBufB, &meshVertBufMemB, meshTotalAttrDim, meshNumVertsB, 
            meshVertsB) != 0) {
        meshFinalizeIndexBuffer(&meshTriBufA, &meshTriBufMemA);
        meshFinalizeVertexBuffer(&meshVertBufA, &meshVertBufMemA);
        shaFinalize(&shaProg);
        return 3;
    }
    if (meshInitializeIndexBuffer(
            &meshTriBufB, &meshTriBufMemB, meshNumTrisB, meshTrisB) != 0) {
        meshFinalizeVertexBuffer(&meshVertBufB, &meshVertBufMemB);
        meshFinalizeIndexBuffer(&meshTriBufA, &meshTriBufMemA);
        meshFinalizeVertexBuffer(&meshVertBufA, &meshVertBufMemA);
        shaFinalize(&shaProg);
        return 2;
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



/*** CONNECTION BETWEEN SWAP CHAIN AND SCENE **********************************/

/* New: This entire section, where we build the connection between the swap 
chain and the scene (artwork, etc.). At this point there are two pieces of 
machinery: the pipeline and the command buffers. */
VkPipelineLayout connPipelineLayout;
VkPipeline connGraphicsPipeline;
/* This dynamically allocated array has length swap.numImages. */
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
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = NULL;
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
    /* Here the arguments about artwork get used. */
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
    if (initializePipeline(&shaProg, &vertexInputInfo, &inputAssembly) != 0)
        return 2;
    if (initializeCommandBuffers() != 0) {
        finalizePipeline();
        return 1;
    }
    return 0;
}

/* Releases the connection between the swap chain and the scene. */
void finalizeConnection() {
    finalizeCommandBuffers();
    finalizePipeline();
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
    /* New: Release the connection before releasing the swap chain. */
    finalizeConnection();
    swapFinalize(&swap);
    if (swapInitialize(&swap) != 0)
        return 2;
    /* New: Create the connection after creating the swap chain. */
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
    /* Prepare to submit a request to render the new frame. */
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {swap.imageAvailSems[swap.curFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    /* New: We now have one command buffer instead of zero command buffers. */
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
    /* New: Initialize the connection after the swap chain and scene. */
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
    /* New: Finalize the connection before the scene and swap chain. */
    finalizeConnection();
    finalizeArtwork();
    swapFinalize(&swap);
    vulFinalize(&vul);
    guiFinalize(&gui);
    return 0;
}



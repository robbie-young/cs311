


/*

This file is the second step in my Vulkan tutorials, which are based on the 2020 
April version of Alexander Overvoorde's Vulkan tutorial at 
    https://www.vulkan-tutorial.com/
in consultation with the Vulkan Programming Guide by Graham Sellers and other 
sources. In this step, we introduce the swap chain infrastructure. Now the 
application shows a window cleared to black. Unfortunately, the validation 
layers throw an error on each frame. We fix that in the next tutorial. Anyway, 
the code that is new, compared to the preceding tutorial, is marked 'New' in the 
comments.

On macOS, make sure that NUMDEVICEEXT below is 2, and then compile with 
    clang 450mainSwap.c -lglfw -lvulkan
Then run the program with 
    ./a.out

On Linux, make sure that NUMDEVICEEXT below is 1, and then compile with 
    clang 450mainSwap.c -lglfw -lvulkan -lm
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

/* Remember (from the previous tutorial) to read from globals as you like but 
write to globals only through their accessor functions. */
#include "440gui.c"
guiGUI gui;
#include "440vulkan.c"
vulVulkan vul;

/* New: Three files and one global variable. */
#include "450buffer.c"
#include "450image.c"
#include "450swap.c"
swapChain swap;

/* New: I have ripped out the user interface section. Refer back to the earlier 
tutorial, when you want to handle user input. */



/*** MAIN *********************************************************************/

/* New: This function is called by presentFrame whenever the swap chain needs to 
be deconstructed and reconstructed. Returns an error code (0 on success). On 
success, remember to call the appropriate finalizers when you're done. By the 
way, if an initializer in here really failed, then that would presumably cause 
cascading errors in all later frames, so the user would have to quit. I'm not 
sure how to handle that situation better. */
int reinitializeSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(gui.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(gui.window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vul.device);
    swapFinalize(&swap);
    if (swapInitialize(&swap) != 0)
        return 2;
    return 0;
}

/* New: Now that the swap chain is in place, we can place a bunch of swap-
related machinery into presentFrame. */
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
    /* Prepare to submit a request to render a new frame. */
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {swap.imageAvailSems[swap.curFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 0;
    submitInfo.pCommandBuffers = NULL;
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
        fprintf(stderr, "error: presentFrame: \n");
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
    /* New: Initialize the swap chain. */
    if (swapInitialize(&swap) != 0) {
        vulFinalize(&vul);
        guiFinalize(&gui);
        return 3;
    }
    /* New: I'm no longer registering keyboard and mouse handlers. */
    guiSetFramePresenter(&gui, presentFrame);
    guiRun(&gui);
    vkDeviceWaitIdle(vul.device);
    /* New: Finalize the swap chain before Vulkan and the GUI. */
    swapFinalize(&swap);
    vulFinalize(&vul);
    guiFinalize(&gui);
    return 0;
}



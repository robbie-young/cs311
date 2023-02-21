


/*

This file is the first step in my Vulkan tutorials. My tutorials are based on 
the 2020 April version of Alexander Overvoorde's Vulkan tutorial at 
    https://www.vulkan-tutorial.com/
I have translated that tutorial from C++ to C, and I have drastically refactored 
the code. Meanwhile I have also repeatedly consulted the Vulkan Programming 
Guide by Graham Sellers and other sources. This program makes a Vulkan-ready 
GLFW window and responds to mouse and keyboard events, but it doesn't show any 
imagery.

On macOS, make sure that NUMDEVICEEXT below is 2, and then compile with 
    clang 440mainVulkan.c -lglfw -lvulkan
Then run the program with 
    ./a.out

On Linux, make sure that NUMDEVICEEXT below is 1, and then compile with 
    clang 440mainVulkan.c -lglfw -lvulkan -lm
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
NUMINSTANCEEXT and NUMDEVICEEXT. */
#define NUMVALLAYERS 1
/* Instance extensions required by GLFW are activated automatically and excluded 
from NUMINSTANCEEXT. */
#define NUMINSTANCEEXT 1
/* I think that NUMDEVICEEXT should be 1 on Linux and 2 on macOS. */
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

/* Helper files just declare types, constants, and functions. There are no 
forward references; that is, earlier files never depend on later files. They 
don't declare global variables; those are all in main.c. Read from global 
variables as you like, but write to them only through their accessors. */
#include "440gui.c"
guiGUI gui;
#include "440vulkan.c"
vulVulkan vul;



/*** USER INTERFACE ***********************************************************/

/* A frivolous example of how to handle keyboard presses. For more details, 
consult the GLFW documentation online. */
void handleKey(
        GLFWwindow *window, int key, int scancode, int action, int mods) {
    /* Detect which modifier keys are down. */
    int shiftIsDown, controlIsDown, altOptionIsDown, superCommandIsDown;
    shiftIsDown = mods & GLFW_MOD_SHIFT;
    controlIsDown = mods & GLFW_MOD_CONTROL;
    altOptionIsDown = mods & GLFW_MOD_ALT;
    superCommandIsDown = mods & GLFW_MOD_SUPER;
    /* Print silly messages. */
    if (action == GLFW_PRESS && key == GLFW_KEY_A)
        printf("You pressed the A key.\n");
    else if (action == GLFW_RELEASE && key == GLFW_KEY_RIGHT)
        printf("You released the right-arrow key.\n");
    else if (action == GLFW_REPEAT)
        printf("You're holding down a key.\n");
}

/* A frivolous example of how to handle mouse button presses. For more 
details, consult the GLFW documentation online. */
void handleMouseButton(GLFWwindow *window, int button, int action, int mods) {
    /* Detect which modifier keys are down. */
    int shiftIsDown, controlIsDown, altOptionIsDown, superCommandIsDown;
    shiftIsDown = mods & GLFW_MOD_SHIFT;
    controlIsDown = mods & GLFW_MOD_CONTROL;
    altOptionIsDown = mods & GLFW_MOD_ALT;
    superCommandIsDown = mods & GLFW_MOD_SUPER;
    /* Get the mouse position (x, y) relative to the lower left corner of the 
    window. */
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    y = height - y;
    /* Print silly messages. */
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT)
        printf("You pressed the left mouse button at (%f, %f).\n", x, y);
    else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT)
        printf("You released the right mouse button at (%f, %f).\n", x, y);
}

/* A frivolous example of how to handle mouse moves. For more details, consult 
the GLFW documentation online. */
void handleMouseMove(GLFWwindow *window, double x, double y) {
    /* Get the mouse position (x, y) relative to the lower left corner of the 
    window. */
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    y = height - y;
    /* Print silly messages? On second thought, let's not. */
    /* printf("You moved the mouse to (%f, %f).\n", x, y); */
}



/*** MAIN *********************************************************************/

/* Presents one animation frame to the window, returning an error code (0 on 
success). Right now it does nothing, but it has to be here, because guiRun needs 
it. */
int presentFrame() {
    return 0;
}

int main() {
    /* Initialize the graphical user interface and Vulkan infrastructure. */
    if (guiInitialize(&gui, 512, 512, "Vulkan") != 0)
        return 5;
    if (vulInitialize(&vul) != 0) {
        guiFinalize(&gui);
        return 4;
    }
    /* Run the user interface. */
    glfwSetKeyCallback(gui.window, handleKey);
    glfwSetCursorPosCallback(gui.window, handleMouseMove);
    glfwSetMouseButtonCallback(gui.window, handleMouseButton);
    guiSetFramePresenter(&gui, presentFrame);
    guiRun(&gui);
    /* Clean up. */
    vkDeviceWaitIdle(vul.device);
    vulFinalize(&vul);
    guiFinalize(&gui);
    return 0;
}



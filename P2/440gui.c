


/* This file builds a simple graphical user interface (GUI) consisting of a 
window that is capable of receiving user input (mouse clicks, key presses, etc.) 
and showing Vulkan graphics. If we were all doing this work on a single 
operating system, such as macOS, then we would use that operating system's tools 
to make such a window. However, we are trying to write code that can run on both 
Linux and macOS, so we use the GLFW toolkit instead.

It is worth emphasizing that GLFW has (almost) nothing to do with Vulkan. In 
fact, this file does not contain a single Vulkan function call. */

/* Feel free to read from this struct's members, but don't write to them except 
through the accessors below. */
typedef struct guiGUI guiGUI;
struct guiGUI {
    GLFWwindow *window;
    int framebufferResized;
    double startTime, lastTime, currentTime;
    int (*presentFrame)(void);
};

/* Usually you call this function after handling a resizing event. You pass 0 to 
the function, to indicate that there's no longer a pending resizing event. */
void guiSetFramebufferResized(guiGUI *gui, int framebufferResized) {
    gui->framebufferResized = framebufferResized;
}

/* Returns the current time in seconds since some distant past time (1970?). */
double guiGetTime(const guiGUI *gui) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

/* This GLFW callback is called whenever GLFW throws errors. */
void guiErrorCallback(int error, const char *description) {
    fprintf(
        stderr, "error: guiErrorCallback: GLFW code %d, message...\n%s\n",
        error, description);
}

/* This GLFW callback is called whenever the window resizes. */
void guiResizingCallback(GLFWwindow* window, int width, int height) {
    /* A pointer to the GUI has been stored inside the GLFW window. */
    guiGUI *gui = glfwGetWindowUserPointer(window);
    /* The actual resizing happens asynchronously in presentFrame. */
    gui->framebufferResized = 1;
}

/* Initializes the GUI, returning an error code (0 on success). On success, 
don't forget to call guiFinalize when you're done. */
int guiInitialize(guiGUI *gui, int width, int height, const char *title) {
    /* Start keeping track of time. */
    gui->startTime = guiGetTime(gui);
    gui->lastTime = gui->startTime;
    gui->currentTime = gui->startTime;
    /* Initialize GLFW and one window. */
    glfwSetErrorCallback(guiErrorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    gui->window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwSetWindowUserPointer(gui->window, gui);
    glfwSetFramebufferSizeCallback(gui->window, guiResizingCallback);
    gui->framebufferResized = 0;
    gui->presentFrame = NULL;
    return 0;
}

/* Sets the callback function, that guiRun calls to present a frame to the 
screen. The presentFrame function's return value must be an error code: 0 
signaling no error, non-zero signaling an error.*/
void guiSetFramePresenter(guiGUI *gui, int (*presentFrame)(void)) {
    gui->presentFrame = presentFrame;
}

/* Runs the user interface event loop. */
void guiRun(guiGUI *gui) {
    int numErrors = 0, numFramesPerSecond = 0;
    while (!glfwWindowShouldClose(gui->window)) {
        /* Deal with the user. */
        glfwPollEvents();
        /* Keep track of time. */
        gui->lastTime = gui->currentTime;
        gui->currentTime = guiGetTime(gui);
        /* Show frames per second if VERBOSE. */
        numFramesPerSecond += 1;
        if (VERBOSE && (floor(gui->currentTime) > floor(gui->lastTime))) {
            fprintf(stderr, "info: guiRun: %d frames/s\n", numFramesPerSecond);
            numFramesPerSecond = 0;
        }
        /* Render. Print more diagnostics if VERBOSE. */
        if (gui->presentFrame() != 0 && VERBOSE) {
            numErrors += 1;
            if (numErrors == 100) {
                fprintf(stderr, "warning: guiRun: 100 more strange frames\n");
                numErrors = 0;
            }
        }
    }
}

/* Releases the resources backing the GUI. */
void guiFinalize(guiGUI *gui) {
    glfwDestroyWindow(gui->window);
    glfwTerminate();
}



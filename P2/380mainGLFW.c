


/* On macOS, compile with...
    clang 380mainGLFW.c -lglfw -framework OpenGL -framework Cocoa -framework IOKit -Wno-deprecated
On Ubuntu, compile with...
    cc 380mainGLFW.c -lglfw -lGL -lm -ldl
*/

#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>

/* Renders the window using OpenGL. */
void render(void) {
    /* Clear the OpenGL framebuffer to black, before we start rendering. By the 
    way, all OpenGL function names begin with 'gl' (and not 'glfw'), and all 
    OpenGL constants have names beginning with 'GL_'. */
    glClear(GL_COLOR_BUFFER_BIT);
    /* OpenGL's projection matrix corresponds to the matrix P in our software 
    graphics engine. Here we set an orthographic projection, in terms of left, 
    right, bottom, top, near, and far parameters. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    /* This is the simplest way to draw a triangle in OpenGL. It is also the 
    worst way. We never use it again after this tutorial. */
    glBegin(GL_TRIANGLES);
    glColor3f(1.0, 1.0, 0.0);
    glVertex2f(0.1, 0.1);
    glVertex2f(0.9, 0.2);
    glVertex2f(0.6, 0.9);
    glEnd();
}

/* Returns the time, in seconds, since some specific moment in the distant 
past. Supposedly accurate down to microseconds, but I wouldn't count on it. */
double getTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

/* This is a GLFW callback, which we register with GLFW below. It is 
automatically called whenever GLFW throws an error. */
void handleError(int error, const char *description) {
    fprintf(stderr, "handleError: %d\n%s\n", error, description);
}

/* This is another GLFW callback, that is automatically called when the GLFW 
window is resized. */
void handleResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

/* Performs whatever per-frame animation tasks our application needs. newTime is 
the time of the current frame, and oldTime is the time of the previous frame. */
void handleTimeStep(GLFWwindow *window, double oldTime, double newTime) {
    if (floor(newTime) - floor(oldTime) >= 1.0)
        printf("handleTimeStep: %f frames/sec\n", 1.0 / (newTime - oldTime));
    /* Because this tutorial's window contents are static, re-rendering them on 
    every frame is wasteful. But in many applications we want to re-render on 
    every frame, so let's set things up that way. */
    render();
    /* The GLFW window is double-buffered. We render into the back buffer. Then 
    we quickly swap the contents of the back buffer into the front buffer, where 
    the user can see them. This technique makes it easier to synchronize 
    rendering with the graphics hardware, and prevents a user from ever seeing a 
    frame half-drawn. By the way, all GLFW function names begin with 'glfw'. */
    glfwSwapBuffers(window);
}

/* Initializes the window, including its associated OpenGL context. */
GLFWwindow *initializeWindow(int width, int height, const char *name) {
    glfwSetErrorCallback(handleError);
    if (glfwInit() == 0) {
        fprintf(stderr, "initializeWindow: glfwInit failed.\n");
        return NULL;
    }
    GLFWwindow *window;
    window = glfwCreateWindow(width, height, name, NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "initializeWindow: glfwCreateWindow failed.\n");
        glfwTerminate();
        return NULL;
    }
    glfwSetFramebufferSizeCallback(window, handleResize);
    glfwMakeContextCurrent(window);
    fprintf(stderr, "initializeWindow: using OpenGL %s and GLSL %s.\n", 
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    return window;
}

/* Cleans up the resources associated with the GLFW window. */
void finalizeWindow(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(void) {
    /* For the sake of animation, we keep track of the time of the previous 
    animation frame and the current animation frame. This is how they begin. */
    double oldTime;
    double newTime = getTime();
    /* Initialize GLFW and handle errors. */
    GLFWwindow *window = initializeWindow(1024, 768, "Learning GLFW");
    if (window == NULL)
        return 1;
    /* Run the user interface loop, until the user closes/quits. */
    while (glfwWindowShouldClose(window) == 0) {
        /* Update our sense of time since the last animation frame. */
        oldTime = newTime;
        newTime = getTime();
        /* Do whatever per-frame animation tasks our application requires. */
        handleTimeStep(window, oldTime, newTime);
        /* Process any pending user interface events using the callbacks. */
        glfwPollEvents();
    }
    /* Although it doesn't matter in this program (because the program is about 
    to quit, and all of its resources are reclaimed then), we should get in the 
    habit of properly cleaning up resources. */
    finalizeWindow(window);
    return 0;
}



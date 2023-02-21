


/* On macOS, compile with...
    clang 390mainOpenGL14.c -lglfw -framework OpenGL -framework Cocoa -framework IOKit -Wno-deprecated
On Ubuntu, compile with...
    cc 390mainOpenGL14.c -lglfw -lGL -lm -ldl
*/

#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>



/*** Scene objects ***/

/* This is the only quantity that we animate in this tutorial. */
double animationAngle;

/* Here we build a mesh by hand. It is a unit cube. At each vertex it has an XYZ 
position and an RGB color. */
#define TRINUM 12
#define VERTNUM 8
/* For compatibility across hardware, operating systems, and compilers, 
OpenGL defines its own versions of double, int, unsigned int, etc. Use these 
OpenGL types whenever passing arrays or pointers to OpenGL (unless OpenGL says 
otherwise). Do not assume that these types equal your usual C types. */
GLdouble positions[VERTNUM * 3] = {
    0.0, 0.0, 0.0, 
    1.0, 0.0, 0.0, 
    0.0, 1.0, 0.0, 
    0.0, 0.0, 1.0, 
    1.0, 1.0, 0.0, 
    0.0, 1.0, 1.0, 
    1.0, 0.0, 1.0, 
    1.0, 1.0, 1.0};
GLdouble colors[VERTNUM * 3] = {
    0.0, 0.0, 0.0, 
    1.0, 0.0, 0.0, 
    0.0, 1.0, 0.0, 
    0.0, 0.0, 1.0, 
    1.0, 1.0, 0.0, 
    0.0, 1.0, 1.0, 
    1.0, 0.0, 1.0, 
    1.0, 1.0, 1.0};
GLuint triangles[TRINUM * 3] = {
    0, 2, 1, 
    1, 2, 4, 
    0, 1, 3, 
    1, 6, 3, 
    1, 4, 7, 
    1, 7, 6, 
    3, 6, 5, 
    5, 6, 7, 
    0, 3, 2, 
    2, 3, 5, 
    2, 5, 7, 
    2, 7, 4};

int initializeScene(void) {
    /* Enable a few OpenGL features. */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    /* We are going to pass two kinds of attributes to OpenGL: vertex positions 
    and vertex colors. */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    /* Although it's unnecessary, I explicitly disable the other ways of passing 
    attributes, so that you get an idea of what OpenGL 1.4 can do. */
    glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_FOG_COORD_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    return 0;
}

void finalizeScene(void) {
    /* In later tutorials, this function will actually do something --- namely, 
    it will clean up any resources dynamically allocated by initializeScene. */
    return;
}



/*** Rendering ***/

void render(void) {
    /* Clear not just the color buffer, but also the depth buffer. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Similarly, to the previous tutorial, set the projection matrix. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 2.0);
    /* OpenGL's modelview matrix corresponds to C^-1 M in the notation of our 
    software graphics engine. That is, it's all of the isometries before 
    projection. Here, we animate by rotating the cube about the unit vector that 
    points from the origin toward <1, 1, 1>. */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(animationAngle / (M_PI / 180.0), 1.0, 1.0, 1.0);
    /* Send the arrays of attribute data to OpenGL. */
    glVertexPointer(3, GL_DOUBLE, 0, positions);
    glColorPointer(3, GL_DOUBLE, 0, colors);
    /* Draw the triangles. */
    glDrawElements(GL_TRIANGLES, TRINUM * 3, GL_UNSIGNED_INT, triangles);
}



/*** User interface ***/

double getTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

void handleError(int error, const char *description) {
    fprintf(stderr, "handleError: %d\n%s\n", error, description);
}

void handleResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void handleTimeStep(GLFWwindow *window, double oldTime, double newTime) {
    if (floor(newTime) - floor(oldTime) >= 1.0)
        printf("handleTimeStep: %f frames/sec\n", 1.0 / (newTime - oldTime));
    /* We could set animationAngle in the render function. And then we wouldn't 
    even need to make it global. But the organizational idea is that     
    handleTimeStep updates state, while render merely draws. */
    animationAngle = fmod(newTime, 2.0 * M_PI);
    render();
    glfwSwapBuffers(window);
}

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

void finalizeWindow(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(void) {
    double oldTime;
    double newTime = getTime();
    GLFWwindow *window = initializeWindow(1024, 768, "Learning OpenGL 1.4");
    if (window == NULL)
        return 1;
    if (initializeScene() != 0) {
        finalizeWindow(window);
        return 2;
    }
    while (glfwWindowShouldClose(window) == 0) {
        oldTime = newTime;
        newTime = getTime();
        handleTimeStep(window, oldTime, newTime);
        glfwPollEvents();
    }
    finalizeScene();
    finalizeWindow(window);
    return 0;
}



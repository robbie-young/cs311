


/* On macOS, compile with...
    clang 400mainOpenGL15.c -lglfw -framework OpenGL -framework Cocoa -framework IOKit -Wno-deprecated
On Ubuntu, compile with...
    cc 400mainOpenGL15.c -lglfw -lGL -lm -ldl
*/

#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>

/* These shortcuts help us dissect meshes as they sit in GPU memory. */
#define GLDOUBLEOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLdouble)))
#define GLUINTOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLuint)))



/*** Scene objects ***/

double animationAngle;
/* We're going to use the same mesh as in the preceding tutorial, but we're 
going to load it into GPU memory, instead of keeping it in CPU memory. Once 
it's loaded, we will refer to it using the two unsigned integers in this 
cubeVBOs variable. */
GLuint cubeVBOs[2];

#define TRINUM 12
#define VERTNUM 8
#define ATTRDIM 6

void initializeMesh(void) {
    /* Here are the mesh data, which no longer need to be global. I've combined 
    the positions and colors into a single array, with attrDim == 6 attributes 
    per vertex. */
    GLdouble attributes[VERTNUM * ATTRDIM] = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
        1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 
        0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 
        1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 
        0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 
        1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
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
    /* Ask OpenGL for two buffers in GPU memory. Copy the attributes and 
    triangles into them. GL_STATIC_DRAW is a performance hint. It means that we 
    expect to render using these buffers many times, without editing them. 
    Performance hints help OpenGL decide how to organize the data in memory. */
    glGenBuffers(2, cubeVBOs);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, VERTNUM * ATTRDIM * sizeof(GLdouble),
        (GLvoid *)attributes, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVBOs[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, TRINUM * 3 * sizeof(GLuint),
        (GLvoid *)triangles, GL_STATIC_DRAW);
}

int initializeScene(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    /* Even though the mesh is stored in GPU memory, we still must call 
    glEnableClientState, to tell OpenGL to incorporate the right kinds of 
    attributes into the rendering. */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    initializeMesh();
    return 0;
}

void finalizeScene(void) {
    /* Unlike in the previous tutorial, this finalizeScene has some resources to 
    clean up. */
    glDeleteBuffers(2, cubeVBOs);
}



/*** Rendering ***/

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 2.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(animationAngle / (M_PI / 180.0), 1.0, 1.0, 1.0);
    /* As in the preceding tutorial, rendering the mesh requires calls to 
    glVertexPointer, glNormalPointer, and glColorPointer. What's different is 
    that we don't pass buffers in CPU memory to those functions anymore. 
    Instead we 'bind' a GPU-side buffer and pass offsets into that. */
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBOs[0]);
    glVertexPointer(3, GL_DOUBLE, ATTRDIM * sizeof(GLdouble), 
        GLDOUBLEOFFSET(0));
    /* While the vertex positions and normals start at index 0 of the buffer, 
    the color data start at index 3. */
    glColorPointer(3, GL_DOUBLE, ATTRDIM * sizeof(GLdouble), 
        GLDOUBLEOFFSET(3));
    /* Wondering what 'ATTRDIM * sizeof(GLdouble)' was doing in those 
    functions? It is the 'stride' between the start of one vertex and the start 
    of another. It is needed here, because we have packed multiple kinds of 
    attributes into a single attribute array. */
    /* Passing the triangle indices is similar to passing the vertex 
    attributes: bind a GPU-side buffer and pass an index into it. */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVBOs[1]);
    glDrawElements(GL_TRIANGLES, TRINUM * 3, GL_UNSIGNED_INT, GLUINTOFFSET(0));
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
    GLFWwindow *window = initializeWindow(1024, 768, "Learning OpenGL 1.5");
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



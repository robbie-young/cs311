


/* On macOS, compile with...
    clang 420mainOpenGL20.c -lglfw -framework OpenGL -framework Cocoa -framework IOKit -Wno-deprecated
On Ubuntu, compile with...
    cc 420mainOpenGL20.c -lglfw -lGL -lm -ldl
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>

#include "410shading.c"
/* Back from the dead. */
#include "250vector.c"
#include "280matrix.c"

#define GLDOUBLEOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLdouble)))
#define GLUINTOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLuint)))



/*** Shaders ***/

GLuint program;
GLint positionLoc, colorLoc;
/* Instead of using OpenGL's modelview and projection matrices, we're going to 
use our own transformation matrices, just as in our software graphics engine. 
So we need to record their locations. */
GLint viewingLoc, modelingLoc;

int initializeShaderProgram(void) {
    /* The two matrices are sent to the vertex shader as uniforms. */
    GLchar vertexCode[] = "\
        uniform mat4 viewing;\
        uniform mat4 modeling;\
        attribute vec3 position;\
        attribute vec3 color;\
        varying vec4 rgba;\
        void main() {\
            gl_Position = viewing * modeling * vec4(position, 1.0);\
            rgba = vec4(color, 1.0);\
        }";
    GLchar fragmentCode[] = "\
        varying vec4 rgba;\
        void main() {\
            gl_FragColor = rgba;\
        }";
    program = shaMakeProgram(vertexCode, fragmentCode);
    if (program != 0) {
        glUseProgram(program);
        positionLoc = glGetAttribLocation(program, "position");
        colorLoc = glGetAttribLocation(program, "color");
        /* Record the uniform locations. */
        viewingLoc = glGetUniformLocation(program, "viewing");
        modelingLoc = glGetUniformLocation(program, "modeling");
    }
    return (program == 0);
}

void finalizeShaderProgram(void) {
    glDeleteProgram(program);
}



/*** Scene objects ***/

double animationAngle;
GLuint cubeVBOs[2];

#define TRINUM 12
#define VERTNUM 8
#define ATTRDIM 6

int initializeMesh(void) {
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
    glGenBuffers(2, cubeVBOs);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, VERTNUM * ATTRDIM * sizeof(GLdouble),
        (GLvoid *)attributes, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVBOs[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, TRINUM * 3 * sizeof(GLuint),
        (GLvoid *)triangles, GL_STATIC_DRAW);
    return 0;
}

void finalizeMesh(void) {
    glDeleteBuffers(2, cubeVBOs);
}

int initializeScene(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    if (initializeShaderProgram() != 0)
        return 3;
    if (initializeMesh() != 0) {
        finalizeShaderProgram();
        return 1;
    }
    return 0;
}

void finalizeScene(void) {
    finalizeShaderProgram();
    finalizeMesh();
}



/*** Rendering ***/

void render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Send the modeling transformation M to the vertex shader. */
    double axis[3] = {1.0 / sqrt(3.0), 1.0 / sqrt(3.0), 1.0 / sqrt(3.0)};
    double rot[3][3];
    mat33AngleAxisRotation(animationAngle, axis, rot);
    double model[4][4] = {
        {rot[0][0], rot[0][1], rot[0][2], 0.0}, 
        {rot[1][0], rot[1][1], rot[1][2], 0.0}, 
        {rot[2][0], rot[2][1], rot[2][2], 0.0}, 
        {0.0, 0.0, 0.0, 1.0}};
    shaSetUniform44(model, modelingLoc);
    /* Send the viewing transformation P C^-1 to the vertex shader. Actually C =    
    I and P is what kind of projection? */
    double viewing[4][4] = {
        {0.5, 0.0, 0.0, 0.0}, 
        {0.0, 0.5, 0.0, 0.0}, 
        {0.0, 0.0, 0.5, 0.0}, 
        {0.0, 0.0, 0.0, 1.0}};
    shaSetUniform44(viewing, viewingLoc);
    /* The rest of the function is just as in the preceding tutorial. */
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(colorLoc);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBOs[0]);
    glVertexAttribPointer(positionLoc, 3, GL_DOUBLE, GL_FALSE, 
        ATTRDIM * sizeof(GLdouble), GLDOUBLEOFFSET(0));
    glVertexAttribPointer(colorLoc, 3, GL_DOUBLE, GL_FALSE, 
        ATTRDIM * sizeof(GLdouble), GLDOUBLEOFFSET(3));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVBOs[1]);
    glDrawElements(GL_TRIANGLES, TRINUM * 3, GL_UNSIGNED_INT, GLUINTOFFSET(0));
    glDisableVertexAttribArray(positionLoc);
    glDisableVertexAttribArray(colorLoc);
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
    GLFWwindow *window = initializeWindow(1024, 768, "Learning OpenGL 2.0 More");
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



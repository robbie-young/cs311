


/* On macOS, compile with...
    clang 410mainOpenGL20.c -lglfw -framework OpenGL -framework Cocoa -framework IOKit -Wno-deprecated
On Ubuntu, compile with...
    cc 410mainOpenGL20.c -lglfw -lGL -lm -ldl
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <GLFW/glfw3.h>

/* The big new feature in OpenGL 2.0 is shader programs. This file will help us 
set them up. */
#include "410shading.c"

#define GLDOUBLEOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLdouble)))
#define GLUINTOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLuint)))



/*** Shaders ***/

/* Once we build our shader program, we can refer to it using this variable. */
GLuint program;
/* We will pass data into the shader program through certain 'locations' in the 
GPU memory. So we need to keep track of those locations. */
GLint positionLoc, colorLoc;

/* This helper function builds a particular shader program. Don't worry about 
the details of the GLSL code right now. Returns 0 on success, non-zero on error. 
On success, remember to call glDeleteProgram when you're finished with the 
program. */
int initializeShaderProgram(void) {
    /* But how do you get a big string of GLSL code into your C program? There 
    are three popular ways. The first way is to write the code in a separate 
    text file and use C file functions to load it. Let's not do that. The second 
    way is to write each line enclosed by quotation marks, as follows. The lines 
    are automatically concatenated into a single string. */
    GLchar vertexCode[] = 
        "attribute vec3 position;"
        "attribute vec3 color;"
        "varying vec4 rgba;"
        "void main() {"
        "    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);"
        "    rgba = vec4(color, 1.0);"
        "}";
    /* Another way to build a C string over multiple lines is to terminate each 
    line with a backslash. Use whichever way you prefer. */
    GLchar fragmentCode[] = "\
        varying vec4 rgba;\
        void main() {\
            gl_FragColor = rgba;\
        }";
    /* Just for educational purposes, print those two strings. Notice that the 
    strings don't actually contain any newline characters. You could make these 
    printouts more readable by inserting newlines into the code above, but then 
    the code itself would be less readable. */
    printf("initializeShaderProgram: vertexCode:\n%s\n", vertexCode);
    printf("initializeShaderProgram: fragmentCode:\n%s\n", fragmentCode);
    /* Turn the code into a compiled shader program using the big helper 
    function in 410shader.c. Then record the locations. */
    program = shaMakeProgram(vertexCode, fragmentCode);
    if (program != 0) {
        glUseProgram(program);
        positionLoc = glGetAttribLocation(program, "position");
        colorLoc = glGetAttribLocation(program, "color");
    }
    return (program == 0);
}



/*** Scene objects ***/

double animationAngle;
GLuint cubeVBOs[2];

#define TRINUM 12
#define VERTNUM 8
#define ATTRDIM 6

void initializeMesh(void) {
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
}

int initializeScene(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    /* Don't do the glEnableClientState stuff anymore. Instead initialize the 
    shader program. */
    if (initializeShaderProgram() != 0)
        return 3;
    initializeMesh();
    return 0;
}

void finalizeScene(void) {
    glDeleteProgram(program);
    glDeleteBuffers(2, cubeVBOs);
}



/*** Rendering ***/

void render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Make sure the intended shader program is active. (In a serious 
    application, we might switch among several shader programs rapidly.) */
    glUseProgram(program);
    /* As in the previous tutorial, send the transformations to the vertex 
    shader. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 2.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(animationAngle / (M_PI / 180.0), 1.0, 1.0, 1.0);
    /* Very much unlike the previous tutorial, connect our attribute array to 
    the attributes in the vertex shader. */
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(colorLoc);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBOs[0]);
    glVertexAttribPointer(positionLoc, 3, GL_DOUBLE, GL_FALSE, 
        ATTRDIM * sizeof(GLdouble), GLDOUBLEOFFSET(0));
    glVertexAttribPointer(colorLoc, 3, GL_DOUBLE, GL_FALSE, 
        ATTRDIM * sizeof(GLdouble), GLDOUBLEOFFSET(3));
    /* Exactly as in the previous tutorial, draw the triangles. */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVBOs[1]);
    glDrawElements(GL_TRIANGLES, TRINUM * 3, GL_UNSIGNED_INT, GLUINTOFFSET(0));
    /* Disable the attribute arrays when finished with them. */
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
    GLFWwindow *window = initializeWindow(1024, 768, "Learning OpenGL 2.0");
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



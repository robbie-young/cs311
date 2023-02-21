


/* On macOS, compile with...
    clang 430mainOpenGL32.c /usr/local/gl3w/src/gl3w.o -lglfw -framework OpenGL -framework Cocoa -framework IOKit -Wno-deprecated
On Ubuntu, compile with...
    cc 430mainOpenGL32.c /usr/local/gl3w/src/gl3w.o -lglfw -lGL -lm -ldl
On either operating system, you might have to change the location of gl3w.o 
based on your installation.

This tutorial is designed to be self-contained. It tells you how to use a recent 
(meaning, 3.0 or later) version of OpenGL. Specifically, it uses OpenGL 3.2. To 
obtain an OpenGL-capable window, we use GLFW. If we were programming 
specifically for macOS, then we'd replace GLFW with Cocoa; for Windows, with 
.NET; for Linux, with maybe Qt or GTK; and so forth. In addition to using GLFW 
to get the window, we also must use an OpenGL function loader, GL3W, to make the 
OpenGL 3.2 functionality available.

If you've read my earlier OpenGL tutorials, then there are four big changes in 
this tutorial. First, we ask GLFW for an OpenGL 3.2 context, rather than letting 
GLFW default to OpenGL 2.1. Second, we link to and initialize GL3W, which 
actually provides the OpenGL 3.2 functions to us. Third, we adjust our GLSL code 
to conform to GLSL 1.4. Fourth, we wrap each of our meshes in a vertex array 
object (VAO) that captures how the mesh interacts with the shader program. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
/* Include the GL3W header before any other OpenGL-related headers. */
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

/* These are your files from earlier in the course. */
#include "410shading.c"
#include "250vector.c"
#include "280matrix.c"

/* These shortcuts help us dissect meshes as they sit in GPU memory. */
#define GLDOUBLEOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLdouble)))
#define GLUINTOFFSET(bytes) ((GLubyte *)NULL + (bytes * sizeof(GLuint)))



/*** Shaders ***/

/* We use a shader program consisting of a vertex shader connected to a fragment 
shader. How OpenGL stores this information is entirely opaque. You refer to the 
shader program using an unsigned integer. */
GLuint program;
/* A location is a place for passing information into a shader program. Here we 
specify two attribute locations and two uniform locations. */
GLint positionLoc, colorLoc;
GLint viewingLoc, modelingLoc;

int initializeShaderProgram(void) {
    /* The vertex shader is written in OpenGL Shading Language (GLSL). You 
    supply the shader code as a C string, and OpenGL compiles it for you. The 
    vertex shader code must begin with a GLSL version number declaration --- in 
    this case, 1.40. Uniform variables are signalled by the keyword 'uniform'. 
    Attribute variables are signalled by 'in'. Varying variables are signalled 
    by 'out'. */
    /* But how do you get a big string of GLSL code into your C program? There 
    are three popular ways. The first way is to write the code in a separate 
    text file and use C file functions to load it. Let's not do that. The second 
    way is to write each line enclosed by quotation marks, as follows. The lines 
    are automatically concatenated into a single string. */
    GLchar vertexCode[] = 
        "#version 140\n"
        "uniform mat4 viewing;"
        "uniform mat4 modeling;"
        "in vec3 position;"
        "in vec3 color;"
        "out vec4 rgba;"
        "void main() {"
        "    gl_Position = viewing * modeling * vec4(position, 1.0);"
        "    rgba = vec4(color, 1.0);"
        "}";
    /* The fragment shader is similar. Now 'in' signals varying variables. We 
    declare our own output variable fragColor. As long as there is just one 
    4D vector output, OpenGL assumes that it's a color to be sent to the default 
    framebuffer. */
    /* This snippet illustrates the third way to get a big string of GLSL code 
    into your C program: Write it as a single string, with each line terminated 
    by a backslash. Use whichever approach to strings you prefer. */
    GLchar fragmentCode[] = "\
        #version 140\n\
        in vec4 rgba;\
        out vec4 fragColor;\
        void main() {\
            fragColor = rgba;\
        }";
    /* Using the functions in 300shading.c, we ask OpenGL to compile our shader 
    code into a shader program. */
    program = shaMakeProgram(vertexCode, fragmentCode);
    if (program != 0) {
        glUseProgram(program);
        /* These functions interrogate the shader program, to figure out where 
        the locations are. We store the locations in our C program, so that 
        later we can pass information into them. Notice that uniform locations 
        and attribute locations require different functions. */
        positionLoc = glGetAttribLocation(program, "position");
        colorLoc = glGetAttribLocation(program, "color");
        viewingLoc = glGetUniformLocation(program, "viewing");
        modelingLoc = glGetUniformLocation(program, "modeling");
    }
    return (program == 0);
}

void finalizeShaderProgram(void) {
    glDeleteProgram(program);
}



/*** Scene objects ***/

/* This is the only quantity that we animate in this tutorial. */
double animationAngle;
/* We need a buffer in GPU memory to store the vertices of our mesh. And we need 
another buffer to store the triangles. These buffers are called vertex buffer 
objects (VBOs). */
GLuint cubeVBOs[2];
/* We also need a vertex array object (VAO), which stores configuration 
information about how the mesh is to be rendered. */
GLuint cubeVAO;

#define TRINUM 12
#define VERTNUM 8
#define ATTRDIM 6

int initializeMesh(void) {
    /* Ordinarily we'd load a mesh from file. Or maybe we'd use a function such 
    as mesh3DInitializeSphere. But for now let's build a mesh manually. */
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
    /* Make the VBOs and fill them with data. GL_STATIC_DRAW tells OpenGL that 
    we're going to draw this mesh many times without changing it. Such 
    performance hints help OpenGL manage GPU memory efficiently. */
    glGenBuffers(2, cubeVBOs);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, VERTNUM * ATTRDIM * sizeof(GLdouble),
        (GLvoid *)attributes, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVBOs[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, TRINUM * 3 * sizeof(GLuint),
        (GLvoid *)triangles, GL_STATIC_DRAW);
    /* Make the VAO. Begin to tell it about the VBOs... */
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBOs[0]);
    /* ...namely, that each vertex consists of ATTRDIM doubles, and the 
    position attribute consists of 3 doubles and begins at index 0... */
    glEnableVertexAttribArray(positionLoc);
    glVertexAttribPointer(positionLoc, 3, GL_DOUBLE, GL_FALSE, 
        ATTRDIM * sizeof(GLdouble), GLDOUBLEOFFSET(0));
    /* ...and the color consists of 3 doubles and begins at index 3... */
    glEnableVertexAttribArray(colorLoc);
    glVertexAttribPointer(colorLoc, 3, GL_DOUBLE, GL_FALSE, 
        ATTRDIM * sizeof(GLdouble), GLDOUBLEOFFSET(3));
    /* ...and the triangles are in the other VBO. */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVBOs[1]);
    /* When you're done issuing commands to a VAO, unbind it by binding the 
    trivial VAO. (When I first wrote this code, I forgot this step, and it cost 
    me several hours of debugging.) */
    glBindVertexArray(0);
    return 0;
}

void finalizeMesh(void) {
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(2, cubeVBOs);
}

int initializeScene(void) {
    /* These OpenGL configurations are pretty common. */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    /* Initialize the shader program before the mesh, so that the shader 
    locations are already set up by the time the VAO is initialized. */
    if (initializeShaderProgram() != 0)
        return 4;
    if (initializeMesh() != 0) {
        finalizeShaderProgram();
        return 1;
    }
    return 0;
}

/* Cleans up whatever OpenGL resources were allocated in initializeScene. */
void finalizeScene(void) {
    finalizeMesh();
    finalizeShaderProgram();
}



/*** Rendering ***/

void render(void) {
    /* Clear both the RGB color buffer and the depth buffer. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Send the modeling isometry M to the vertex shader. */
    double axis[3] = {1.0 / sqrt(3.0), 1.0 / sqrt(3.0), 1.0 / sqrt(3.0)};
    double rot[3][3];
    mat33AngleAxisRotation(animationAngle, axis, rot);
    double model[4][4] = {
        {rot[0][0], rot[0][1], rot[0][2], 0.0}, 
        {rot[1][0], rot[1][1], rot[1][2], 0.0}, 
        {rot[2][0], rot[2][1], rot[2][2], 0.0}, 
        {0.0, 0.0, 0.0, 1.0}};
    shaSetUniform44(model, modelingLoc);
    /* Send the viewing transformation P C^-1 to the vertex shader. In this 
    tutorial, C = I. And P is what kind of projection? */
    double viewing[4][4] = {
        {0.5, 0.0, 0.0, 0.0}, 
        {0.0, 0.5, 0.0, 0.0}, 
        {0.0, 0.0, 0.5, 0.0}, 
        {0.0, 0.0, 0.0, 1.0}};
    shaSetUniform44(viewing, viewingLoc);
    /* Draw the scene object using the VBOs and VAO in GPU memory. */
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, TRINUM * 3, GL_UNSIGNED_INT, GLUINTOFFSET(0));
    glBindVertexArray(0);
}



/*** User interface ***/

/* Returns the time in seconds since some fixed time in the distant past. */
double getTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

/* This is a GLFW callback, that is invoked by GLFW when an error occurs. Before 
you do anything else with GLFW, you must give it this callback. */
void handleError(int error, const char *description) {
    fprintf(stderr, "handleError: %d\n%s\n", error, description);
}

/* This is a GLFW callback, that is invoked by GLFW when the window is resized. 
Later, if you use a camera to control the projection, then you might have to 
call camSetFrustum here too. */
void handleResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

/* This is not a GLFW callback. It is invoked by main on each frame of the 
animation. */
void handleTimeStep(GLFWwindow *window, double oldTime, double newTime) {
    if (floor(newTime) - floor(oldTime) >= 1.0)
        printf("handleTimeStep: %f frames/sec\n", 1.0 / (newTime - oldTime));
    /* The organizing principle is that handleTimeStep updates state, while 
    render merely draws whatever the current state is. But you do not have to 
    follow this principle in your work. */
    animationAngle = fmod(newTime, 2.0 * M_PI);
    /* The OpenGL window is double-buffered. We draw into the back buffer. Then 
    we quickly swap the contents of the back buffer into the front buffer, where 
    the user can see them. */
    render();
    glfwSwapBuffers(window);
}

/* Obtains an OpenGL 3.2-capable window. Returns NULL on failure. On success, 
you must remember to call finalizeWindow() when you're done with it. */
GLFWwindow *initializeWindow(int width, int height, const char *name) {
    /* Register the GLFW error-handling callback even before initializing. */
    glfwSetErrorCallback(handleError);
    if (glfwInit() == 0) {
        fprintf(stderr, "initializeWindow: glfwInit failed.\n");
        return NULL;
    }
    /* Ask GLFW to supply an OpenGL 3.2 context --- meaning, a connection to 
    OpenGL, able to do everything guaranteed by version 3.2. */
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    GLFWwindow *window;
    window = glfwCreateWindow(width, height, name, NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "initializeWindow: glfwCreateWindow failed.\n");
        glfwTerminate();
        return NULL;
    }
    /* Register the resizing callback. In the future, when you add more 
    callbacks for keyboard, mouse, etc., register them here. */
    glfwSetFramebufferSizeCallback(window, handleResize);
    /* Tell OpenGL that subsequent OpenGL calls are intended for this context, 
    as opposed to other contexts that might hypothetically exist. (In a 
    complicated program, you might have multiple OpenGL contexts existing 
    simultaneously, and you have to tell OpenGL which one you're currently 
    operating on.) */
    glfwMakeContextCurrent(window);
    /* You might think that getting an OpenGL 3.2 context would make OpenGL 3.2 
    available to us. But you'd be wrong. The following call 'loads' a bunch of 
    OpenGL 3.2 functions, so that we can use them. This is why we use GL3W. */
    if (gl3wInit() != 0) {
        fprintf(stderr, "initializeWindow: gl3wInit failed.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return NULL;
    }
    /* As an aside, here is the OpenGL version that we're using. We might be 
    using a later version of OpenGL than we asked for, if it's running in a 
    backward-compatible mode. */
    fprintf(stderr, "initializeWindow: using OpenGL %s and GLSL %s.\n", 
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    return window;
}

/* Cleans up GLFW and GL3W resources. */
void finalizeWindow(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(void) {
    /* Initialize everything. */
    double oldTime;
    double newTime = getTime();
    GLFWwindow *window = initializeWindow(1024, 768, "Learning OpenGL 3.2");
    if (window == NULL)
        return 1;
    if (initializeScene() != 0) {
        finalizeWindow(window);
        return 2;
    }
    /* Run the event loop. */
    while (glfwWindowShouldClose(window) == 0) {
        oldTime = newTime;
        newTime = getTime();
        handleTimeStep(window, oldTime, newTime);
        glfwPollEvents();
    }
    /* Finalize everything in the opposite order of initialization. Although it 
    doesn't matter in this program (because the program is about to quit, and 
    all of its resources are reclaimed then), we should get in the habit of 
    properly cleaning up resources. */
    finalizeScene();
    finalizeWindow(window);
    return 0;
}






/* The shaMakeProgram function below helps you compile and link OpenGL shader 
programs. The process is simpler than it looks. In pseudocode, the core idea is 
10 steps:
    glCreateShader(vertexShader);
        glShaderSource(vertexShader, vertexCode);
        glCompileShader(vertexShader);
    glCreateShader(fragmentShader);
        glShaderSource(fragmentShader, fragmentCode);
        glCompileShader(fragmentShader);
    glCreateProgram(program);
        glAttachShader(vertexShader);
        glAttachShader(fragmentShader);
        glLinkProgram(program);
The code below is much longer than 10 lines, only because it has lots of error 
checking. */

/* Compiles a shader from GLSL source code. type is either GL_VERTEX_SHADER or 
GL_FRAGMENT_SHADER. If an error occurs, then returns 0. Otherwise, returns a 
compiled shader, which the user must eventually deallocate with glDeleteShader. 
(But usually this function is called from shaMakeProgram, which calls 
glDeleteShader on the user's behalf.) */
GLuint shaMakeShader(GLenum type, const GLchar *shaderCode) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        fprintf(stderr, "shaMakeShader: glCreateShader failed\n");
        return 0;
    }
    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLsizei length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        GLchar *infoLog = (GLchar *)malloc(length);
        glGetShaderInfoLog(shader, length, &length, infoLog);
        fprintf(stderr, "shaMakeShader: glGetShaderInfoLog:\n%s\n", infoLog);
        free(infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/* Compiles and links a shader program from two pieces of GLSL source code. If 
an error occurs, then returns 0. Otherwise, returns a shader program, which the 
user should eventually deallocate using glDeleteProgram. */
GLuint shaMakeProgram(const GLchar *vertexCode, const GLchar *fragmentCode) {
    GLuint vertexShader, fragmentShader, program;
    vertexShader = shaMakeShader(GL_VERTEX_SHADER, vertexCode);
    if (vertexShader == 0)
        return 0;
    fragmentShader = shaMakeShader(GL_FRAGMENT_SHADER, fragmentCode);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }
    program = glCreateProgram();
    if (program == 0) {
        fprintf(stderr, "shaMakeProgram: glCreateProgram failed\n");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLsizei length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *infoLog = (GLchar *)malloc(length);
        glGetProgramInfoLog(program, length, &length, infoLog);
        fprintf(stderr, "shaMakeProgram: glGetProgramInfoLog:\n%s\n", infoLog);
        free(infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }
    /* Success. The shaders are built into the program and don't need to be 
    remembered separately, so delete them. */
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

/* Checks the validity of a shader program against the rest of the current 
OpenGL state. If you choose to use this function, invoke it *after* 
shaMakeProgram, setting up textures, etc. Returns 0 if okay, non-zero if error. 
*/
int shaValidateProgram(GLuint program) {
    GLint validation;
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &validation);
    if (validation != GL_TRUE) {
        GLsizei length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *infoLog = (GLchar *)malloc(length);
        glGetProgramInfoLog(program, length, &length, infoLog);
        fprintf(stderr, "shaValidateProgram: glGetProgramInfoLog:\n%s\n", 
            infoLog);
        free(infoLog);
        return 1;
    }
    return 0;
}

/* We want to pass 4x4 matrices into uniforms in OpenGL shaders, but there are 
two obstacles. First, our matrix library uses double matrices, but OpenGL 
shaders expect GLfloat matrices. Second, C matrices are implicitly stored one-
row-after-another, while OpenGL shaders expect matrices to be stored one-column-
after-another. This function plows through both of those obstacles. */
void shaSetUniform44(double m[4][4], GLint uniformLocation) {
    GLfloat mTFloat[4][4];
    for (int i = 0; i < 4; i += 1)
        for (int j = 0; j < 4; j += 1)
            mTFloat[i][j] = m[j][i];
    glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, (GLfloat *)mTFloat);
}

/* Here is a similar function for vectors. */
void shaSetUniform3(double v[3], GLint uniformLocation) {
    GLfloat vFloat[3];
    for (int i = 0; i < 3; i += 1)
        vFloat[i] = v[i];
    glUniform3fv(uniformLocation, 1, vFloat);
}

/* Here is a similar function for vectors. */
void shaSetUniform4(double v[4], GLint uniformLocation) {
    GLfloat vFloat[4];
    for (int i = 0; i < 4; i += 1)
        vFloat[i] = v[i];
    glUniform4fv(uniformLocation, 1, vFloat);
}



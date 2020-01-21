// Using SDL, GLEW
#include <GL/glew.h>
#include <SDL.h>

bool glLoadShader(GLuint& shader, const GLenum shaderType, const GLchar* shaderString) noexcept
{
    // Build and link the shader program
    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderString, nullptr);
    glCompileShader(shader);

    // Check for errors
    GLint testReturn;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &testReturn);
    if (testReturn == GL_FALSE) {
        GLchar infoLog[1024];
        int32_t errorLength;
        glGetShaderInfoLog(shader, 1024, &errorLength, infoLog);
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to compile shader: %s\n", infoLog);
        glDeleteShader(shader);
        return false;
    }
    return true;
}

bool glLoadShaders(GLuint& shader, const GLuint vertexShader, const GLuint fragmentShader) noexcept
{
    // Link the shaders
    shader = glCreateProgram();
    glAttachShader(shader, vertexShader);
    glAttachShader(shader, fragmentShader);
    glLinkProgram(shader);

    //Check for error in link
    GLint testReturn;
    glGetProgramiv(shader, GL_LINK_STATUS, &testReturn);
    if (testReturn == GL_FALSE) {
        GLchar infoLog[1024];
        int32_t errorLength;
        glGetShaderInfoLog(shader, 1024, &errorLength, infoLog);
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to link shaders: %s\n", infoLog);
        glDeleteProgram(shader);
        return false;
    }
    return true;
}
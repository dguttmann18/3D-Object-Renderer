#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "geometry.h"

class OpenGLWindow
{
public:
    OpenGLWindow();

    void initGL();
    void render();
    bool handleEvent(SDL_Event e);
    void cleanup();

private:
    SDL_Window* sdlWin;

    GLuint vao;
    GLuint shader;
    GLuint vertexBuffer;

    GeometryData geometryData;
    int colorLoc;
    int colourIdx;
    int vertexLoc;
    glm::mat4 modelMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 model;
    glm::mat4 MVP;
    glm::mat4 totalTransformations;

    char transformationMode;
    char axis;
};

#endif

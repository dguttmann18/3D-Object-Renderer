#include <iostream>
#include <stdio.h>

#include "SDL.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glwindow.h"
#include "geometry.h"
                                                                                                                                                                            
using namespace std;

const char* glGetErrorString(GLenum error)
{
    switch(error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNRECOGNIZED";
    }
}

void glPrintError(const char* label="Unlabelled Error Checkpoint", bool alwaysPrint=false)
{
    GLenum error = glGetError();
    if(alwaysPrint || (error != GL_NO_ERROR))
    {
        printf("%s: OpenGL error flag is %s\n", label, glGetErrorString(error));
    }
}

GLuint loadShader(const char* shaderFilename, GLenum shaderType)
{
    FILE* shaderFile = fopen(shaderFilename, "r");
    if(!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    long shaderSize = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    char* shaderText = new char[shaderSize+1];
    size_t readCount = fread(shaderText, 1, shaderSize, shaderFile);
    shaderText[readCount] = '\0';
    fclose(shaderFile);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char**)&shaderText, NULL);
    glCompileShader(shader);

    delete[] shaderText;

    return shader;
}

GLuint loadShaderProgram(const char* vertShaderFilename,
                       const char* fragShaderFilename)
{
    GLuint vertShader = loadShader(vertShaderFilename, GL_VERTEX_SHADER);
    GLuint fragShader = loadShader(fragShaderFilename, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &logLength, message);
        cout << "Shader load error: " << message << endl;
        return 0;
    }

    return program;
}

OpenGLWindow::OpenGLWindow()
{
}


void OpenGLWindow::initGL()
{
    // We need to first specify what type of OpenGL context we need before we can create the window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdlWin = SDL_CreateWindow("OpenGL Prac 1",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_OPENGL);
    if(!sdlWin)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to create window", 0);
    }
    SDL_GLContext glc = SDL_GL_CreateContext(sdlWin);
    SDL_GL_MakeCurrent(sdlWin, glc);
    SDL_GL_SetSwapInterval(1);

    //modelMatrix = glm::mat4(1.0f);
    

    glewExperimental = true;
    GLenum glewInitResult = glewInit();
    glGetError(); // Consume the error erroneously set by glewInit()
    if(glewInitResult != GLEW_OK)
    {
        const GLubyte* errorString = glewGetErrorString(glewInitResult);
        cout << "Unable to initialize glew: " << errorString;
    }

    int glMajorVersion;
    int glMinorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
    cout << "Loaded OpenGL " << glMajorVersion << "." << glMinorVersion << " with:" << endl;
    cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
    cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
    cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
    cout << "\tGLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0,0,0,1);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Note that this path is relative to your working directory
    // when running the program (IE if you run from within build
    // then you need to place these files in build as well)
    shader = loadShaderProgram("simple.vert", "simple.frag");
    glUseProgram(shader);

    totalTransformations = glm::mat4(1.0f);
    projectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    //glm::mat4 View = glm::lookAt(glm::vec3(4,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0) );
    viewMatrix = glm::lookAt(glm::vec3(0,0,5), glm::vec3(0,0,0), glm::vec3(0,1,0) );
    model = glm::mat4(1.0f);
    MVP = projectionMatrix * viewMatrix * model;
    GLuint MatrixID = glGetUniformLocation(shader, "MVP");
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    colorLoc = glGetUniformLocation(shader, "objectColor");
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    colourIdx = 0;

    // Load the model that we want to use and buffer the vertex attributes
    GeometryData geometry = GeometryData();
    geometry.loadFromOBJFile("doggo.obj");
    geometryData = geometry;

    vertexLoc = glGetAttribLocation(shader, "position");
    
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * geometry.vertexCount() * 3, geometry.vertexData(), GL_STATIC_DRAW);
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(vertexLoc);

    transformationMode = 's';
    axis = 'x';

    glPrintError("Setup complete", true);
}

void OpenGLWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, geometryData.vertexCount());

    // Swap the front and back buffers on the window, effectively putting what we just "drew"
    // onto the screen (whereas previously it only existed in memory)
    SDL_GL_SwapWindow(sdlWin);
}

// The program will exit if this function returns false
bool OpenGLWindow::handleEvent(SDL_Event e)
{
    // A list of keycode constants is available here: https://wiki.libsdl.org/SDL_Keycode
    // Note that SDL provides both Scancodes (which correspond to physical positions on the keyboard)
    // and Keycodes (which correspond to symbols on the keyboard, and might differ across layouts)
    if(e.type == SDL_KEYDOWN)
    {
        
        if(e.key.keysym.sym == SDLK_c)
        {
            colourIdx++;

            if (colourIdx == 0)
                glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // yellow
            else if (colourIdx == 1)
                glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // red
            else if (colourIdx == 2)
                glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f); // green
            else if (colourIdx == 3)
                glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f); // blue 
            else if (colourIdx == 4)
                glUniform3f(colorLoc, 0.0f, 1.0f, 1.0f); // turquoise
            else if (colourIdx == 5)            
            {
                glUniform3f(colorLoc, 1.0f, 0.0f, 1.0f); // purple
            }
            else if (colourIdx == 6)
            {
                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // white
                colourIdx = -1;
            }
            
            return false;
        }

        if(e.key.keysym.sym == SDLK_t)
        {
            transformationMode = 't';
            cout << "The transformation mode has been set to translation." << endl;         
                        
            return false;
        }

        if(e.key.keysym.sym == SDLK_r)
        {
            transformationMode = 'r';
            cout << "The transformation mode has been set to rotation." << endl;
            
            return false;
        }

        if(e.key.keysym.sym == SDLK_s)
        {
            transformationMode = 's';
            cout << "The transformation mode has been set to scaling." << endl;
        }

        if(e.key.keysym.sym == SDLK_x)
        {
            axis = 'x';
            cout << "The transformation axis has been set to the x-axis." << endl;
        }

        if(e.key.keysym.sym == SDLK_y)
        {
            axis = 'y';
            cout << "The transformation axis has been set to the y-axis." << endl;
        }

        if(e.key.keysym.sym == SDLK_z)
        {
            axis = 'z';
            cout << "The transformation axis has been set to the z-axis." << endl;
        }

        if(e.key.keysym.sym == SDLK_KP_PLUS || e.key.keysym.sym == SDLK_PLUS)
        {
            float changeMatrix[3] = {0.0f, 0.0f, 0.0f};
            
            if (transformationMode == 't')
            {
                if (axis == 'x')
                    changeMatrix[0] = 0.02f;
                else if (axis == 'y')
                    changeMatrix[1] = 0.02f;
                else if (axis == 'z')
                    changeMatrix[2] = 0.02f;
            
                modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(changeMatrix[0], changeMatrix[1], changeMatrix[2]));
                totalTransformations = modelMatrix * totalTransformations;
                MVP = projectionMatrix * viewMatrix * totalTransformations * model;
                GLuint MatrixID = glGetUniformLocation(shader, "MVP");
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            }
            else if (transformationMode == 'r')
            {
                if (axis == 'x')
                    changeMatrix[0] = 0.02f;
                else if (axis == 'y')
                    changeMatrix[1] = 0.02f;
                else if (axis == 'z')
                    changeMatrix[2] = 0.02f;

                glm::mat4 trans = glm::mat4(1.0f);
                trans = glm::rotate(trans, glm::radians(20.0f), glm::vec3(changeMatrix[0], changeMatrix[1], changeMatrix[2]));
                
                totalTransformations = trans * totalTransformations;
                MVP = projectionMatrix * viewMatrix * totalTransformations * model;
                GLuint MatrixID = glGetUniformLocation(shader, "MVP");
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            }
            else if (transformationMode == 's')
            {
                glm::mat4 trans = glm::mat4(1.0f);
                trans = glm::scale(trans, glm::vec3(1.02f, 1.02f, 1.02f));
                
                totalTransformations = trans * totalTransformations;
                MVP = projectionMatrix * viewMatrix * totalTransformations * model;
                GLuint MatrixID = glGetUniformLocation(shader, "MVP");
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            }
        }

        if(e.key.keysym.sym == SDLK_KP_MINUS || e.key.keysym.sym == SDLK_MINUS)
        {
            float changeMatrix[3] = {0.0f, 0.0f, 0.0f};
            
            if (transformationMode == 't')
            {
                if (axis == 'x')
                    changeMatrix[0] = -0.02f;
                else if (axis == 'y')
                    changeMatrix[1] = -0.02f;
                else if (axis == 'z')
                    changeMatrix[2] = -0.02f;
            
                modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(changeMatrix[0], changeMatrix[1], changeMatrix[2]));
                totalTransformations = modelMatrix * totalTransformations;
                MVP = projectionMatrix * viewMatrix * totalTransformations * model;
                GLuint MatrixID = glGetUniformLocation(shader, "MVP");
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            }
            else if (transformationMode == 'r')
            {
                if (axis == 'x')
                    changeMatrix[0] = -0.02f;
                else if (axis == 'y')
                    changeMatrix[1] = -0.02f;
                else if (axis == 'z')
                    changeMatrix[2] = -0.02f;

                glm::mat4 trans = glm::mat4(1.0f);
                trans = glm::rotate(trans, glm::radians(20.0f), glm::vec3(changeMatrix[0], changeMatrix[1], changeMatrix[2]));
                
                totalTransformations = trans * totalTransformations;
                MVP = projectionMatrix * viewMatrix * totalTransformations * model;
                GLuint MatrixID = glGetUniformLocation(shader, "MVP");
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            }
            else if (transformationMode == 's')
            {
                glm::mat4 trans = glm::mat4(1.0f);
                trans = glm::scale(trans, glm::vec3(0.98f, 0.98f, 0.98f));
                
                totalTransformations = trans * totalTransformations;
                MVP = projectionMatrix * viewMatrix * totalTransformations * model;
                GLuint MatrixID = glGetUniformLocation(shader, "MVP");
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            }
        }
        else if (e.key.keysym.sym == SDLK_BACKSPACE)
        {
            MVP = projectionMatrix * viewMatrix * model;
            totalTransformations = glm::mat4(1.0f);
            GLuint MatrixID = glGetUniformLocation(shader, "MVP");
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        }

    }
    return true;
}

void OpenGLWindow::cleanup()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vao);
    SDL_DestroyWindow(sdlWin);
}

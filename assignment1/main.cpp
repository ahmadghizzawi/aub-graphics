// _____________________________________________________
//|                                                   
//|  Ahmad Ghizzawi - Chukri Soueidi                    
//|  Assigntment One - CMPS 385   
//|____________________________________________________



#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#ifdef __MAC__
#   include <OpenGL/gl3.h>
#   define __gl_h_        // prevents glut.h from including the old GL header
#   include <GLUT/glut.h>
#else

#   include <GL/glew.h>
#   include <GL/glut.h>

#endif

#include "ppm.h"
#include "glsupport.h"
#include "lib/glm/glm.hpp"
#include "lib/glm/gtc/matrix_transform.hpp"
#include "lib/glm/gtc/type_ptr.hpp"


using namespace std;    // for string, vector, iostream, smart pointers, and other standard C++ stuff

static int g_width = 800;       // screen width
static int g_height = 800;       // screen height
static float g_initialWidth = 800.0;
static float g_initialHeight = g_initialWidth;

static bool g_leftClicked = false;     // is the left mouse button down?
static bool g_rightClicked = false;     // is the right mouse button down?
static int g_leftClickX, g_leftClickY;      // coordinates for mouse left click event
static int g_rightClickX, g_rightClickY;    // coordinates for mouse right click event

// the scale variable whose value will be copied to the shader variable
static float g_objScale = 1.0;       // scale factor for object

glm::mat4 trans;

// our global shader states
struct SquareShaderState {
    GlProgram program;

    GLfloat h_uVertexScale;

    GLint h_uTex0;
    GLint h_uTex1;

    GLfloat h_uXSpan;
    GLfloat h_uYSpan;

    // Handles to vertex attributes
    GLint h_aPosition;
    GLint h_aTexCoord;
};

// our global shader states
struct TriangleShaderState {
    GlProgram program;

    GLfloat h_vertexScale;

    GLint h_uTex0;
    GLint h_uTransformation;
    GLint h_uXSpan;
    GLint h_uYSpan;

    // Handles to vertex attributes
    GLint h_aPosition;
    GLint h_aTexCoord;
    GLint h_aColor;
};

static shared_ptr <SquareShaderState> g_squareShaderState;
static shared_ptr <TriangleShaderState> g_triangleShaderState;

// our global geometries
struct GeometryPX {
    GlArrayObject vao;
    GlBufferObject posVbo, texVbo, colorVbo;
};

static shared_ptr <GeometryPX> g_square, g_triangle;

static shared_ptr <GlTexture> g_sqTex0, g_sqTex1, g_triTex0
;

// _____________________________________________________
//|                                                     |
//|  Callbacks                                          |
//|_____________________________________________________|
///

static void drawSquare() {
    // using a VAO is necessary to run on OS X.
    glBindVertexArray(g_square->vao);

    // activate the glsl program
    glUseProgram(g_squareShaderState->program);

    /* Compute coefficients for maintaining aspect ratio */
    float scaleCoefficient = min(g_width / g_initialWidth, g_height / g_initialHeight);

    // Uniform vars here
    // set the value of the shader variable to the value of g_objScale
    safe_glUniform1f(g_squareShaderState->h_uVertexScale, g_objScale);
    safe_glUniform1f(g_squareShaderState->h_uXSpan, g_initialWidth / g_width * scaleCoefficient);
    safe_glUniform1f(g_squareShaderState->h_uYSpan, g_initialHeight / g_height * scaleCoefficient);

    // bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *g_sqTex0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, *g_sqTex1);

    // set glsl uniform variables
    safe_glUniform1i(g_squareShaderState->h_uTex0, 0); // 0 means GL_TEXTURE0
    safe_glUniform1i(g_squareShaderState->h_uTex1, 1); // 1 means GL_TEXTURE1

    // bind vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, g_square->posVbo);

    safe_glVertexAttribPointer(g_squareShaderState->h_aPosition,
                               2, // every two floats are a single vertex.
                               GL_FLOAT,
                               GL_FALSE,
                               0,
                               0);

    // and bind the texture vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, g_square->texVbo);
    safe_glVertexAttribPointer(g_squareShaderState->h_aTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // This is very important.
    safe_glEnableVertexAttribArray(g_squareShaderState->h_aPosition);
    safe_glEnableVertexAttribArray(g_squareShaderState->h_aTexCoord);

    // draw using 6 vertices, forming two triangles.
    // Executes the shader program. Tells the shader that these are vertices.
    glDrawArrays(GL_TRIANGLES, 0, 6);

    safe_glDisableVertexAttribArray(g_squareShaderState->h_aPosition);
    safe_glEnableVertexAttribArray(g_squareShaderState->h_aTexCoord);

    glBindVertexArray(0);

    // check for errors
    checkGlErrors();
}

static void drawTriangle() {
    // using a VAO is necessary to run on OS X.
    glBindVertexArray(g_triangle->vao);

    // activate the glsl program
    glUseProgram(g_triangleShaderState->program);

    // bind textures
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, *g_triTex0);

    /* Compute coefficients for maintaining aspect ratio */
    float scaleCoefficient = min(g_width / g_initialWidth, g_height / g_initialHeight);

    // Uniform vars here
    // set glsl uniform variables
    safe_glUniform1i(g_triangleShaderState->h_uTex0, 2); // 2 means GL_TEXTURE2
    glUniformMatrix4fv(g_triangleShaderState->h_uTransformation, 1, GL_FALSE, glm::value_ptr(trans));
    safe_glUniform1f(g_triangleShaderState->h_uXSpan, g_initialWidth / g_width * scaleCoefficient);
    safe_glUniform1f(g_triangleShaderState->h_uYSpan, g_initialHeight / g_height * scaleCoefficient);

    // bind vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, g_triangle->posVbo);
    safe_glVertexAttribPointer(g_triangleShaderState->h_aPosition,
                               2, // every two floats are a single vertex.
                               GL_FLOAT,
                               GL_FALSE,
                               0,
                               0);

    glBindBuffer(GL_ARRAY_BUFFER, g_triangle->texVbo);
    safe_glVertexAttribPointer(g_triangleShaderState->h_aTexCoord,
                               3,
                               GL_FLOAT,
                               GL_FALSE,
                               0,
                               0);
   // and bind the texture vertex buffer
   glBindBuffer(GL_ARRAY_BUFFER, g_triangle->colorVbo);
   safe_glVertexAttribPointer(g_triangleShaderState->h_aColor, 3, GL_FLOAT, GL_FALSE, 0, 0);


    // This is very important.
    safe_glEnableVertexAttribArray(g_triangleShaderState->h_aPosition);
    safe_glEnableVertexAttribArray(g_triangleShaderState->h_aTexCoord);
    safe_glEnableVertexAttribArray(g_triangleShaderState->h_aColor);

    // draw using 3 vertices, forming two triangles.
    // Executes the shader program. Tells the shader that these are vertices.
    glDrawArrays(GL_TRIANGLES, 0, 3);

    safe_glDisableVertexAttribArray(g_triangleShaderState->h_aPosition);
    safe_glDisableVertexAttribArray(g_triangleShaderState->h_aTexCoord);
    safe_glDisableVertexAttribArray(g_triangleShaderState->h_aColor);

    glBindVertexArray(0);

    // check for errors
    checkGlErrors();
}

// _____________________________________________________
//|                                                     |
//|  display                                            |
//|_____________________________________________________|
///
///  Whenever OpenGL requires a screen refresh
///  it will call display() to draw the scene.
///  We specify that this is the correct function
///  to call with the glutDisplayFunc() function
///  during initialization

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawSquare();
    drawTriangle();

    glutSwapBuffers();

    // check for errors
    checkGlErrors();
}


// _____________________________________________________
//|                                                     |
//|  reshape                                            |
//|_____________________________________________________|
///
///  Whenever a window is resized, a "resize" event is
///  generated and glut is told to call this reshape
///  callback function to handle it appropriately.

static void reshape(int w, int h) {
    g_width = w;
    g_height = h;

    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

// _____________________________________________________
//|                                                     |
//|  keyboard                                           |
//|_____________________________________________________|
///
///  Whenever a keyboard key is clicked, a "keyboad" event is
///  generated and glut is told to call this keyboard
///  callback function to handle it appropriately.

static void keyboard(unsigned char key, int x, int y) {

    glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f);
    switch (key) {
        case 'i':
            //move up
            trans = glm::translate(trans, glm::vec3(0.0f, 0.1f, 0.0f));
            break;
        case 'j':
            //move left
            trans = glm::translate(trans, glm::vec3(-0.1f, 0.0f, 0.0f));
            break;
        case 'k':
            //move down
            trans = glm::translate(trans, glm::vec3(0.0f, -0.1f, 0.0f));
            break;
        case 'l':
            //move right
            trans = glm::translate(trans, glm::vec3(0.1f, 0.0f, 0.0f));

            break;
        case 'h':
            cout << " ============== H E L P ==============\n"
                 << "h\t\thelp menu\n"
                 << "s\t\tsave screenshot\n"
                 << "\n\n"
                 << " ============ C O N T R O L S ============\n"
                 << "i\t\tmove up\n"
                 << "j\t\tmove left\n"
                 << "k\t\tmove right\n"
                 << "l\t\tmove down\n";
            break;
        case 'q':
            exit(0);
        case 's':
            glFinish();
            writePpmScreenshot(g_width, g_height, "out.ppm");
            break;
    }
    glutPostRedisplay();
}

// _____________________________________________________
//|                                                     |
//|  mouse                                           |
//|_____________________________________________________|
///
///  Whenever a mouse button is clicked, a "mouse" event
///  is generated and this mouse callback function is
///  called to handle the user input.

static void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      // right mouse button has been clicked
      g_leftClicked = true;
      g_leftClickX = x;
      g_leftClickY = g_height - y - 1;
    }
    else {
      // right mouse button has been released
      g_leftClicked = false;
    }
  }
  if (button == GLUT_RIGHT_BUTTON) {
    if (state == GLUT_DOWN) {
      // right mouse button has been clicked
      g_rightClicked = true;
      g_rightClickX = x;
      g_rightClickY = g_height - y - 1;
    }
    else {
      // right mouse button has been released
      g_rightClicked = false;
    }
  }
}

// _____________________________________________________
//|                                                     |
//|  motion                                             |
//|_____________________________________________________|
///
///  Whenever the mouse is moved while a button is pressed,
///  a "mouse move" event is triggered and this callback is
///  called to handle the event.


static void motion(int x, int y) {
  const int newx = x;
  const int newy = g_height - y - 1;
  if (g_leftClicked) {
    g_leftClickX = newx;
    g_leftClickY = newy;
  }
  if (g_rightClicked) {
    float deltax = (newx - g_rightClickX) * 0.02;
    g_objScale += deltax;

    g_rightClickX = newx;
    g_rightClickY = newy;
  }
  glutPostRedisplay();
}

// _____________________________________________________
//|                                                     |
//|  Helper Functions                                   |
//|_____________________________________________________|
///

static void initGlutState(int argc, char **argv) {
    glutInit(&argc, argv);
#ifdef __MAC__
    // core profile flag is required for GL 3.2 on Mac
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE|GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
#else
    //  RGBA pixel channels and double buffering
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowSize(g_width, g_height);      // create a window
    glutCreateWindow("Ahmad Ghizzawi | Chukri Soueidi: assignment 1");  // title the window

    glutDisplayFunc(display);                   // display rendering callback
    glutReshapeFunc(reshape);                   // window reshape callback
    glutKeyboardFunc(keyboard);                 // key press callback
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
}

static void initGLState() {
    glClearColor(0. / 255, 0. / 255, 0, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void loadSquareShader(SquareShaderState &ss) {
    const GLuint h = ss.program; // short hand

    readAndCompileShader(ss.program, "shaders/sq.vshader.glsl", "shaders/sq.fshader.glsl");

    // Retrieve handles to vertex attributes.
    // The first parameter is the handle.
    // The second parameter is the name of the variable in OpenGL.
    ss.h_aPosition = safe_glGetAttribLocation(h, "aPosition");

    // uniform variable: uVertexScale.
    ss.h_uVertexScale = safe_glGetUniformLocation(h, "uVertexScale");
    ss.h_uXSpan = safe_glGetUniformLocation(h, "uXSpan");
    ss.h_uYSpan = safe_glGetUniformLocation(h, "uYSpan");

    // retrieve handles to uniform variables
    ss.h_uTex0 = safe_glGetUniformLocation(h, "uTex0");
    ss.h_uTex1 = safe_glGetUniformLocation(h, "uTex1");

    // retrieve handles to vertex attributes
    ss.h_aTexCoord = safe_glGetAttribLocation(h, "aTexCoord");

    glBindFragDataLocation(h, 0, "fragColor");
    checkGlErrors();
}

static void loadTriangleShader(TriangleShaderState &ss) {
    const GLuint h = ss.program; // short hand

    readAndCompileShader(ss.program, "shaders/tri.vshader.glsl", "shaders/tri.fshader.glsl");

    // Retrieve handles to vertex attributes.
    // The first parameter is the handle.
    // The second parameter is the name of the variable in OpenGL.
    ss.h_aPosition = safe_glGetAttribLocation(h, "aPosition");
    // retrieve handles to vertex attributes
    ss.h_aTexCoord = safe_glGetAttribLocation(h, "aTexCoord");
    ss.h_aColor = safe_glGetAttribLocation(h, "aColor");

    // retrieve handles to uniform variables
    ss.h_uTex0 = safe_glGetUniformLocation(h, "uTex0");
    ss.h_uTransformation = safe_glGetUniformLocation(h, "uTransform");
    ss.h_uXSpan = safe_glGetUniformLocation(h, "uXSpan");
    ss.h_uYSpan = safe_glGetUniformLocation(h, "uYSpan");

    glBindFragDataLocation(h, 0, "fragColor");
    checkGlErrors();
}

static void initShaders() {
    g_squareShaderState.reset(new SquareShaderState);
    g_triangleShaderState.reset(new TriangleShaderState);

    loadSquareShader(*g_squareShaderState);
    loadTriangleShader(*g_triangleShaderState);
}

static void loadSquareGeometry(const GeometryPX &g) {
    // Square geometry. This is not a mesh.
    GLfloat pos[6 * 2] = {
            -.5, -.5,
            .5, .5,
            .5, -.5,
            -.5, -.5,
            -.5, .5,
            .5, .5
    };

    GLfloat tex[12] = {
      0, 0,
      1, 1,
      1, 0,
      0, 0,
      0, 1,
      1, 1
    };

    // bind to buffer before writing/reading.
    glBindBuffer(GL_ARRAY_BUFFER, g.posVbo);
    // send data to GPU.
    glBufferData(
            GL_ARRAY_BUFFER,
            12 * sizeof(GLfloat), // send 12*sizeof(GLfloat) from pos.
            pos,
            GL_STATIC_DRAW);
    checkGlErrors();

    glBindBuffer(GL_ARRAY_BUFFER, g.texVbo);
    glBufferData(
            GL_ARRAY_BUFFER,
            18 * sizeof(GLfloat), // send 12*sizeof(GLfloat) starting from col.
            tex,
            GL_STATIC_DRAW);
    checkGlErrors();

    checkGlErrors();
}

static void loadTriangleGeometry(const GeometryPX &g) {
    // Triangle geometry. This is not a mesh.
    GLfloat pos[3 * 2] = {
            .5, -.5,
            -.5, -.5,
            0, 0,
    };

    GLfloat tex[3 * 2] = {
            .5, -.5,
            -.5, -.5,
            0, 0,
    };

    GLfloat color[3 * 3] = {
            0.5, 1, 0,
            1, 0.5, 1,
            1, 0, 0,
    };

    // bind to buffer before writing/reading.
    glBindBuffer(GL_ARRAY_BUFFER, g.posVbo);
    // send data to GPU.
    glBufferData(
            GL_ARRAY_BUFFER,
            6 * sizeof(GLfloat), // send 12*sizeof(GLfloat) from pos.
            pos,
            GL_STATIC_DRAW);
    checkGlErrors();

    glBindBuffer(GL_ARRAY_BUFFER, g.texVbo);
    glBufferData(
            GL_ARRAY_BUFFER,
            6 * sizeof(GLfloat), // send 6*sizeof(GLfloat) starting from col.
            tex,
            GL_STATIC_DRAW);
    checkGlErrors();

    glBindBuffer(GL_ARRAY_BUFFER, g.colorVbo);
    glBufferData(
            GL_ARRAY_BUFFER,
            6 * sizeof(GLfloat), // send 6*sizeof(GLfloat) starting from col.
            color,
            GL_STATIC_DRAW);
    checkGlErrors();
}

// Initialize textures
static void loadTexture(GLuint texHandle, const char *ppmFilename) {
  int texWidth, texHeight;
  vector<PackedPixel> pixData;

  ppmRead(ppmFilename, texWidth, texHeight, pixData);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, texWidth, texHeight,
               0, GL_RGB, GL_UNSIGNED_BYTE, &pixData[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  checkGlErrors();
}

static void initGeometry() {
    g_square.reset(new GeometryPX());
    g_triangle.reset(new GeometryPX());
    loadSquareGeometry(*g_square);
    loadTriangleGeometry(*g_triangle);
}

static void initTextures() {
  g_sqTex0.reset(new GlTexture());
  g_sqTex1.reset(new GlTexture());
  g_triTex0.reset(new GlTexture());

  loadTexture(*g_sqTex0, "textures/tex0.pbm");
  loadTexture(*g_sqTex1, "textures/tex1.pbm");
  loadTexture(*g_triTex0, "textures/aub.pbm");
}

// _____________________________________________________
//|                                                     |
//|  main                                               |
//|_____________________________________________________|
///
///  The main entry-point for the application.

int main(int argc, char **argv) {
    try {
        initGlutState(argc, argv);

#ifndef __MAC__
        glewInit(); // load the OpenGL extensions
#endif

        initGLState();
        initShaders();
        initGeometry();
        initTextures();

        glutMainLoop();
        return 0;
    }
    catch (const runtime_error &e) {
        cout << "Exception caught: " << e.what() << endl;
        return -1;
    }
}

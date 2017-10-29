#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __MAC__
#include <OpenGL/gl3.h>
#define __gl_h_ // prevents glut.h from including the old GL header
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#include "headers/arcball.h"
#include "headers/cvec.h"
#include "headers/geometrymaker.h"
#include "headers/glsupport.h"
#include "headers/matrix4.h"
#include "headers/ppm.h"
#include "headers/quat.h"
#include "headers/rigtform.h"

using namespace std; // for string, vector, iostream, smart pointers, and other
                     // standard C++ stuff

// G L O B A L S ///////////////////////////////////////////////////

// To complete the assignment you need to edit the shader files that get loaded
// ----------------------------------------------------------------------------

static const float g_frustMinFov = 60.0; // A minimal of 60 degree field of view
static float g_frustFovY =
    g_frustMinFov; // FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -0.1;  // near plane
static const float g_frustFar = -50.0;  // far plane
static const float g_groundY = -2.0;    // y coordinate of the ground
static const float g_groundSize = 10.0; // half the ground length

static int g_windowWidth = 512;
static int g_windowHeight = 512;
static bool g_mouseClickDown = false; // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static bool g_spaceDown = false; // space state, for middle mouse emulation
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;

static int g_activeObject = 0;
static int g_activeEye = 2;
static bool g_worldSky = true;

static const int OBJECT0 = 0;
static const int OBJECT1 = 1;
static const int SKY = 2;

/*
For communication with shaders.
*/
struct ShaderState {
  GlProgram program;

  // Handles to uniform variables
  GLint h_uLight, h_uLight2;
  GLint h_uProjMatrix;
  GLint h_uModelViewMatrix;
  GLint h_uNormalMatrix;
  GLint h_uColor;

  // Handles to vertex attributes
  GLint h_aPosition;
  GLint h_aNormal;

  ShaderState(const char *vsfn, const char *fsfn) {
    readAndCompileShader(program, vsfn, fsfn);

    const GLuint h = program; // short hand

    // Retrieve handles to uniform variables
    h_uLight = safe_glGetUniformLocation(h, "uLight");
    h_uLight2 = safe_glGetUniformLocation(h, "uLight2");
    h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
    h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
    h_uNormalMatrix = safe_glGetUniformLocation(h, "uNormalMatrix");
    h_uColor = safe_glGetUniformLocation(h, "uColor");

    // Retrieve handles to vertex attributes
    h_aPosition = safe_glGetAttribLocation(h, "aPosition");
    h_aNormal = safe_glGetAttribLocation(h, "aNormal");

    glBindFragDataLocation(h, 0, "fragColor");
    checkGlErrors();
  }
};

static const int g_numShaders = 2;
static const char *const g_shaderFiles[g_numShaders][2] = {
    {"./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader"},
    {"./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader"}};

// our global shader states
static vector<shared_ptr<ShaderState> > g_shaderStates;

// --------- Geometry

// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) &(((StructType *)0)->field)

// A vertex with floating point position and normal
struct VertexPN {
  Cvec3f p, n;

  VertexPN() {}
  VertexPN(float x, float y, float z, float nx, float ny, float nz)
      : p(x, y, z), n(nx, ny, nz) {}

  // Define copy constructor and assignment operator from GenericVertex so we
  // can use make* functions from geometrymaker.h
  VertexPN(const GenericVertex &v) { *this = v; }

  VertexPN &operator=(const GenericVertex &v) {
    p = v.pos;
    n = v.normal;
    return *this;
  }
};

struct Geometry {
  GlBufferObject vbo, ibo;
  GlArrayObject vao;
  int vboLen, iboLen;

  Geometry(VertexPN *vtx, unsigned short *idx, int vboLen, int iboLen) {
    this->vboLen = vboLen;
    this->iboLen = iboLen;

    // Now create the VBO and IBO (Index buffer object)
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPN) * vboLen, vtx,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboLen, idx,
                 GL_STATIC_DRAW);
  }

  void draw(const ShaderState &curSS) {
    // bind the object's VAO
    glBindVertexArray(vao);

    // Enable the attributes used by our shader
    safe_glEnableVertexAttribArray(curSS.h_aPosition);
    safe_glEnableVertexAttribArray(curSS.h_aNormal);

    // bind vbo
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    safe_glVertexAttribPointer(curSS.h_aPosition, 3, GL_FLOAT, GL_FALSE,
                               sizeof(VertexPN), FIELD_OFFSET(VertexPN, p));
    safe_glVertexAttribPointer(curSS.h_aNormal, 3, GL_FLOAT, GL_FALSE,
                               sizeof(VertexPN), FIELD_OFFSET(VertexPN, n));

    // bind ibo
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    // draw!
    glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);

    // Disable the attributes used by our shader
    safe_glDisableVertexAttribArray(curSS.h_aPosition);
    safe_glDisableVertexAttribArray(curSS.h_aNormal);

    // disable VAO
    glBindVertexArray(0);
  }
};

// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_cube_1;

// --------- Scene

static const Cvec3 g_light1(2.0, 3.0, 14.0),
    g_light2(-2, -3.0, -5.0); // define two lights positions in world space

/* rigid body translation*/
// CHANGE
// Replaced Matrix4 by RigTForm
static RigTForm g_skyRbt(Cvec3(0.0, 0.25, 4.0));

// CHANGE
// Replaced Matrix4 by RigTForm
static RigTForm g_worldRbt(Cvec3(0.0, 0.0, 0.0));

// CHANGE
// Replaced Matrix4 by RigTForm
// 2 cubes are defined.
static RigTForm g_objectRbt[2] = {RigTForm(Cvec3(-0.75, 0, 0)),
                                  RigTForm(Cvec3(0.75, 0, 0))};

static Cvec3f g_objectColors[2] = {Cvec3f(1, 0, 0), Cvec3f(0, 1, 0)};

///////////////// END OF G L O B A L S
/////////////////////////////////////////////////////

static void initGround() {
  // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
  VertexPN vtx[4] = {
      VertexPN(-g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
      VertexPN(-g_groundSize, g_groundY, g_groundSize, 0, 1, 0),
      VertexPN(g_groundSize, g_groundY, g_groundSize, 0, 1, 0),
      VertexPN(g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
  };
  unsigned short idx[] = {0, 1, 2, 0, 2, 3};
  g_ground.reset(new Geometry(&vtx[0], &idx[0], 4, 6));
}

static void initCubes() {
  int ibLen, vbLen;
  getCubeVbIbLen(vbLen, ibLen);

  // Temporary storage for cube geometry
  vector<VertexPN> vtx(vbLen);
  vector<unsigned short> idx(ibLen);

  makeCube(1, vtx.begin(), idx.begin());
  g_cube.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));

  // TODO: Add another cube.
  makeCube(1, vtx.begin(), idx.begin());
  g_cube_1.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));
}

// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(const ShaderState &curSS,
                                 const Matrix4 &projMatrix) {
  GLfloat glmatrix[16];
  projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
  safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}

// takes MVM and its normal matrix to the shaders (MVM = from local frame to eye
// frame)
static void sendModelViewNormalMatrix(const ShaderState &curSS,
                                      const Matrix4 &MVM, const Matrix4 &NMVM) {
  GLfloat glmatrix[16];
  MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
  safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

  NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
  safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);
}

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
  if (g_windowWidth >= g_windowHeight)
    g_frustFovY = g_frustMinFov;
  else {
    const double RAD_PER_DEG = 0.5 * CS175_PI / 180;
    g_frustFovY =
        atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight / g_windowWidth,
              cos(g_frustMinFov * RAD_PER_DEG)) /
        RAD_PER_DEG;
  }
}

// CHANGE
// Replaced Matrix4 by RigTForm
// Does a RBT Q to object O with respect to A
// Returns A * Q * A-1 * O
static RigTForm doQtoOwrtA(const RigTForm &Q, const RigTForm &O,
                           const RigTForm &A) {
  return A * Q * inv(A) * O;
}

// CHANGE
// Replaced Matrix4 by RigTForm
// Factorizes O and E and returns a mixed frame composed of trans(O) * lin(E)
// Returns a mixed frame composed of trans(O) * lin(E)
static RigTForm makeMixedFrame(const RigTForm &O, const RigTForm &E) {
  return transFact(O) * linFact(E);
}

static Matrix4 makeProjectionMatrix() {
  return Matrix4::makeProjection(
      g_frustFovY, g_windowWidth / static_cast<double>(g_windowHeight),
      g_frustNear, g_frustFar);
}

// CHANGE
// Replaced Matrix4 by RigTForm
// Returns the current eyeRbt based on the active view.
static RigTForm getEyeRbt() {
  switch (g_activeEye) {
  case OBJECT0:
    return g_objectRbt[0];
  case OBJECT1:
    return g_objectRbt[1];
  case SKY:
  default:
    return g_skyRbt;
  }
}

static void drawStuff() {
  // short hand for current shader state
  const ShaderState &curSS = *g_shaderStates[g_activeShader];

  // build & send proj. matrix to vshader
  const Matrix4 projmat = makeProjectionMatrix();
  sendProjectionMatrix(curSS, projmat);

  // Select eyeRbt based on the currently active view and then invert it.
  const RigTForm invEyeRbt = inv(getEyeRbt());

  // g_light1 position in eye coordinates
  const Cvec3 eyeLight1 = Cvec3(invEyeRbt * Cvec4(g_light1, 1));

  // g_light2 position in eye coordinates
  const Cvec3 eyeLight2 = Cvec3(invEyeRbt * Cvec4(g_light2, 1));

  // send light positions to shader.
  safe_glUniform3f(curSS.h_uLight, eyeLight1[0], eyeLight1[1], eyeLight1[2]);
  safe_glUniform3f(curSS.h_uLight2, eyeLight2[0], eyeLight2[1], eyeLight2[2]);

  // draw ground
  // ===========
  //
  const RigTForm groundRbt = RigTForm(); // identity
  Matrix4 MVM = rigTFormToMatrix(invEyeRbt * groundRbt);
  Matrix4 NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, 0.1, 0.95, 0.1); // set color
  g_ground->draw(curSS);

  // draw cubes
  // ==========
  //
  // Draw CUBE 1
  MVM = rigTFormToMatrix(invEyeRbt * g_objectRbt[0]);
  NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, g_objectColors[0][0], g_objectColors[0][1],
                   g_objectColors[0][2]);
  g_cube->draw(curSS);

  // Draw CUBE 2
  MVM = rigTFormToMatrix(invEyeRbt * g_objectRbt[1]);
  NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, g_objectColors[1][0], g_objectColors[1][1],
                   g_objectColors[1][2]);

  g_cube_1->draw(curSS);
}



// CHANGE
// Replaced Matrix4 by RigTForm
// Manipulates objects' RBTs based on the active objects and eye.
static void manipulateObjects(RigTForm &Q) {
  /* code */
  // Active eye is sky and active object is a cube.
  if ((g_activeObject < 2) && g_activeEye == SKY) {
    RigTForm A = makeMixedFrame(g_objectRbt[g_activeObject], g_skyRbt);
    g_objectRbt[g_activeObject] = doQtoOwrtA(Q, g_objectRbt[g_activeObject], A);
  }
  // Active eye and object are cubes.
  else if (g_activeObject < 2 && g_activeEye < 2) {
    // TODO: should be for i and j
    RigTForm A =
        makeMixedFrame(g_objectRbt[g_activeObject], g_objectRbt[g_activeEye]);
    g_objectRbt[g_activeObject] = doQtoOwrtA(Q, g_objectRbt[g_activeObject], A);
  }
  // Active eye and object is sky.
  else {
    RigTForm world;
    // World-sky frame
    if (g_worldSky) {
      RigTForm A = makeMixedFrame(g_worldRbt, g_skyRbt);
      g_skyRbt = doQtoOwrtA(Q, g_skyRbt, A);
    }
    // Sky-sky
    else {
      g_skyRbt = g_skyRbt * Q;
    }
  }
}

static void display() {
  glUseProgram(g_shaderStates[g_activeShader]->program);
  glClear(GL_COLOR_BUFFER_BIT |
          GL_DEPTH_BUFFER_BIT); // clear framebuffer color&depth

  drawStuff();

  glutSwapBuffers(); // show the back buffer (where we rendered stuff)

  checkGlErrors();
}

static void reshape(const int w, const int h) {
  g_windowWidth = w;
  g_windowHeight = h;
  glViewport(0, 0, w, h);
  cerr << "Size of window is now " << w << "x" << h << endl;
  updateFrustFovY();
  glutPostRedisplay();
}

static void motion(const int x, const int y) {
  double dx = x - g_mouseClickX;
  double dy = g_windowHeight - y - 1 - g_mouseClickY;

  // variables used to invert the required values.
  // Applied to rotation and trans.
  int factor = 1;

  // Applied to rotation only.
  int factor2 = -1;

  if ((g_activeObject < 2) && (g_activeObject != g_activeEye)) {
    factor2 = 1;
  } else if (g_activeEye == SKY && g_activeObject == SKY && g_worldSky) {
    factor = -1;
    factor2 = 1;
  }

  RigTForm m;
  // left button down?
  if (g_mouseLClickButton && !g_mouseRClickButton && !g_spaceDown) {
    m = RigTForm(Quat::makeXRotation(factor2 * factor * -dy) *
        Quat::makeYRotation(factor2 * factor * dx));

  }
  // right button down?
  else if (g_mouseRClickButton && !g_mouseLClickButton) {
    m = RigTForm(Cvec3(factor * dx, factor * dy, 0) * 0.01);
  }
  // middle or (left and right, or left + space)
  // button down?
  else if (g_mouseMClickButton ||
             (g_mouseLClickButton && g_mouseRClickButton) ||
             (g_mouseLClickButton && !g_mouseRClickButton &&
              g_spaceDown)) {
    m = RigTForm(Cvec3(0, 0, factor * -dy) * 0.01);
  }

  // we always redraw if we changed the scene
  if (g_mouseClickDown) {
    manipulateObjects(m);
    glutPostRedisplay();
  }

  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1;
}

static void printActiveView(int activeView) {
  switch (activeView) {
  case SKY:
    std::cout << "Sky camera frame is active." << '\n';
    break;
  case OBJECT0:
    std::cout << "Object 0 frame is active." << '\n';
    break;
  case OBJECT1:
    std::cout << "Object 1 frame is active." << '\n';
    break;
  }
}

static void printActiveObject(int activeObject) {
  switch (activeObject) {
  case OBJECT0:
    cout << "Object 0 is active." << '\n';
    break;
  case OBJECT1:
    cout << "Object 1 is active." << '\n';
    break;
  case SKY:
    cout << "Sky is now active." << '\n';
    break;
  default:
    cout << "Object 0 is active." << '\n';
  }
}



static void mouse(const int button, const int state, const int x, const int y) {
  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1; // conversion from GLUT
                                          // window-coordinate-system to OpenGL
                                          // window-coordinate-system

  g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
  g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
  g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

  g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
  g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
  g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

  g_mouseClickDown =
      g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;
}

static void keyboardUp(const unsigned char key, const int x, const int y) {
  switch (key) {
  case ' ':
    g_spaceDown = false;
    break;
  }
  glutPostRedisplay();
}

static void keyboard(const unsigned char key, const int x, const int y) {
  switch (key) {
  case 27:
    exit(0); // ESC
  case 'h':
    cout << " ============== H E L P ==============\n\n"
         << "h\t\thelp menu\n"
         << "s\t\tsave screenshot\n"
         << "f\t\tToggle flat shading on/off.\n"
         << "o\t\tCycle object to edit\n"
         << "v\t\tCycle view\n"
         << "drag left mouse to rotate\n"
         << endl;
    break;
  case 's':
    glFlush();
    writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
    break;
  case 'f':
    g_activeShader ^= 1;
    break;
  case 'o':
    g_activeObject++;
    if (g_activeObject > 2) {
      g_activeObject = 0;
    }
    printActiveObject(g_activeObject);
    break;
  case 'v':
    g_activeEye++;
    if (g_activeEye > 2) {
      g_activeEye = 0;
    }
    printActiveView(g_activeEye);
    break;
  case 'm':
    if (g_activeEye == SKY && g_activeObject == SKY) {
      g_worldSky = !g_worldSky;
      if (g_worldSky) {
        cout << "World-sky view is active." << '\n';
      } else {
        cout << "Sky-sky view is active." << '\n';
      }
    }
    break;
  case ' ':
    g_spaceDown = true;
    break;
  }
  glutPostRedisplay();
}

static void initGlutState(int argc, char *argv[]) {
  glutInit(&argc, argv); // initialize Glut based on cmd-line args
#ifdef __MAC__
  glutInitDisplayMode(
      GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE |
      GLUT_DEPTH); // core profile flag is required for GL 3.2 on Mac
#else
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE |
                      GLUT_DEPTH); //  RGBA pixel channels and double buffering
#endif
  glutInitWindowSize(g_windowWidth, g_windowHeight); // create a window
  glutCreateWindow("Assignment 2");                  // title the window

  glutIgnoreKeyRepeat(true); // avoids repeated keyboard calls when holding
                             // space to emulate middle mouse

  glutDisplayFunc(display); // display rendering callback
  glutReshapeFunc(reshape); // window reshape callback
  glutMotionFunc(motion);   // mouse movement callback
  glutMouseFunc(mouse);     // mouse click callback
  glutKeyboardFunc(keyboard);
  glutKeyboardUpFunc(keyboardUp);
}

static void initGLState() {
  glClearColor(128. / 255., 200. / 255., 255. / 255., 0.);
  glClearDepth(0.);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glReadBuffer(GL_BACK);
  glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initShaders() {
  g_shaderStates.resize(g_numShaders);
  for (int i = 0; i < g_numShaders; ++i)
    g_shaderStates[i].reset(
        new ShaderState(g_shaderFiles[i][0], g_shaderFiles[i][1]));
}

static void initGeometry() {
  initGround();
  initCubes();
}

int main(int argc, char *argv[]) {
  try {
    initGlutState(argc, argv);

    // on Mac, we shouldn't use GLEW.
#ifndef __MAC__
    glewInit(); // load the OpenGL extensions
#endif

    initGLState();
    initShaders();
    initGeometry();

    glutMainLoop();
    return 0;
  } catch (const runtime_error &e) {
    cout << "Exception caught: " << e.what() << endl;
    return -1;
  }
}

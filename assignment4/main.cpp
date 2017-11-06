// _____________________________________________________
//|                                                     |
//|  Ahmad Ghizzawi - Chukri Soueidi                    |
//|  Assigntment IV - CMPS 385                         |
//|____________________________________________________ |

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
#include "headers/scenegraph.h"
#include "headers/asstcommon.h"
#include "headers/drawer.h"
#include "headers/picker.h"

using namespace std; // for string, vector, iostream, smart pointers, and other
                     // standard C++ stuff

// _____________________________________________________
//|                                                     |
//|  GLOBALS                                            |
//|_____________________________________________________|
///
// A minimal of 60 degree field of view
static const float g_frustMinFov = 60.0;
// FOV in y direction (updated by updateFrustFovY)
static float g_frustFovY = g_frustMinFov;
// near plane
static const float g_frustNear = -0.1;
// far plane
static const float g_frustFar = -50.0;
// y coordinate of the ground
static const float g_groundY = -2.0;
// half the ground length
static const float g_groundSize = 10.0;

static int g_windowWidth = 512;
static int g_windowHeight = 512;
static bool g_mouseClickDown = false; // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static bool g_spaceDown = false; // space state, for middle mouse emulation
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;

// Constants that act as object ids
static const int OBJECT0 = 0;
static const int OBJECT1 = 1;
static const int SKY = 2;

// Default view and object.
static int g_activeEye = SKY;
static bool g_worldSkyFrame = true;

static double g_arcballScreenRadius = 1.0;
static double g_arcballScale = 1.0;

static bool g_pickObject = false;

static const int PICKING_SHADER = 2; // index of the picking shader is g_shaderFiles
static const int g_numShaders = 3; // 3 shaders instead of 2

static const char *const g_shaderFiles[g_numShaders][2] = {
    {"./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader"},
    {"./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader"},
    {"./shaders/basic-gl3.vshader", "./shaders/pick-gl3.fshader"}
  };

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

typedef SgGeometryShapeNode<Geometry> MyShapeNode;

// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_cube_1, g_arcball;

// --------- Scene

// define two lights positions in world space
static const Cvec3 g_light1(2.0, 3.0, 14.0), g_light2(-2, -3.0, -5.0);

static shared_ptr<SgRootNode> g_world;
static shared_ptr<SgRbtNode> g_skyNode, g_groundNode, g_robot1Node, g_robot2Node;
// current picked object. Default is sky
static shared_ptr<SgRbtNode> g_currentPickedRbtNode;
// current view. Default is sky
static shared_ptr<SgRbtNode> g_currentView;

/* rigid body translation*/

static RigTForm g_arcballRbt(Cvec3(0, 0, 0));
static Cvec3f g_arcballColor(0.5, 0.5, 0.5);

static Cvec3f g_objectColors[2] = {Cvec3f(1, 0, 0), Cvec3f(0, 1, 0)};

// _____________________________________________________
//|                                                     |
//|  END OF GLOBALS                                     |
//|_____________________________________________________|
///

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

  makeCube(1, vtx.begin(), idx.begin());
  g_cube_1.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));
}

// initialize sphere geometry to draw arcball.
static void initArcball() {
  int ibLen, vbLen;
  int slices = 25;
  int stacks = 25;
  getSphereVbIbLen(slices, stacks, vbLen, ibLen);

  // Temporary storage for cube geometry
  vector<VertexPN> vtx(vbLen);
  vector<unsigned short> idx(ibLen);


  makeSphere(1, slices, stacks, vtx.begin(), idx.begin());
  g_arcball.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));
}

// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(const ShaderState &curSS,
                                 const Matrix4 &projMatrix) {
  GLfloat glmatrix[16];
  projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
  safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
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

static Matrix4 makeProjectionMatrix() {
  return Matrix4::makeProjection(
      g_frustFovY, g_windowWidth / static_cast<double>(g_windowHeight),
      g_frustNear, g_frustFar);
}

// Does a RBT Q to object O with respect to A
// Returns A * Q * A-1 * O
static RigTForm doQtoOwrtA(const RigTForm &Q, const RigTForm &O,
                           const RigTForm &A) {
  return A * Q * inv(A) * O;
}

// Factorizes O and E and returns a mixed frame composed of trans(O) * lin(E)
// Returns a mixed frame composed of trans(O) * lin(E)
static RigTForm makeMixedFrame(const RigTForm &O, const RigTForm &E) {
  return transFact(O) * linFact(E);
}

// Returns the current eyeRbt based on the active view.
static RigTForm getEyeRbt() {
  switch (g_activeEye) {
  case OBJECT0:
    return getPathAccumRbt(g_world, g_robot1Node);
  case OBJECT1:
    return getPathAccumRbt(g_world, g_robot2Node);
  case SKY:
  default:
    return getPathAccumRbt(g_world, g_skyNode);
  }
}

// Returns true if active object and eye are the SKY camera wrt world-sky frame.
static bool isWorldSkyFrameActive() {
  return (g_currentPickedRbtNode == g_skyNode && g_activeEye == SKY && g_worldSkyFrame);
}

// Returns true if active object is a cube and isn't wrt to itself.
static bool isCubeActive() {
  return g_currentPickedRbtNode != g_skyNode && g_currentPickedRbtNode != g_currentView;
}

// Arcball interface will come into place under two conditions only:
// 1. Active object and eye are the SKY camera wrt world-sky frame.
// 2. Active object is a cube and isn't wrt to itself.
// Returns true if arcball should be active, false otherwise.
static bool isArcballActive() {
  return isWorldSkyFrameActive() || isCubeActive();
}

// Arcball Rbt will be based on the current case:
// 1. Center is the world's origin
// 2. Center is the cube being manipulated.
// Returns the RBT of arcball.
static RigTForm getArcballRbt() {
  // Active object is a cube and isn't wrt to itself.
  if (isCubeActive()) {
    return getPathAccumRbt(g_world, g_currentPickedRbtNode);
  }
  // Active object and eye are the SKY camera wrt world-sky frame.
  return g_world->getRbt();
}

// Returns ArcballRotation.
static RigTForm getArcballRotation(int current_x, int current_y) {
  const RigTForm eyeRbt = getEyeRbt();
  const RigTForm object = getArcballRbt();

  // Get the sphere's center in Screen coordinates
  Cvec2 screenSpaceCoordinates = getScreenSpaceCoord(
    (inv(eyeRbt) * object).getTranslation(),
    makeProjectionMatrix(),
    g_frustNear,
    g_frustFovY,
    g_windowWidth,
    g_windowHeight
  );

  Cvec3 sphereCenter = Cvec3(screenSpaceCoordinates, 0);

  // point1[0] = x - cx; point1[1] = y - cy;
  Cvec3 point1 = Cvec3(g_mouseClickX, g_mouseClickY, 0) - sphereCenter;

  // point2[0] = x - cx; point2[1] = y - cy;
  Cvec3 point2 = Cvec3(current_x, current_y, 0) - sphereCenter;

  // z = sqrt((radius)^2 - (x - cx)^2 - (y - cy)^2)
  Cvec3 vector1 = normalize(Cvec3(point1[0], point1[1], sqrt(max(0.0, pow(g_arcballScreenRadius, 2) - pow(point1[0], 2) - pow(point1[1], 2)))));

  // z = sqrt((radius)^2 - (x - cx)^2 - (y - cy)^2)
  Cvec3 vector2 = normalize(Cvec3(point2[0], point2[1], sqrt(max(0.0, pow(g_arcballScreenRadius, 2) - pow(point2[0], 2) - pow(point2[1], 2)))));


  if (isWorldSkyFrameActive()) {
    return RigTForm(Quat(0, vector1 * -1.0) * Quat(0, vector2));
  }
  else {
    return RigTForm(Quat(0, vector2) * Quat(0, vector1 * -1.0));
  }
}

static void drawArcball(const ShaderState& curSS, const RigTForm& invEyeRbt) {
  // draw wireframe
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  const double scalingValue = g_arcballScale * g_arcballScreenRadius;
  const Matrix4 scale = Matrix4::makeScale(Cvec3(scalingValue, scalingValue, scalingValue));
  Matrix4 MVM = rigTFormToMatrix(invEyeRbt * getArcballRbt()) * scale;
  Matrix4 NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, g_arcballColor[0], g_arcballColor[1],
                   g_arcballColor[2]);
  g_arcball->draw(curSS);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void drawStuff(const ShaderState& curSS, bool picking) {
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



  // draw cubes, ground, and arball
  // ==========
  //

  if (!picking) {
      Drawer drawer(invEyeRbt, curSS);
      g_world->accept(drawer);



  }
  else {
    Picker picker(invEyeRbt, curSS);
    g_world->accept(picker);
    glFlush();
    g_currentPickedRbtNode = picker.getRbtNodeAtXY(g_mouseClickX, g_mouseClickY);
    if (g_currentPickedRbtNode == g_groundNode)
      g_currentPickedRbtNode = g_skyNode;
  }

  // draw arcball
  // ==========
  //
  if (isArcballActive()) {
      if (!g_mouseMClickButton && !(g_mouseLClickButton && g_mouseRClickButton) && !(g_spaceDown)) {
        g_arcballScale = getScreenToEyeScale(
             (inv(getEyeRbt()) * getArcballRbt()).getTranslation()[2],
             g_frustFovY,
             g_windowHeight
          );
      }

      drawArcball(curSS, invEyeRbt);
  }
}

static void pick() {
  // We need to set the clear color to black, for pick rendering.
  // so let's save the clear color
  GLdouble clearColor[4];
  glGetDoublev(GL_COLOR_CLEAR_VALUE, clearColor);

  glClearColor(0, 0, 0, 0);

  // using PICKING_SHADER as the shader
  glUseProgram(g_shaderStates[PICKING_SHADER]->program);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawStuff(*g_shaderStates[PICKING_SHADER], true);

  // Uncomment below and comment out the glutPostRedisplay in mouse(...) call back
  // to see result of the pick rendering pass
  // glutSwapBuffers();

  //Now set back the clear color
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

  checkGlErrors();
}

// Manipulates objects' RBTs based on the active objects and eye.
static void manipulateObjects(RigTForm &Q) {

  // Active eye is sky and active object is a cube.
  if (isCubeActive() && g_activeEye == SKY) {
    RigTForm A = makeMixedFrame(g_currentPickedRbtNode->getRbt(), g_currentView->getRbt());
    g_currentPickedRbtNode->setRbt(doQtoOwrtA(Q, g_currentPickedRbtNode->getRbt(), A));
  }
  // Active eye and object are cubes.
  else if (isCubeActive()) {
    RigTForm A =
        makeMixedFrame(g_currentPickedRbtNode->getRbt(), g_currentView->getRbt());
    g_currentPickedRbtNode->setRbt(doQtoOwrtA(Q, g_currentPickedRbtNode->getRbt(), A));
  }
  // Active eye and object is sky.
  else {
    RigTForm world;
    // World-sky frame
    if (g_worldSkyFrame) {
      RigTForm A = makeMixedFrame(g_world->getRbt(), g_skyNode->getRbt());
      g_skyNode->setRbt(doQtoOwrtA(Q, g_skyNode->getRbt(), A));
    }
    // Sky-sky
    else {
      g_skyNode->setRbt(g_skyNode->getRbt() * Q);
    }
  }
}

static void toggleActiveView() {
  g_activeEye++;
  if (g_activeEye > 2) {
    g_activeEye = 0;
  }
  switch (g_activeEye) {
  case SKY:
    std::cout << "Sky camera view is active." << '\n';
    g_currentView = g_skyNode;
    break;
  case OBJECT0:
    std::cout << "Robot 1 view is active." << '\n';
    g_currentView = g_robot1Node;
    break;
  case OBJECT1:
    std::cout << "Robot 2 view is active." << '\n';
    g_currentView = g_robot2Node;
    break;
  }
}

// _____________________________________________________
//|                                                     |
//|  GLUT CALLBACKS                                     |
//|_____________________________________________________|
///

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
static void display() {
  glUseProgram(g_shaderStates[g_activeShader]->program);
  glClear(GL_COLOR_BUFFER_BIT |
          GL_DEPTH_BUFFER_BIT); // clear framebuffer color&depth

  drawStuff(*g_shaderStates[g_activeShader], false);

  glutSwapBuffers(); // show the back buffer (where we rendered stuff)

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
static void reshape(const int w, const int h) {
  g_windowWidth = w;
  g_windowHeight = h;
  glViewport(0, 0, w, h);
  cerr << "Size of window is now " << w << "x" << h << endl;

  g_arcballScreenRadius = 0.25 * min(g_windowWidth, g_windowHeight);

  updateFrustFovY();
  glutPostRedisplay();
}

// _____________________________________________________
//|                                                     |
//|  motion                                             |
//|_____________________________________________________|
///
///  Whenever the mouse is moved while a button is pressed,
///  a "mouse move" event is triggered and this callback is
///  called to handle the event.
static void motion(const int x, const int y) {
  int current_x = x;
  int current_y = g_windowHeight - y - 1;

  double dx = current_x - g_mouseClickX;
  double dy = current_y - g_mouseClickY;

  // variables used to invert the required values.
  // Applied to rotation and trans.
  int factor = 1;

  // Applied to rotation only.
  int factor2 = -1;

  if (isCubeActive()) {
    factor2 = 1;
  } else if (isWorldSkyFrameActive()) {
    factor = -1;
    factor2 = 1;
  }

  // Use g_arcballScale to scale translation when using arcball only.
 double translateFactor;
 if (isArcballActive()) {
   translateFactor = g_arcballScale;
 } else {
   translateFactor = 0.01;
 }

  RigTForm m;
  // left button down?
  if (g_mouseLClickButton && !g_mouseRClickButton && !g_spaceDown) {
    if (isArcballActive()) {
      m = getArcballRotation(current_x, current_y);
    }
    else {
      m = RigTForm(Quat::makeXRotation(factor2 * factor * -dy) *
          Quat::makeYRotation(factor2 * factor * dx));
    }
  }
  // right button down?
  else if (g_mouseRClickButton && !g_mouseLClickButton) {
    m = RigTForm(Cvec3(factor * dx, factor * dy, 0) * translateFactor);
  }
  // middle or (left and right, or left + space)
  // button down?
  else if (g_mouseMClickButton ||
             (g_mouseLClickButton && g_mouseRClickButton) ||
             (g_mouseLClickButton && !g_mouseRClickButton &&
              g_spaceDown)) {
    m = RigTForm(Cvec3(0, 0, factor * -dy) * translateFactor);
  }


  // we always redraw if we changed the scene
  if (g_mouseClickDown) {
    manipulateObjects(m);

    glutPostRedisplay();
  }

  g_mouseClickX = current_x;
  g_mouseClickY = current_y;
}

// _____________________________________________________
//|                                                     |
//|  mouse                                              |
//|_____________________________________________________|
///
///  Whenever a mouse button is clicked, a "mouse" event
///  is generated and this mouse callback function is
///  called to handle the user input.
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

  // left button down?
  if (g_mouseLClickButton && !g_mouseRClickButton && !g_spaceDown && g_pickObject) {
    pick();
    g_pickObject = false;
  }

  // Call glutPostRedisplay()
  glutPostRedisplay();
}

static void keyboardUp(const unsigned char key, const int x, const int y) {
  switch (key) {
  case ' ':
    g_spaceDown = false;
    break;
  }
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
  case 'v':
    toggleActiveView();
    break;
  case 'm':
    if (g_activeEye == SKY && g_currentView == g_skyNode) {
      g_worldSkyFrame = !g_worldSkyFrame;
      if (isWorldSkyFrameActive()) {
        cout << "World-sky frame is active." << '\n';
      } else {
        cout << "Sky-sky frame is active." << '\n';
      }
    }
    break;
  case 'p':
    cout << "Picking mode is active." << '\n';
    g_pickObject = true;
    break;
  case ' ':
    g_spaceDown = true;
    break;
  }
  glutPostRedisplay();
}

// _____________________________________________________
//|                                                     |
//|  Helper Functions                                   |
//|_____________________________________________________|
///
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
  glutCreateWindow("Assignment 4");                  // title the window

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
  initArcball();
}

static void constructRobot(shared_ptr<SgTransformNode> base, const Cvec3& color) {

  const double ARM_LEN = 0.7,
               ARM_THICK = 0.25,
               TORSO_LEN = 1.5,
               TORSO_THICK = 0.25,
               TORSO_WIDTH = 1,
               HEAD_RAD = 0.30,
               LEG_LEN = 0.7,
               LEG_THICK = 0.25;
  const int NUM_JOINTS = 10,
            NUM_SHAPES = 10;

  struct JointDesc {
    int parent;
    float x, y, z;
  };

  JointDesc jointDesc[NUM_JOINTS] = {
    {-1}, // torso
    {0,  TORSO_WIDTH/2, TORSO_LEN/2, 0}, // upper right arm
    {1,  ARM_LEN, 0, 0}, // lower right arm
    {0, 0, TORSO_LEN/2, 0}, // head
    {0,  -TORSO_WIDTH/2, TORSO_LEN/2, 0}, // upper left arm
    {4,  -ARM_LEN, 0, 0}, // lower left arm
    {0,  TORSO_WIDTH/2, -TORSO_LEN/2, 0}, // upper right leg
    {6,  0, -LEG_LEN, 0}, // lower right leg
    {0,  -TORSO_WIDTH/2, -TORSO_LEN/2, 0}, // upper left leg
    {8,  0, -LEG_LEN, 0} // lower left leg
  };

  struct ShapeDesc {
    int parentJointId;
    float x, y, z, sx, sy, sz;
    shared_ptr<Geometry> geometry;
  };

  ShapeDesc shapeDesc[NUM_SHAPES] = {
    {0, 0,         0, 0, TORSO_WIDTH, TORSO_LEN, TORSO_THICK, g_cube}, // torso
    {1, ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // upper right arm
    {2, ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower right arm
    {3, 0, HEAD_RAD, 0, HEAD_RAD, HEAD_RAD, HEAD_RAD, g_arcball}, // head
    {4, -ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // upper left arm
    {5, -ARM_LEN/2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower left arm
    {6, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // upper right Leg
    {7, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // lower right Leg
    {8, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // upper left Leg
    {9, 0, -LEG_LEN/2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube}, // lower left Leg
  };

  shared_ptr<SgTransformNode> jointNodes[NUM_JOINTS];

  for (int i = 0; i < NUM_JOINTS; ++i) {
    if (jointDesc[i].parent == -1)
      jointNodes[i] = base;
    else {
      jointNodes[i].reset(new SgRbtNode(RigTForm(Cvec3(jointDesc[i].x, jointDesc[i].y, jointDesc[i].z))));
      jointNodes[jointDesc[i].parent]->addChild(jointNodes[i]);
    }
  }
  for (int i = 0; i < NUM_SHAPES; ++i) {
    shared_ptr<MyShapeNode> shape(
      new MyShapeNode(shapeDesc[i].geometry,
                      color,
                      Cvec3(shapeDesc[i].x, shapeDesc[i].y, shapeDesc[i].z),
                      Cvec3(0, 0, 0),
                      Cvec3(shapeDesc[i].sx, shapeDesc[i].sy, shapeDesc[i].sz)));
    jointNodes[shapeDesc[i].parentJointId]->addChild(shape);
  }
}

static void initScene() {
  g_world.reset(new SgRootNode());

  g_skyNode.reset(new SgRbtNode(RigTForm(Cvec3(0.0, 0.25, 4.0))));

  g_groundNode.reset(new SgRbtNode());
  g_groundNode->addChild(shared_ptr<MyShapeNode>(
                           new MyShapeNode(g_ground, Cvec3(0.1, 0.95, 0.1))));

  g_robot1Node.reset(new SgRbtNode(RigTForm(Cvec3(-2, 1, 0))));
  g_robot2Node.reset(new SgRbtNode(RigTForm(Cvec3(2, 1, 0))));

  // default selection and view
  g_currentPickedRbtNode = g_skyNode;
  g_currentView = g_skyNode;

  constructRobot(g_robot1Node, Cvec3(1, 0, 0)); // a Red robot
  constructRobot(g_robot2Node, Cvec3(0, 0, 1)); // a Blue robot

  g_world->addChild(g_skyNode);
  g_world->addChild(g_groundNode);
  g_world->addChild(g_robot1Node);
  g_world->addChild(g_robot2Node);
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
    initScene();
    glutMainLoop();
    return 0;
  } catch (const runtime_error &e) {
    cout << "Exception caught: " << e.what() << endl;
    return -1;
  }
}

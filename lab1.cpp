/*
 *  Lab exercise 1.0
 *  CSc 474, Computer Graphics
 *  Chris Buckalew, modified from Jason L. McKesson, Ed Angel, and others
 *
 *  Preparation for the lab - here's what I suggest.
 *  Make a directory 474/labs/lab1.  Copy my lab1 files into it:
 *     cp ~buckalew/Class/F13/474/labs/lab1/* .
 *
 *  Move the glmTar.tar file to the labs directory:
 *     mv glmTar.tar ..
 *
 *  cd to the labs directory and untar the glmTar file:
 *     tar -xf glmTar.tar
 *
 *  Now return to the lab1 file and run make - hopefully everything works!
 *
 *  This lab implements simple linear translation for a sphere along the
 *  x-axis.  I want you to:
 *
 *  1) make the sphere move from the left marker to the right marker instead of from
 *     the middle to the right marker
 *
 *  2) make the translation reverse for the return trip rather than jump back to the start.
 *     Hint: use half the period (t=0 to 1) for moving back.
 *
 *  3) make the sphere rotate (instead of translate) about the y axis at a
 *     radius of 5 units (the y axis is vertical, of course).
 *
 *  4) make the rotation faster - three or four revolutions during the period.
 *
 *  5) now add a translation element to move the rotating sphere 5 units up as
 *     as it rotates.  Now the sphere should describe a screw-thread motion as it goes from
 *     bottom to top.
 *
 *  6) a damping element that decreases the radius as the sphere moves
 *     upward.  *Don't* use spiral motion for this; use the exp()
 *     function.
 *
 *  7) change the damping motion so that it damps very quickly.
 *
 */

#include <string>
#include <vector>
#include <stack>
#include <math.h>
#include <stdio.h>
//#include <GL/freeglut.h>
#include <GL/glut.h>
#include "../glm/glm.hpp"
#include "../glm/ext.hpp"
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>
#include "Sphere.cpp"
#include "util.cpp"

// prototypes and variables associated with the trackball viewer
void mouseCallback(int button, int state, int x, int y);
void mouseMotion(int x, int y);

bool trackballEnabled = true;
bool trackballMove = false;
bool trackingMouse = false;
bool redrawContinue = false;
bool zoomState = false;
bool shiftState = false;

int winWidth, winHeight;
float angle = 0.0, axis[3];

float lightXform[4][4] = {
   {1.0, 0.0, 0.0, 0.0},
   {0.0, 1.0, 0.0, 0.0},
   {0.0, 0.0, 1.0, 0.0},
   {0.0, 0.0, 0.0, 1.0}
};

float objectXform[4][4] = {
   {1.0, 0.0, 0.0, 0.0},
   {0.0, 1.0, 0.0, 0.0},
   {0.0, 0.0, 1.0, 0.0},
   {0.0, 0.0, 0.0, 1.0}
};

float *objectXformPtr = (float *)objectXform;
float *lightXformPtr = (float *)lightXform;
float *trackballXform = (float *)objectXform;

// initial viewer position
static float modelTrans[] = {0.0f, 0.0f, -10.0f};

struct ProgramData
{
   // info for accessing the shaders
   GLuint theProgram;
   GLuint modelToWorldMatrixUnif;
   GLuint worldToCameraMatrixUnif;
   GLuint cameraToClipMatrixUnif;
   GLuint worldSpaceMoveMatrixUnif;
   GLuint lightIntensityUnif;
   GLuint ambientIntensityUnif;
   GLuint cameraSpaceLightPosUnif;
   GLuint shininessFactorUnif;
   GLuint diffuseColorUnif;
};

float nearClipPlane = 0.1f;
float farClipPlane = 1000.0f;

// buffers used to communicate with the GPU
GLuint vertexBufferObject;
GLuint indexBufferObject;
GLuint vao;

ProgramData PhongShade;

ProgramData LoadProgram(const std::string &strVertexShader, const std::string &strFragmentShader)
{
   std::vector<GLuint> shaderList;

   shaderList.push_back(LoadShader(GL_VERTEX_SHADER, strVertexShader));
   shaderList.push_back(LoadShader(GL_FRAGMENT_SHADER, strFragmentShader));

   ProgramData data;
   data.theProgram = CreateProgram(shaderList);

   // the uniforms needed for the shaders
   data.modelToWorldMatrixUnif = glGetUniformLocation(data.theProgram, "modelToWorldMatrix");
   data.worldToCameraMatrixUnif = glGetUniformLocation(data.theProgram, "worldToCameraMatrix");
   data.cameraToClipMatrixUnif = glGetUniformLocation(data.theProgram, "cameraToClipMatrix");
   data.worldSpaceMoveMatrixUnif = glGetUniformLocation(data.theProgram, "worldSpaceMoveMatrix");
   data.lightIntensityUnif = glGetUniformLocation(data.theProgram, "lightIntensity");
   data.ambientIntensityUnif = glGetUniformLocation(data.theProgram, "ambientIntensity");
   data.cameraSpaceLightPosUnif = glGetUniformLocation(data.theProgram, "cameraSpaceLightPos");
   data.shininessFactorUnif = glGetUniformLocation(data.theProgram, "shininessFactor");
   data.diffuseColorUnif = glGetUniformLocation(data.theProgram, "diffuseColor");

   return data;
}

void InitializeProgram()
{
   // load and compile shaders, link the program and return it
   PhongShade = LoadProgram("PCN.vert", "PhongLighting.frag");
}

// set up the sphere statics in case there's going to be a sphere
// can remove these lines if no sphere
bool Sphere::sphereInitialized = false;
GLuint Sphere::sphereVao = vao;
GLuint Sphere::sphereVertexBufferObject = vertexBufferObject;
GLuint Sphere::sphereIndexBufferObject = indexBufferObject;

// two variables you may need for retracing the path
float lastTimeP = 0.0f;
bool retrace = false;
bool chg = false;

// this function returns a transform matrix that is applied to an object to be moved.  The input is a time
// parameter and the transform is initially the identity
glm::mat4 moveObj(float timeP) {

   // create an identity matrix
   glm::mat4 moveMatrix;
   if (timeP < 0.0f) {
      // don't move anything
      if (timeP != -1.0f && chg == false)
         chg = true;
      else
         chg = false;

      return moveMatrix;
   }

   // for one of the steps you'll need a rotation.  The format is: (matrix, angle in degrees, axis of rotation)
   //moveMatrix = glm::rotate(moveMatrix, 0.0f, glm::vec3(x, y, z));

   // this step multiplies the matrix by a translation vector
   else {
      printf("time is %f\n", timeP);
      if (chg == false) {
         moveMatrix = glm::translate(moveMatrix, glm::vec3(20.0f * timeP, 0.0f, 0.0f));
      }
      else {
         moveMatrix = glm::translate(moveMatrix, glm::vec3(20.0f + (-20.0f * timeP), 0.0f, 0.0f));
      }
    }
   return moveMatrix;
}

//Called after the window and OpenGL are initialized. Called exactly once, before the main loop.
void init()
{
   glutMouseFunc(mouseCallback);
   glutMotionFunc(mouseMotion);
   InitializeProgram();

   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glFrontFace(GL_CW);

   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
   glDepthFunc(GL_LEQUAL);
   glDepthRange(0.0f, 1.0f);
   glEnable(GL_DEPTH_CLAMP);
}

// marker sphere to the left - the transform passed in is the identity in this case
void drawFirstSphere(glm::mat4 modelMatrix)
{
   Sphere *sphere1 = new Sphere();
   glUniformMatrix4fv(PhongShade.worldSpaceMoveMatrixUnif, 1, GL_FALSE, glm::value_ptr(moveObj(-1.0))); // don't move it

   // translate to the left 5 units
   modelMatrix = glm::translate(modelMatrix, glm::vec3(-5.0f, 0.0f, 0.0f));

   // scale it small - recall that transforms are applied in reverse coding order, so this scale occurs first
   modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f, 0.25f, 0.25f));

   // now set the transform in the shader
   glUniformMatrix4fv(PhongShade.modelToWorldMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix));

   // color
   glUniform4f(PhongShade.diffuseColorUnif, 1.0f, 0.2f, 0.2f, 1.0f); // low saturation red

   // couple of other reflectance parameters
   glUniform4f(PhongShade.ambientIntensityUnif, 0.2f, 0.2f, 0.2f, 1.0f);
   glUniform1f(PhongShade.shininessFactorUnif, 20.0f);

   // tells the shader to draw it
   sphere1->DrawUnitSphere();
}

// this is the moving sphere
void drawSecondSphere(glm::mat4 modelMatrix, float timeParameter)
{
   Sphere *sphere2 = new Sphere();

   // this time send the shader a non-identity transform to move in world space
   glUniformMatrix4fv(PhongShade.worldSpaceMoveMatrixUnif, 1, GL_FALSE, glm::value_ptr(moveObj(timeParameter)));

   modelMatrix = glm::translate(modelMatrix, glm::vec3(-5.0f, 0.0f, 0.0f));
   modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 0.5f, 1.0f)); // scale is different from the marker
   glUniformMatrix4fv(PhongShade.modelToWorldMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix));
   glUniform4f(PhongShade.diffuseColorUnif, 0.2f, 1.0f, 0.2f, 1.0f); // color is different from the marker
   glUniform4f(PhongShade.ambientIntensityUnif, 0.2f, 0.2f, 0.2f, 1.0f);
   glUniform1f(PhongShade.shininessFactorUnif, 20.0f);

   sphere2->DrawUnitSphere();
}

// the right marker sphere
void drawThirdSphere(glm::mat4 modelMatrix)
{
   Sphere *sphere3 = new Sphere();
   glUniformMatrix4fv(PhongShade.worldSpaceMoveMatrixUnif, 1, GL_FALSE, glm::value_ptr(moveObj(-1.0)));

   modelMatrix = glm::translate(modelMatrix, glm::vec3(5.0f, 0.0f, 0.0f));
   modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f, 0.25f, 0.25f));
   glUniformMatrix4fv(PhongShade.modelToWorldMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix));
   glUniform4f(PhongShade.diffuseColorUnif, 1.0f, 0.2f, 0.2f, 1.0f);
   glUniform4f(PhongShade.ambientIntensityUnif, 0.2f, 0.2f, 0.2f, 1.0f);
   glUniform1f(PhongShade.shininessFactorUnif, 20.0f);

   sphere3->DrawUnitSphere();
}

//Called to update the display.
//You should call glutSwapBuffers after all of your rendering to display what you rendered.
//If you need continuous updates of the screen, call glutPostRedisplay() at the end of the function.
void display()
{
   const float epsilon = 0.001f; // necessary for the trackball viewer
   const float period = 5.0;  // repeat time in seconds of movement
   float fElapsedTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // time since programs started in seconds
   float timeParameter = (float) fmodf(fElapsedTime, period)/period; // parameterized time
   //float timeParameter = 0.0f; // no animation - also comment out the glutPostRedisplay() at the bottom of this function

   glClearColor(0.0f, 0.0f, 0.2f, 0.0f); // unshaded objects will show up on this almost-black background
   glClearDepth(1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // light stuff
   glUniform3f(PhongShade.cameraSpaceLightPosUnif, 0.0, 0.0, 100.0);  // this is a headlight
   glUniform4f(PhongShade.lightIntensityUnif, 1.0f, 1.0f, 1.0f, 1.0f); // light color

   // compute trackball interface (world to camera) transform --------------------------------
   // don't change any of this (unless you can make it better!)
   glm::mat4 camMatrix;
   glm::mat4 cam2Matrix;
   camMatrix = glm::translate(camMatrix, glm::vec3(modelTrans[0], modelTrans[1], modelTrans[2]));
   // in the middle of a left-button drag
   if (trackballMove) {
      // check to make sure axis is not zero vector
      if (!(-epsilon < axis[0] && axis[0] < epsilon && -epsilon < axis[1] && axis[1] < epsilon && -epsilon < axis[2] && axis[2] < epsilon)) {

         cam2Matrix = glm::rotate(cam2Matrix, angle, glm::vec3(axis[0], axis[1], axis[2]));
         cam2Matrix = cam2Matrix * (glm::mat4(trackballXform[0], trackballXform[1], trackballXform[2], trackballXform[3],
                  trackballXform[4], trackballXform[5], trackballXform[6], trackballXform[7],
                  trackballXform[8], trackballXform[9], trackballXform[10], trackballXform[11],
                  trackballXform[12], trackballXform[13], trackballXform[14], trackballXform[15]));
         glm::mat4 tempM = cam2Matrix;
         // copy current transform back into trackball matrix
         for (int i=0; i<4; i++) {
            for (int j=0; j<4; j++)
               trackballXform[i*4 + j] = tempM[i][j];
         }
      }
   }
   camMatrix = camMatrix * (glm::mat4(objectXformPtr[0], objectXformPtr[1], objectXformPtr[2], objectXformPtr[3],
            objectXformPtr[4], objectXformPtr[5], objectXformPtr[6], objectXformPtr[7],
            objectXformPtr[8], objectXformPtr[9], objectXformPtr[10], objectXformPtr[11],
            objectXformPtr[12], objectXformPtr[13], objectXformPtr[14], objectXformPtr[15]));
   // end of world to camera transform -----------------------------------------------

   // pass it on to the shaders
   glUniformMatrix4fv(PhongShade.worldToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(camMatrix));

   // draw objects - pass in the transform matrix to use
   glm::mat4 modelMatrix;  //starts with identity

   drawFirstSphere(modelMatrix); // modelMatrix is not modified by the draw functions
   drawSecondSphere(modelMatrix, timeParameter);
   drawThirdSphere(modelMatrix);

   glutSwapBuffers();
   glutPostRedisplay();
}

//Called whenever the window is resized. The new window size is given, in pixels.
//This is an opportunity to call glViewport or glScissor to keep up with the change in size.
void reshape (int w, int h)
{
   glm::mat4 persMatrix = glm::perspective(45.0f, (w / (float)h), nearClipPlane, farClipPlane);
   winWidth = w;
   winHeight = h;

   glUseProgram(PhongShade.theProgram);
   glUniformMatrix4fv(PhongShade.cameraToClipMatrixUnif, 1, GL_FALSE, glm::value_ptr(persMatrix));

   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glutPostRedisplay();
}

// Trackball-like interface functions - no need to ever change any of this--------------------------------------------------
float lastPos[3] = {0.0, 0.0, 0.0};
int curx, cury;
int startX, startY;

void trackball_ptov(int x, int y, int width, int height, float v[3]) {
   float d, a;
   // project x, y onto a hemisphere centered within width, height
   v[0] = (2.0f*x - width) / width;
   v[1] = (height - 2.0f*y) / height;
   d = (float) sqrt(v[0]*v[0] + v[1]*v[1]);
   v[2] = (float) cos((3.14159f/2.0f) * ((d<1.0f)? d : 1.0f));
   a = 1.0f / (float) sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
   v[0] *= a;
   v[1] *= a;
   v[2] *= a;
}

void mouseMotion(int x, int y) {
   float curPos[3], dx, dy, dz;

   if (zoomState == false && shiftState == false) {

      trackball_ptov(x, y, winWidth, winHeight, curPos);

      dx = curPos[0] - lastPos[0];
      dy = curPos[1] - lastPos[1];
      dz = curPos[2] - lastPos[2];

      if (dx||dy||dz) {
         angle = 90.0f * sqrt(dx*dx + dy*dy + dz*dz);

         axis[0] = lastPos[1]*curPos[2] - lastPos[2]*curPos[1];
         axis[1] = lastPos[2]*curPos[0] - lastPos[0]*curPos[2];
         axis[2] = lastPos[0]*curPos[1] - lastPos[1]*curPos[0];

         lastPos[0] = curPos[0];
         lastPos[1] = curPos[1];
         lastPos[2] = curPos[2];
      }

   }
   else if (zoomState == true) {
      curPos[1] = (float) y;
      dy = curPos[1] - lastPos[1];

      if (dy) {
         modelTrans[2] += dy * 0.01f;
         lastPos[1] = curPos[1];
      }
   }
   else if (shiftState == true) {
      curPos[0] = (float) x;
      curPos[1] =(float)  y;
      dx = curPos[0] - lastPos[0];
      dy = curPos[1] - lastPos[1];

      if (dx) {
         modelTrans[0] += dx * 0.01f;
         lastPos[0] = curPos[0];
      }
      if (dy) {
         modelTrans[1] -= dy * 0.01f;
         lastPos[1] = curPos[1];
      }
   }
   glutPostRedisplay( );
}

void startMotion(long time, int button, int x, int y) {
   if (!trackballEnabled) return;

   trackingMouse = true;
   redrawContinue = false;
   startX = x; startY = y;
   curx = x; cury = y;
   trackball_ptov(x, y, winWidth, winHeight, lastPos);
   trackballMove = true;
}

void stopMotion(long time, int button, int x, int y) {
   if (!trackballEnabled) return;

   trackingMouse = false;

   if (startX != x || startY != y)
      redrawContinue = true;
   else {
      angle = 0.0f;
      redrawContinue = false;
      trackballMove = false;
   }
}

// Called when a mouse button is pressed or released
void mouseCallback(int button, int state, int x, int y) {

   switch (button) {
   case GLUT_LEFT_BUTTON:
      trackballXform = (float *)objectXform;
      break;
   case GLUT_RIGHT_BUTTON:
   case GLUT_MIDDLE_BUTTON:
      trackballXform = (float *)lightXform;
      break;
   }
   switch (state) {
   case GLUT_DOWN:
      if (button == GLUT_RIGHT_BUTTON) {
         zoomState = true;
         lastPos[1] = (float) y;
      }
      else if (button == GLUT_MIDDLE_BUTTON) {
         shiftState = true;
         lastPos[0] = (float) x;
         lastPos[1] = (float) y;
      }
      else startMotion(0, 1, x, y);
      break;
   case GLUT_UP:
      trackballXform = (float *)lightXform; // turns off mouse effects
      if (button == GLUT_RIGHT_BUTTON) {
         zoomState = false;
      }
      else if (button == GLUT_MIDDLE_BUTTON) {
         shiftState = false;
      }
      else stopMotion(0, 1, x, y);
      break;
   }
}

// end of trackball mouse functions--------------------------------------------------------------------------


//Called whenever a key on the keyboard was pressed.
//The key is given by the ''key'' parameter, which is in ASCII.
void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
   case 27:
      return;
   case 'z': modelTrans[2] += 1.0f; break;
   case 'Z': modelTrans[2] -= 1.0f; break;
   case 'x': modelTrans[0] += 1.0f; break;
   case 'X': modelTrans[0] -= 1.0f; break;
   case 'y': modelTrans[1] += 1.0f; break;
   case 'Y': modelTrans[1] -= 1.0f; break;
   case 'h':
             lightXformPtr[0] = objectXformPtr[0] = lightXformPtr[5] = objectXformPtr[5] =
                lightXformPtr[10] = objectXformPtr[10] = lightXformPtr[15] = objectXformPtr[15] = 1.0f;
             lightXformPtr[1] = objectXformPtr[1] = lightXformPtr[2] = objectXformPtr[2] = lightXformPtr[3] = objectXformPtr[3] =
                lightXformPtr[4] = objectXformPtr[4] = lightXformPtr[6] = objectXformPtr[6] = lightXformPtr[7] = objectXformPtr[7] =
                lightXformPtr[8] = objectXformPtr[8] = lightXformPtr[9] = objectXformPtr[9] = lightXformPtr[11] = objectXformPtr[11] =
                lightXformPtr[12] = objectXformPtr[12] = lightXformPtr[13] = objectXformPtr[13] = lightXformPtr[14] = objectXformPtr[14] = 0.0;
             modelTrans[0] = modelTrans[1] = 0.0; modelTrans[2] = -10.0;
             axis[0] = axis[1] = axis[2] = 0.0;
             angle = 0;
             break;
   case 'd':
             break;
   }
   glutPostRedisplay();
}

int main(int argc, char** argv)
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 20, 20 );
   glutInitWindowSize( 500, 500 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow("Lab 1 - Procedural Motion");

   //test the openGL version
   getGLversion();

   init();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMainLoop();
   return 0;
}

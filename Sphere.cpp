/*
 * This is a sphere object
 */

//#include <string>
//#include <vector>
//#include <stack>
#include <math.h>
#include <stdio.h>
//#include <GL/freeglut.h>
//#include <GL/glut.h>
//#include "../glm/glm.hpp"
//#include "../glm/ext.hpp"
//#include <fstream>
//#include <sstream>
//#include <exception>
//#include <stdexcept>
//#include <string.h>
//#include <algorithm>
//#include <string>
//#include <vector>

class Sphere {

   // these statics are for OpenGL's purpose, so that one sphere object can be used for multiple spheres
   public:static bool sphereInitialized;
   public:static GLuint sphereVao;
   public:static GLuint sphereVertexBufferObject;
   public:static GLuint sphereIndexBufferObject;

   // horiz is longitute, vert is latitude
   int horizTessResolution;
   int vertTessResolution;

   // the vertex array
   float *sphereArray;
   // the index array
   GLshort *sphereIndices;

   Sphere() {  // default resolution - can't build two different spheres with different resolutions in the shader
      horizTessResolution = 40;
      vertTessResolution = 40;
   }

   void GenerateSphereTriangles(int horizTessellationResolution, int vertTessellationResolution) {
      // allocate array for vertices
      const int floatCoords = 3; // number of floats per triangle vertex

      sphereArray = new float[(horizTessellationResolution*(vertTessellationResolution+1))*(3+3)];  // position and normal values
      sphereIndices = new GLshort[(horizTessellationResolution*vertTessellationResolution)*2*3];  // 2 triangles per quad, three indices per triangle

      double horizAngleIncr = 2.0*3.14159/horizTessellationResolution;
      double vertAngleIncr = 3.14159/vertTessellationResolution;
      int normalOffset = horizTessellationResolution*(vertTessellationResolution+1) * (3); // offset into buffer where the normals start

      double vertAngle = -3.14159/2.0;
      double horizAngle;
      int index = 0;
      for (int i=0; i<vertTessellationResolution+1; i++, vertAngle+=vertAngleIncr) {
         horizAngle = 0.0;

         for (int j=0; j<horizTessellationResolution; j++, horizAngle+=horizAngleIncr, index+=3) {
            // position
            sphereArray[index+0] = (float)(cos(horizAngle)*cos(vertAngle));
            sphereArray[index+1] = (float)sin(vertAngle);
            sphereArray[index+2] = (float)(sin(horizAngle)*cos(vertAngle));

            // normals - same as position since this is a unit sphere 
            // A UNIT SPHERE IS THE ONLY OBJECT IN THE UNIVERSE FOR WHICH THIS IS TRUE!
            sphereArray[index+0 + normalOffset] = sphereArray[index+0];
            sphereArray[index+1 + normalOffset] = sphereArray[index+1];
            sphereArray[index+2 + normalOffset] = sphereArray[index+2];
         }		
      }

      // now the indices
      index = 0;
      for (int i=0; i<vertTessellationResolution; i++) {
         int j;
         int A, B, C, D;
         for (j=0; j<horizTessellationResolution-1; j++) {
            // two triangles per quad - faster but more complex to use triangle strip
            //ABCD quad, A lower-left corner going clockwise
            A = i*vertTessellationResolution + j;
            B = i*vertTessellationResolution + j + horizTessellationResolution;
            C = i*vertTessellationResolution + j + 1 + horizTessellationResolution;
            D = i*vertTessellationResolution + j + 1;
            sphereIndices[index++] = A; 	sphereIndices[index++] = C;		sphereIndices[index++] = B;
            sphereIndices[index++] = A;		sphereIndices[index++] = D;		sphereIndices[index++] = C;
         }
         // last face
         A = i*vertTessellationResolution + j;
         B = i*vertTessellationResolution + j + horizTessellationResolution;
         C = i*vertTessellationResolution + 0 + horizTessellationResolution;
         D = i*vertTessellationResolution + 0;
         sphereIndices[index++] = A; 	sphereIndices[index++] = C;		sphereIndices[index++] = B;
         sphereIndices[index++] = A;		sphereIndices[index++] = D;		sphereIndices[index++] = C;
      }
   }

   void InitializeSphereVertexBuffer(int horizTessellationResolution, int vertTessellationResolution)	{
      GenerateSphereTriangles(horizTessellationResolution, vertTessellationResolution);
      glGenBuffers(1, &sphereVertexBufferObject);

      glBindBuffer(GL_ARRAY_BUFFER, sphereVertexBufferObject);
      glBufferData(GL_ARRAY_BUFFER, (horizTessellationResolution*(vertTessellationResolution+1))*(3*4*2), sphereArray, GL_STATIC_DRAW); //3 floats per vert, 4 bytes per float, 2 sets of data (loc, normal)
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      glGenBuffers(1, &sphereIndexBufferObject);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIndexBufferObject);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, (horizTessellationResolution*vertTessellationResolution)*3*2*2, sphereIndices, GL_STATIC_DRAW); // 3*2 shorts per triangle, 2 bytes per short
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      free(sphereArray);
      free(sphereIndices);
   }

   void InitializeSphereBuffer(int horizTessellationResolution, int vertTessellationResolution) {
      InitializeSphereVertexBuffer(horizTessellationResolution, vertTessellationResolution);
	
      glGenVertexArrays(1, &sphereVao);
      glBindVertexArray(sphereVao);

      // code for sphere
      size_t normalDataOffset = sizeof(float) * (3) * (horizTessellationResolution*(vertTessellationResolution+1)); // three floats per vertex
      glBindBuffer(GL_ARRAY_BUFFER, sphereVertexBufferObject);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)normalDataOffset);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIndexBufferObject);
   }

   void DrawUnitSphere()  {

      if (!sphereInitialized) {
         // buffer array for vertices, normals, and indices is shared by all spheres
         InitializeSphereBuffer(horizTessResolution, vertTessResolution);
         sphereInitialized = true;
      }

      glBindVertexArray(sphereVao);
      // number of indices - #triangles * verts 
      glDrawElements(GL_TRIANGLES, (horizTessResolution*vertTessResolution*3)*2, GL_UNSIGNED_SHORT, 0); // 3 verts per triangle
      glBindVertexArray(0);
   }
};


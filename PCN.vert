#version 120 

#extension GL_ARB_gpu_shader5 : enable

// this is syntax for GLSL 3.3
//layout(location = 0) in vec3 position;
//layout(location = 1) in vec3 normal;
//out vec3 vertexNormal;
//out vec3 cameraSpacePosition;

// this is syntax for GLSL 1.2
in vec3 position;
in vec3 normal;
varying vec3 vertexNormal;
varying vec3 cameraSpacePosition;

uniform mat4 worldSpaceMoveMatrix;
uniform mat4 worldToCameraMatrix;
uniform mat4 modelToWorldMatrix;
uniform mat4 cameraToClipMatrix;

void main()
{
   mat4 modelToCameraMatrix = worldToCameraMatrix * worldSpaceMoveMatrix * modelToWorldMatrix;
   vec4 tempCamPosition = (worldToCameraMatrix * (worldSpaceMoveMatrix * (modelToWorldMatrix * vec4(position, 1.0))));
   gl_Position = cameraToClipMatrix * tempCamPosition;

   mat3 normalModelToCameraMatrix = mat3(vec3(modelToCameraMatrix[0]), vec3(modelToCameraMatrix[1]), vec3(modelToCameraMatrix[2]));
   normalModelToCameraMatrix = transpose(inverse(normalModelToCameraMatrix));
   vertexNormal = normalModelToCameraMatrix * normal;
   cameraSpacePosition = vec3(tempCamPosition);
}

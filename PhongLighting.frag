#version 330

in vec3 vertexNormal;
in vec3 cameraSpacePosition;

out vec4 outputColor;

uniform vec4 diffuseColor;
uniform vec4 lightIntensity;
uniform vec4 ambientIntensity;

uniform vec3 cameraSpaceLightPos;

const vec4 specularColor = vec4(0.25, 0.25, 0.25, 1.0);
uniform float shininessFactor;

void main()
{
   vec3 lightDifference =  cameraSpaceLightPos - cameraSpacePosition;
   vec3 lightDir =  normalize(lightDifference);

   vec3 surfaceNormal = normalize(vertexNormal);
   float cosAngIncidence = dot(surfaceNormal, lightDir);
   cosAngIncidence = clamp(cosAngIncidence, 0, 1);
	
   vec3 viewDirection = normalize(-cameraSpacePosition);
   vec3 reflectDir = reflect(-lightDir, surfaceNormal);
   float phongTerm = dot(viewDirection, reflectDir);
   phongTerm = clamp(phongTerm, 0, 1.0); 
   phongTerm = cosAngIncidence != 0.0 ? phongTerm : 0.0;
   phongTerm = pow(phongTerm, shininessFactor);

   outputColor = (diffuseColor * lightIntensity * cosAngIncidence) +
                 (specularColor * lightIntensity * phongTerm) +
                 (diffuseColor * ambientIntensity);
}

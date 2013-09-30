
#include <string>
#include <vector>
#include <stack>
//#include <math.h>
#include <stdio.h>
//#include <GL/freeglut.h>
//#include <GL/glut.h>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <string.h>
//#include <algorithm>
#include <string>
#include <vector>

std::string FindFileOrThrow( const std::string &strBasename );

// begin compile shader stuff

   class GLException : public std::exception
   {
      public:
        GLException(char * strError) { m_strError = strError; }

        char * m_strError;
   };

   std::exception CompileLinkException( GLuint shader )
   {
      GLint infoLogLength;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

      char *strInfoLog = new GLchar[infoLogLength + 1];
      glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

      throw std::runtime_error(strInfoLog);
      std::runtime_error *result = new std::runtime_error(strInfoLog);

      delete[] strInfoLog;

      glDeleteShader(shader);

      return *result;
   }
	
   void ThrowIfShaderCompileFailed(GLuint shader)
   {
      GLint status;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
      if (status == GL_FALSE)
         throw CompileLinkException(shader);
   }

   GLuint CompileShader( GLenum shaderType, const std::string &shaderText )
   {
      GLuint shader = glCreateShader(shaderType);
      GLint textLength = (GLint)shaderText.size();
      const GLchar *pText = static_cast<const GLchar *>(shaderText.c_str());
      glShaderSource(shader, 1, &pText, &textLength);
      glCompileShader(shader);

      ThrowIfShaderCompileFailed(shader);

      return shader;
   }
//end compile shader stuff

GLuint LoadShader(GLenum eShaderType, const std::string &strShaderFilename)
{
   std::string strFilename = FindFileOrThrow(strShaderFilename);
   std::ifstream shaderFile(strFilename.c_str());
   std::stringstream shaderData;
   shaderData << shaderFile.rdbuf();
   shaderFile.close();

   try {
   	return CompileShader(eShaderType, shaderData.str());
   }
   catch(std::exception &e) {
   	fprintf(stderr, "%s\n", e.what());
   	throw;
   }
}

// begin LinkProgram stuff

std::exception CompileLinkException( GLuint program, bool )
{
   GLint infoLogLength;
   glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

   char *strInfoLog = new GLchar[infoLogLength + 1];
   glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);

   throw std::runtime_error(strInfoLog);
   std::runtime_error *result = new std::runtime_error(strInfoLog);

   delete[] strInfoLog;
   glDeleteProgram(program);
   return *result;
}

void ThrowIfProgramLinkFailed(GLuint program)
{
   GLint status;
   glGetProgramiv (program, GL_LINK_STATUS, &status);
   if (status == GL_FALSE) {
      throw CompileLinkException(program, true);
   }
}

GLuint LinkProgram( GLuint program, const std::vector<GLuint> &shaders )
{
   for(size_t loop = 0; loop < shaders.size(); ++loop)
      glAttachShader(program, shaders[loop]);

   glLinkProgram(program);
   ThrowIfProgramLinkFailed(program);

   for(size_t loop = 0; loop < shaders.size(); ++loop)
      glDetachShader(program, shaders[loop]);

   return program;
}

GLuint LinkProgram( const std::vector<GLuint> &shaders) //, bool isSeparable )
{
   GLuint program = glCreateProgram();
   return LinkProgram(program, shaders);
}
// end link program stuff

GLuint CreateProgram(const std::vector<GLuint> &shaderList)
{
   try {
      GLuint prog = LinkProgram(shaderList);
      std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
      return prog;
   }
   catch(std::exception &e) {
      std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
      fprintf(stderr, "%s\n", e.what());
      throw;
   }
}

std::string FindFileOrThrow( const std::string &strBasename )
{
   std::string strFilename = /*LOCAL_FILE_DIR  + */ strBasename;
   std::ifstream testFile(strFilename.c_str());
   if(testFile.is_open())
      return strFilename;
		
   strFilename = /*GLOBAL_FILE_DIR + */ strBasename;
   testFile.open(strFilename.c_str());
   if(testFile.is_open())
      return strFilename;

   throw std::runtime_error("Could not find the file " + strBasename);
}

void APIENTRY DebugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                        const GLchar* message, GLvoid* userParam)
{
   std::string srcName;
   switch(source) {
      case GL_DEBUG_SOURCE_API_ARB: srcName = "API"; break;
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: srcName = "Window System"; break;
      case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: srcName = "Shader Compiler"; break;
      case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: srcName = "Third Party"; break;
      case GL_DEBUG_SOURCE_APPLICATION_ARB: srcName = "Application"; break;
      case GL_DEBUG_SOURCE_OTHER_ARB: srcName = "Other"; break;
   }

   std::string errorType;
   switch(type) {
      case GL_DEBUG_TYPE_ERROR_ARB: errorType = "Error"; break;
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: errorType = "Deprecated Functionality"; break;
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: errorType = "Undefined Behavior"; break;
      case GL_DEBUG_TYPE_PORTABILITY_ARB: errorType = "Portability"; break;
      case GL_DEBUG_TYPE_PERFORMANCE_ARB: errorType = "Performance"; break;
      case GL_DEBUG_TYPE_OTHER_ARB: errorType = "Other"; break;
   }

   std::string typeSeverity;
   switch(severity) {
      case GL_DEBUG_SEVERITY_HIGH_ARB: typeSeverity = "High"; break;
      case GL_DEBUG_SEVERITY_MEDIUM_ARB: typeSeverity = "Medium"; break;
      case GL_DEBUG_SEVERITY_LOW_ARB: typeSeverity = "Low"; break;
   }

   printf("%s from %s,\t%s priority\nMessage: %s\n",
          errorType.c_str(), srcName.c_str(), typeSeverity.c_str(), message);
}

unsigned int defaults(unsigned int displayMode, int &width, int &height) {return displayMode;}

void getGLversion() {
   int major, minor;
	
   major = minor =0;
   const char *verstr = (const char *)glGetString(GL_VERSION);
	
   if ((verstr == NULL) || (sscanf(verstr, "%d.%d", &major, &minor) !=2)) {
      printf("Invalid GL_VERSION format %d %d\n", major, minor);
   }
   //if( major <2) {
   //   printf("This shader example will not work due to opengl version, which is %d %d (%s)\n", major, minor, verstr);
    //  exit(0);
   //}
   printf("GL_VERSION format %d %d\n", major, minor);
}


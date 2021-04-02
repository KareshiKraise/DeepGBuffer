#pragma once

#include <gl/glew.h>
#include <iostream>


//Nota para o professor: as macros a seguir são para debug e error testing no windows apenas, nao ha necessidade de alterar nada
//Elas serao desativadas automaticamente durante compilacao em outro sistema

#ifdef _MSC_VER
#define ASSERT(x) if (!(x)) __debugbreak();
#else
//#include <csignal>
//#define ASSERT(x) std::raise(SIGINT);
#endif

#define DEBUG

#ifdef DEBUG
#define GLCall(x) GLClearError(); \
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#else
#define GLCall(x) x
#endif

#define DRAW_TEST_TRI glBegin(GL_TRIANGLES); \
	glVertex3f(-0.5, -0.5, 0.0);		\
	glVertex3f(0.5, 0.0, 0.0);			\
	glVertex3f(0.0, 0.5, 0.0);			\
	glEnd()


/*Error Checking */
static void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum err = glGetError())
	{
		std::cout << "[OpenGL Error] (" << std::hex << err << "):" << function << " " << file << " " << line << std::endl;
		return false;
	}
	return true;
}
/*End of error checking */

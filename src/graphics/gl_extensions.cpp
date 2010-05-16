/*
 * RCX Copyright (C) 2009-2010 Mats Wahlberg ("Slinger")
 *
 * This program comes with ABSOLUTELY NO WARRANTY!
 *
 * This is free software, and you are welcome to
 * redistribute it under certain conditions.
 *
 * See license.txt and README for more info
 */

#include <SDL/SDL.h>
#include "gl_extensions.hpp"
#include <string.h>

//functions (extern from hpp) instantiated here:
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
//end of function pointers




//taken directly from mesa3d docs:
GLboolean CheckExtension( char *extName )
{
/*
 ** Search for extName in the extensions string.  Use of strstr()
 ** is not sufficient because extension names can be prefixes of
 ** other extension names.  Could use strtok() but the constant
 ** string returned by glGetString can be in read-only memory.
 */
char *p = (char *) glGetString(GL_EXTENSIONS);
char *end;
int extNameLen;

extNameLen = strlen(extName);
end = p + strlen(p);

while (p < end) {
    int n = strcspn(p, " ");
    if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
	return GL_TRUE;
    }
    p += (n + 1);
}
return GL_FALSE;
}
//end of stolen code



//loader
bool Load_GL_Extensions()
{
	printf("TODO: replace extension tester code!\n");
	//see if got vbo extension
	if (!CheckExtension("GL_ARB_vertex_buffer_object"))
		return false;

	//ok, try to find them all
	glGenBuffers = (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffersARB");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffersARB");
	glBindBuffer = (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBufferARB");
	glBufferData = (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferDataARB");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC) SDL_GL_GetProcAddress("glBufferSubDataARB");

	//assume they could all be found...
	return true;
}

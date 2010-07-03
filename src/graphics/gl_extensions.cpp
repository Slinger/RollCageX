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
#include <string.h>

#include "gl_extensions.hpp"
#include "../shared/printlog.hpp"

//functions (extern from hpp) instantiated here:
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
//end of function pointers



//searches extension list for existence of needed extension
bool Verify_Extension(const char *name)
{
	char *p= (char *)glGetString(GL_EXTENSIONS); //list of extensions
	size_t nl = strlen(name); //length of wanted extension name
	size_t l; //length of focused word in list

	while (*p != '\0') //not at end
	{
		for (; *p==' '; ++p); //skip possible whitespace(s)

		for (l=0; p[l]!='\0'&&p[l]!=' '; ++l); //find length of word (chars before end of list or word)

		if (l==nl && !strncmp(name, p, l)) //equal length, strings matches
		{
			printlog(2, "Extension \"%s\" is supported", name);
			return true;
		}

		p+=l; //go to position after last word
	}

	printlog(0, "WARNING: extension \"%s\" not supported!", name);
	return false;
}


//loader
bool Load_GL_Extensions()
{
	printlog(1, "Loading needed GL extensions");

	//see if got vbo extension
	if (!Verify_Extension("GL_ARB_vertex_buffer_object"))
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

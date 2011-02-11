/*
 * RCX  Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger")
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

#include "gl_functions.hpp"
#include "../shared/printlog.hpp"

//functions (extern from hpp) instantiated here:
//VBO:
PFNGLGENBUFFERSPROC glGenBuffers=NULL;
PFNGLDELETEBUFFERSPROC glDeleteBuffers=NULL;
PFNGLBINDBUFFERPROC glBindBuffer=NULL;
PFNGLBUFFERDATAPROC glBufferData=NULL;
PFNGLBUFFERSUBDATAPROC glBufferSubData=NULL;
//GLSL:
//PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray=NULL;
//PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray=NULL;
//PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer=NULL;
//for enabling legacy "mode" rendering (no shading)
//bool legacy_vbo=false;


//searches extension list for existence of needed extension
bool Check_Extension(const char *name)
{
	char *p= (char *)glGetString(GL_EXTENSIONS); //list of extensions
	size_t nl = strlen(name); //length of wanted extension name
	size_t l; //length of focused word in list

	while (*p != '\0') //not at end
	{
		for (; *p==' '; ++p); //skip possible whitespace(s)

		for (l=0; p[l]!='\0'&&p[l]!=' '; ++l); //find length of word (chars before end of list or word)

		if (l==nl && !strncmp(name, p, l)) //equal length, strings matches
			return true;

		p+=l; //go to position after last word
	}

	return false;
}

//compares version string to see if bigger or equal to needed version
bool Check_Version(const char *version)
{
	char *s= (char *)glGetString(GL_VERSION); //reported version

	//if first characters bigger or equal to wanted, return true
	if (strncmp(s, version, strlen(version)) >= 0)
		return true;

	//older version
	return false;
}


//loader
bool Load_GL_Functions()
{
	printlog(1, "Loading needed GL extensions");

	//TODO: GLSL
	//2.0 (and later) introduces new functions instead of the gl*Pointer ones (for GLSL)
	//the old functions are deprecated in 3.0, so if possible, use the new ones instead
	/*if (Check_Version("2.0"))
	{
		printlog(1, "modern VBO rendering+generation using core functions (GL >= 2.0)");
		legacy_vbo=false; //use modern

		glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) SDL_GL_GetProcAddress("glEnableVertexAttribArray");
		glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) SDL_GL_GetProcAddress("glDisableVertexAttribArray");
		glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) SDL_GL_GetProcAddress("glVertexAttribPointer");
		[...]

		glGenBuffers = (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffers");
		glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffers");
		glBindBuffer = (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBuffer");
		glBufferData = (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferData");
		glBufferSubData = (PFNGLBUFFERSUBDATAPROC) SDL_GL_GetProcAddress("glBufferSubData");
	}
	//not supported? see if legacy approach (or extensions) is possible
	else*/ if (Check_Version("1.5") || Check_Extension("GL_ARB_vertex_buffer_object"))
	{
		//TODO: GLSL
		/*//ok, we _can_ use VBO, lets see if the 2.0 functions exists as an extension:
		if (Check_Extension("GL_ARB_vertex_shader") || Check_Extension("GL_ARB_vertex_program"))
		{
			printlog(1, "modern VBO rendering using ARB extension functions...");
			legacy_vbo=false; //use modern

			glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) SDL_GL_GetProcAddress("glEnableVertexAttribArrayARB");
			glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) SDL_GL_GetProcAddress("glDisableVertexAttribArrayARB");
			glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) SDL_GL_GetProcAddress("glVertexAttribPointerARB");
		}
		else
		{
			printlog(1, "legacy VBO rendering...");
			legacy_vbo=true; //use legacy
		}*/

		//ok, the rest of the functions also needed (adopted into core at 1.5)
		if (Check_Version("1.5"))
		{
			printlog(1, "VBO handling using core functions (GL 1.5 or later)");

			glGenBuffers = (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffers");
			glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffers");
			glBindBuffer = (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBuffer");
			glBufferData = (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferData");
			glBufferSubData = (PFNGLBUFFERSUBDATAPROC) SDL_GL_GetProcAddress("glBufferSubData");
		}
		else //ok, the earlier check assures that at least the extension exists
		{
			printlog(1, "VBO handling using ARB extension functions (GL before 1.5)");

			glGenBuffers = (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffersARB");
			glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffersARB");
			glBindBuffer = (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBufferARB");
			glBufferData = (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferDataARB");
			glBufferSubData = (PFNGLBUFFERSUBDATAPROC) SDL_GL_GetProcAddress("glBufferSubDataARB");
		}
	}
	else
	{
		printlog(0, "Graphics hardware/software too old: require at least GL 1.5 or ARB_vertex_buffer_object extension!");
		return false;
	}

	//ok!
	return true;
}

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

#ifndef _RCX_GL_FUNCTIONS_H
#define _RCX_GL_FUNCTIONS_H

//includes typedefs for many opengl calls, some needed here
#include <SDL/SDL_opengl.h>

//declaration of functions to load:

//
//VBO handling (generating+rendering)
//
//adopted into core for GL 1.5 or later
//(or ARB_vertex_buffer_object extension)
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;

//
//TODO: will be used for GLSL (many more functions needed)
//
//extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
//extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
//extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
//in case these functions are not supported
//extern bool legacy_vbo;

//
//checking and loading of functions:
//
bool Load_GL_Functions();

#endif

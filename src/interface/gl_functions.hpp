/*
 * RollCageX - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of RollCageX.
 *
 * RollCageX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RollCageX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RollCageX.  If not, see <http://www.gnu.org/licenses/>.
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

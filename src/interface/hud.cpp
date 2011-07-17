/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of ReCaged.
 *
 * ReCaged is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReCaged is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
 */ 

//
//NOTE: just a hack!
//assumes width and height is power of two, and rgb channels
//does not delete texture or vbo...!
//
//the "font.bmp" was created with the Terminus font
//Terminus was designed by Dimitar Zhekov and GPL2, OFL
//

#include "hud.hpp"
#include "gl_functions.hpp"

#include "../shared/internal.hpp"
#include "../shared/printlog.hpp"

//#include <stdlib.h>
//#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <ode/ode.h>

//offset for vbo
#define BUFFER_OFFSET(i) ((char *)NULL + (i))



//
//HUD HACK, settings (for file "data/font.bmp"):
//
const char list[] = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:,;-_+!\"#%&/()=?\\<>|";
const int resx=12;
const int resy=24;
//
////////////
//


//store stuff
GLuint texture;
GLuint buffer;

float stride=32;
int font_count=1;

bool HUD_Load()
{
	printlog(0, "HUD HACK!: loading");

	//fonts:
	SDL_Surface *surface = SDL_LoadBMP("font.bmp");

	if (!surface)
	{
		printlog(0, "HUD HACK!: could not load font file!");
		return false;
	}

	printf("%s\n", list);
	//number of characters in font
	font_count=strlen(list);

	if (	(surface->w != resx*font_count)	||
		(surface->h != resy)		)
	{
		printf("HUD HACK!: wrong resolution, expects: %i x %i\n", resx*font_count, resy);
		return false;
	}

	GLint bpp = surface->format->BytesPerPixel;
	GLenum format;
        if (bpp == 4)
        {
                if (surface->format->Rmask == 0x000000ff)
                        format = GL_RGBA;
                else
                        format = GL_BGRA;
        }
	else if (bpp == 3)
        {
                if (surface->format->Rmask == 0x000000ff)
                        format = GL_RGB;
                else
                        format = GL_BGR;
        } else {
                printf("HUD HACK!: number of colours is %i?\n", bpp);
		return false;
        }

	stride=1.0/(float)font_count;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, bpp, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);

	//vbo buffer
	glGenBuffers(1, &buffer);

	return true;
}

void HUD_Render(const char string[])
{
	//configure rendering (disable some stuff not needed)
	glDisable(GL_LIGHTING);
	glShadeModel (GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	//set textures
	//glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);

	//create
	int l = strlen(string);
	float tmp [4*4*l];

	//
	int count=0;
	int posx=0;
	int posy=0;
	//

	for (int i=0; i<l; ++i)
	{
		if (string[i] == '\n')
		{
			posx=0;
			posy+=resy;
		}
		else
		{
			int c;
			for (c=0; list[c]&&list[c]!=string[i]; ++c); //find
			if (!list[c])
				printlog(0, "HUD HACK!: character not found (\"%c\"), add to font!\n", string[i]);
			else
			{
				//position
				tmp[count+2]=posx;
				tmp[count+3]=posy;
				tmp[count+6]=posx;
				tmp[count+7]=posy+resy;
				tmp[count+10]=posx+resx;
				tmp[count+11]=posy+resy;
				tmp[count+14]=posx+resx;
				tmp[count+15]=posy;

				//texture
				tmp[count+0]=stride*c;
				tmp[count+1]=0;
				tmp[count+4]=stride*c;
				tmp[count+5]=1;
				tmp[count+8]=stride*(c+1);
				tmp[count+9]=1;
				tmp[count+12]=stride*(c+1);
				tmp[count+13]=0;

				//move
				posx+=resx;
				count+=4*4;
			}
		}
	}

	//set/load vertices (vbo)
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*count, tmp, GL_STREAM_DRAW);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glTexCoordPointer(2, GL_FLOAT, sizeof(float)*4, BUFFER_OFFSET(sizeof(float)*0));

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(float)*4, BUFFER_OFFSET(sizeof(float)*2));

	//draw
	glDrawArrays(GL_QUADS, 0, count/4);

	//done
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
}


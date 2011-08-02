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

#include "../shared/internal.hpp"
#include "../shared/printlog.hpp"

//#include <stdlib.h>
//#include <string.h>
#include <SDL/SDL.h>
#include <ode/ode.h>
#include <GL/glew.h>

//offset for vbo
#define BUFFER_OFFSET(i) ((char *)NULL + (i))



//
//HUD HACK, settings (for file "data/font.bmp"):
//
//@ is not "@", but an arrow
const char list[] = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:,;-_+!\"#%&/()=?\\<>|@";
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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, bpp, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);

	//vbo buffer
	glGenBuffers(1, &buffer);

	return true;
}

void HUD_Render_Text(const char string[], int posx, int posy)
{
	//configure rendering (disable some stuff not needed)
	glDisable(GL_LIGHTING);
	glShadeModel (GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable (GL_FOG);

	//set textures
	//glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);

	//create
	int l = strlen(string);
	float tmp [4*4*l];

	//
	int count=0;
	int xorigin=posx;
	//

	for (int i=0; i<l; ++i)
	{
		if (string[i] == '\n')
		{
			posx=xorigin;
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

void HUD_Render_Graph(float (*function)(float x), int posx, int posy, int sizex, int sizey, float r, float g, float b)
{
	//configure rendering (disable some stuff not needed)
	glDisable(GL_LIGHTING);
	glShadeModel (GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable (GL_FOG);

	//build
	float tmp[5*(4+sizex)];
	//background
	tmp[0]=posx; tmp[1]=posy;		tmp[2]=0; tmp[3]=0; tmp[4]=0;
	tmp[5]=posx; tmp[6]=posy+sizey;		tmp[7]=0; tmp[8]=0; tmp[9]=0;
	tmp[10]=posx+sizex; tmp[11]=posy+sizey;	tmp[12]=0; tmp[13]=0; tmp[14]=0;
	tmp[15]=posx+sizex; tmp[16]=posy;	tmp[17]=0; tmp[18]=0; tmp[19]=0;

	//plot
	float x;
	for (int i=0; i<sizex; ++i)
	{
		x=((float)i+0.0)/(float)sizex;
		tmp[20+i*5]=0.5+posx+i; tmp[21+i*5]=0.5+posy+(sizey-1)*(1-function(x));
		tmp[22+i*5]=r; tmp[23+i*5]=g; tmp[24+i*5]=b;
	}

	//set/load vertices (vbo)
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*5*(4+sizex), tmp, GL_STREAM_DRAW);

	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(3, GL_FLOAT, sizeof(float)*5, BUFFER_OFFSET(sizeof(float)*2));

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(float)*5, BUFFER_OFFSET(sizeof(float)*0));

	//draw
	glDrawArrays(GL_QUADS, 0, 4);
	glDrawArrays(GL_LINE_STRIP, 4, sizex);

	//done
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

}

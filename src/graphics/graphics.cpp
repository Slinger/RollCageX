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

//handle drawing of 3d/2d accelerated graphics

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "../shared/internal.hpp"
#include "../shared/info.hpp"
#include "../shared/track.hpp"
#include "../shared/runlevel.hpp"
#include "../shared/threads.hpp"
#include "../shared/printlog.hpp"

#include "../shared/camera.hpp"
#include "gl_extensions.hpp"
#include "graphic_list.hpp"
#include "geom_render.hpp"

SDL_Surface *screen;
Uint32 flags = SDL_OPENGL | SDL_RESIZABLE;

//count frames
unsigned int graphics_count = 0;

//controls what to render:
bool render_models = true;
bool render_geoms = false;

//if multithreading, event thread will alert graphics thread about resizing events (to avoid stealing the context)
bool graphics_event_resize = false;
int graphics_event_resize_w, graphics_event_resize_h;
//

//needed by graphics_list:
float view_angle_rate_x=0.0;
float view_angle_rate_y=0.0;
//


void graphics_resize (int new_w, int new_h)
{
	screen = SDL_SetVideoMode (new_w, new_h, 0, flags);
	int w=screen->w;
	int h=screen->h;

	glViewport (0,0,w,h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();

	//lets calculate viewing angle based on the players _real_ viewing angle...
	//the player should specify an eye_distance in internal.conf
	//
	//(divide your screens resolution height or width with real height or
	//width, and multiply that with your eyes distance from the screen)
	//
	float angle;

	if (!internal.force) //good
	{
		//angle between w/2 (distance from center of screen to right edge) and players eye distance
		angle = atan( (((GLfloat) w)/2.0)/internal.dist );
		printlog(1, "(perspective: %f degrees, based on (your) eye distance: %f pixels", angle*180/M_PI, internal.dist);
	}
	else //bad...
	{
		printlog(1, "Angle forced to: %f degrees. And you are an evil person...", internal.angle);
		angle = (internal.angle * M_PI/180);;
	}

	//useful for more things:
	//x = rate*depth
	view_angle_rate_x = tan(angle);
	//y = rate*depth (calculated from ratio of window resolution aspect)
	view_angle_rate_y = view_angle_rate_x * ((GLfloat) h/w);
	//

	//x position at close clipping
	GLfloat x = internal.clipping[0] * view_angle_rate_x;
	//y -''- 
	GLfloat y = internal.clipping[0] * view_angle_rate_y;


	//set values
	glFrustum(-x, x, -y, y, internal.clipping[0], internal.clipping[1]);

	//switch back to usual matrix
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
}

bool graphics_init(void)
{
	printlog(0, "Initiating graphics");

	SDL_Init(SDL_INIT_VIDEO);
	screen = SDL_SetVideoMode (internal.res[0], internal.res[1], 0, flags);

	if (!screen)
	{
		printlog(0, "Error: couldn't set video mode");
		return false;
	}

	//title:
	char name[10+strlen(VERSION)+40+1];
	strcpy (name,"RollCageX ");
	strcat (name,VERSION);
	strcat (name," (C) 2009-2010 Mats Wahlberg (\"Slinger\")"); 
	SDL_WM_SetCaption (name, "RCX");

	//first of all, make sure we got all needed extansions:
	if (!Load_GL_Extensions())
	{
		printlog(0, "Sorry, your hardware and/or software is too old to support rcx (requires opengl 1.5 compatibility)!");
		return false;
	}

	//hide cursor
	SDL_ShowCursor (SDL_DISABLE);

	//toggle fullscreen (if requested)
	if (internal.fullscreen)
		if (!SDL_WM_ToggleFullScreen(screen))
			printlog(0, "Error: unable to toggle fullscreen");

	//set up window, as if resized
	graphics_resize (screen->w, screen->h);

	//everything ok
	return true;
}



int graphics_loop ()
{
	printlog(1, "Starting graphics loop");

	while (runlevel == running)
	{
		//make sure only render frame after it's been simulated
		//quckly lock mutex in order to listen to physics broadcasts
		if (internal.sync_graphics)
		{
			SDL_mutexP(sync_mutex);
			SDL_CondWaitTimeout (sync_cond, sync_mutex, 500); //if no signal in half a second, stop waiting
			SDL_mutexV(sync_mutex);
		}


		//in case event thread can't pump SDL events (limit of some OSes)
		SDL_mutexP(sdl_mutex);
		SDL_PumpEvents();

		//see if we need to resize
		if (graphics_event_resize)
		{
			screen = SDL_SetVideoMode (graphics_event_resize_w, graphics_event_resize_h, 0, flags);

			if (screen)
			{
				graphics_resize (screen->w, screen->h);
				graphics_event_resize = false;
			}
			else
				printlog(0, "Warning: resizing failed, will retry");
		}

		//done with sdl
		SDL_mutexV(sdl_mutex);

		//start rendering

		//clear screen
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//	glLoadIdentity();

		glPushMatrix();

			//move camera
			camera.Graphics_Step();

			//place sun
			glLightfv (GL_LIGHT0, GL_POSITION, track.position);

			//render models
			if (render_models)
				Graphic_List_Render();

			//render geoms
			if (render_geoms)
				Geom_Render();

		glPopMatrix();

		SDL_GL_SwapBuffers();

		//keep track of how many rendered frames
		++graphics_count;
	}

	//during rendering, memory might be allocated to use as buffers
	//(this will quickly be reallocated in each race and can be removed)
	Geom_Render_Clear();

	return 0;
}

void graphics_quit(void)
{
	printlog(1, "Quit graphics");
	SDL_Quit();
}


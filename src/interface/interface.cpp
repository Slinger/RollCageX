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

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "../shared/internal.hpp"
#include "../shared/info.hpp"
#include "../shared/track.hpp"
#include "../shared/runlevel.hpp"
#include "../shared/threads.hpp"
#include "../shared/printlog.hpp"
#include "../shared/profile.hpp"

#include "../shared/camera.hpp"
#include "render_lists.hpp"
#include "geom_render.hpp"

//hack
#include "hud.hpp"
float fps=0;
float fpscount=0;
float fpstime=0;
//

SDL_Surface *screen;
const Uint32 flags = SDL_OPENGL | SDL_RESIZABLE;

//TMP: keep track of demo spawn stuff
Object_Template *box = NULL;
Object_Template *sphere = NULL;
Object_Template *funbox = NULL;
Object_Template *molecule = NULL;

//count frames
unsigned int interface_count = 0;

//needed by graphics_buffers:
float view_angle_rate_x=0.0;
float view_angle_rate_y=0.0;
//

//remember if rendering background or not
bool background = true;
//

//hack
int width=0, height=0;

void Resize (int new_w, int new_h)
{
	screen = SDL_SetVideoMode (new_w, new_h, 0, flags);
	int w=screen->w;
	int h=screen->h;

	//tmp, hack!
	width=w;
	height=h;

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

	if (!internal.angle) //good
	{
		//angle between w/2 (distance from center of screen to right edge) and players eye distance
		angle = atan( (((GLfloat) w)/2.0)/internal.dist );
		printlog(1, "(perspective: %f degrees, based on (your) eye distance: %f pixels", angle*180/M_PI, internal.dist);
	}
	else //bad...
	{
		printlog(1, "Angle forced to: %f degrees. And you are an evil person...", internal.angle);
		angle = ( (internal.angle/2.0) * M_PI/180);;
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

bool Interface_Init(void)
{
	printlog(0, "Initiating interface");

	//initiate sdl
	SDL_Init (SDL_INIT_VIDEO);

	//set title:
	SDL_WM_SetCaption (TITLE, "RC");

	//TODO: set icon (SDL_WM_SetIcon, from embedded into the executable?)

	//configure properties before creating window
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1); //make sure double-buffering
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16); //good default (Z buffer)
	SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 0); //not used yet

	//try to create window
	//TODO: when SDL 1.3 is released, SDL_CreateWindow is deprecated in favor of:
	//SDL_CreateWindow and SDL_GL_CreateContext
	screen = SDL_SetVideoMode (internal.res[0], internal.res[1], 0, flags);

	if (!screen)
	{
		printlog(0, "Error: couldn't set video mode");
		return false;
	}

	//check if graphics is good enough
	if (glewInit() == GLEW_OK)
	{
		//not 1.5
		if (!GLEW_VERSION_1_5)
		{
			//should check ARB extensions if GL<1.5, but since this only affects old
			//systems (the 1.5 standard was released in 2003), I'll ignore it...
			printlog(0, "Error: you need GL 1.5 or later");
			return false;
		}
	}
	else
	{
		printlog(0, "Error: couldn't init glew");
		return false;
	}

	//hide cursor
	SDL_ShowCursor (SDL_DISABLE);

	//toggle fullscreen (if requested)
	if (internal.fullscreen)
		if (!SDL_WM_ToggleFullScreen(screen))
			printlog(0, "Error: unable to toggle fullscreen");

	//set up window, as if resized
	Resize (screen->w, screen->h);

	//hack
	if (!HUD_Load())
		return false;

	//
	//options
	//
	if (internal.culling)
		culling=true;

	//if positive and closer than far clipping
	if (internal.fog > 0.0 && internal.fog < internal.clipping[1])
	{
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogf(GL_FOG_START, internal.fog);
		glFogf(GL_FOG_END, internal.clipping[1]);
		fog=true;
	}

	//make sure everything is rendered with highest possible quality
	glHint(GL_FOG_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	//things possibly used in future:
	/*
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);
	glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
	*/

	//everything ok
	return true;
}



int Interface_Loop ()
{
	printlog(1, "Starting interface loop");

	//just make sure not rendering geoms yet
	geom_render_level = 0;

	//store current time
	Uint32 time_old = SDL_GetTicks();

	//only stop render if done with race
	while (runlevel != done)
	{
		//make sure only render frame after it's been simulated
		//quckly lock mutex in order to listen to physics broadcasts
		if (internal.sync_interface)
		{
			SDL_mutexP(sync_mutex);
			SDL_CondWaitTimeout (sync_cond, sync_mutex, 500); //if no signal in half a second, stop waiting
			SDL_mutexV(sync_mutex);
		}

		//get time
		Uint32 time = SDL_GetTicks();
		//how much it differs from old
		Uint32 delta = time-time_old;
		time_old = time;
		//

		//current car
		Car *car = profile_head->car;

		//check all sdl events
		SDL_Event event;
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode (event.resize.w, event.resize.h, 0, flags);

					if (screen)
						Resize (screen->w, screen->h);
					else
						printlog(0, "Warning: resizing failed");
				break;

				case SDL_QUIT:
					runlevel = done;
				break;

				case SDL_ACTIVEEVENT:
					if (event.active.gain == 0)
						printlog(1, "(FIXME: pause when losing focus (or being iconified)!)");
				break;

				//check for special key presses (debug/demo keys)
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							runlevel = done;
						break;

						//box spawning
						case SDLK_F5:
							SDL_mutexP(ode_mutex);
							box->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//sphere spawning
						case SDLK_F6:
							SDL_mutexP(ode_mutex);
							sphere->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//spawn funbox
						case SDLK_F7:
							SDL_mutexP(ode_mutex);
							funbox->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//molecule
						case SDLK_F8:
							SDL_mutexP(ode_mutex);
							molecule->Spawn (0,0,10);
							SDL_mutexV(ode_mutex);
						break;

						//switch car
						case SDLK_F9:
							//not null
							if (car)
							{
								//next in list
								car = car->next;
								//in case at end of list, go to head
								if (!car)
									car = Car::head;

								//set new car
								profile_head->car = car;
								camera.Set_Car(car);
							}
						break;

						//respawn car
						case SDLK_F10:
							if (car)
							{
								SDL_mutexP(ode_mutex);
								car->Respawn(track.start[0], track.start[1], track.start[2]);
								SDL_mutexV(ode_mutex);
							}
						break;

						//paus physics
						case SDLK_F11:
							if (runlevel == paused)
								runlevel = running;
							else
								runlevel = paused;
						break;

						//switch what to render
						case SDLK_F12:
							//increase geom rendering level, and reset if at last
							if (++geom_render_level >= 6)
								geom_render_level = 0;
						break;

						default:
							break;
					}
				break;

				default:
					break;
			}
		}

		//(tmp) camera movement keys:
		Uint8 *keys = SDL_GetKeyState(NULL);

		if (keys[SDLK_d]) //+x
			camera.Move(+0.03*delta, 0, 0);
		if (keys[SDLK_a]) //-x
			camera.Move(-0.03*delta, 0, 0);

		if (keys[SDLK_w]) //+y
			camera.Move(0, +0.03*delta, 0);
		if (keys[SDLK_s]) //-y
			camera.Move(0, -0.03*delta, 0);

		if (keys[SDLK_q]) //+z
			camera.Move(0, 0, +0.03*delta);
		if (keys[SDLK_e]) //-z
			camera.Move(0, 0, -0.03*delta);
		//

		//car control
		if (runlevel == running)
			Profile_Events_Step(delta);



		//done with sdl
		SDL_mutexV(sdl_mutex);

		//start rendering

		//check if now switching to geom rendering with no background...
		if (geom_render_level == 5 && background)
		{
			//black background
			glClearColor (0.0, 0.0, 0.0, 1.0);
			background = false;
		}
		//if now switching from geom rendering...
		else if (!background && geom_render_level != 5)
		{
			//restore track specified background
			glClearColor (track.sky[0],track.sky[1],track.sky[2],1.0);
			background = true;
		}

		//clear screen
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//move camera
		camera.Graphics_Step();

		//place sun
		glLightfv (GL_LIGHT0, GL_POSITION, track.position);

		//render models (if not 2 or more level of geom rendering)
		if (geom_render_level < 3)
			Render_List_Render();

		//render geoms (if nonzero level)
		if (geom_render_level)
			Geom_Render();

		//
		//hack!
		//
		//set projection (1:1 mapping)
		glMatrixMode (GL_PROJECTION);
		glPushMatrix(); //copy projection matrix
		glLoadIdentity(); //reset
		glOrtho (0, width, height, 0, 0, 1); //mapping = resolution
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity();

		char string[1000];
		//snprintf(string, 1000, "ABC - a is for apple, or all mighty pi\nb is for banana\nand c is for carbon fibre\n\npi=%f\n\n # fonts are working (including some special characters!) # ", M_PI);
		fpstime+=delta/1000.0;
		fpscount+=1;
		if (fpstime >= 0.5)
		{
			fps=fpscount/fpstime;
			fpstime=0;
			fpscount=0;
		}
		snprintf(string, 1000, "velocity: %.0fkm/h\nfps: %.0f", camera.car->velocity*3.6, fps);
		HUD_Render(string);

		glMatrixMode (GL_PROJECTION);
		glPopMatrix(); //return to old projection
		glMatrixMode (GL_MODELVIEW);
		//
		//
		//

		//swap the 2 gl buffers
		SDL_GL_SwapBuffers();

		//keep track of how many rendered frames
		++interface_count;
	}

	//during rendering, memory might be allocated to use as buffers
	//(this will quickly be reallocated in each race and can be removed)
	Geom_Render_Clear();

	return 0;
}

void Interface_Quit(void)
{
	printlog(1, "Quit interface");
	SDL_Quit();
}


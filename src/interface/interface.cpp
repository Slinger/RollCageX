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
#include <SDL/SDL_opengl.h>

#include "../shared/internal.hpp"
#include "../shared/info.hpp"
#include "../shared/track.hpp"
#include "../shared/runlevel.hpp"
#include "../shared/threads.hpp"
#include "../shared/printlog.hpp"
#include "../shared/profile.hpp"

#include "../shared/camera.hpp"
#include "gl_extensions.hpp"
#include "graphic_buffers.hpp"
#include "geom_render.hpp"

SDL_Surface *screen;
Uint32 flags = SDL_OPENGL | SDL_RESIZABLE;

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

void Resize (int new_w, int new_h)
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
	Resize (screen->w, screen->h);

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

		glPushMatrix();

			//move camera
			camera.Graphics_Step();

			//place sun
			glLightfv (GL_LIGHT0, GL_POSITION, track.position);

			//render models (if not 2 or more level of geom rendering)
			if (geom_render_level < 3)
				Graphic_List_Render();

			//render geoms (if nonzero level)
			if (geom_render_level)
				Geom_Render();

		glPopMatrix();

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


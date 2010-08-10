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

//handle events, both real events like window resizing, termination and
//keyboard, but also respond to simulations (usually when objects collides
//and they are meant to respond to collisions - like building destructions...)

#include <SDL/SDL.h>
#include <ode/ode.h>
#include "../shared/threads.hpp"
#include "../shared/internal.hpp"
#include "../shared/runlevel.hpp"
#include "../shared/printlog.hpp"
#include "../shared/geom.hpp"
#include "../shared/profile.hpp"
#include "../shared/joint.hpp"
#include "../shared/camera.hpp"
#include "timers.hpp"


unsigned int events_count = 0;

//TMP: keep track of demo spawn stuff
Object_Template *box = NULL;
Object_Template *sphere = NULL;
Object_Template *funbox = NULL;
Object_Template *molecule = NULL;


int events_loop (void *d)
{
	printlog(1, "Starting event loop");

	SDL_Event event;
	Uint32 time, time_old, delta;
	time_old = SDL_GetTicks();

	while (runlevel != done)
	{
		//if syncing, sleep until physics signals
		if (internal.sync_events)
		{
			SDL_mutexP(sync_mutex);
			SDL_CondWaitTimeout (sync_cond, sync_mutex, 500); //if no signal in half a second, stop waiting
			SDL_mutexV(sync_mutex);
		}

		//wait for permission for ode (in case some event causes ode manipulation)
		SDL_mutexP(ode_mutex);

		time = SDL_GetTicks();
		delta = time-time_old;

		//process events (if running)
		if (runlevel == running)
		{
			//TODO: runlevel = locked;
			Geom::TMP_Events_Step(delta);
			Joint::TMP_Events_Step(delta);
			Body::TMP_Events_Step(delta);

			Object::Events_Step(); //remove inactive objects

			//timers
			Animation_Timer::Events_Step(delta);

			//TODO: runlevel = normal
		}

		//current car
		Car *car = profile_head->car;

		//get SDL events
		SDL_mutexP(sdl_mutex); //make sure not colliding with other threads

		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_VIDEORESIZE:
					graphics_event_resize_w = event.resize.w;
					graphics_event_resize_h = event.resize.h;
					graphics_event_resize = true;
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
							box->Spawn (0,0,10);
						break;

						//sphere spawning
						case SDLK_F6:
							sphere->Spawn (0,0,10);
						break;

						//spawn funbox
						case SDLK_F7:
							funbox->Spawn (0,0,10);
						break;

						//molecule
						case SDLK_F8:
							molecule->Spawn (0,0,10);
						break;

						//paus physics
						case SDLK_F9:
							if (runlevel == paused)
								runlevel = running;
							else
								runlevel = paused;
						break;

						//switch car
						case SDLK_F10:
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

						//switch what to render
						case SDLK_F11:
							if (render_models && !render_geoms)
							{
								printlog(1, "rendering models and geoms");
								render_geoms = true;
							}
							else if (render_models && render_geoms)
							{
								printlog(1, "rendering only geoms");
								render_models = false;
							}
							else //!models && geoms
							{
								printlog(1, "rendering only models");
								render_models = true;
								render_geoms = false;
							}
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
			camera.Move(+(delta*0.03), 0, 0);
		if (keys[SDLK_a]) //-x
			camera.Move(-(delta*0.03), 0, 0);

		if (keys[SDLK_w]) //+y
			camera.Move(0, +(delta*0.03), 0);
		if (keys[SDLK_s]) //-y
			camera.Move(0, -(delta*0.03), 0);

		if (keys[SDLK_q]) //+z
			camera.Move(0, 0, +(delta*0.03));
		if (keys[SDLK_e]) //-z
			camera.Move(0, 0, -(delta*0.03));
		//

		//car control
		if (runlevel == running)
			Profile_Events_Step(delta);


		//unlock sdl access
		SDL_mutexV(sdl_mutex);


		//done
		SDL_mutexV(ode_mutex);

		++events_count;
		time_old = time;
	}
	return 0;
}


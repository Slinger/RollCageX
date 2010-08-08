/* RollCageX (sci-fi racer inspired by the original RollCage games)
 * Copyright (C) 2009-2010  Mats Wahlberg ("Slinger" on gorcx.net forum)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

//Required stuff:
#include <SDL.h>

//local stuff:
#include "shared/info.hpp"
#include "shared/internal.hpp"
#include "shared/threads.hpp"
#include "shared/printlog.hpp"
#include "shared/runlevel.hpp"
#include "shared/profile.hpp"
#include "shared/track.hpp"
#include "shared/trimesh.hpp"

Uint32 starttime = 0;
Uint32 racetime = 0;
Uint32 simtime = 0; 

//this function is just until proper menus
bool select_and_load_race(Profile *prof)
{
	//MENU: select race type
	// - assuming 1P free roam -

	//MENU: select world/track
	if (!load_track(internal.usr_track))
		return false; //GOTO: track selection menu

	//MENU: P1: select team/car
	Car_Template *car_template = Car_Template::Load(internal.usr_car);
	if (!car_template)
		return false; //GOTO: car selection menu

	//models for rim and tyre
	Trimesh_3D *tyre, *rim;

	//MENU: P1: select {track,world}/tyre
	tyre = Trimesh_3D::Quick_Load_Conf(internal.usr_tyre, "tyre.conf");

	//MENU: P1: select {car,team}/rim
	rim = Trimesh_3D::Quick_Load_Conf(internal.usr_rim, "rim.conf");

	//TMP: load box for online spawning
	if (	!(box = Object_Template::Load("data/objects/misc/box"))		||
		!(sphere = Object_Template::Load("data/objects/misc/beachball"))||
		!(funbox = Object_Template::Load("data/objects/misc/funbox"))	||
		!(molecule = Object_Template::Load("data/objects/misc/NH4"))	)
		return false;

	//spawn car
	Car *player_car = car_template->Spawn(track.start[0], track.start[1], track.start[2],  tyre, rim);
	prof->car = player_car;
	camera.Set_Car(player_car);

	//ok!
	return true;
}

Uint32 start_time = 0;
void Run_Race(void)
{
	//start
	printlog (0, "Starting Race");

	ode_mutex = SDL_CreateMutex(); //create mutex for ode locking
	sdl_mutex = SDL_CreateMutex(); //only use sdl in 1 thread

	sync_mutex = SDL_CreateMutex();
	sync_cond = SDL_CreateCond();

	runlevel  = running;

	starttime = SDL_GetTicks(); //how long it took for race to start

	//launch threads
	SDL_Thread *physics = SDL_CreateThread (physics_loop, NULL);
	SDL_Thread *events = SDL_CreateThread (events_loop, NULL);
	graphics_loop(); //we already got opengl context in main thread

	//wait for threads
	SDL_WaitThread (events, NULL);
	SDL_WaitThread (physics, NULL);

	//cleanup
	SDL_DestroyMutex(ode_mutex);
	SDL_DestroyMutex(sdl_mutex);
	SDL_DestroyMutex(sync_mutex);
	SDL_DestroyCond(sync_cond);

	//done!
	printlog(0, "Race Done!");

	racetime = SDL_GetTicks() - starttime;
	simtime = physics_time - starttime;
}

//simple demo:
int main (int argc, char *argv[])
{
	//issue
	printf("\n     -=[ Hello, and welcome to RollCageX version %s ]=-\n\n%s\n", VERSION, ISSUE);
	//end

	if (argc != 1)
		printf("(Passing arguments - not supported)\n\n");

	//check if program was called with another pwd (got '/' in "name")
	if (char *s = strrchr(argv[0], '/'))
	{
		*s='\0'; //modify string to end at last slash
		printf("(changing pwd: %s)\n", argv[0]);
		chdir (argv[0]);
	}

	printlog(0, "Loading...\n");

	load_conf ("data/internal.conf", (char *)&internal, internal_index);

	if (!graphics_init())
		return false;

	//TODO: there should be menus here, but menu/osd system is not implemented yet... also:
	//on failure, rcx should not just terminate but instead abort the race and warn the user
	
	//MENU: welcome to rcx, please select profile or create a new profile
	Profile *prof = Profile_Load (internal.usr_profile);
	if (!prof)
		return -1; //GOTO: profile menu

	if (!physics_init())
	{
		//menu: warn and quit!
		graphics_quit();
		return -1;
	}


	//tmp: instead of menus, just load some defaults...
	if (!select_and_load_race(prof))
	{
		printlog(0, "One or more errors, can not start!");
		return -1; //just quit if failure
	}

	//MENU: race configured, start?
	Run_Race();

	//race done, remove all objects...
	Object::Destroy_All();

	//MENU: race done, replay, play again, quit?
	// - assuming quit -
	
	//remove loaded data (not all data, only "racetime" - for this race)
	Racetime_Data::Destroy_All();

	//MENU: back to main menu here
	// - assuming player wants to log out -
	physics_quit();
	Profile_Remove_All(); //should only be one active profile right now, but any case, remove all

	//MENU: select profile
	// - assumes player wants to quit -
	graphics_quit();
	
	//might be interesting
	printlog(1, "\n\n   <[ Info ]>");
	printlog(1, "Startup time:		%ums", starttime);
	printlog(1, "Race time:			%ums", racetime);

	printlog(1, "Simulated time:		%ums (%u%% of real time)",
						simtime, (100*simtime)/racetime);

	printlog(1, "Avarage physics/second:	%u steps (%u in total)",
						(1000*physics_count)/racetime, physics_count);

	printlog(1, "Physics lag:			%u%% of steps (%u in total)",
						(100*physics_lag)/physics_count, physics_lag);

	printlog(1, "Avarage graphics/second:	%u steps (FPS) (%u%% of physics steps)",
						(1000*graphics_count)/racetime, (100*graphics_count)/physics_count);

	printlog(1, "Avarage events/second:	%u steps (%u in total, %u%% of physics steps)",
						(1000*events_count)/racetime, events_count, (100*events_count)/physics_count);

	printf("\nBye!\n\n");

	return 0;
}


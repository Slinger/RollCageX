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

//
//tmp:
//
#include "loaders/text_file.hpp"

//instead of menus...
//try to load "data/tmp menu selections" for menu simulation
//what we do is try to open this file, and then try to find menu selections in it
//note: if selections are not found, will still fall back on safe defaults
bool tmp_menus()
{
	//initiate graphics
	if (!graphics_init())
		return false;

	std::string sprofile, sworld, strack, steam, scar, sdiameter, styre, srim; //easy text manipulation...
	Text_File file; //for parsing
	file.Open("data/tmp menu selections"); //just assume it opens...

	//MENU: welcome to rcx, please select profile or create a new profile
	sprofile = "data/profiles/";
	if (file.Read_Line() && file.word_count == 2 && !strcmp(file.words[0], "profile"))
		sprofile += file.words[1];
	else
		sprofile += "default";

	Profile *prof = Profile_Load (sprofile.c_str());
	if (!prof)
		return false; //GOTO: profile menu


	//initiate physics
	if (!physics_init())
	{
		//menu: warn and quit!
		graphics_quit();
		return false;
	}


	//MENU: select race type...

	//MENU: select world
	sworld = "data/worlds/";
	if (file.Read_Line() && file.word_count == 2 && !strcmp(file.words[0], "world"))
		sworld+=file.words[1];
	else
		sworld+="Sandbox";

	//MENU: select track
	strack = sworld;
	strack += "/tracks/";
	if (file.Read_Line() && file.word_count == 2 && !strcmp(file.words[0], "track"))
		strack+=file.words[1];
	else
		strack+="Box";

	//TODO: probably Racetime_Data::Destroy_All() here
	if (!load_track(strack.c_str()))
		return false; //GOTO: track selection menu

	//TMP: load some objects for online spawning
	if (	!(box = Object_Template::Load("data/objects/misc/box"))		||
		!(sphere = Object_Template::Load("data/objects/misc/beachball"))||
		!(funbox = Object_Template::Load("data/objects/misc/funbox"))	||
		!(molecule = Object_Template::Load("data/objects/misc/NH4"))	)
		return false;
	//

	//MENU: players, please select team/car

	Car_Template *car_template = NULL;
	Car *car = NULL;
	//models for rim and tyre
	Trimesh_3D *tyre = NULL, *rim = NULL;

	while (1)
	{
		//no more data in file...
		if (!file.Read_Line())
		{
			//lets make sure we got at least one car here...
			if (!car)
			{
				//no, load some defaults...
				if (!car_template)
					car_template = Car_Template::Load("data/teams/Nemesis/cars/Venom");

				//if above failed
				if (!car_template)
					return false;

				//try to load tyre and rim (if possible)
				if (!tyre)
					tyre = Trimesh_3D::Quick_Load_Conf("data/worlds/Sandbox/tyres/2/Slick", "tyre.conf");
				if (!rim)
					rim = Trimesh_3D::Quick_Load_Conf("data/teams/Nemesis/rims/2/Split", "rim.conf");
				//good, spawn
				car = car_template->Spawn(
					track.start[0], //x
					track.start[1], //y
					track.start[2], //z
					tyre, rim);
			}

			//then break this loop...
			break;
		}

		//team selected in menu
		if (file.word_count == 2 && !strcmp(file.words[0], "team"))
		{
			steam = "data/teams/";
			steam += file.words[1];
		}
		//car selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "car"))
		{
			scar = steam;
			scar += "/cars/";
			scar += file.words[1];

			if (! (car_template = Car_Template::Load(scar.c_str())) )
				return false;

		}
		//tyre diameter selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "diameter"))
		{
			sdiameter = file.words[1];
		}
		//tyre selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "tyre"))
		{
			//try loading from world tyres
			styre = sworld;
			styre += "/tyres/";
			styre += sdiameter;
			styre += "/";
			styre += file.words[1];

			//if failure...
			if (! (tyre = Trimesh_3D::Quick_Load_Conf(styre.c_str(), "tyre.conf")) )
			{
				//try seing if its track specific tyre
				styre = strack;
				styre += "/tyres/";
				styre += sdiameter;
				styre += "/";
				styre += file.words[1];

				tyre = Trimesh_3D::Quick_Load_Conf(styre.c_str(), "tyre.conf");
			}
		}
		//rim selected in menu
		else if (file.word_count == 2 && !strcmp(file.words[0], "rim"))
		{
			//try loading from team rims
			srim = steam;
			srim += "/rims/";
			srim += sdiameter;
			srim += "/";
			srim += file.words[1];

			//if failure...
			if (! (rim = Trimesh_3D::Quick_Load_Conf(srim.c_str(), "rim.conf")) )
			{
				//try seing if car specific

				srim = scar;
				srim += "/rims/";
				srim += sdiameter;
				srim += "/";
				srim += file.words[1];

				//don't car if fails...
				rim = Trimesh_3D::Quick_Load_Conf(srim.c_str(), "rim.conf");
			}
		}
		//manual position required for spawning
		else if (file.word_count == 3 && !strcmp(file.words[0], "position"))
		{
			if (!car_template)
			{
				printlog(0, "ERROR in menu: specify car before position!");
				return false;
			}

			car = car_template->Spawn(
					(track.start[0]+atof(file.words[1])), //x
					(track.start[1]+atof(file.words[2])), //y
					(track.start[2]), //z
					tyre, rim);
		}
	}

	//this single player/profile controls all cars for now... and ladt one by default
	prof->car = car;
	camera.Set_Car(car);

	//MENU: race configured, start? yes!
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

	return true;
}

//
//
//


//main function, will change a lot in future versions...
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

	//
	//TODO: there should be menus here, but menu/osd system is not implemented yet... also:
	//on failure, rcx should not just terminate but instead abort the race and warn the user
	if (!tmp_menus())
	{
		printlog(0, "One or more errors, can not start!");
		return -1; //just quit if failure
	}
	//

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


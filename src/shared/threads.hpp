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

#ifndef _RC_THREADS_H
#define _RC_THREADS_H

#include <SDL/SDL_mutex.h>

//mutexes
extern SDL_mutex *ode_mutex;
extern SDL_mutex *sdl_mutex;
extern SDL_mutex *sync_mutex;
extern SDL_cond  *sync_cond;
//


//prototypes for communication/statistic variables
//simulation:
extern unsigned int simulation_lag;
extern unsigned int simulation_count;
extern Uint32 simulation_time;

//interface:
extern unsigned int interface_count;
extern bool render_models;
extern bool render_geoms;

//functions
bool Interface_Init(void);
void Interface_Quit(void);
bool Simulation_Init(void);
void Simulation_Quit (void);

int Interface_Loop (void);
int Simulation_Loop (void *d);



//TMP: used for keeping track for objects spawning
#include "object.hpp"
extern Object_Template *box;
extern Object_Template *sphere;
extern Object_Template *funbox;
extern Object_Template *molecule;

#endif

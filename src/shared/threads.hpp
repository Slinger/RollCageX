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

#ifndef _RCX_THREADS_H
#define _RCX_THREADS_H

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

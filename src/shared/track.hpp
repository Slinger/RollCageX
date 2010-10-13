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

#ifndef _RCX_TRACK_H
#define _RCX_TRACK_H
#include "../loaders/conf.hpp"
#include "object.hpp"
#include <SDL/SDL_opengl.h>
#include <ode/ode.h>
//track: the main "world", contains simulation and rendering data for one
//large 3D file for the rigid environment, and more simulation data (like
//gravity)

extern dWorldID world;
extern dSpaceID space;
extern dJointGroupID contactgroup;

//Allocated at start
//(in contrary to the other structs, this is actually not allocated on runtime!)
extern struct Track_Struct {
	//placeholder for stuff like if it's raining/snowing and lightsources
	float sky[3]; //RGB, alpha is always 1.0f

	float ambient[4];
	float diffuse[4];
	float specular[4];
	float position[4]; //light position
	
	dReal gravity;

	dReal density; //for air drag (friction)
	dReal wind[3];

	dReal start[3];
	float cam_start[3];
	float focus_start[3];

	dReal respawn;

	Object *object;
	Space *space;
} track;
//index:

const struct Track_Struct track_defaults = {
	{0.5,0.8,0.8},
	{0.0,0.0,0.0},
	{1.0,1.0,1.0},
	{1.0,1.0,1.0},
	{-1.0,0.5,1.0,0.0},
	9.82,
	1.29,
	{0.5,1.0,0.0},
	{0,-20,1.5},
	{15,8,15},
	{0,0,0},
	0.0};

const struct Conf_Index track_index[] = {
	{"sky",		'f',3,	offsetof(Track_Struct, sky[0])},
	{"ambient",	'f',3,	offsetof(Track_Struct, ambient[0])},
	{"diffuse",	'f',3,	offsetof(Track_Struct, diffuse[0])},
	{"specular",	'f',3,	offsetof(Track_Struct, specular[0])},
	{"position",	'f',4,	offsetof(Track_Struct, position[0])},
	{"gravity",	'R',1,	offsetof(Track_Struct, gravity)},
	{"density",	'R',1,	offsetof(Track_Struct, density)},
	{"wind",	'R',3,	offsetof(Track_Struct, wind)},
	{"start",	'R',3,	offsetof(Track_Struct, start)},
	{"cam_start",	'f',3,	offsetof(Track_Struct, cam_start)},
	{"focus_start",	'f',3,	offsetof(Track_Struct, focus_start)},
	{"respawn",	'R',1,	offsetof(Track_Struct, respawn)},
	{"",0,0}};//end

bool load_track (const char *path);

#endif

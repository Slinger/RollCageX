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

#ifndef _RCX_INTERNAL_H
#define _RCX_INTERNAL_H
#include "../loaders/conf.hpp"
#include <stddef.h>
#include <ode/ode.h>

//important system configuration variables
extern struct internal_struct {
	int verbosity;

	//for multithreading
	bool sync_physics, sync_graphics, sync_events;

	//physics
	dReal stepsize;
	int iterations;
	int multiplier;
	int contact_points;
	//bool finite_rotation;
	dReal scale; //TODO
	dReal max_wheel_rotation;
	dReal rim_angle;
	dReal erp,cfm;
	dReal mu,slip,bounce;
	dReal linear_drag, angular_drag;

	dReal dis_linear, dis_angular, dis_time;
	int dis_steps;


	//graphics
	int res[2]; //resolution
	int vbo_size;
	float dist;
	bool force;
	float angle;
	float clipping[2];
	bool fullscreen;
	bool culling;
	

	//TMP: menu selections
	Conf_String usr_profile,usr_track,usr_car,usr_tyre,usr_rim;
} internal;

const struct internal_struct internal_defaults = {
	1, //verbosity 1 until cheanged
	true,true,true,
	0.01,
	5,
	4,
	20,
	0.1,
	20,
	0.6,
	0.8, 0.01,
	1.0,1.0,0.0,
	5,5,
	0.05,0.10,0.5,1,
	//graphics
	{1200,800},
	16000000,
	2800.0,
	false,
	60.0,
	{1.0, 1000.0},
	false,
	true,
	//TMP menu:
	"data/profiles/default",
	"data/worlds/Sandbox/tracks/Box",
	"data/teams/Nemesis/cars/Venom",
	"data/worlds/Sandbox/tyres/2/Slick.obj",
	"data/teams/Nemesis/rims/2/Split.obj"};

const struct Conf_Index internal_index[] = {
	{"verbosity",		'i',1, offsetof(struct internal_struct, verbosity)},
	{"sync_physics",	'b',1, offsetof(struct internal_struct, sync_physics)},
	{"sync_graphics",	'b',1, offsetof(struct internal_struct, sync_graphics)},
	{"sync_events",		'b',1, offsetof(struct internal_struct, sync_events)},
	{"stepsize",		'R',1, offsetof(struct internal_struct, stepsize)},
	{"iterations",		'i',1, offsetof(struct internal_struct, iterations)},
	{"multiplier",		'i',1, offsetof(struct internal_struct, multiplier)},
	{"contact_points",	'i',1, offsetof(struct internal_struct, contact_points)},
	//{"finite_rotation",	'b',1, offsetof(struct internal_struct, finite_rotation)},
	//TODO: SCALE
	{"max_wheel_rotation",	'R',1, offsetof(struct internal_struct, max_wheel_rotation)},
	{"rim_angle",		'R',1, offsetof(struct internal_struct, rim_angle)},
	{"erp",			'R',1, offsetof(struct internal_struct, erp)},
	{"cfm",			'R',1, offsetof(struct internal_struct, cfm)},
	{"default_mu",		'R',1, offsetof(struct internal_struct, mu)},
	{"default_slip",	'R',1, offsetof(struct internal_struct, slip)},
	{"default_bounce",	'R',1, offsetof(struct internal_struct, bounce)},
	{"default_linear_drag",	'R',1, offsetof(struct internal_struct, linear_drag)},
	{"default_angular_drag",'R',1, offsetof(struct internal_struct, angular_drag)},
	{"auto_disable_linear",	'R',1, offsetof(struct internal_struct, dis_linear)},
	{"auto_disable_angular",'R',1, offsetof(struct internal_struct, dis_angular)},
	{"auto_disable_time",	'R',1, offsetof(struct internal_struct, dis_time)},
	{"auto_disable_steps",	'i',1, offsetof(struct internal_struct, dis_steps)},
	//graphics
	{"resolution",		'i',2, offsetof(struct internal_struct, res)},
	{"VBO_size",		'i',1, offsetof(struct internal_struct, vbo_size)},
	{"eye_distance",	'f',1, offsetof(struct internal_struct, dist)},
	{"force_angle",		'b',1, offsetof(struct internal_struct, force)},
	{"view_angle",		'f',1, offsetof(struct internal_struct, angle)},
	{"fullscreen",		'b',1, offsetof(struct internal_struct, fullscreen)},
	{"backface_culling",	'b',1, offsetof(struct internal_struct, culling)},
	{"clipping",		'f',2, offsetof(struct internal_struct, clipping)},

	//TMP:
	{"TMP:profile",		's',1, offsetof(struct internal_struct, usr_profile)},
	{"TMP:track",		's',1, offsetof(struct internal_struct, usr_track)},
	{"TMP:car",		's',1, offsetof(struct internal_struct, usr_car)},
	{"TMP:tyre",		's',1, offsetof(struct internal_struct, usr_tyre)},
	{"TMP:rim",		's',1, offsetof(struct internal_struct, usr_rim)},
	{"",0,0}};


#endif

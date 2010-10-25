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
	bool sync_simulation, sync_interface;
	bool spinning;

	//physics
	dReal stepsize;
	int iterations;
	int multiplier;
	int contact_points;
	dReal collision_erp,collision_cfm;
	dReal joint_erp,joint_cfm;
	dReal linear_drag, angular_drag;

	dReal dis_linear, dis_angular, dis_time;
	int dis_steps;

	int hash_levels[2];

	bool temporal_coherence;

	//graphics
	int res[2]; //resolution
	int vbo_size;
	float dist;
	bool force;
	float angle;
	float clipping[2];
	bool fullscreen;
	bool culling;
} internal;

const struct internal_struct internal_defaults = {
	1, //verbosity 1 until cheanged
	true,true,
	false,
	0.01,
	5,
	4,
	20,
	0.8, 0.00001,
	0.8, 0.00001,
	5.0,5.0,
	0.05,0.10,0.5,
	1,
	{-1,4},
	true,
	//graphics
	{1200,800},
	16000000,
	2800.0,
	false,
	60.0,
	{1.0, 1000.0},
	false,
	true};

const struct Conf_Index internal_index[] = {
	{"verbosity",		'i',1, offsetof(struct internal_struct, verbosity)},
	{"sync_simulation",	'b',1, offsetof(struct internal_struct, sync_simulation)},
	{"sync_interface",	'b',1, offsetof(struct internal_struct, sync_interface)},
	{"spinning",		'b',1, offsetof(struct internal_struct, spinning)},

	//physics
	{"stepsize",		'R',1, offsetof(struct internal_struct, stepsize)},
	{"iterations",		'i',1, offsetof(struct internal_struct, iterations)},
	{"multiplier",		'i',1, offsetof(struct internal_struct, multiplier)},
	{"contact_points",	'i',1, offsetof(struct internal_struct, contact_points)},
	{"collision_erp",	'R',1, offsetof(struct internal_struct, collision_erp)},
	{"collision_cfm",	'R',1, offsetof(struct internal_struct, collision_cfm)},
	{"joint_erp",		'R',1, offsetof(struct internal_struct, joint_erp)},
	{"joint_cfm",		'R',1, offsetof(struct internal_struct, joint_cfm)},
	{"default_linear_drag",	'R',1, offsetof(struct internal_struct, linear_drag)},
	{"default_angular_drag",'R',1, offsetof(struct internal_struct, angular_drag)},
	{"auto_disable_linear",	'R',1, offsetof(struct internal_struct, dis_linear)},
	{"auto_disable_angular",'R',1, offsetof(struct internal_struct, dis_angular)},
	{"auto_disable_time",	'R',1, offsetof(struct internal_struct, dis_time)},
	{"auto_disable_steps",	'i',1, offsetof(struct internal_struct, dis_steps)},
	{"hash_levels",		'i',2, offsetof(struct internal_struct, hash_levels)},
	{"temporal_coherence",	'b',1, offsetof(struct internal_struct, temporal_coherence)},

	//graphics
	{"resolution",		'i',2, offsetof(struct internal_struct, res)},
	{"VBO_size",		'i',1, offsetof(struct internal_struct, vbo_size)},
	{"eye_distance",	'f',1, offsetof(struct internal_struct, dist)},
	{"force_angle",		'b',1, offsetof(struct internal_struct, force)},
	{"view_angle",		'f',1, offsetof(struct internal_struct, angle)},
	{"fullscreen",		'b',1, offsetof(struct internal_struct, fullscreen)},
	{"backface_culling",	'b',1, offsetof(struct internal_struct, culling)},
	{"clipping",		'f',2, offsetof(struct internal_struct, clipping)},

	{"",0,0}};


#endif

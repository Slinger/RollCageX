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

#ifndef _RCX_PROFILE_H
#define _RCX_PROFILE_H

#include "SDL/SDL_keyboard.h"
#define UNUSED_KEY SDLK_QUESTION //key that's not used during race ("safe" default)

#include "car.hpp"
#include "camera.hpp"
#include "joint.hpp"

//profile: stores the user's settings (including key list)
struct Profile {
	//the car the user is controlling
	Car *car;

	//settings (loaded from conf)
	dReal steer_speed;
	dReal steer_max;
	dReal throttle_speed;

	//keys (loaded from keys.lst)
	SDLKey up;
	SDLKey down;
	SDLKey right;
	SDLKey left;
	SDLKey drift_break;

	struct Camera_Settings cam[4];
	int camera;
	SDLKey cam1;
	SDLKey cam2;
	SDLKey cam3;
	SDLKey cam4;

	//for list
	Profile *prev,*next;
};

extern Profile *profile_head;

const Profile profile_defaults = {
	NULL, //car
	//steering
	0.2,
	30.0,
	1.0,
	//control
	SDLK_UP,
	SDLK_DOWN,
	SDLK_RIGHT,
	SDLK_LEFT,
	SDLK_SPACE,
	//camera settings:
	{
	//1:
	{{0,5,0.5},
	{0,3.4,0.5}, {0,0,0},
	true,
	0, 0,
	0,
	0,
	0,
	true,
	0,
	false, false,
	0, 0,
	0},
	//2:
	{{0,7,1.4},
	{0,-1.5,-1.2}, {0,0,0},
	false,
	0, 0,
	500,
	0,
	25,
	true,
	20,
	true, false,
	0, 0,
	0},
	//3:
	{{0,3,2},
	{0,0,0}, {0,-20,5},
	false,
	3, 70,
	70,
	700,
	10,
	false,
	10,
	true,true,
	0.7, 0.4,
	1.5},
	//4:
	{{0,0,0},
	{0,0,0}, {0,-40,16},
	false,
	4, 60,
	50,
	500,
	4,
	false,
	2,
	true,true,
	0.5, 0.5,
	0}
	//end of settings
	},
	//default camera setting
	3,
	//camera setting selection keys
	SDLK_F1,
	SDLK_F2,
	SDLK_F3,
	SDLK_F4};


const struct Conf_Index profile_index[] = {
	{"steer_speed",			'R' ,1 ,offsetof(Profile, steer_speed)},
	{"steer_max",			'R' ,1 ,offsetof(Profile, steer_max)},
	{"throttle_speed",		'R' ,1 ,offsetof(Profile, throttle_speed)},

	{"camera_default",   	   	'i' ,1 ,offsetof(Profile, camera)},

	{"camera1:target_offset",	'f' ,3 ,offsetof(Profile, cam[0].target)},
	{"camera1:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[0].anchor)},
	{"camera1:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[0].distance)},
	{"camera1:hide_car",		'b' ,1 ,offsetof(Profile, cam[0].hide_car)},
	{"camera1:collision_radius",	'f' ,1 ,offsetof(Profile, cam[0].radius)},
	{"camera1:collision_angle",	'f' ,1 ,offsetof(Profile, cam[0].angle)},
	{"camera1:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[0].linear_stiffness)},
	{"camera1:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[0].angular_stiffness)},
	{"camera1:damping",		'f' ,1 ,offsetof(Profile, cam[0].damping)},
	{"camera1:relative_damping",	'b' ,1 ,offsetof(Profile, cam[0].relative_damping)},
	{"camera1:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[0].rotation_speed)},
	{"camera1:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[0].reverse)},
	{"camera1:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[0].in_air)},
	{"camera1:air_time",		'f', 1, offsetof(Profile, cam[0].air_time)},
	{"camera1:ground_time",		'f', 1, offsetof(Profile, cam[0].ground_time)},
	{"camera1:offset_scale_speed",	'f', 1, offsetof(Profile, cam[0].offset_scale_speed)},

	{"camera2:target_offset",	'f' ,3 ,offsetof(Profile, cam[1].target)},
	{"camera2:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[1].anchor)},
	{"camera2:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[1].distance)},
	{"camera2:hide_car",		'b' ,1 ,offsetof(Profile, cam[1].hide_car)},
	{"camera2:collision_radius",	'f' ,1 ,offsetof(Profile, cam[1].radius)},
	{"camera2:collision_angle",	'f' ,1 ,offsetof(Profile, cam[1].angle)},
	{"camera2:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[1].linear_stiffness)},
	{"camera2:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[1].angular_stiffness)},
	{"camera2:damping",		'f' ,1 ,offsetof(Profile, cam[1].damping)},
	{"camera2:relative_damping",	'b' ,1 ,offsetof(Profile, cam[1].relative_damping)},
	{"camera2:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[1].rotation_speed)},
	{"camera2:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[1].reverse)},
	{"camera2:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[1].in_air)},
	{"camera2:air_time",		'f', 1, offsetof(Profile, cam[1].air_time)},
	{"camera2:ground_time",		'f', 1, offsetof(Profile, cam[1].ground_time)},
	{"camera2:offset_scale_speed",	'f', 1, offsetof(Profile, cam[1].offset_scale_speed)},

	{"camera3:target_offset",	'f' ,3 ,offsetof(Profile, cam[2].target)},
	{"camera3:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[2].anchor)},
	{"camera3:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[2].distance)},
	{"camera3:hide_car",		'b' ,1 ,offsetof(Profile, cam[2].hide_car)},
	{"camera3:collision_radius",	'f' ,1 ,offsetof(Profile, cam[2].radius)},
	{"camera3:collision_angle",	'f' ,1 ,offsetof(Profile, cam[2].angle)},
	{"camera3:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[2].linear_stiffness)},
	{"camera3:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[2].angular_stiffness)},
	{"camera3:damping",		'f' ,1 ,offsetof(Profile, cam[2].damping)},
	{"camera3:relative_damping",	'b' ,1 ,offsetof(Profile, cam[2].relative_damping)},
	{"camera3:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[2].rotation_speed)},
	{"camera3:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[2].reverse)},
	{"camera3:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[2].in_air)},
	{"camera3:air_time",		'f', 1, offsetof(Profile, cam[2].air_time)},
	{"camera3:ground_time",		'f', 1, offsetof(Profile, cam[2].ground_time)},
	{"camera3:offset_scale_speed",	'f', 1, offsetof(Profile, cam[2].offset_scale_speed)},

	{"camera4:target_offset",	'f' ,3 ,offsetof(Profile, cam[3].target)},
	{"camera4:anchor_offset",	'f' ,3 ,offsetof(Profile, cam[3].anchor)},
	{"camera4:anchor_distance",	'f' ,3 ,offsetof(Profile, cam[3].distance)},
	{"camera4:hide_car",		'b' ,1 ,offsetof(Profile, cam[3].hide_car)},
	{"camera4:collision_radius",	'f' ,1 ,offsetof(Profile, cam[3].radius)},
	{"camera4:collision_angle",	'f' ,1 ,offsetof(Profile, cam[3].angle)},
	{"camera4:linear_stiffness",	'f' ,1 ,offsetof(Profile, cam[3].linear_stiffness)},
	{"camera4:angular_stiffness",	'f' ,1 ,offsetof(Profile, cam[3].angular_stiffness)},
	{"camera4:damping",		'f' ,1 ,offsetof(Profile, cam[3].damping)},
	{"camera4:relative_damping",	'b' ,1 ,offsetof(Profile, cam[3].relative_damping)},
	{"camera4:rotation_speed",	'f' ,1 ,offsetof(Profile, cam[3].rotation_speed)},
	{"camera4:enable_reverse",	'b' ,1 ,offsetof(Profile, cam[3].reverse)},
	{"camera4:enable_in_air",	'b' ,1 ,offsetof(Profile, cam[3].in_air)},
	{"camera4:air_time",		'f', 1, offsetof(Profile, cam[3].air_time)},
	{"camera4:ground_time",		'f', 1, offsetof(Profile, cam[3].ground_time)},
	{"camera4:offset_scale_speed",	'f', 1, offsetof(Profile, cam[3].offset_scale_speed)},
	{"",0,0}}; //end

//list of all buttons
const struct {
	const char *name;
	size_t offset;
} profile_key_list[] = {
	{"up",			offsetof(Profile, up)},
	{"down",		offsetof(Profile, down)},
	{"right",		offsetof(Profile, right)},
	{"left",		offsetof(Profile, left)},
	{"drift_break",		offsetof(Profile, drift_break)},

	{"camera1",		offsetof(Profile, cam1)},
	{"camera2",		offsetof(Profile, cam2)},
	{"camera3",		offsetof(Profile, cam3)},
	{"camera4",		offsetof(Profile, cam4)},
	{"",0}}; //end
	

//functions
Profile *Profile_Load(const char*);
void Profile_Remove(Profile*);
void Profile_Remove_All();
void Profile_Events_Step(Uint32);

#endif

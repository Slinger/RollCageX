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

#ifndef _RCX_CAR_H
#define _RCX_CAR_H
//car: pointer to object and extra data, adjusted for controlled cars. No
//scripting - used to keep track of components and objects (like weapons)
//bellonging to the player during the race
//Allocated at start
#include "racetime_data.hpp"
#include "object.hpp"
#include "body.hpp"
#include "geom.hpp"
#include "../loaders/conf.hpp"

#include <vector>
#include <string>

//for loading car.conf
struct Car_Conf
{
	dReal motor_power, gear_limit;
	dReal max_break;
	dReal body_mass, wheel_mass;
	dReal suspension_spring, suspension_damping;
	dReal rim_mu, rim_angle, rollres, tyre_spring, tyre_damping;

	dReal xpeak, xshape, xpos[2], xsharp[2];
	dReal ypeak, yshape, ypos[2], ysharp[2], yshift;

	dReal body_mu;

	dReal body_linear_drag[3], body_angular_drag, wheel_linear_drag, wheel_angular_drag;

	dReal body[3];

	//values for moving steering/breaking/turning between front/rear wheels
	dReal dsteer, dbreak;
	bool drive[2];
	dReal max_steer;
	dReal steer_decrease;
	dReal diff_res;
	bool smartsteer, smartdrive;
	dReal air_torque;

	Conf_String model; //filename+path for model
	float resize, rotate[3], offset[3];

	//debug sizes
	dReal s[4],w[2],wp[2],jx;
};

const struct Car_Conf car_conf_defaults = {
	800000.0, 0.5,
	60000.0,
	6000.0, 500.0,
	150000.0, 5000.0,
	0.1, 45.0, 20, 300000.0, 10000.0,
	2000.0, 1.5, {0.1, 0.0}, {20.0, -0.4},
	1500.0, 1.5, {13.0, -0.2}, {0.05, 0.6}, 0.02,
	0.1,
	{10,5,15}, 1, 4, 0.5,
	{3.5,8.2,1},
	1.0, 0.5,
	{false, true},
	40.0,
	0.4,
	100.0,
	true, true,
	100.0,
	"",
	1, {0,0,0}, {0,0,0},
	{5.8,4.4,2,1.5}, {1.5,1.7}, {2.9,2.2}, 2.4};

const struct Conf_Index car_conf_index[] = {
	{"motor_power",		'R',1, offsetof(struct Car_Conf, motor_power)},
	{"gear_limit",		'R',1, offsetof(struct Car_Conf, gear_limit)},
	{"max_break",		'R',1, offsetof(struct Car_Conf, max_break)},
	{"body_mass",		'R',1, offsetof(struct Car_Conf, body_mass)},
	{"wheel_mass",		'R',1, offsetof(struct Car_Conf, wheel_mass)},

	{"steer_distribution",	'R',1, offsetof(struct Car_Conf, dsteer)},
	{"break_distribution",	'R',1, offsetof(struct Car_Conf, dbreak)},
	{"front-rear_drive",	'b',2, offsetof(struct Car_Conf, drive)},
	{"max_steer",		'R',1, offsetof(struct Car_Conf, max_steer)},
	{"steer_decrease",	'R',1, offsetof(struct Car_Conf, steer_decrease)},
	{"diff_resistance",	'R',1, offsetof(struct Car_Conf, diff_res)},
	{"air_torque_limit",	'R',1, offsetof(struct Car_Conf, air_torque)},
	{"smart_steering",	'b',1, offsetof(struct Car_Conf, smartsteer)},
	{"smart_driving",	'b',1, offsetof(struct Car_Conf, smartdrive)},

	{"model",		's',1, offsetof(struct Car_Conf, model)},
	{"model:resize",	'f',1, offsetof(struct Car_Conf, resize)},
	{"model:rotate",	'f',3, offsetof(struct Car_Conf, rotate)},
	{"model:offset",	'f',3, offsetof(struct Car_Conf, offset)},

	{"suspension_spring",	'R',1, offsetof(struct Car_Conf, suspension_spring)},
	{"suspension_damping",	'R',1, offsetof(struct Car_Conf, suspension_damping)},

	{"rim_angle",		'R',1, offsetof(struct Car_Conf, rim_angle)},
	{"rim_mu",		'R',1, offsetof(struct Car_Conf, rim_mu)},

	{"rolling_resistance",	'R',1, offsetof(struct Car_Conf, rollres)},

	{"tyre_spring",		'R',1, offsetof(struct Car_Conf, tyre_spring)},
	{"tyre_damping",	'R',1, offsetof(struct Car_Conf, tyre_damping)},

	{"tyre.x:peak",		'R',1, offsetof(struct Car_Conf, xpeak)},
	{"tyre.x:shape",	'R',1, offsetof(struct Car_Conf, xshape)},
	{"tyre.x:position",	'R',2, offsetof(struct Car_Conf, xpos)},
	{"tyre.x:sharpness",	'R',2, offsetof(struct Car_Conf, xsharp)},

	{"tyre.y:peak",		'R',1, offsetof(struct Car_Conf, ypeak)},
	{"tyre.y:shape",	'R',1, offsetof(struct Car_Conf, yshape)},
	{"tyre.y:position",	'R',2, offsetof(struct Car_Conf, ypos)},
	{"tyre.y:sharpness",	'R',2, offsetof(struct Car_Conf, ysharp)},
	{"tyre.y:shift",	'R',1, offsetof(struct Car_Conf, yshift)},

	{"body",		'R',3, offsetof(struct Car_Conf, body)},
	{"body_mu",		'R',1, offsetof(struct Car_Conf, body_mu)},

	{"body_linear_drag",	'R',3, offsetof(struct Car_Conf, body_linear_drag)},
	{"body_angular_drag",	'R',1, offsetof(struct Car_Conf, body_angular_drag)},
	{"wheel_linear_drag",	'R',1, offsetof(struct Car_Conf, wheel_linear_drag)},
	{"wheel_angular_drag",	'R',1, offsetof(struct Car_Conf, wheel_angular_drag)},
	
	//the following is for sizes not yet determined
	{"s",	'R',	4,	offsetof(struct Car_Conf, s)}, //flipover
	{"w",	'R',	2,	offsetof(struct Car_Conf, w)}, //wheel
	{"wp",	'R',	2,	offsetof(struct Car_Conf, wp)}, //wheel pos
	{"jx",	'R',	1,	offsetof(struct Car_Conf, jx)}, //joint x position
	{"",0,0}};//end


class Car_Template:public Racetime_Data
{
	public:
		static Car_Template *Load(const char *path);
		class Car *Spawn(dReal x, dReal y, dReal z, Trimesh_3D *tyre, Trimesh_3D *rim);

	private:
		Car_Template(const char *name); //only allocate through spawn function

		//conf:
		struct Car_Conf conf; //loads from conf

		//more data:
		char *name;
		dReal fsteer, rsteer, fmotor, rmotor, fbreak, rbreak;
		Wheel wheel;

		//geoms
		struct box {
			dReal size[3];
			dReal pos[3];
			dReal rot[3];
		};

		std::vector<class box> boxes;

		struct sphere {
			dReal radius;
			dReal pos[3];
		};

		std::vector<class sphere> spheres;

		struct capsule {
			dReal size[2];
			dReal pos[3];
			dReal rot[3];
		};

		std::vector<class capsule> capsules;

		Trimesh_3D *model;
};

class Car:public Object
{
	public:
		//destructor (remove car)
		~Car();

		//change car position (and reset rotation and velocity)
		void Respawn(dReal x, dReal y, dReal z);

		static void Physics_Step(dReal step);

		//public for now
		//controlling values
		bool drift_breaks;
		dReal throttle, steering; //-1.0 to +1.0
		dReal velocity; //keep track of car velocity

	private:
		Car(); //not allowed to be allocated freely
		friend class Car_Template; //only one allowed to create Car objects
		friend class Camera; //needs access to car info

		//configuration data (copied from Car_Template)
		dReal motor_power, gear_limit, max_break, max_steer;
		dReal diffres;
		dReal airtorque;

		dReal steerdecr;
		dReal dsteer, dbreak;
		bool fwd, rwd;
		bool smart_steer, smart_drive;

		//just for keeping track
		dBodyID bodyid,wheel_body[4];
		dJointID joint[4];

		Geom *wheel_geom_data[4];
		Wheel *wheel; //wheel data

		//flipover sensors
		Geom *sensor1, *sensor2;
		dReal dir; //direction, 1 or -1


		//tmp: wheel position...
		dReal wx, wy;

		//appart from the object list, keep a list of all cars
		static Car *head;
		Car *prev, *next;

		//tmp: needs access to above pointers
		friend int events_loop (void *d);
};

#endif

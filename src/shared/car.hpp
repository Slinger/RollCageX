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
	dReal max_torque, gear_tweak, max_break;
	bool torque_compensator;
	dReal body_mass, wheel_mass;
	dReal suspension_spring, suspension_damping;
	dReal rim_mu, rim_angle, tyre_spring, tyre_damping;
	dReal body_mu;

	dReal body_linear_drag[3], body_angular_drag, wheel_linear_drag, wheel_angular_drag;

	dReal body[3];

	//values for moving steering/breaking/turning between front/rear wheels
	float dsteer, dbreak;
	bool drive[2];

	Conf_String model; //filename+path for model
	float resize, rotate[3], offset[3];

	//debug sizes
	dReal s[4],w[2],wp[2],jx;
};

const struct Car_Conf car_conf_defaults = {
	800000, 1.0, 60000,
	true,
	6000, 500,
	150000.0, 5000.0,
	0.1, 45.0, 300000.0, 10000.0,
	0.1,
	{10,5,15}, 1, 4, 0.5,
	{3.5,8.2,1},
	1.0, 0.5,
	{false, true},
	"",
	1, {0,0,0}, {0,0,0},
	{5.8,4.4,2,1.5}, {1.5,1.7}, {2.9,2.2}, 2.4};

const struct Conf_Index car_conf_index[] = {
	{"max_torque",		'R',1, offsetof(struct Car_Conf, max_torque)},
	{"gear_tweak",		'R',1, offsetof(struct Car_Conf, gear_tweak)},
	{"max_break",		'R',1, offsetof(struct Car_Conf, max_break)},
	{"torque_compensator",	'b',1, offsetof(struct Car_Conf, torque_compensator)},
	{"body_mass",		'R',1, offsetof(struct Car_Conf, body_mass)},
	{"wheel_mass",		'R',1, offsetof(struct Car_Conf, wheel_mass)},

	{"steer_distribution",	'f',1, offsetof(struct Car_Conf, dsteer)},
	{"break_distribution",	'f',1, offsetof(struct Car_Conf, dbreak)},
	{"front-rear_drive",	'b',2, offsetof(struct Car_Conf, drive)},

	{"model",		's',1, offsetof(struct Car_Conf, model)},
	{"model:resize",	'f',1, offsetof(struct Car_Conf, resize)},
	{"model:rotate",	'f',3, offsetof(struct Car_Conf, rotate)},
	{"model:offset",	'f',3, offsetof(struct Car_Conf, offset)},

	{"suspension_spring",	'R',1, offsetof(struct Car_Conf, suspension_spring)},
	{"suspension_damping",	'R',1, offsetof(struct Car_Conf, suspension_damping)},

	{"rim_angle",		'R',1, offsetof(struct Car_Conf, rim_angle)},
	{"rim_mu",		'R',1, offsetof(struct Car_Conf, rim_mu)},

	//TODO: tyre friction
	{"tyre_spring",		'R',1, offsetof(struct Car_Conf, tyre_spring)},
	{"tyre_damping",	'R',1, offsetof(struct Car_Conf, tyre_damping)},

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
		dReal max_torque, gear_tweak, max_break;
		bool torque_compensator;

		dReal dsteer, dbreak;
		bool fwd, rwd;

		//just for keeping track
		dBodyID bodyid,wheel_body[4];
		dJointID joint[4];

		Geom *wheel_geom_data[4];

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

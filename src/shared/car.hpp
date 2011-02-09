/*
 * RCX  Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger")
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
#include "trimesh.hpp"
#include "../loaders/conf.hpp"

#include <vector>
#include <string>

//for loading car.conf
struct Car_Conf
{
	//motor
	dReal power, gear_limit, electric_torque;
	dReal air_torque;
	bool dist_motor[2];
	bool diff;
	bool redist[2];
	bool adapt_redist;
	dReal redist_force;

	//break
	dReal max_break;
	dReal dist_break;
	bool handbreak_lock;

	//steer
	dReal max_steer;
	dReal dist_steer;
	dReal steer_decrease;
	dReal min_steer;
	dReal limit_speed;
	bool adapt_steer;
	dReal toe[2];

	//other
	dReal body_mass, mass_position, body[3], wheel_mass;
	dReal suspension_spring, suspension_damping;
	dReal body_linear_drag[3], body_angular_drag, wheel_linear_drag, wheel_angular_drag;
	dReal wheel_spring, wheel_damping, rollres, rim_angle, rim_mu;

	dReal xpeak[2], xshape, xpos[2], xsharp[2];
	dReal ypeak[2], yshape, ypos[2], ysharp[2], yshift;


	Conf_String model; //filename+path for model
	float resize, rotate[3], offset[3];

	//debug
	bool turn, gyro, approx1;
	dReal fixedmu;
	dReal downforce[3];
	//debug sizes
	dReal s[4],w[2],wp[2],jx;
};

const struct Car_Conf car_conf_defaults = {
	500000.0, 2.0, 0.0,
	10.0,
	{false, true},
	true,
	{true, true},
	true,
	1000.0,

	100000.0,
	0.5,
	false,

	30.0,
	1.0,
	0.4,
	15.0,
	0.01,
	true,
	{0.0, 0.0},

	2500.0, 0, {2.6,5.8,0.7}, 250.0,
	150000.0, 5000.0,
	{5.0,2.0,10.0}, 2.0, 2.0, 2.0,
	300000.0, 10000.0, 20.0, 50.0, 0.1,

	{2000.0, -30.0}, 1.5, {0.1, 0.0}, {20.0, -0.4},
	{1500.0, -10.0}, 1.5, {13.0, -0.2}, {0.05, 0.6}, 0.02,

	"",
	1.0, {0,0,0}, {0,0,0},

	true, true, true,
	0.0,
	{0.0, 0.0},

	{4.83,3.67,1.67,1.25}, {1.25,1.42}, {2.42,1.83}, 2.0};

const struct Conf_Index car_conf_index[] = {
	{"power",		'R',1, offsetof(struct Car_Conf, power)},
	{"gear_limit",		'R',1, offsetof(struct Car_Conf, gear_limit)},
	{"electric_torque",	'R',1, offsetof(struct Car_Conf, electric_torque)},
	{"air_torque_limit",	'R',1, offsetof(struct Car_Conf, air_torque)},
	{"motor_distribution",	'b',2, offsetof(struct Car_Conf, dist_motor)},
	{"differential",	'b',1, offsetof(struct Car_Conf, diff)},
	{"torque_redistribution",'b',2, offsetof(struct Car_Conf, redist)},
	{"adaptive_redistribution",'b',1, offsetof(struct Car_Conf, adapt_redist)},
	{"redistribution_force",'R',1, offsetof(struct Car_Conf, redist_force)},

	{"max_break",		'R',1, offsetof(struct Car_Conf, max_break)},
	{"break_distribution",	'R',1, offsetof(struct Car_Conf, dist_break)},
	{"handbreak_lock",	'b',1, offsetof(struct Car_Conf, handbreak_lock)},

	{"max_steer",		'R',1, offsetof(struct Car_Conf, max_steer)},
	{"steer_distribution",	'R',1, offsetof(struct Car_Conf, dist_steer)},
	{"steer_decrease",	'R',1, offsetof(struct Car_Conf, steer_decrease)},
	{"min_decreased_steer",	'R',1, offsetof(struct Car_Conf, min_steer)},
	{"steer_limit_speed",	'R',1, offsetof(struct Car_Conf, limit_speed)},
	{"adaptive_steering",	'b',1, offsetof(struct Car_Conf, adapt_steer)},
	{"toe",			'R',2, offsetof(struct Car_Conf, toe)},

	{"body_mass",		'R',1, offsetof(struct Car_Conf, body_mass)},
	{"body_mass_position",	'R',1, offsetof(struct Car_Conf, mass_position)},
	{"body",		'R',3, offsetof(struct Car_Conf, body)},
	{"wheel_mass",		'R',1, offsetof(struct Car_Conf, wheel_mass)},
	{"suspension_spring",	'R',1, offsetof(struct Car_Conf, suspension_spring)},
	{"suspension_damping",	'R',1, offsetof(struct Car_Conf, suspension_damping)},
	{"body_linear_drag",	'R',3, offsetof(struct Car_Conf, body_linear_drag)},
	{"body_angular_drag",	'R',1, offsetof(struct Car_Conf, body_angular_drag)},
	{"wheel_linear_drag",	'R',1, offsetof(struct Car_Conf, wheel_linear_drag)},
	{"wheel_angular_drag",	'R',1, offsetof(struct Car_Conf, wheel_angular_drag)},
	{"wheel_spring",	'R',1, offsetof(struct Car_Conf, wheel_spring)},
	{"wheel_damping",	'R',1, offsetof(struct Car_Conf, wheel_damping)},
	{"rolling_resistance",	'R',1, offsetof(struct Car_Conf, rollres)},
	{"rim_angle",		'R',1, offsetof(struct Car_Conf, rim_angle)},
	{"rim_mu",		'R',1, offsetof(struct Car_Conf, rim_mu)},

	{"tyre.x:peak",		'R',2, offsetof(struct Car_Conf, xpeak)},
	{"tyre.x:shape",	'R',1, offsetof(struct Car_Conf, xshape)},
	{"tyre.x:position",	'R',2, offsetof(struct Car_Conf, xpos)},
	{"tyre.x:sharpness",	'R',2, offsetof(struct Car_Conf, xsharp)},

	{"tyre.y:peak",		'R',2, offsetof(struct Car_Conf, ypeak)},
	{"tyre.y:shape",	'R',1, offsetof(struct Car_Conf, yshape)},
	{"tyre.y:position",	'R',2, offsetof(struct Car_Conf, ypos)},
	{"tyre.y:sharpness",	'R',2, offsetof(struct Car_Conf, ysharp)},
	{"tyre.y:shift",	'R',1, offsetof(struct Car_Conf, yshift)},

	{"model",		's',1, offsetof(struct Car_Conf, model)},
	{"model:resize",	'f',1, offsetof(struct Car_Conf, resize)},
	{"model:rotate",	'f',3, offsetof(struct Car_Conf, rotate)},
	{"model:offset",	'f',3, offsetof(struct Car_Conf, offset)},

	//debug options:
	{"debug:turn",		'b',1, offsetof(struct Car_Conf, turn)},
	{"debug:gyroscopic",	'b',1, offsetof(struct Car_Conf, gyro)},
	{"debug:contactapprox1",'b',1, offsetof(struct Car_Conf, approx1)},
	{"debug:fixedmu",	'R',1, offsetof(struct Car_Conf, fixedmu)},
	{"debug:downforce",	'R',3, offsetof(struct Car_Conf, downforce)},
	
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
		Wheel wheel;

		//geoms
		struct geom { //can describe any supported geom
			int type;
			dReal size[3];
			Trimesh_Geom *mesh;
			dReal pos[3];
			dReal rot[3];
			Surface surf;
		};

		std::vector<class geom> geoms;

		//3D rendering model
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


	private:
		Car(); //not allowed to be allocated freely
		friend class Car_Template; //only one allowed to create Car objects
		friend class Camera; //needs access to car info

		//configuration data (copied from Car_Template)
		dReal power, gear_limit;
		dReal airtorque;

		dReal max_steer, steerdecr, min_steer, limit_speed, oldsteerlimit;
		dReal max_break;
		bool hinge2_dbreaks;

		bool diff;
		bool fwd, rwd;
		bool fwredist, rwredist;
		bool adapt_steer, adapt_redist;
		dReal redist_force;
		dReal dsteer, dbreak;
		dReal fwtoe, rwtoe;

		dReal offset;

		//just for keeping track
		dBodyID bodyid,wheel_body[4];
		dJointID joint[4];

		Geom *wheel_geom_data[4];
		Wheel *wheel; //wheel data

		//flipover sensors
		Geom *sensor1, *sensor2;
		dReal dir; //direction, 1 or -1

		//controlling values
		bool drift_breaks;
		dReal throttle, steering; //-1.0 to +1.0
		dReal velocity; //keep track of car velocity

		//debug options:
		bool turn;
		dReal downforce, maxdownforce, distdownforce;

		//tmp: wheel+hinge position...
		dReal jx, wx, wy;

		//appart from the object list, keep a list of all cars
		static Car *head;
		Car *prev, *next;

		//controls car
		friend void Profile_Events_Step(Uint32 step);

		//tmp: needs access to above pointers
		friend int Interface_Loop ();
};

#endif

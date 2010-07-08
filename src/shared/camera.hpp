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

#ifndef _RCX_CAMERA_H
#define _RCX_CAMERA_H
//TODO: make class
//
#include <ode/ode.h>
#include "car.hpp"

struct Camera_Settings {
	float target[3];
	float anchor[3], distance[3];
	float radius;
	float linear_stiffness;
	float angular_stiffness;
	float damping;
	bool relative_damping;
	float rotation_speed;
	float focus_speed;
	bool reverse, in_air;
	float air_time, ground_time;
	float offset_scale_speed;
};

class Camera
{
	public:
		Camera();
		void Set_Settings(Camera_Settings *settings);

		void Set_Pos(float p[3], float d[3]);

		//movement (might change or be removed at some point)
		void Move(float x, float y, float z);

		//these should probably be static (for using more cameras), but this will do for now
		void Physics_Step(dReal step);
		void Graphics_Step();

		//public for now...
		Car *car;
	private:
		struct Camera_Settings *settings;
		float pos[3];
		float dir[3];
		float vel[3];
		float up[3];
		float air_timer;
		float offset_scale; //0-1   0 in air, 1 on ground
		bool reverse;
		bool in_air;

		friend void Graphic_List_Render();

		//physics simulation functions
		void Accelerate(dReal step);
		void Collide(dReal step);
		void Damp(dReal step);
		void Rotate(dReal step);
};

extern Camera camera;
#endif

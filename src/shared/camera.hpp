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

#ifndef _RC_CAMERA_H
#define _RC_CAMERA_H
//TODO: make class
//
#include <ode/ode.h>
#include "car.hpp"

struct Camera_Settings {
	float target[3];
	float anchor[3], distance[3];
	bool hide_car;
	float radius, angle;
	float linear_stiffness;
	float angular_stiffness;
	float damping;
	bool relative_damping;
	float rotation_speed;
	bool reverse, in_air;
	float air_time, ground_time;
	float offset_scale_speed;
};

class Camera
{
	public:
		//constructor
		Camera();

		//select settings
		void Set_Settings(Camera_Settings *settings);

		//focus on this car
		void Set_Car(Car*);

		//sets position and target of camera from two points
		void Set_Pos(float px, float py, float pz, float tx, float ty, float tz);

		//movement (might change or be removed at some point)
		void Move(float x, float y, float z);

		//these should probably be static (for using more cameras), but this will do for now
		void Physics_Step(dReal step);
		void Generate_Matrix();
		void Graphics_Step();

	private:
		struct Camera_Settings *settings;
		Car *car;
		friend void HUD(Uint32);
		Object *hide;

		//matrix (generated from everything else)
		float matrix[16];

		//position/velocity
		float pos[3];
		float vel[3];

		//camera direction (rotation matrix)
		float rotation[3*3];
		//(right, dir, up)

		float air_timer;
		float offset_scale; //0-1   0 in air, 1 on ground
		bool reverse;
		bool in_air;

		friend class Car;
		friend void Render_List_Render();

		//physics simulation functions
		void Accelerate(dReal step);
		void Collide(dReal step);
		void Damp(dReal step);
		void Rotate(dReal step);
};

extern Camera camera;
#endif

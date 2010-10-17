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

#include "camera.hpp"
#include "runlevel.hpp"

Camera camera;

//just make sure all variables are zeroed
Camera::Camera()
{
	settings=NULL;
	car = NULL;
	hide = NULL;

	//right, dir, up
	rotation[0]=0; rotation[1]=1; rotation[2]=0; 
	rotation[3]=1; rotation[4]=1; rotation[5]=0; 
	rotation[6]=0; rotation[6]=1; rotation[7]=1; 

	pos[0]=0;
	pos[1]=0;
	pos[2]=0;

	vel[0]=0;
	vel[1]=0;
	vel[2]=0;

	air_timer = 0;
	offset_scale = 0;
	reverse = false;
	in_air = false;
}

void Camera::Set_Settings (Camera_Settings *set)
{
	settings = set;

	if (settings)
	{

		//if this camera mode doesn't have reverse enabled, make sure camera isn't stuck in reverse
		if (!settings->reverse)
			reverse = false;

		//if this camera mode has "air mode" disabled, make sure camera isn't stuck in "in air" mode from before
		if (!settings->in_air)
		{
			in_air = false;
			offset_scale = 1;
		}
	}

	//if not rendering car
	if (settings->hide_car)
		hide=car;
	else
		hide=NULL;
}

void Camera::Set_Car (Car *c)
{
	car = c;

	//just make sure car stays hidden if current settings wants that
	if (settings && settings->hide_car)
		hide=car;
	else
		hide=NULL;
}

//length of vector
#define VLength(V) (sqrt( (V)[0]*(V)[0] + (V)[1]*(V)[1] + (V)[2]*(V)[2] ))

//cross product (A=BxC)
#define VCross(A,B,C){ \
	(A)[0]=(B)[1]*(C)[2]-(B)[2]*(C)[1]; \
	(A)[1]=(B)[2]*(C)[0]-(B)[0]*(C)[2]; \
	(A)[2]=(B)[0]*(C)[1]-(B)[1]*(C)[0];}

//normalization of vector (A=A/|A|)
#define VNormalize(V){ \
	float l=VLength(V); \
	(V)[0]/=l; (V)[1]/=l; (V)[2]/=l;}

void Camera::Set_Pos(float px, float py, float pz, float tx, float ty, float tz)
{
	//set position directly
	pos[0]=px;
	pos[1]=py;
	pos[2]=pz;

	//direction to look at
	float new_dir[3] = {tx-px, ty-py, tz-pz};

	//no direction (keep original rotation)
	if (new_dir[0] == 0 && new_dir[1] == 0 && new_dir[2] == 0)
		return;

	//ok, find up direction (might be tricky)
	//along z, set up to right...
	float new_right[3], new_up[3];
	if (new_dir[0] == 0 && new_dir[2] == 0)
	{
		new_up[0] = 1; new_up[1] = 0; new_up[2] = 0; 
	}
	else //no, ok to use up = real up
	{
		new_up[0] = 0; new_up[1] = 0; new_up[2] = 1; 
	}

	VCross(new_right, new_dir, new_up); //calculate right
	VCross(new_up, new_right, new_dir); //recalculate proper up

	//normalize:
	VNormalize(new_dir);
	VNormalize(new_right);
	VNormalize(new_up);

	//set matrix to these values
	rotation[0]=new_right[0]; rotation[1]=new_dir[0]; rotation[2]=new_up[0];
	rotation[3]=new_right[1]; rotation[4]=new_dir[1]; rotation[5]=new_up[1];
	rotation[6]=new_right[2]; rotation[7]=new_dir[2]; rotation[8]=new_up[2];
}

void Camera::Move(float x, float y, float z)
{
	if (settings && runlevel == running)
	{
		settings->distance[0] += x;
		settings->distance[1] += y;
		settings->distance[2] += z;
	}
	else //camera got no settings, or is paused
	{
		//we probably got a car?
		if (car)
		{
			//good, lets look at center of car
			const dReal *p = dBodyGetPosition(car->bodyid);
			Set_Pos((pos[0]+x), (pos[1]+y), (pos[2]+z), p[0], p[1], p[2]);
		}
		//ok, no car... lets just keep old rotation
		else
		{
			pos[0]+=x;
			pos[1]+=y;
			pos[2]+=z;
		}
	}
}

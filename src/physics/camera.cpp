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

#include "../shared/camera.hpp"
#include "../shared/internal.hpp"
#include "../shared/track.hpp"

#include <math.h>

//length of vector (=|V|)
#define VLength(V) (sqrt( (V)[0]*(V)[0] + (V)[1]*(V)[1] + (V)[2]*(V)[2] ))

//normalization of vector (A=A/|A|)
#define VNormalize(V){ \
	float l=VLength(V); \
	(V)[0]/=l; (V)[1]/=l; (V)[2]/=l;}

//cross product (A=BxC)
#define VCross(A,B,C){ \
	(A)[0]=(B)[1]*(C)[2]-(B)[2]*(C)[1]; \
	(A)[1]=(B)[2]*(C)[0]-(B)[0]*(C)[2]; \
	(A)[2]=(B)[0]*(C)[1]-(B)[1]*(C)[0];}

//dot product (=AxB)
#define VDot(A,B) ( (A)[0]*(B)[0] + (A)[1]*(B)[1] + (A)[2]*(B)[2] )

//subtraction of one vector from another (A=B-C)
#define VSubtract(A,B,C){ \
       (A)[0]=(B)[0]-(C)[0]; \
       (A)[1]=(B)[1]-(C)[1]; \
       (A)[2]=(B)[2]-(C)[2];}

//
//spring physics for calculating acceleration
//


void Camera::Accelerate(dReal step)
{
	//calculate some needed values
	dVector3 result;

	if (reverse && !in_air) //move target and position to opposite side (if not just spinning in air)
		dBodyVectorToWorld(car->bodyid, settings->distance[0]*car->dir, -settings->distance[1], settings->distance[2]*car->dir, result);
	else //normal
		dBodyVectorToWorld(car->bodyid, settings->distance[0]*car->dir, settings->distance[1], settings->distance[2]*car->dir, result);

	float c_pos[3];
	c_pos[0]=result[0];
	c_pos[1]=result[1];
	c_pos[2]=result[2];


	//position and velocity of anchor
	dVector3 a_pos;
	dBodyGetRelPointPos (car->bodyid, settings->anchor[0], settings->anchor[1], settings->anchor[2]*car->dir, a_pos);

	//relative pos and vel of camera (from anchor)
	float r_pos[3] = {pos[0]-a_pos[0], pos[1]-a_pos[1], pos[2]-a_pos[2]};

	//vector lengths
	float r_pos_l = VLength(r_pos);
	//how far from car we want to stay
	//(TODO: could be computed just once - only when changing camera)
	float c_pos_l = VLength(c_pos);

	//unit vectors
	float r_pos_u[3] = {r_pos[0]/r_pos_l, r_pos[1]/r_pos_l, r_pos[2]/r_pos_l};
	float c_pos_u[3] = {c_pos[0]/c_pos_l, c_pos[1]/c_pos_l, c_pos[2]/c_pos_l};



	//"linear spring" between anchor and camera (based on distance)
	float dist = r_pos_l-c_pos_l;

	if (settings->linear_stiffness == 0) //disabled smooth movement, jump directly
	{
		//chanses are we have an anchor distance of 0, then vel=0
		if (c_pos_l == 0)
		{
			//position at wanted
			pos[0]=a_pos[0];
			pos[1]=a_pos[1];
			pos[2]=a_pos[2];

			//velocity 0
			vel[0]=0;
			vel[1]=0;
			vel[2]=0;
		}
		else
		{
			//set position
			pos[0]-=r_pos_u[0]*dist;
			pos[1]-=r_pos_u[1]*dist;
			pos[2]-=r_pos_u[2]*dist;

			//velocity towards/from anchor = 0
			//vel towards anchor
			float dot = (r_pos_u[0]*vel[0] + r_pos_u[1]*vel[1] + r_pos_u[2]*vel[2]);

			//remove vel towards anchor
			vel[0]-=r_pos_u[0]*dot;
			vel[1]-=r_pos_u[1]*dot;
			vel[2]-=r_pos_u[2]*dot;
		}
	}
	else //smooth movement
	{
		//how much acceleration (based on distance from wanted distance)
		float acceleration = step*(settings->linear_stiffness)*dist;

		vel[0]-=r_pos_u[0]*acceleration;
		vel[1]-=r_pos_u[1]*acceleration;
		vel[2]-=r_pos_u[2]*acceleration;
	}

	//perpendicular "angular spring" to move camera behind car
	if (c_pos_l > 0 && !in_air) //actually got distance, and camera not in "air mode"
	{
		//dot between wanted and current rotation
		float dot = (c_pos_u[0]*r_pos_u[0] + c_pos_u[1]*r_pos_u[1] + c_pos_u[2]*r_pos_u[2]);

		if (dot < 1.0) //if we aren't exactly at wanted position (and prevent possibility of acos a number bigger than 1.0)
		{
			//angle
			float angle = acos(dot);

			//how much acceleration
			float accel = step*angle*(settings->angular_stiffness);

			//direction of acceleration (remove part of wanted that's along current pos)
			float dir[3];
			dir[0]=c_pos_u[0]-dot*r_pos_u[0];
			dir[1]=c_pos_u[1]-dot*r_pos_u[1];
			dir[2]=c_pos_u[2]-dot*r_pos_u[2];

			//not unit, get length and modify accel to compensate for not unit
			accel /= VLength(dir);

			vel[0]+=(accel*dir[0]);
			vel[1]+=(accel*dir[1]);
			vel[2]+=(accel*dir[2]);
		}
	}
}

void Camera::Collide(dReal step)
{
	//
	//check for collision, and if so, remove possible movement into collision direction
	//

	if (settings->radius > 0)
	{
		dGeomID geom = dCreateSphere (0, settings->radius);
		dGeomSetPosition(geom, pos[0], pos[1], pos[2]);

		dContactGeom contact[internal.contact_points];
		int count = dCollide ( (dGeomID)(track.space->space_id), geom, internal.contact_points, &contact[0], sizeof(dContactGeom));

		int i;
		float V;
		float A;
		float Amax = cos(settings->angle*M_PI/180.0);

		for (i=0; i<count; ++i)
		{
			//remove movement into colliding object
			//velocity along collision axis
			V = vel[0]*contact[i].normal[0] +
				vel[1]*contact[i].normal[1] +
				vel[2]*contact[i].normal[2];

			A = rotation[1]*contact[i].normal[0] +
				rotation[4]*contact[i].normal[1] +
				rotation[7]*contact[i].normal[2];

			//right direction (not away from collision), in valid angle?
			if (V > 0 && A < Amax)
			{
				//remove direction
				vel[0]-=V*contact[i].normal[0];
				vel[1]-=V*contact[i].normal[1];
				vel[2]-=V*contact[i].normal[2];
			}
		}

		dGeomDestroy (geom);
	}
}

void Camera::Damp(dReal step)
{
	//
	//damping of current velocity
	//

	if (settings->relative_damping)
	{
		//damping (of relative movement)
		dVector3 a_vel; //anchor velocity
		dBodyGetRelPointVel (car->bodyid, settings->anchor[0], settings->anchor[1], settings->anchor[2]*car->dir, a_vel);
		float r_vel[3] = {vel[0]-a_vel[0], vel[1]-a_vel[1], vel[2]-a_vel[2]}; //velocity relative to anchor

		float damping = (step*settings->damping);
		if (damping > 1)
			damping=1;

		vel[0]-=damping*r_vel[0];
		vel[1]-=damping*r_vel[1];
		vel[2]-=damping*r_vel[2];
	}
	else
	{
		//absolute damping
		float damping = 1-(step*settings->damping);

		if (damping < 0)
			damping=0;

		vel[0]*=damping;
		vel[1]*=damping;
		vel[2]*=damping;
	}
}

//
//the following is smooth rotation and focusing
//

//rotate vector V towards vector V1, along axis A by angle (angle1 - between V and V1)
//this could use a simple rotation matrix instead, but a trig solution seems cleanest
void VRotate(float *V, float *V1, float *A, float angle)
{
	//remove part of vectors along axis:
	float proj = VDot(V, A); //how much of V (and V1) is along axis
	float Vp[3] = {proj*A[0], proj*A[1], proj*A[2]}; //part of Vector Along axis

	//modify given vector so perpendicular to axis (remove projection on axis)
	V[0]-=Vp[0]; V[1]-=Vp[1]; V[2]-=Vp[2];

	// V
	// ^       V1
	// | angle ^
	// |__   /
	// |  \ /
	// |   /
	// |_ /
	// *-|----------> Vx
	//
	// (need Vx to calculate V1)
	//
	
	//just use cross product to find Vx
	float Vx[3];
	VCross(Vx, V, A);

	//note: since A is unit, perpendicular to V, |Vx|=|V| !)

	//new value on V:
	//(V1 negative proj on Vx: rotation "wrong way")
	if (VDot(Vx, V1) < 0.0)
	{
		V[0] = (V[0]*cos(angle)) - (Vx[0]*sin(angle));
		V[1] = (V[1]*cos(angle)) - (Vx[1]*sin(angle));
		V[2] = (V[2]*cos(angle)) - (Vx[2]*sin(angle));
	}
	else
	{
		V[0] = (V[0]*cos(angle)) + (Vx[0]*sin(angle));
		V[1] = (V[1]*cos(angle)) + (Vx[1]*sin(angle));
		V[2] = (V[2]*cos(angle)) + (Vx[2]*sin(angle));
	}

	//removed part of V along axis before, give it back:
	V[0]+=Vp[0]; V[1]+=Vp[1]; V[2]+=Vp[2];
}

//rotate and focus camera
//could of course use spring physics, but this is simpler and works ok
void Camera::Rotate(dReal step)
{
	//
	//the camera rotation is in form of a 3x3 rotation matrix
	//the wanted (target) rotation is also in a 3x3 rotation matrix
	//
	//on order to move from the current matrix to the wanted (but not directly)
	//the matrix is rotated. the rotation is performed around one single axis
	//which will have to be computed...
	//

	//---
	//first: needed values
	//---

	//while working with new camera values, store them here
	float c_right[3] = {rotation[0], rotation[3], rotation[6]};
	float c_dir[3] = {rotation[1], rotation[4], rotation[7]};
	float c_up[3] = {rotation[2], rotation[5], rotation[8]};


	//calculate wanted
	float t_right[3];
	float t_dir[3];
	float t_up[3];

	dVector3 result;
	if (reverse && !in_air) //move target and position to opposite side (if not just spinning in air)
		dBodyGetRelPointPos (car->bodyid, settings->target[0]*car->dir, -settings->target[1], settings->target[2]*car->dir, result);
	else //normal
	{
		dBodyGetRelPointPos (car->bodyid, settings->target[0]*offset_scale*car->dir,
				settings->target[1]*offset_scale, settings->target[2]*car->dir*offset_scale, result);
	}

	t_dir[0]=result[0]-pos[0];
	t_dir[1]=result[1]-pos[1];
	t_dir[2]=result[2]-pos[2];


	if (in_air) //if in air, use absolute up instead
	{
		t_up[0]=0.0;
		t_up[1]=0.0;
		t_up[2]=1.0;
	}
	else //use car up
	{
		const dReal *rotation = dBodyGetRotation (car->bodyid);
		t_up[0] = rotation[2]*car->dir;
		t_up[1] = rotation[6]*car->dir;
		t_up[2] = rotation[10]*car->dir;
	}
	
	//target right from dirXup
	VCross(t_right, t_dir, t_up);

	//modify t_up to be perpendicular to t_dir (and t_right)
	VCross(t_up, t_right, t_dir);

	//will need all vectors to be unit...
	VNormalize(t_dir);
	VNormalize(t_up);
	VNormalize(t_right);



	//no smooth rotation?...
	if (settings->rotation_speed == 0.0)
	{
		rotation[0]=t_right[0]; rotation[1]=t_dir[0]; rotation[2]=t_up[0];
		rotation[3]=t_right[1]; rotation[4]=t_dir[1]; rotation[5]=t_up[1];
		rotation[6]=t_right[2]; rotation[7]=t_dir[2]; rotation[8]=t_up[2];
		return;
	}


	//---
	//find axis to rotate around
	//---
	//
	//NOTE: while one could calculate the axis from any cross product (3 alternatives),
	//lets calculate all 3 and see which one could be most accurate.
	//The same for calculating the maximum rotation around the axis - any 3 can be used,
	//but here we just choose one of the two axis change that were used to calculate the axis
	//(since the one discarded must be the least reliable)
	
	//"difference" between current and wanted rotations
	float d_right[3], d_dir[3], d_up[3];
	VSubtract(d_right, t_right, c_right);
	VSubtract(d_dir, t_dir, c_dir);
	VSubtract(d_up, t_up, c_up);

	//3 alternative rotation axes (are equal)
	float A1[3], A2[3], A3[3];
	VCross(A1, d_right, d_dir);
	VCross(A2, d_dir, d_up);
	VCross(A3, d_up, d_right);

	//compare lengths and choose the one that seems most accurate
	float L1, L2, L3;
	L1 = VLength(A1);
	L2 = VLength(A2);
	L3 = VLength(A3);

	//what we select
	float L, *A, *D;
	//L - length, A - axis, D - change of direction)

	//if 1 is bigger than 2
	if (L1 > L2)
	{
		L = L1;
		A = A1;
		D = d_right;
	}
	else //no, 2 is bigger
	{
		L = L2;
		A = A2;
		D = d_dir;
	}
	if (L3 > L) //wait! - 3 was even bigger
	{
		L = L3;
		A = A3;
		D = d_up;
	}

	//make sure not too small
	//(since too equal current and wanted rotation, or possibly some computation error?)
	if (L < 0.000001) //TODO: good?
	{
		rotation[0]=t_right[0]; rotation[1]=t_dir[0]; rotation[2]=t_up[0];
		rotation[3]=t_right[1]; rotation[4]=t_dir[1]; rotation[5]=t_up[1];
		rotation[6]=t_right[2]; rotation[7]=t_dir[2]; rotation[8]=t_up[2];
		return;
	}

	//ok, normalize axis:
	A[0]/=L; A[1]/=L; A[2]/=L;


	//---
	//nice, got an axis, lets start rotation...
	//---

	//max needed rotation angle:
	//since difference in direction is perpendicular to axis of rotation
	//and kinda like the hypotenuse of the c and t directions, Vmax is easily gotten:
	float Vmax = 2.0*asin(VLength(D)/2.0);

	//D can get just a bit above 1.0 (calculation precision errors)
	//if this happens, asin returns NAN.
	if (isnan(Vmax))
		Vmax=M_PI; //180°

	//and how much to rotate!
	float Vspeed = internal.stepsize*settings->rotation_speed*Vmax;

	//check if we can reach target in this step, if so just jump
	if (Vspeed > Vmax)
	{
		rotation[0]=t_right[0]; rotation[1]=t_dir[0]; rotation[2]=t_up[0];
		rotation[3]=t_right[1]; rotation[4]=t_dir[1]; rotation[5]=t_up[1];
		rotation[6]=t_right[2]; rotation[7]=t_dir[2]; rotation[8]=t_up[2];
		return;
	}


	//
	//nope, we will have to rotate the axes...
	//

	//while VRotate got a fairly good accuracy, it got limited calculation precision.
	//so vectors might not be perpendicular and unit after many simulations...
	//thus: only rotate two axes:
	VRotate(c_dir, t_dir, A, Vspeed);
	VRotate(c_up, t_up, A, Vspeed);

	//calculate the third, and recalculate one of the first two:
	VCross(c_right, c_dir, c_up);
	VCross(c_dir, c_up, c_right);

	//and make them unit:
	VNormalize(c_right);
	VNormalize(c_dir);
	VNormalize(c_up);

	//---
	//update values:
	//---

	//just copy the (now modified) values back
	rotation[0] = c_right[0]; rotation[1] = c_dir[0]; rotation[2] = c_up[0];
	rotation[3] = c_right[1]; rotation[4] = c_dir[1]; rotation[5] = c_up[1];
	rotation[6] = c_right[2]; rotation[7] = c_dir[2]; rotation[8] = c_up[2];
}

//collide camera with track, generate acceleration on camera if collisding
void Camera::Physics_Step(dReal step)
{
	//some values that are easy to deal with:

	if (car && settings)
	{

		//
		//if camera got a targeted car and proper settings, simulate movment
		//

		//check for some exceptions
		if (settings->reverse) //enabled
		{
			if (car->throttle > 0.0) //wanting to go forward
				reverse = false;
			else if (car->throttle < 0.0 && car->velocity < 0.0) //wanting and going backwards
				reverse = true;
		}

		if (settings->in_air) //in air enabled
		{
			if (!(car->sensor1->colliding) && !(car->sensor2->colliding)) //in air
			{
				if (in_air) //in ground mode
				{
					//smooth transition between offset and center (and needed)
					if (settings->offset_scale_speed != 0 && offset_scale > 0)
						offset_scale -= (settings->offset_scale_speed*step);
					else //jump directly
						offset_scale = 0;
				}
				if (!in_air) //camera not in "air mode"
				{
					if (air_timer > settings->air_time)
					{
						in_air = true; //go to air mode
						air_timer = 0; //reset timer
					}
					else
						air_timer += step;
				}
			}
			else //not in air
			{
				if (in_air) //camera in "air mode"
				{
					if (air_timer > settings->ground_time)
					{
						in_air = false; //leave air mode
						air_timer = 0; //reset timer
					}
					else
						air_timer += step;
				}
				else //camera in "ground mode"
				{
					//smooth transition between center and offset (and needed)
					if (settings->offset_scale_speed != 0 && offset_scale < 1)
						offset_scale += (settings->offset_scale_speed*step);
					else //jump directly
						offset_scale = 1;
				}
			}
		}

		//store old velocity
		dReal old_vel[3] = {vel[0], vel[1], vel[2]};

		//perform movement
		Accelerate(step);
		Collide(step);
		Damp(step);

		//during the step, camera will have linear acceleration from old velocity to new
		pos[0]+=((vel[0]+old_vel[0])/2)*step;
		pos[1]+=((vel[1]+old_vel[1])/2)*step;
		pos[2]+=((vel[2]+old_vel[2])/2)*step;

		//avarge velocity over the step is between new and old velocity
		//rotate camera (focus and rotation)
		Rotate(step);
	}
}

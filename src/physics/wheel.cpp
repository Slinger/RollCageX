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

extern bool is_now;
extern bool first;
extern int is_it;

#include "wheel.hpp"
#include "../shared/internal.hpp" 

//This code tries to implement a reasonably simple and realistic tyre friction model.
//(it's mostly inspired by different equations based on Pacejka's "magic formula")
//It also determines if the tyre or rim of the wheel is colliding.
//I'm probably not using all correct variable names, and I might have made some typo
//somewhere. There are also a lot of features that could/should be implemented:
//
//	* antispin - will be necessary (to make the cars even able to move)
//			we calculate the wheel slip here, then send to physics/car.cpp?
//
//	* force feedback - can be calculated here, but sdl lack force feedback right now
//				(supports gamepads, but not ff specific)
//
//	* rolling resistance - would help with realism and makes sence to implement here
//
//	* surface friction - different surfaces should give different mu and mu2
//
//	* the improved wheel friction might make it necessary to improve the suspension
//		(toe, caster, camber, (computerized) differential, etc...)
//
//	* todo...............
//

//useful vector calculations (from physics/camera.cpp):
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


//nothing here yet...
Wheel::Wheel()
{}

//simulation of wheel
void Wheel::Set_Contacts(dBodyID wbody, dBodyID obody, dReal ospring, dReal odamping,
		dContact *contact, int count)
{
	//
	//variables:
	//
	//directions (X:longitudinal/wheel travel, Y:lateral/sideway, Z:normal/"up"):
	dReal X[3], Y[3], Z[3];
	//forces:
	dReal Fx, Fy, Fz;
	//slip:
	dReal slip_ratio, slip_angle;
	//other things that affects simulation:
	dReal camber;

	//tmp variables:
	//for slip:
	dVector3 pos; //contact point position
	const dReal *bvel; //velocity of wheel body
	dVector3 v1, v2; //velocity (on wheel body and other body)
	dReal v1x, v1y, v1z; //wheel point velocity along wheel dirs
	dReal v2x, v2y, v2z; //road point velocity along wheel dirs

	//
	//use spring+damping for tyre collision forces:
	dReal cspring = 1/( 1/(spring) + 1/(ospring) );
	dReal cdamping = damping + odamping;

	//calculate erp+cfm from stepsize, spring and damping values:
	dReal stepsize = internal.stepsize;
	dReal erp = (stepsize*cspring)/(stepsize*cspring +cdamping);
	dReal cfm = 1.0/(stepsize*cspring +cdamping);
	//

	//all of these values are input values, Fx and Fy, but not Fz!, are the output (used as mu1 and mu2)
	for (int i=0; i<count; ++i)
	{
		//
		//1) input values:
		//


		//directions:

		//Z: vertical to ground/normal direction
		Z[0] = contact[i].geom.normal[0];
		Z[1] = contact[i].geom.normal[1];
		Z[2] = contact[i].geom.normal[2];

		//Y: simply wheel axis Z
		//NOTE: this is actually incorrect, since this "Y" might not be tangental to ground.
		const dReal *rot = dBodyGetRotation(wbody);
		Y[0] = rot[2];
		Y[1] = rot[6];
		Y[2] = rot[10];

		//X: along wheel direction (perpendicular to Y and Z)
		//note: will be tangental, so ok.
		VCross (X, Z, Y);
		VNormalize(X);

		//Y is not correct (not tangental to ground), but X is, and Z is also ok.
		//Y can be recalculated from X and Z, but first, this Y use useful:

		//camber:

		//first get angle between current Y and Z
		dReal tmp = VDot (Z, Y);
		camber = (180.0/M_PI)*acos(tmp);
		//camber is 0-180 between axis and normal...
		//we want between wheel up and normal...
		camber -= 90.0;
		//this gives -90-90 but we want 0-90...
		camber = fabs(camber);

		//
		//ok, we can now calculate the correct Y!
		VCross(Y, X, Z);
		//note: no need to normalize, since both X and Y are unit.
		//


		//slip+slip_angle:

		//first, we need to calculate some values here:
		//(copy the position (damn, c++ doesn't allow vector assignment, right now...)
		pos[0] = contact[i].geom.pos[0];
		pos[1] = contact[i].geom.pos[1];
		pos[2] = contact[i].geom.pos[2];

		//we need the velocity of this contact point as if it was a point at the wheel and road:
		dBodyGetPointVel(wbody, pos[0], pos[1], pos[2], v1);

		//not sure the other geom got a body...
		if (obody) //the surface got a body, so slip is relative to both
		{
			//get other vel
			dBodyGetPointVel(obody, pos[0], pos[1], pos[2], v2);
		}
		else //not body...
		{
			//vel=0
			v2[0]=0.0;
			v2[1]=0.0;
			v2[2]=0.0;
		}

		//ok, the current velocities are relative to world, but we need them relative to the wheel:
		bvel = dBodyGetLinearVel (wbody);

		//wheel point:
		v1[0]-=bvel[0];
		v1[1]-=bvel[1];
		v1[2]-=bvel[2];
		//road point:
		v2[0]-=bvel[0];
		v2[1]-=bvel[1];
		v2[2]-=bvel[2];

		//put these velocities along the X, Y and Z axes defined before:
		v1x = VDot(X, v1);
		//v1y = VDot(Y, v1); not needed?
		v1z = VDot(Z, v1);

		v2x = VDot(X, v2);
		v2y = VDot(Y, v2);
		v2z = VDot(Z, v2);
		//end of the needed values



		//slip_rate: defined as: (wheel velocity/ground velocity)-1
		//this is velocity along the X axis...
		slip_ratio = v1x/v2x-1;
		//NOTE: is v2x get low (wheel standing still) this gets quite high

		//slip_angle: angle (in degrees) between X and actual direction of movement.
		//velocity up/down (velZ) is not part of this, since only the tangental
		//movement along the surface has to do with the actual wheel vs surface
		//friction stuff:
		//use the relative velocity of ground vs wheel along x and y to calculate this:
		slip_angle = fabs((180.0/M_PI)*atan( v2y/v2x ));
		//NOTE: if v2x gets really low (wheel standing still) the reliability is lost...

		//normal force: how many kN of force caused by tyre compression.
		//since using a linear spring+damping solution to calculations, this is easy:
		//(note: primitive solution... and ignores features like bouncy collisions)
		Fz = contact[i].geom.depth*cspring + ((v1z-v2z)*cdamping);

		if (first)
		{
			first = false;
			printf("guess: %f (%f)\n", Fz, v1z-v2z);
			is_now = true;
			is_it = i;
		}

		Fz /= 1000.0; //change from N to kN
		//contact joints only generates forces pulling the colliding bodies _appart_
		//so negative (traction) forces are not added (complying with reality btw)
		//(in this case it's the damping force getting higher than spring force when
		//the two bodies are moving appart from each others)
		if (Fz < 0.0) //not going to be any friction of this contact
			continue;

		//rim (outside range for tyre)
		if (camber > rim_angle)
		{
			//enable the usual contact friction approximation
			contact[i].surface.mode |= dContactApprox1;

			//note: since rim_mu is used as the mu of the wheel, we already
			//got the wanted mu calculated
		}
		else //tyre
		{
			//
			//2) compute output values:
			//
			//TODO!
			//hmmm.... perhaps the angles should be in radians? or degrees as specified?
			//tmp values to make car driveable:
			Fx = 5000.0;
			Fy = 5000.0;

			//
			//3) set output values:
			//
			//enable: separate mu for dir 1&2, specify dir 1
			//(note: dir2 is automatically calculated by ode)
			//also enable erp+cfm specifying (for spring+damping)
			contact[i].surface.mode |= dContactMu2 | dContactFDir1 |
				dContactSoftERP | dContactSoftCFM;

			//fdir1
			contact[i].fdir1[0] = X[0];
			contact[i].fdir1[1] = X[1];
			contact[i].fdir1[2] = X[2];

			//erp+cfm (spring+damping)
			contact[i].surface.soft_erp = erp;
			contact[i].surface.soft_cfm = cfm;

			//mu1 and mu2
			contact[i].surface.mu = Fx;
			contact[i].surface.mu2 = Fy;
		}
	}
}



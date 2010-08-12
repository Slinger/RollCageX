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
	dVector3 v1, v2; //velocity (on wheel body and other body)
	dVector3 vel; //velocity (v1 relative v2)
	dReal velX, velY, velZ; //velocity along directions

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
		if (camber < 0.0)
			camber = -camber;

		//
		//ok, we can now calculate the correct Y!
		VCross(Y, X, Z);
		//note: no need to normalize, since both X and Y are unit.
		//


		//slip+slip_angle:

		//(copy the position (damn, c++ doesn't allow vector assignment, right now...)
		pos[0] = contact[i].geom.pos[0];
		pos[1] = contact[i].geom.pos[1];
		pos[2] = contact[i].geom.pos[2];
		//)

		//slip: get velocity of contact point (as a point on each body, take the difference)
		//and slip is the size/length tangental to surface...
		dBodyGetPointVel(wbody, pos[0], pos[1], pos[2], v1);

		//not sure the other geom got a body...
		if (obody) //the surface got a body, so slip is relative to both
		{
			//get other vel
			dBodyGetPointVel(obody, pos[0], pos[1], pos[2], v2);
			//relative vel
			vel[0]=v1[0]-v2[0];
			vel[1]=v1[1]-v2[1];
			vel[2]=v1[2]-v2[2];
		}
		else //not body...
		{
			//vel=v1
			vel[0]=v1[0];
			vel[1]=v1[1];
			vel[2]=v1[2];
		}

		//get velocity along the simulation X, Y and Z:
		velX = VDot(X, vel);
		velY = VDot(Y, vel);
		velZ = VDot(Z, vel);

		//TODO: the slip should be the ratio between v1 and v2 (v1/v2-1)...
		slip_ratio = velX; //slip is along x

		//slip_angle: angle (in degrees) between X and direction of slip. the velocity up/down
		//(velZ) is not part of this, since only the tangental movement along the surface has to
		//do with the actual wheel vs surface friction stuff:
		slip_angle = (180.0/M_PI)*atan(velY/velX);


		//TODO:normal force (load). don't know how to solve this, but I suspect it could
		//be calculated from stepsize, erp, cfm and mass... somehow...
		//Fz = contact[i].geom.depth*what? +/* (-)velZ*what?;


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



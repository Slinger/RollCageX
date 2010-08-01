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

//simulation of wheel (mf5.2)
void Wheel::Set_Contacts(dGeomID wheel, dGeomID other, dContact *contact, int count)
{
	//variables:
	//directions (X:longitudinal/wheel heading, Y:lateral/sideway, Z:normal/"up"):
	dReal X[3], Y[3], Z[3];
	//angles:
	dReal Acamber, Aslip;
	//forces
	dReal Fx, Fy, Fz;
	//slip:
	dReal slip;

	for (int i=0; i<count; ++i)
	{
		//1) input values:
		//directions:
		//Z: vertical to ground/normal direction
		Z[0] = contact[i].geom.normal[0];
		Z[1] = contact[i].geom.normal[1];
		Z[2] = contact[i].geom.normal[2];

		//Y: simply wheel axis Z
		//NOTE: this is actually incorrect, since this "Y" might not be tangental to ground.
		const dReal *rot = dGeomGetRotation(wheel);
		Y[0] = rot[2];
		Y[1] = rot[6];
		Y[2] = rot[10];

		//X: along wheel direction (perpendicular to Y and Z)
		//note: will be tangental, so ok.
		VCross (X, Z, Y);
		VNormalize(X);

		//Y is not correct (not tangental to ground), but X is, and Z is also ok.
		//But luckily, ode will recalculate this dir internally, and we won't need tangental
		//for our uses... I think. NOTE TO SELF: can use cross between X and Z to find correct Y
#warning "Y incorrect for now - Y=XxZ to recalculate"

		//
		//TODO:slip input!
		//

		//angles:
		//get angle between this Y and Z (for camber)
		dReal tmp = VDot (Z, Y);
		Acamber = (180/M_PI)*acos(tmp);
		//acamber is 0-180 between axis and normal...
		//we want between wheel up and normal...
		Acamber -= 90.0;
		//this gives -90-90 but we want 0-90...
		if (Acamber < 0.0)
			Acamber = -Acamber;

		//rim (outside range for tyre)
		if (Acamber > rim_angle)
		{
			//enable the usual contact friction approximation
			contact[i].surface.mode |= dContactApprox1;

			//note: since rim_mu is used as the mu of the wheel, we already
			//got the wanted mu calculated
		}
		else //tyre
		{
			//2) compute output values:
			//TODO!

			//3) set output values:
			//enable: separate mu for dir 1&2, specify dir 1
			//(note: dir2 is automatically calculated by ode)
			contact[i].surface.mode |= dContactMu2 | dContactFDir1;

			contact[i].fdir1[0] = X[0];
			contact[i].fdir1[1] = X[1];
			contact[i].fdir1[2] = X[2];

			contact[i].surface.mu = 0.0; //TODO!
			contact[i].surface.mu2 = 0.0; //TODO!
		}
	}
}



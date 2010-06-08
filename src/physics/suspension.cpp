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

#include "suspension.hpp"
#include "../shared/printlog.hpp"

Suspension::Suspension(Object *obj, dBodyID b, dBodyID w, dReal t): Component(obj), body(b), wheel(w), tensor(t)
{
	printlog(2, "configuring Suspension class");
	printf("TODO: intertia tensor????...\n");

	//variables:
	torque_compensator = true;
	axis[0] = 1.0; axis[1] = 0.0; axis[2] = 0.0;
	elevation=0;
	end = 1.0;
	spring = 100.0;
	damping = 0.0;
}

dReal Suspension::RotationSpeed()
{
	const dReal *bvel = dBodyGetAngularVel(body);
	const dReal *wvel = dBodyGetAngularVel(wheel);
	dReal axis[3];
	dBodyVectorToWorld(wheel, 0,0,1, axis);

	//DOT:
	dReal brate = axis[0]*bvel[0]+axis[1]*bvel[1]+axis[2]*bvel[2];
	dReal wrate = axis[0]*wvel[0]+axis[1]*wvel[1]+axis[2]*wvel[2];

	return (wrate-brate);
}

void Suspension::LockWheel()
{
	if (torque_compensator)
	{
		const dReal *r = dBodyGetAngularVel(body);
		dBodySetAngularVel(wheel, r[0], r[1], r[2]);
		dBodySetAngularVel(wheel, r[0], r[1], r[2]);
	}
	else
	{
		/*dJointSetHinge2Param (carp->joint[1],dParamVel2,0);
		dJointSetHinge2Param (carp->joint[1],dParamFMax2,dInfinity);
		dJointSetHinge2Param (carp->joint[2],dParamVel2,0);
		dJointSetHinge2Param (carp->joint[2],dParamFMax2,dInfinity);*/
	}
}

void Suspension::BreakWheel(dReal t, dReal step)
{
	if (torque_compensator)
	{
		const dReal *r = dBodyGetAngularVel(body); //in case stopping wheel
		dReal rotation, torque_needed;
		dReal torque[4] = {0,0,0,0};

		int i;
		for (i=0; i<4; ++i)
		{
			rotation = RotationSpeed();
			torque_needed = (tensor*rotation/step); //T=I*a/t

			//negative rotation, negative values...
			if (torque_needed < 0)
			{
				//the usual situation: only enough torque to slow down the wheel
				if (-torque_needed > t)
					torque[i] = +t;
				else //wheel will stop rotating
					dBodySetAngularVel(wheel, r[0],r[1],r[2]);
			}
			else //positive rotation, positive values
			{
				//the usual situation: only enough torque to slow down the wheel
				if (torque_needed > t)
					torque[i] = -t;
				else //wheel will stop rotating
					dBodySetAngularVel(wheel, r[0],r[1],r[2]);
			}
		}

		//add breaking torques (even if possibly 0)
		dBodyAddRelTorque(wheel, 0, 0, -torque[0]);
		dBodyAddRelTorque(wheel, 0, 0, -torque[1]);
		dBodyAddRelTorque(wheel, 0, 0, torque[2]);
		dBodyAddRelTorque(wheel, 0, 0, torque[3]);
	}
	else
	{
		/*dJointSetHinge2Param (carp->joint[1],dParamVel2,0);
		dJointSetHinge2Param (carp->joint[1],dParamFMax2,carp->max_break*carp->rbreak);
		dJointSetHinge2Param (carp->joint[2],dParamVel2,0);
		dJointSetHinge2Param (carp->joint[2],dParamFMax2,carp->max_break*carp->rbreak);

		dJointSetHinge2Param (carp->joint[0],dParamVel2,0);
		dJointSetHinge2Param (carp->joint[0],dParamFMax2,carp->max_break*carp->fbreak);
		dJointSetHinge2Param (carp->joint[3],dParamVel2,0);
		dJointSetHinge2Param (carp->joint[3],dParamFMax2,carp->max_break*carp->fbreak);*/
	}
}

void Suspension::AddTorque(dReal t)
{
	if (torque_compensator)
	{
		dBodyAddRelTorque(wheel, 0, 0, t);
	}
	else
	{
		/*dJointAddHinge2Torques (carp->joint[0],0,torque[0]*carp->throttle*carp->dir*carp->fmotor);
		dJointAddHinge2Torques (carp->joint[1],0,torque[1]*carp->throttle*carp->dir*carp->rmotor);
		dJointAddHinge2Torques (carp->joint[2],0,torque[2]*carp->throttle*carp->dir*carp->rmotor);
		dJointAddHinge2Torques (carp->joint[3],0,torque[3]*carp->throttle*carp->dir*carp->fmotor);*/
	}
}

void Suspension::TurnWheel(dReal a)
{

		/*dJointSetHinge2Param (carp->joint[0],dParamLoStop,carp->steering*carp->dir *carp->fsteer);
		dJointSetHinge2Param (carp->joint[0],dParamHiStop,carp->steering*carp->dir *carp->fsteer);
		dJointSetHinge2Param (carp->joint[3],dParamLoStop,carp->steering*carp->dir *carp->fsteer);
		dJointSetHinge2Param (carp->joint[3],dParamHiStop,carp->steering*carp->dir *carp->fsteer);

		dJointSetHinge2Param (carp->joint[1],dParamLoStop,carp->steering*carp->dir *carp->rsteer);
		dJointSetHinge2Param (carp->joint[1],dParamHiStop,carp->steering*carp->dir *carp->rsteer);
		dJointSetHinge2Param (carp->joint[2],dParamLoStop,carp->steering*carp->dir *carp->rsteer);
		dJointSetHinge2Param (carp->joint[2],dParamHiStop,carp->steering*carp->dir *carp->rsteer);*/
}

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

void Suspension::Physics_Step(dReal step)
{
	/*
	 * find forces/torques that will keep suspension bodies as wanted
	 * 1: find out how much different the position/rotation of wheel is from wanted
	 * 2: determine velocity/rotation rate needed for correcting everything in one step
	 * 3: compare to already existing velocity/rotation rate and see how much to change
	 * 4: determine the needed linear/rotational acceleration for this change
	 * 5: find what forces/torques will give this acceletation
	 * 6: send forces to ode
	 * 7: suspension spring force (and damping)
	 */

	bool forces_on_z = false;
	//1:
	//b
	/*dBodyGetVelocityAtPoint();
	dBodyGetRotationRateAtPoint();
	//w
	dBodyGetVelocity();
	dBodyGetRotationRate();*/
	//2:
	/*delta/step...*/
	//3:
	/*v-v.old...*/
	//4:
	//NOTE: since ode uses an iterative method in which velocity is changed by forces
	//_before_ movement (and not during movement), it's not necessary (or even wanted)
	//to compute these forces as if the acceleration would be applied during the movement
	//So one can simply just focus on the wanted acceleration!
	/*acceleration = diff/step;*/
	//5:
	/*check ode mass matrix for each of the two bodies (should contain inertia tensor and similar)
	 * TODO: how does ode figure out gyroscopic simulation? will probably be needed...*/
	//6:
	/*dBodyAddTorque();
	dBodyAddTorque();
	dBodyAddForce();
	dBodyAddForce();*/
	
	//7:
	/*dBodyAddForce(z*spring);
	dBodyAddForce(-z*spring);
	damping...*/
}


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

void Suspension::LockWheel(dReal step)
{
	AddTorque(ForceToStop(step)); //add the torque needed in one step
}

void Suspension::BreakWheel(dReal t, dReal step)
{
	if (torque_compensator)
	{
		dReal torque_needed;
		dReal torque;

		int i;
		for (i=0; i<4; ++i)
		{
			torque_needed = ForceToStop(step);

			//negative rotation, negative values...
			if (torque_needed < 0)
			{
				//the usual situation: only enough torque to slow down the wheel
				if (-torque_needed > t)
					torque = -t;
				else //wheel will stop rotating
					torque=torque_needed;
			}
			else //positive rotation, positive values
			{
				//the usual situation: only enough torque to slow down the wheel
				if (torque_needed > t)
					torque = +t;
				else //wheel will stop rotating
					torque=torque_needed;
			}
		}

		//add breaking torques (even if possibly 0)
		AddTorque(torque);
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

dReal Suspension::ForceToStop(dReal step)
{
	if (torque_compensator)
	{
		dReal rotation = RotationSpeed();
		return -(tensor*rotation/step); //T=I*a/t
	}
	else
	{
		return 0;
		//TODO!
	}
}

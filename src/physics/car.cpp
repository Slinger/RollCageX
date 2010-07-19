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

#include "../shared/car.hpp"
#include "../shared/internal.hpp"

void Car::Physics_Step(dReal step)
{
	Car *carp = head;
	bool antigrav;
	while (carp != NULL)
	{
		//first flipover detection (+ antigrav forces)

		//both sensors are triggered, not flipping, only antigrav
		if (carp->sensor1->colliding && carp->sensor2->colliding)
			antigrav = true;
		//only one sensor, flipping+antigrav
		else if (carp->sensor1->colliding)
		{
			antigrav = true;
			carp->dir = 1.0;
		}
		//same
		else if (carp->sensor2->colliding)
		{
			antigrav = true;
			carp->dir = -1.0;
		}
		//no sensor active, no flipping, no antigrav
		else
			antigrav = false;

		//sensors have been read, reset them
		carp->sensor1->colliding = false;
		carp->sensor2->colliding = false;

		if (antigrav) //TODO
		{
//			dBodyAddRelForce (carp->bodyid,0,0, carp->dir*100);
		}

		//control
		if (carp->drift_breaks) //breaks (lock rear wheels)
		{
			if (carp->torque_compensator)
			{
				const dReal *r = dBodyGetAngularVel(carp->bodyid);
				dBodySetAngularVel(carp->wheel_body[1], r[0], r[1], r[2]);
				dBodySetAngularVel(carp->wheel_body[2], r[0], r[1], r[2]);
			}
			else
			{
				dJointSetHinge2Param (carp->joint[1],dParamVel2,0);
				dJointSetHinge2Param (carp->joint[1],dParamFMax2,dInfinity);
				dJointSetHinge2Param (carp->joint[2],dParamVel2,0);
				dJointSetHinge2Param (carp->joint[2],dParamFMax2,dInfinity);
			}
		}
		else //acceleration or soft (non-locking) breaks
		{
			//make sure drift break is not locked...
			if (!carp->torque_compensator)
			{
				dJointSetHinge2Param (carp->joint[1],dParamFMax2,0);
				dJointSetHinge2Param (carp->joint[2],dParamFMax2,0);
			}

			//collect wheel torques wanted:
			dReal torque[4];
			dReal rotation, gear_torque;
			int i;
			for (i=0; i<4; ++i)
			{
				//wheel rotation
				rotation = dJointGetHinge2Angle2Rate (carp->joint[i]);

				dReal absrotation; //always positive
				if (rotation < 0.0)
					absrotation=-rotation;
				else
					absrotation=rotation;

				//how much torque motor could add at this rotation
				gear_torque=carp->max_torque/(1+absrotation*carp->gear_tweak);


				//check if accelerating or decelerating wheel...
				
				//if throttle and and current direction is the same, accelerate. or if rotation is
				//wrong way, but so slow that the motor is better than breaks, also use accelerate
				if (	(rotation > 0.0 && carp->throttle > 0.0) ||
					(rotation < 0.0 && carp->throttle < 0.0) ||
					(gear_torque > carp->max_break) )
				{
					//make sure we don't exceed gloobal wheel rotation limit
					if (absrotation > internal.max_wheel_rotation) //in air and rotating too fast
					{
						torque[i]=0.0; //make sure no torque
						continue; //go to next wheel
					}

					//if front wheel, front motor, if rear, rear motor
					if (i == 0 || i == 3)
						torque[i]=carp->fmotor*gear_torque;
					else
						torque[i]=carp->rmotor*gear_torque;

				}
				else //wheel must break (before it can start rotating in wanted direction)
				{
					//if front wheel, front break, if rear, rear break
					if (i == 0 || i == 3)
						torque[i]=carp->fbreak*carp->max_break;
					else
						torque[i]=carp->rbreak*carp->max_break;
				}
			}

			//apply torques:
			if (carp->torque_compensator) //only on wheels
			{
				dBodyAddRelTorque(carp->wheel_body[0], 0, 0, -torque[0]*carp->throttle*carp->dir);
				dBodyAddRelTorque(carp->wheel_body[1], 0, 0, -torque[1]*carp->throttle*carp->dir);
				dBodyAddRelTorque(carp->wheel_body[2], 0, 0, torque[2]*carp->throttle*carp->dir);
				dBodyAddRelTorque(carp->wheel_body[3], 0, 0, torque[3]*carp->throttle*carp->dir);
			}
			else //between wheels and body
			{
				dJointAddHinge2Torques (carp->joint[0],0,torque[0]*carp->throttle*carp->dir);
				dJointAddHinge2Torques (carp->joint[1],0,torque[1]*carp->throttle*carp->dir);
				dJointAddHinge2Torques (carp->joint[2],0,torque[2]*carp->throttle*carp->dir);
				dJointAddHinge2Torques (carp->joint[3],0,torque[3]*carp->throttle*carp->dir);
			}
		}

		//steering
		dJointSetHinge2Param (carp->joint[0],dParamLoStop,carp->steering*carp->dir *carp->fsteer);
		dJointSetHinge2Param (carp->joint[0],dParamHiStop,carp->steering*carp->dir *carp->fsteer);
		dJointSetHinge2Param (carp->joint[3],dParamLoStop,carp->steering*carp->dir *carp->fsteer);
		dJointSetHinge2Param (carp->joint[3],dParamHiStop,carp->steering*carp->dir *carp->fsteer);

		dJointSetHinge2Param (carp->joint[1],dParamLoStop,carp->steering*carp->dir *carp->rsteer);
		dJointSetHinge2Param (carp->joint[1],dParamHiStop,carp->steering*carp->dir *carp->rsteer);
		dJointSetHinge2Param (carp->joint[2],dParamLoStop,carp->steering*carp->dir *carp->rsteer);
		dJointSetHinge2Param (carp->joint[2],dParamHiStop,carp->steering*carp->dir *carp->rsteer);


		//save car velocity
		const dReal *vel = dBodyGetLinearVel (carp->bodyid);
		const dReal *rot = dBodyGetRotation  (carp->bodyid);
		carp->velocity = (rot[1]*vel[0] + rot[5]*vel[1] + rot[9]*vel[2]);

		//done, next car...
		carp=carp->next;
	}
}

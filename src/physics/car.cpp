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
		if (carp->drift_breaks)
		{
			carp->suspension[1]->LockWheel();
			carp->suspension[2]->LockWheel();
		}
		else if (carp->breaks)
		{
			//breaking needs stepsize to determine if stopping...
			carp->suspension[0]->BreakWheel(carp->max_break*carp->fbreak, step);
			carp->suspension[1]->BreakWheel(carp->max_break*carp->rbreak, step);
			carp->suspension[2]->BreakWheel(carp->max_break*carp->rbreak, step);
			carp->suspension[3]->BreakWheel(carp->max_break*carp->fbreak, step);
		}
		else //acceleration
		{
			dReal torque[4];
			int i;
			for (i=0; i<4; ++i)
			{
				//add torques directly (no "motor")
				dReal rotation = carp->suspension[i]->RotationSpeed();

				//we want total speed, not negative
				if (rotation < 0)
					rotation = -rotation;

				//in case wheel is already rotating so fast we get simulation errors, no simulation
				//only when wheel is in air
				if ( !(carp->wheel_geom_data[i]->colliding) && rotation > internal.max_wheel_rotation)
					torque[i] = 0.0;
				else
				{
					//else we will add torque
					//motor torque is geared by stepless gearbox
					torque[i]=carp->max_torque/(1+rotation*carp->motor_tweak);
				}

				//since we are using the wheel collision detection, reset it each time
				carp->wheel_geom_data[i]->colliding = false; //reset
			}

			carp->suspension[0]->AddTorque(-torque[0]*carp->throttle*carp->dir*carp->fmotor);
			carp->suspension[1]->AddTorque(-torque[1]*carp->throttle*carp->dir*carp->rmotor);
			carp->suspension[2]->AddTorque(torque[2]*carp->throttle*carp->dir*carp->rmotor);
			carp->suspension[3]->AddTorque(torque[3]*carp->throttle*carp->dir*carp->fmotor);
		}

		carp->suspension[0]->TurnWheel(carp->steering*carp->dir*carp->fsteer);
		carp->suspension[1]->TurnWheel(carp->steering*carp->dir*carp->rsteer);
		carp->suspension[2]->TurnWheel(carp->steering*carp->dir*carp->rsteer);
		carp->suspension[3]->TurnWheel(carp->steering*carp->dir*carp->fsteer);

		//save car velocity
		const dReal *vel = dBodyGetLinearVel (carp->bodyid);
		const dReal *rot = dBodyGetRotation  (carp->bodyid);
		carp->velocity = (rot[1]*vel[0] + rot[5]*vel[1] + rot[9]*vel[2]);

		//done, next car...
		carp=carp->next;
	}
}

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
	int i;
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

		//rotation speed of wheels
		dReal rotv[4];
		for (i=0; i<4; ++i)
			rotv[i] = dJointGetHinge2Angle2Rate (carp->joint[i]);

		//save car velocity
		//(calculated from the avarage wheel rotation)
		carp->velocity = carp->dir*carp->wheel->radius*(rotv[0]+rotv[1]+rotv[2]+rotv[3])/4;


		//calculate turning:
		dReal A[4];

		//length from turning axis to front and rear wheels
		dReal L1 = (carp->wy*2.0)*carp->dsteer;
		dReal L2 = (carp->wy*2.0)*(carp->dsteer-1.0);

		//turning radius (done by _assuming_ turning only with front wheels - but works for all situations)
		dReal maxsteer = carp->max_steer*(M_PI/180.0); //can steer more than this

		//should not steer more than this (-turning radius = v²*conf- and then calculated into turning angle)
		dReal limitsteer = atan( (carp->wy*2.0) /(carp->steerdecr * fabs(carp->velocity)) );

		//if limit is within what the actual turning can handle, use the limited as the max:
		if (limitsteer < maxsteer)
			maxsteer = limitsteer;

		//calculate turning radius
		dReal R = (carp->wy*2.0)/tan(maxsteer*carp->steering*carp->dir);

		if (carp->smart_steer)
		{
			//turning angle of each wheel:
			A[0] = atan(L1/(R-carp->wx));
			A[1] = atan(L2/(R-carp->wx));
			A[2] = atan(L2/(R+carp->wx));
			A[3] = atan(L1/(R+carp->wx));
		}
		else //dumb
		{
			//different distribution and direction of fron and rear wheels
			dReal front = maxsteer*carp->dsteer*carp->steering*carp->dir;
			dReal rear = -maxsteer*(1.0-carp->dsteer)*carp->steering*carp->dir;
			A[0] = front;
			A[1] = rear;
			A[2] = rear;
			A[3] = front;
		}

		//apply steering
		dJointSetHinge2Param (carp->joint[0],dParamLoStop,A[0]);
		dJointSetHinge2Param (carp->joint[0],dParamHiStop,A[0]);
		dJointSetHinge2Param (carp->joint[1],dParamLoStop,A[1]);
		dJointSetHinge2Param (carp->joint[1],dParamHiStop,A[1]);
		dJointSetHinge2Param (carp->joint[2],dParamLoStop,A[2]);
		dJointSetHinge2Param (carp->joint[2],dParamHiStop,A[2]);
		dJointSetHinge2Param (carp->joint[3],dParamLoStop,A[3]);
		dJointSetHinge2Param (carp->joint[3],dParamHiStop,A[3]);
		//

		//breaking/accelerating:

		//(might be needed):
		dReal kpower = carp->dir*carp->motor_power*carp->throttle;

		//(rear and front break: "half of ratio of max by throttle")
		dReal kfbreak = carp->dir*carp->max_break*0.5*carp->dbreak*carp->throttle;
		dReal krbreak = carp->dir*carp->max_break*0.5*(1.0-carp->dbreak)*carp->throttle;

		//front wheels use front breaks, rear wheels use rear breaks
		dReal kbreak[4] = {kfbreak, krbreak, krbreak, kfbreak};

		dReal kinertia = carp->wheel->inertia;
		dReal kdiffres = carp->diffres;
		dReal kairtorque = carp->airtorque;

		dReal radius[4] = {0,0,0,0}; //turning radius (always correct for all wheels)
		dReal r[4] = {0,0,0,0}; //turning radius of each wheel (set to 0 in some situations)


		dReal t[4] = {0,0,0,0}; //torque on each wheel

		if (carp->drift_breaks) //breaks (lock rear wheels)
		{
			//apply torque on rear wheels to lock them
			t[1] = -rotv[1]*kinertia/step;
			t[2] = -rotv[2]*kinertia/step;
		}
		else //acceleration or soft (non-locking) breaks
		{
			if (carp->smart_drive)
			{
				//I'm not confident letting the following calculations process infinite turning radius (no turning)...
				if (R == dInfinity || R == -dInfinity)
				{
					//no turning, set all radius equal. 1 would be a good choice...
					//all wheels:
					radius[0]=1.0; radius[1]=1.0; radius[2]=1.0; radius[3]=1.0;

					//only set these ones to 1 for needed
					if (carp->fwd)
					{
						r[0] = 1.0;
						r[3] = 1.0;
					}
					if (carp->rwd)
					{
						r[1] = 1.0;
						r[2] = 1.0;
					}
				}
				else //ok, lets try this out!
				{
					//length between turning point and right/left wheels
					//(will accompany L1 and L2 values)
					dReal R1 = R-(carp->wx);
					dReal R2 = R+(carp->wx);

					//calculate radius for all wheels
					radius[0] =  sqrt(R1*R1+L1*L1); //right front
					radius[1] =  sqrt(R1*R1+L2*L2); //right rear
					radius[2] =  sqrt(R2*R2+L2*L2); //left rear
					radius[3] =  sqrt(R2*R2+L1*L1); //left front

					//(for those wheels not used the radius will be 0)
					if (carp->fwd)
					{
						r[0] = radius[0];
						r[3] = radius[3];
					}
					if (carp->rwd)
					{
						r[1] = radius[1];
						r[2] = radius[2];
					}
				}


				//wheel rotation speed (if to apply torque, breaks or both)
				for (i=0; i<4; ++i)
				{
					//if rotating in the oposite way of wanted, use breaks
					if (rotv[i]*kpower < 0.0) //(different signs makes negative)
					{
						//this much force (in this direction) is needed to break wheel
						dReal force = -rotv[i]*kinertia/step;

						//ok, lets see if we got enough break power to come to halt:
						if ( force/kbreak[i] < 1.0) //force is smaller than breaking force
						{
							//break as much as needed...
							t[i] = force;
							//...and apply torque (if got motor)
						}
						else
						{
							//and break as much as possible
							t[i] = kbreak[i];

							//0 -> disabled motor on this one
							r[i] = 0;
						}
					}
				}

				//ok, can start working out how much torque to apply to wheels:
				//engine+gearbox (given output rotation, how much torque can be generated)

				//must be accelerating and any of the wheels must have a turning radius
				if (kpower != 0 && (r[0] != 0 || r[1] != 0 || r[2] != 0 || r[3] != 0) )
				{
					dReal torque;
					dReal avrg_rotv; //avarage rotation
					if (carp->fwd && carp->rwd)
						avrg_rotv = fabs(rotv[0]+rotv[1]+rotv[2]+rotv[3])/4.0;
					else if (carp->rwd)
					{
						avrg_rotv = fabs(rotv[1]+rotv[2])/2.0;

						//"disable" front wheels
						r[0] = 0.0;
						r[3] = 0.0;
					}
					else
					{
						avrg_rotv = fabs(rotv[0]+rotv[3])/2.0;
						r[1] = 0.0;
						r[2] = 0.0;
					}

					//is in working range for gearbox?
					if (avrg_rotv < carp->gear_limit)
						torque = kpower/carp->gear_limit;
					else
						torque = kpower/avrg_rotv;

					//ok, now distribute torque over wheels (based on turning radius compared to totabl)
					dReal r_tot = r[0]+r[1]+r[2]+r[3]; //sum of all radius
					t[0] += torque*r[0]/r_tot;
					t[1] += torque*r[1]/r_tot;
					t[2] += torque*r[2]/r_tot;
					t[3] += torque*r[3]/r_tot;
				}
				//(if not, it means all wheels are breaking or something, and we don't set any torque)
	


				//currently, we only apply torque like a (slightly smarter) differential, which opens for the
				//notorious situation where one wheel (like in air) starts rotating more than the others.
				//In order to prevent this, redirect the torque to even out the rotations of wheels.
				//lets make it more intelligent: different rotation allowed based on turning angle... :-)
				if (carp->fwd && carp->rwd)
				{
					dReal tot = (rotv[0]/radius[0]+rotv[1]/radius[1]
							+rotv[2]/radius[2]+rotv[3]/radius[3])/4.0;

					t[0] -= (rotv[0]-radius[0]*tot)*kdiffres;
					t[1] -= (rotv[1]-radius[1]*tot)*kdiffres;
					t[2] -= (rotv[2]-radius[2]*tot)*kdiffres;
					t[3] -= (rotv[3]-radius[3]*tot)*kdiffres;
				}
				else if (carp->rwd)
				{
					dReal tot = (rotv[1]/radius[1]+rotv[2]/radius[2])/2.0;
					t[1] -= (rotv[1]-radius[1]*tot)*kdiffres;
					t[2] -= (rotv[2]-radius[2]*tot)*kdiffres;
				}
				else
				{
					dReal tot = (rotv[0]/radius[0]+rotv[3]/radius[3])/2.0;
					t[0] -= (rotv[0]-radius[0]*tot)*kdiffres;
					t[3] -= (rotv[3]-radius[3]*tot)*kdiffres;
				}


			}
			else //dumb motors (one motor+gearbox for each wheel)
			{
				//torque for wheels
				dReal power[4] = {0,0,0,0};
				if (carp->fwd && carp->rwd)
				{
					power[0] = kpower/4.0;
					power[1] = kpower/4.0;
					power[2] = kpower/4.0;
					power[3] = kpower/4.0;
				}
				else if (carp->fwd)
				{
					power[0] = kpower/2.0;
					power[3] = kpower/2.0;
				}
				else
				{
					power[1] = kpower/2.0;
					power[2] = kpower/2.0;
				}

				//wheel rotation speed
				for (i=0; i<4; ++i)
				{
					//break
					if (rotv[i]*kpower < 0.0)
					{
						dReal force = -rotv[i]*kinertia/step;

						//can only break
						if (force/kbreak[i] > 1.0)
						{
							t[i] = kbreak[i];
							power[i] = 0.0;
						}
					}

					rotv[i] = fabs(rotv[i]);

					if (rotv[i] < carp->gear_limit)
						t[i] += power[i]/carp->gear_limit;
					else
						t[i] += power[i]/rotv[i];
				}
			}
		}

		//if a wheel is in air, lets limit the torques
		for (i=0; i<4; ++i)
		{
			//is in air
			if (!carp->wheel_geom_data[i]->colliding)
			{
				//above limit...
				if (t[i] > kairtorque)
					t[i] = kairtorque;
				else if (t[i] < -kairtorque)
					t[i] = -kairtorque;
			}

			//reset "wheel sensor" for next time
			carp->wheel_geom_data[i]->colliding=false;
		}

		//apply torques
		dJointAddHinge2Torques (carp->joint[0],0, t[0]);
		dJointAddHinge2Torques (carp->joint[1],0, t[1]);
		dJointAddHinge2Torques (carp->joint[2],0, t[2]);
		dJointAddHinge2Torques (carp->joint[3],0, t[3]);

		//

		//done, next car...
		carp=carp->next;
	}
}

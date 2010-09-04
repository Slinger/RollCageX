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

		//calculate turning:

		dReal A[4];

		//length from turning axis to front and rear wheels
		dReal L1 = (carp->wy*2.0)*carp->dsteer;
		dReal L2 = (carp->wy*2.0)*(carp->dsteer-1.0);

		//turning radius (done by _assuming_ turning only with front wheels - but works for all situations)
		dReal maxsteer = carp->max_steer*(M_PI/180.0);
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
			dReal front = carp->dsteer*carp->steering*carp->dir;
			dReal rear = -(1.0-carp->dsteer)*carp->steering*carp->dir;
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
		if (carp->drift_breaks) //breaks (lock rear wheels)
		{
			dJointSetHinge2Param (carp->joint[1],dParamVel2,0);
			dJointSetHinge2Param (carp->joint[1],dParamFMax2,dInfinity);
			dJointSetHinge2Param (carp->joint[2],dParamVel2,0);
			dJointSetHinge2Param (carp->joint[2],dParamFMax2,dInfinity);
		}
		else //acceleration or soft (non-locking) breaks
		{
			//make sure drift break is not locked...
			dJointSetHinge2Param (carp->joint[1],dParamFMax2,0);
			dJointSetHinge2Param (carp->joint[2],dParamFMax2,0);

			//will be needed:
			dReal ktorque = carp->dir*carp->max_torque*carp->throttle;
			//(rear and front break: "half of ratio of max by throttle")
			dReal kfbreak = carp->dir*carp->max_break*0.5*carp->dbreak*carp->throttle;
			dReal krbreak = carp->dir*carp->max_break*0.5*(1.0-carp->dbreak)*carp->throttle;
			//front wheels use front breaks, rear wheels use rear breaks
			dReal kbreak[4] = {kfbreak, krbreak, krbreak, kfbreak};
			dReal gt = carp->gear_tweak;
			dReal wt = carp->wheel_inertia;

			//values to calculate:
			dReal r[4] = {0,0,0,0}; //turning radius of each wheel
			dReal w[4] = {0,0,0,0}; //rotation of each wheel
			dReal t[4] = {0,0,0,0}; //torque on each wheel
			dReal b[4] = {0,0,0,0}; //wheel breaks

			if (carp->smart_drive)
			{
				//TODO: these breaks and "motors" should probably be equipped with antispin.
				//(perhaps something that can be disabled by the driver by holding down a button?)

				//ok, "computerized/intelligent differential"...
				//right now, there's no ESP here. So just assume that all wheels are
				//rotating as we want (but in reality one or another wheel might be rotating
				//too fast or be locked). So what this does is simply distribute the motor
				//power over the wheels based on the turning radius of each wheel.

				//based on the turning radius of each wheel:
				// * torque (wanted/optimal) on each wheel relative to all wheels calculated
				// * based on the current rotation each wheel gearbox will require different
				//   ammount of torque to ive this - calculate how this differs
				// * the motor torque available is distributed based on above

				//(by combining the individual math calculations for these steps a much smaller
				//equation was found (but less intuitive), which will be used)

				//a note about the gearbox: each wheel got its own stepless gearbox, which takes
				//an input torque (t) and based on the rotation of the wheel (r) adds t/(1+r)
				//ammount of torque on the wheel. the "+1" is to prevent division by 0...

				//I'm not confident letting the following calculations process infinite turning radius (no turning)...
				if (R == dInfinity || R == -dInfinity)
				{
					//no turning, set all radius equal. 1 would be a good choice...
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

					//calculate radius as needed
					//(for those wheels not used the radius will be 0)
					if (carp->fwd)
					{
						r[0] = sqrt(R1*R1+L1*L1); //right front
						r[3] = sqrt(R2*R2+L1*L1); //left front
					}
					if (carp->rwd)
					{
						r[1] = sqrt(R1*R1+L2*L2); //right rear
						r[2] = sqrt(R2*R2+L2*L2); //left rear
					}
				}

				//wheel rotation speed
				for (i=0; i<4; ++i)
				{
					dReal rotation = dJointGetHinge2Angle2Rate (carp->joint[i]);

					//set rotation to absolute
					w[i] = fabs(rotation);

					//if rotating in the oposite way of wanted, use breaks
					if (rotation*ktorque < 0.0) //(different signs makes negative)
					{
						//this much force (in this direction) is needed to break wheel
						dReal force = -rotation*wt/step;

						//ok, lets see if we got enough break power to come to halt:
						if ( force/kbreak[i] < 1.0) //force is smaller than breaking force
						{
							//break as much as needed...
							b[i] = force;
							//...and apply torque (if got motor)
						}
						else
						{
							//0 -> disabled motor on this one
							r[i] = 0;
							w[i] = 0;

							//and break as much as possible
							b[i] = kbreak[i];
						}
					}
				}

				//any of the wheels must have a turning radius?
				if (r[0] != 0 || r[1] != 0 || r[2] != 0 || r[3] != 0)
				{
					//this to calculate distribution of motor torque over wheels:
					//"torque koefficient"
					dReal K = ktorque/(r[0]*(gt+w[0]) +r[1]*(gt+w[1]) +r[2]*(gt+w[2]) +r[3]*(gt+w[2]));

					t[0] = r[0]*K;
					t[1] = r[1]*K;
					t[2] = r[2]*K;
					t[3] = r[3]*K;
				}
				//(if not, it means all wheels are breaking or something, and we don't set any torque)
			}
			else //dumb motors
			{
				//(a simplified version of above)

				//all wheels
				dReal torque[4] = {0,0,0,0};
				if (carp->fwd && carp->rwd)
				{
					torque[0] = ktorque/4.0;
					torque[1] = ktorque/4.0;
					torque[2] = ktorque/4.0;
					torque[3] = ktorque/4.0;
				}
				else if (carp->fwd)
				{
					torque[0] = ktorque/2.0;
					torque[3] = ktorque/2.0;
				}
				else
				{
					torque[1] = ktorque/2.0;
					torque[2] = ktorque/2.0;
				}

				//wheel rotation speed
				for (i=0; i<4; ++i)
				{
					dReal rotation = dJointGetHinge2Angle2Rate (carp->joint[i]);

					if (rotation*ktorque < 0.0)
					{
						dReal force = -rotation*wt/step;

						//comes to halt, can accelerate
						if (force/kbreak[i] < 1.0)
						{
							b[i] = force;
							t[i] = torque[i]/(gt+fabs(rotation));;
						}
						else
							b[i] = kbreak[i];
					}
					else
						t[i] = torque[i]/(gt+fabs(rotation));
				}

			}

			//apply torques:
			//(sum torque and break variables - one of them is always zero)
			dJointAddHinge2Torques (carp->joint[0],0, (t[0] + b[0]));
			dJointAddHinge2Torques (carp->joint[1],0, (t[1] + b[1]));
			dJointAddHinge2Torques (carp->joint[2],0, (t[2] + b[2]));
			dJointAddHinge2Torques (carp->joint[3],0, (t[3] + b[3]));
		}
		//

		//save car velocity
		const dReal *vel = dBodyGetLinearVel (carp->bodyid);
		const dReal *rot = dBodyGetRotation  (carp->bodyid);
		carp->velocity = (rot[1]*vel[0] + rot[5]*vel[1] + rot[9]*vel[2]);

		//done, next car...
		carp=carp->next;
	}
}

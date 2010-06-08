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

//NOTE: this is a very specialized suspension implementation:
//it assumes the suspension is along Z of body, the wheel axis
//is through Z of wheel body, and suspension works both ways...
//it also (optionally) supports "torque compensator" which will
//only add forces to wheels (and no opposing torque back on car)

#ifndef _RCX_SUSPENSION_H
#define _RCX_SUSPENSION_H

#include "../shared/object.hpp"
#include <ode/ode.h>

class Suspension:public Component
{
	public:
		static void Physics_Step(dReal step);
		Suspension(Object *obj, dBodyID b, dBodyID w, dReal t);
		void Elevate (dReal e);
		void SetProperties(dReal e, dReal s, dReal d);

		dReal RotationSpeed();
		void TurnWheel (dReal a);
		void AddTorque(dReal t);
		void LockWheel(dReal step);
		void BreakWheel(dReal t, dReal step);

		bool torque_compensator; //remove opposing torque on body?

	private:
		dReal ForceToStop(dReal step); //how much force needed to stop rotation
		dBodyID body,wheel;
		dReal axis[3];
		dReal elevation;
		dReal end, spring, damping;
		dReal tensor; //TODO: remove and replace
		//todo: inertia tensor, etc..
};

#endif

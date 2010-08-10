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

#ifndef _RCX_WHEEL_H
#define _RCX_WHEEL_H

#include <ode/ode.h>

//wheel friction simulation class (created by Car_Template, used by Physics/Geom.cpp)

class Wheel
{
	public:
		//configures contact points with correct mu and similar
		void Set_Contacts(dGeomID wg, dBodyID wb, dGeomID og, dBodyID ob, dContact *contact, int count);
		//wheel geom and body, other geom and body, contact structs to configure, and how many contacts)

	private:
		//not allowing creation and modifying of class unless by friend
		Wheel();

		//mf5.2 data:
		//TODO!

		//extra data:
		dReal rim_angle;
		dReal spring, damping;

		//only car template (wheen loading) is allowed
		friend class Car_Template;
};

#endif

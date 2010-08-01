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
		void Set_Contacts(dGeomID wheel, dGeomID other, dContact *contact, int count);

	private:
		//not allowing creation and modifying of class unless by friend
		Wheel();

		//mf5.2 data:
		//TODO!

		//extra data:
		dReal rim_angle;

		//only car template (wheen loading) is allowed
		friend class Car_Template;
};

#endif

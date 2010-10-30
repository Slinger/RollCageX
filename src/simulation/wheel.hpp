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
		void Set_Contacts(dBodyID wb, dBodyID ob, class Geom *og, bool wf, dContact *contact, int count, dReal stepsize);
		//arguments:
		//wheel body, other body (or NULL if static), other geom, wheel is first geom?,
		//contact structs to configure, and how many contacts, and stepsize time)

	private:
		//not allowing creation and modifying of class unless by friend
		Wheel();

		//friction data:
		dReal xpeak;
		dReal xpeaksch;
		dReal xshape;
		dReal xpos;
		dReal xposch;
		dReal xsharp;
		dReal xsharpch;

		dReal ypeak;
		dReal ypeaksch;
		dReal yshape;
		dReal ypos;
		dReal yposch;
		dReal ysharp;
		dReal ysharpch;
		dReal yshift;

		//extra data:
		dReal rim_angle;
		dReal spring, damping;
		dReal inertia;
		dReal resistance;
		dReal radius;

		//debug data:
		bool approx1;
		dReal fixedmu;

		//only car and car template (wheen loading) is allowed
		friend class Car;
		friend class Car_Template;
};

#endif

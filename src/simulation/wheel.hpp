/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of ReCaged.
 *
 * ReCaged is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReCaged is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef _RC_WHEEL_H
#define _RC_WHEEL_H

#include <ode/ode.h>

//wheel friction simulation class (created by Car_Template, used by Physics/Geom.cpp)

class Wheel
{
	public:
		//configures contact points with correct mu and similar
		void Set_Contact(dBodyID wb, dBodyID ob, class Surface *os, bool wf, dContact *contact, int count, dReal stepsize);
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

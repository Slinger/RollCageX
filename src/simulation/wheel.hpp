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
#include <SDL/SDL_stdinc.h>

//tmp:
//extern class Geom;

//size of each chunk of the list of contact points/joints
#define INITIAL_WHEEL_LIST_SIZE 64

//wheel friction simulation class (created by Car_Template, used by Physics/Geom.cpp)
class Wheel
{
	public:
		//prepare to add contact points
		bool Prepare_Contact(dBodyID wb, dBodyID ob, class Geom *g1, class Geom *g2, class Surface *os, bool wf, dContact *contact, int count, dReal stepsize);

		//go through list, and add them
		static void Generate_Contacts(dReal stepsize);

		//make sure list is removed
		static void Clear_List();

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
		dReal join_dist;
		dReal rim_angle;
		dReal spring, damping;
		dReal inertia;
		dReal resistance;
		dReal radius;

		//debug data:
		bool approx1;
		dReal fixedmu;

		//HACK!
		friend void HUD(Uint32);

		//only car and car template (wheen loading) is allowed
		friend class Car;
		friend class Car_Template;
};

#endif

/*
 * RollCageX - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of RollCageX.
 *
 * RollCageX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RollCageX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RollCageX.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef _RCX_COLLISION_FEEDBACK_H
#define _RCX_COLLISION_FEEDBACK_H

#include <ode/ode.h>
#include "../shared/geom.hpp"

//geoms can be "damaged" by collision forces, computed from feedback of ode "collision joint"s

class Collision_Feedback
{
	public:
		Collision_Feedback(dJointID joint, Geom *g1, Geom *g2);
		static void Physics_Step(dReal step); //processes and clears list

	private:
		//data for simulation
		Geom *geom1, *geom2;
		dJointFeedback feedback;

		//data for keeping track of link members
		Collision_Feedback *next;
		static Collision_Feedback *head;
};
#endif

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

#include "timers.hpp"
#include "../shared/object.hpp"
#include "../shared/geom.hpp"

Animation_Timer *Animation_Timer::head = NULL;

Animation_Timer::Animation_Timer (Object *obj, Script *scr, dReal start, dReal stop,
		dReal duration):object(obj), script(scr), counter(start), goal(stop)
{
	speed = (stop-start)/duration;

	//increase object activity (to prevent object from selfdelete while timer is counting)
	object->Increase_Activity();

	//add to list
	next=head;
	prev=NULL;
	head=this;

	//if another timer in list
	if (next)
		next->prev=this;
}

Animation_Timer::~Animation_Timer()
{
	//remove from list
	if (prev)
		prev->next=next;
	else
		head=next;

	if (next)
		next->prev=prev;

	//timer not part of object activity anymore
	object->Decrease_Activity();
}

void Animation_Timer::Events_Step(dReal  step)
{
	Animation_Timer *timer, *tmp;
	timer = head;
	while (timer)
	{
		//process timer:

		//timer->object->Modify_Variable(name_here, 'f', timer->ounter); //set script variable
		//timer->script->Execute(timer->object); //execute script (using this object for variable tables)

		//
		//instead: TMP: assumes flipper
		//

		//note: TMP: the script is actually a pointer to the flipper geom...
		dGeomID geom = (dGeomID)timer->script;
		const dReal *pos = dGeomGetPosition(geom);
		dGeomSetPosition(geom, pos[0], pos[1], timer->counter);

		//depending on which direction counter goes (increase/decrease) determine if reached goal
		if (	(timer->speed > 0 && timer->counter >= timer->goal) || //counter increased to goal
			(timer->speed < 0 && timer->counter <= timer->goal)  ) //counter decreased to goal
		{
			//pretty much the same as above, "run the same script"
			//timer->object->Modify_Variable(name_here, 'f', timer->goal); //set script variable
			//timer->script->Execute(timer->object); //execute script (using this object for variable tables)

			//TMP:
			dGeomID geom = (dGeomID)timer->script;
			const dReal *pos = dGeomGetPosition(geom);
			dGeomSetPosition(geom, pos[0], pos[1], timer->goal);

			//now that we're done elevating flipper (positive movement), start new timer for lowering it back:
			//goas from old timer's goal to goal-2, during 2 seconds (slower - looks nice)
			if (timer->speed > 0)
				new Animation_Timer(timer->object,(Script*)geom,timer->goal,(timer->goal-2.0), 2.0);
			//end of TMP

			//delete
			tmp=timer;
			timer=timer->next; //next
			delete tmp;
		}
		else //just move to next timer
		{
			//increase counter
			timer->counter += (timer->speed)*step;

			//move to next
			timer=timer->next;
		}
	}
}

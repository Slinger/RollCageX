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

#include "body.hpp"
#include "object.hpp"
#include "internal.hpp"
#include "printlog.hpp"
#include "../simulation/event_buffers.hpp"

Body *Body::head = NULL;

Body::Body (dBodyID body, Object *obj): Component(obj)
{
	printlog(2, "configuring Body class");

	//increase object activity counter
	object_parent->Increase_Activity();

	//ad it to the list
	next = head;
	head = this;
	prev = NULL;

	if (next)
		next->prev = this;
	else
		printlog(2, "(first registered)");

	//add it to the body
	dBodySetData (body, (void*)(this));
	body_id = body;

	//default values
	model = NULL; //don't render
	Update_Mass(); //get current mass...
	Set_Linear_Drag(internal.linear_drag); //...and set up drag
	Set_Angular_Drag(internal.angular_drag);//...
	buffer_event=false; //no events yet
}

//destroys a body, and removes it from the list
Body::~Body()
{
	//lets just hope the given pointer is ok...
	printlog(2, "clearing Body class");

	//remove all events
	Event_Buffer_Remove_All(this);

	//1: remove it from the list
	if (!prev) //head in list, change head pointer
		head = next;
	else //not head in list, got a previous link to update
		prev->next = next;

	if (next) //not last link in list
		next->prev = prev;

	//2: remove it from memory

	dBodyDestroy(body_id);

	//decrease activity and check if 0
	object_parent->Decrease_Activity();
}


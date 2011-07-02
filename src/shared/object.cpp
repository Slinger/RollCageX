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

#include "object.hpp"
#include "printlog.hpp"
#include "track.hpp"
#include "printlog.hpp"
#include "../simulation/event_buffers.hpp"

#include <stdlib.h>

//allocate new script storage, and add it to list
//(not used yet, only for storing 3d list pointers...)
Object_Template::Object_Template(const char *name): Racetime_Data(name)
{
	//debug identification bools set to false
	box = false;
	funbox = false;
	flipper = false;
	NH4 = false;
	building = false;
	sphere = false;
	pillar = false;

	//make sure all model pointers are null
	for (int i=0; i<10; ++i)
		model[i]=NULL;
}


Object *Object::head = NULL;

//allocate a new object, add it to the list and returns its pointer
Object::Object ()
{
	printlog(1, "creating Object");

	prev=NULL;
	next=head;
	head=this;

	if (next)
		next->prev = this;
	else
		printlog(2, "(first registered object)");

	//default values
	components = NULL;
	activity = 0;
	selected_space = NULL;
}

//destroys an object
Object::~Object()
{
	//lets just hope the given pointer is ok...
	printlog(1, "freeing Object");

	//1: remove it from the list
	if (prev == NULL) //first link
		head = next;
	else
		prev->next = next;

	if (next) //not last link
		next->prev = prev;


	//remove components
	while (components)
		delete components; //just removes the one in top each time

	//make sure no events for this object is left
	Event_Buffer_Remove_All(this);
}

void Object::Increase_Activity()
{
	++activity;
}

void Object::Decrease_Activity()
{
	if ((--activity) == 0)
		Event_Buffer_Add_Inactive(this);
}

//destroys all objects
void Object::Destroy_All()
{
	while (head)
		delete (head);
}

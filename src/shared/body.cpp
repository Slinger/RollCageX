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
	Buffer_Event_List::Remove(this);

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


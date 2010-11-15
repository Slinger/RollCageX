/*
 * RCX  Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger")
 *
 * This program comes with ABSOLUTELY NO WARRANTY!
 *
 * This is free software, and you are welcome to
 * redistribute it under certain conditions.
 *
 * See license.txt and README for more info
 */

#ifndef _RCX_EVENT_BUFFERS_H
#define _RCX_EVENT_BUFFERS_H

#include "../shared/component.hpp"
#include "../shared/geom.hpp"
#include "../shared/body.hpp"
#include "../shared/joint.hpp"

//add event to specific buffer
void Event_Buffer_Add_Depleted(Geom*);
void Event_Buffer_Add_Triggered(Geom*);
void Event_Buffer_Add_Depleted(Body*);
void Event_Buffer_Add_Depleted(Joint*);
void Event_Buffer_Add_Inactive(Object*);

//remove all events for specific thing
//(used by destructors)
void Event_Buffer_Remove_All(Geom*);
void Event_Buffer_Remove_All(Body*);
void Event_Buffer_Remove_All(Joint*);
void Event_Buffer_Remove_All(Object*);

//processes all buffered events
void Event_Buffers_Process(dReal step);

#endif

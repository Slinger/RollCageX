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

#ifndef _RC_EVENT_BUFFERS_H
#define _RC_EVENT_BUFFERS_H

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

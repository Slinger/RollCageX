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

#ifndef _RC_TIMERS_H
#define _RC_TIMERS_H

#include "../shared/object.hpp"
#include "../shared/script.hpp"

#include <ode/ode.h>
#include <SDL/SDL_stdinc.h> //Uint32

class Animation_Timer
{
	public:
		Animation_Timer (Object*, Script*, dReal start, dReal stop, dReal duration);
		~Animation_Timer();
		static void Events_Step(dReal step);

	private:
		Object *object;
		Script *script;
		dReal counter;
		dReal goal;
		dReal speed;

		Animation_Timer *next, *prev;
		static Animation_Timer *head;
};

#endif

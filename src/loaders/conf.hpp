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

#ifndef _RC_CONF_H
#define _RC_CONF_H
#include "unistd.h"
#include "stddef.h"

//note: type describes type of variable:
//i - integer
//f - floating point
//d - double floating point
//b - boolean
//R - dReal (float or double depending on ode configureaiton)
//s - string (fixed-size c array, defined as Conf_String below)

#define Conf_String_Size 100 //99 chars (plus \0)

typedef char Conf_String[Conf_String_Size];

//to make the conf loader able to find variable names in structs, use indexes
struct Conf_Index {
	const char *name;
	char type; //f for float, b for bool, i for int, 0 for end of list
	int length; //normaly 1 (or else more)
	size_t offset;
};

bool load_conf (const char *name, char *memory, const struct Conf_Index index[]);
#endif

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

#ifndef _RCX_CONF_H
#define _RCX_CONF_H
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

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

#ifndef _RCX_RUNLEVEL_H
#define _RCX_RUNLEVEL_H

//use a "runlevel" (enum) variable to make all threads/loops aware of status
typedef enum {loading, running, paused, done} runlevel_type;
extern runlevel_type runlevel;

#endif

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

#ifndef _RCX_RENDER_LISTS_H
#define _RCX_RENDER_LISTS_H

//currently just list for components (geoms+bodies)
#define INITIAL_RENDER_LIST_SIZE 150

void Render_List_Update(); //copy pos/rot
void Render_List_Render(); //render

#endif

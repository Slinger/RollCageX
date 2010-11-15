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

#ifndef _RCX_GRAPHIC_BUFFERS_H
#define _RCX_GRAPHIC_BUFFERS_H

//currently just list for components (geoms+bodies)
#define INITIAL_GRAPHIC_BUFFER_SIZE 150

void Graphic_List_Update(); //copy pos/rot
void Graphic_List_Render(); //render

#endif

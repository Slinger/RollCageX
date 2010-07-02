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

#include "geom_render.hpp"
#include "../shared/printlog.hpp"

//only allocate memory and buffers if going to render
//(and then keep the memory until exiting)
bool geom_render_enabled = false;
void Enable_Geom_Render()
{
	printlog(1, "enabling geom rendering (generating buffers)");
	//...
	geom_render_enabled = true;
}

//render geoms
void Geom_Render()
{
	if (!geom_render_enabled)
		Enable_Geom_Render();

	//...
}

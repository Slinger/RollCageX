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

#ifndef _RCX_GEOM_RENDER_H
#define _RCX_GEOM_RENDER_H

//rendering without any eyecandy:
// * no lighting
// * no materials
// * wireframe

//data regenerated for each frame. optimized for speed:
// * VBO vertex buffer for float[3] vertices
// * VBO index buffer for short[1] vertices 
// * transformed when generated, no matrices for GL

void Geom_Render();

#endif

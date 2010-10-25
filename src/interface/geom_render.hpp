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

//arbitrary, but high enough to render most small simulations
//(if not high enough, will be increased)
#define VERTEX_BLOCK 2000 //min number of vertices per mem increase
#define INDEX_BLOCK 3000 //min number of indices per mem increase

//sets level of geom rendering
extern int geom_render_level;

void Geom_Render();
void Geom_Render_Clear(); //for removing VBO+ram memory

#endif

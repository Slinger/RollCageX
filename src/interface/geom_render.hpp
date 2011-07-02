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

#ifndef _RC_GEOM_RENDER_H
#define _RC_GEOM_RENDER_H

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

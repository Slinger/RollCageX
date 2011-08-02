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

//
//this is just a quick hack to add basic OSD text output!
//
#ifndef _RC_HUD_H
#define _RC_HUD_H
#include "../simulation/wheel.hpp"
struct POINT {
	float x,y,z;
	float Fz;
};

extern struct POINT point[100], pointbuild[100];
extern int pcount, pbcount;
bool HUD_Load();
void HUD_Render_Text(const char string[], int posx, int posy); //render
void HUD_Render_Graph(float (*function)(float x), int posx, int posy, int sizex, int sizey, float x, float y, float z); //graph

#endif

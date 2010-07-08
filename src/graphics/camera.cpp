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

#include <SDL/SDL_opengl.h>
#include "../shared/camera.hpp"
//set camera view before rendering
void Camera::Graphics_Step()
{
	//TODO: replace with custom function!
		//set camera
		gluLookAt(camera.pos[0], camera.pos[1], camera.pos[2], camera.pos[0]+camera.dir[0], camera.pos[1]+camera.dir[1], camera.pos[2]+camera.dir[2], camera.up[0], camera.up[1], camera.up[2]);
}



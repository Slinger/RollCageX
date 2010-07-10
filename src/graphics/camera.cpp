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
	//already got correct (unit and perpendicular) "up" (up) and "forward" (dir)
	//use cross product to find "right" ( = | dir X up | )
	float right[3];
	right[0] = camera.dir[1]*camera.up[2] - camera.dir[2]*camera.up[1];
	right[1] = camera.dir[2]*camera.up[0] - camera.dir[0]*camera.up[2];
	right[2] = camera.dir[0]*camera.up[1] - camera.dir[1]*camera.up[0];

	//normalize
	float l = sqrt(right[0]*right[0]+right[1]*right[1]+right[2]*right[2]);
	right[0]/=l; right[1]/=l; right[2]/=l;

	//build matrix...
	GLfloat matrix[16] = {
		camera.right[0], camera.up[0], -camera.dir[0], 0.0,
		camera.right[1], camera.up[1], -camera.dir[1], 0.0,
		camera.right[2], camera.up[2], -camera.dir[2], 0.0,
		0.0,	0.0,	0.0,	1.0};


	//multiply matrix
	glMultMatrixf(matrix);

	//"move" camera
	glTranslatef(-camera.pos[0], -camera.pos[1], -camera.pos[2]);
}



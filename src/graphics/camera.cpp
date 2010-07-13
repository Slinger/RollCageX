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
	//build matrix...
	GLfloat matrix[16] = {
		camera.rotation[0], camera.rotation[2], -camera.rotation[1], 0.0,
		camera.rotation[3], camera.rotation[5], -camera.rotation[4], 0.0,
		camera.rotation[6], camera.rotation[8], -camera.rotation[7], 0.0,
		0.0,	0.0,	0.0,	1.0};
	//right, up, forward

	//multiply matrix
	glMultMatrixf(matrix);

	//"move" camera
	glTranslatef(-camera.pos[0], -camera.pos[1], -camera.pos[2]);
}



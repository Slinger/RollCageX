/*
 * RollCageX - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of RollCageX.
 *
 * RollCageX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RollCageX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RollCageX.  If not, see <http://www.gnu.org/licenses/>.
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



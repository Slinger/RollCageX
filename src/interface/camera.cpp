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

#include <SDL/SDL_opengl.h>
#include "../shared/camera.hpp"
//set camera view before rendering
void Camera::Graphics_Step()
{
	//build matrix...
	GLfloat matrix[16] = {
		//(right, up, forward)
		camera.rotation[0], camera.rotation[2], -camera.rotation[1], 0.0,
		camera.rotation[3], camera.rotation[5], -camera.rotation[4], 0.0,
		camera.rotation[6], camera.rotation[8], -camera.rotation[7], 0.0,

		matrix[12]=-matrix[0]*camera.pos[0]-matrix[4]*camera.pos[1]-matrix[8]*camera.pos[2], //m3
		matrix[13]=-matrix[1]*camera.pos[0]-matrix[5]*camera.pos[1]-matrix[9]*camera.pos[2], //m7
		matrix[14]=-matrix[2]*camera.pos[0]-matrix[6]*camera.pos[1]-matrix[10]*camera.pos[2], //m11

		1.0}; //m15

	//multiply matrix
	glMultMatrixf(matrix);
}



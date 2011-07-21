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

#include <limits.h>
#include <math.h>
#include <GL/glew.h>
#include "geom_render.hpp"
#include "../shared/threads.hpp"
#include "../shared/geom.hpp"
#include "../shared/racetime_data.hpp"
#include "../shared/printlog.hpp"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
int geom_render_level = 0;

//
//two VBO+two tmp data for "stream drawing" of graphics:
//(data is first generated in ram, then sent to vbo in vram)
//

//only allocate memory and buffers _if_ going to render
//(and then keep the memory until end of race)
bool Got_Buffers = false;
GLuint vertexVBO, colourVBO, indexVBO;

//vertices
struct geom_vertex {
	float x;
	float y;
	float z;
	float r;
	float g;
	float b;
};
geom_vertex *vertices; //when building
geom_vertex *v; //pointer for easily looping through

//indices
struct geom_index {
	unsigned int a;
	unsigned int b;
};
geom_index *indices; //when building
geom_index *i; //pointer

//keep track, so not overflowing
unsigned int vertex_size, index_size;
unsigned int vertex_usage, index_usage;


//makes sure got big enough index/vertex buffers
void Assure_Memory(unsigned int vertex_needed, unsigned int index_needed)
{
	int v_lacking = vertex_needed-(vertex_size-vertex_usage);
	int i_lacking = index_needed-(index_size-index_usage);

	//if positive, there is need for more memory than there currently is
	if (v_lacking > 0)
	{
		//try to allocate this chunk of data if possible
		if (v_lacking < VERTEX_BLOCK)
			vertex_size += VERTEX_BLOCK;
		else //no, needed even more memory...
			vertex_size += v_lacking;
		
		printlog(1, "growing geom rendering vertex buffer to %u bytes", sizeof(geom_vertex)*vertex_size);

		geom_vertex *tmp = vertices;
		vertices = new geom_vertex[vertex_size];
		memcpy(vertices, tmp, sizeof(geom_vertex)*vertex_usage);
		delete[] tmp;

		//since we've changed the memory, reconfigure pointer:
		v = &vertices[vertex_usage];
	}

	if (i_lacking > 0)
	{
		if (i_lacking < INDEX_BLOCK)
			index_size += INDEX_BLOCK;
		else
			index_size += i_lacking;
		
		printlog(1, "growing geom rendering index buffer to %u bytes", sizeof(geom_index)*index_size);

		geom_index *tmp = indices;
		indices = new geom_index[index_size];
		memcpy(indices, tmp, sizeof(geom_index)*index_usage);
		delete[] tmp;

		i = &indices[index_usage];
	}
}

//creates vbo and allocates memory
void Geom_Render_Create()
{
	printlog(1, "generating buffers for geom rendering");

	//no allocation yet
	vertices = NULL;
	vertex_size = 0;
	indices = NULL;
	index_size = 0;


	//create
	glGenBuffers(1, &vertexVBO);
	glGenBuffers(1, &colourVBO);
	glGenBuffers(1, &indexVBO);

	//ok!
	Got_Buffers = true;
}

//removes vbo and memory
void Geom_Render_Clear()
{
	//indicate now removed
	Got_Buffers = false;

	//delete
	glDeleteBuffers(1, &vertexVBO);
	glDeleteBuffers(1, &colourVBO);
	glDeleteBuffers(1, &indexVBO);

	//delete building arrays
	free(vertices);
	free(indices);
}


//
//render geoms
//

//macros to reduce repetitive typing...

//vertex (absolute rotation and position)
#define AAVertex(X,Y,Z){ \
	(v->x)=(X); \
	(v->y)=(Y); \
	(v->z)=(Z); \
	(v->r)=(colour[0]); \
	(v->g)=(colour[1]); \
	(v->b)=(colour[2]); \
	++v; ++new_vertices;}

//vertex (absolute rotation)
#define AVertex(X,Y,Z){ \
	(v->x)=(pos[0]+(X)); \
	(v->y)=(pos[1]+(Y)); \
	(v->z)=(pos[2]+(Z)); \
	(v->r)=(colour[0]); \
	(v->g)=(colour[1]); \
	(v->b)=(colour[2]); \
	++v; ++new_vertices;}

//vertex (relative rotation)
#define RVertex(X,Y,Z){ \
	(v->x)=(pos[0]+((X)*rot[0]+(Y)*rot[1]+(Z)*rot[2])); \
	(v->y)=(pos[1]+((X)*rot[4]+(Y)*rot[5]+(Z)*rot[6])); \
	(v->z)=(pos[2]+((X)*rot[8]+(Y)*rot[9]+(Z)*rot[10])); \
	(v->r)=(colour[0]); \
	(v->g)=(colour[1]); \
	(v->b)=(colour[2]); \
	++v; ++new_vertices;}

//index
#define Index(A,B){ \
	(i->a)=(vertex_usage+(A)); \
	(i->b)=(vertex_usage+(B)); \
	++i; ++new_indices;}


//change colour:
float colour[3];
void Volume_Colour(float v)
{
	//pseudorandom colour:
	colour[0]= (1.6*v);
	colour[1]= (2.3*v);
	colour[2]= (4.2*v);

	//make range -1 to 1...
	colour[0] -= floor(colour[0]);
	colour[1] -= floor(colour[1]);
	colour[2] -= floor(colour[2]);
}


//render geoms
void Geom_Render()
{
	//check if rendering
	if (!Got_Buffers)
		Geom_Render_Create();

	//build data
	vertex_usage=0;
	index_usage=0;

	//index/vertex looping pointers
	v = &vertices[0];
	i = &indices[0];

	//geom
	Geom *geom;
	dGeomID g;

	//position/rotation of geom
	const dReal *pos;
	const dReal *rot;

	//different variables for sizes
	dVector3 result, v0, v1, v2;
	dReal x,y,z,l,r;

	//misc
	unsigned int loop, new_indices, new_vertices;
	int tloop, triangles, collidingtriangles;
	float vseg = 2.0*M_PI/8.0;

	//lock ode: make sure no geoms change while we build render
	//(not likely, but just to be sure)
	SDL_mutexP(ode_mutex);
	for (geom=Geom::head; geom; geom=geom->next)
	{
		g = geom->geom_id;
		new_vertices=0;
		new_indices=0;

		//check if rendering with collision indication
		if (geom_render_level == 5)
		{
			if (geom->colliding)
			{
				colour[0]= 1.0;
				colour[1]= 0.0;
				colour[2]= 0.0;
			}
			//if not
			else
			{
				colour[0]= 0.0;
				colour[1]= 1.0;
				colour[2]= 0.0;
			}
		}

		//ok, render
		switch (dGeomGetClass(g))
		{
			case dSphereClass:
				Assure_Memory (24, 24);

				pos = dGeomGetPosition(g);
				r = dGeomSphereGetRadius(g);

				//colour based on volume (if needed)
				if (geom_render_level != 5)
					Volume_Colour(r*r*M_PI);

				//vertices:
				//circles around sphere
				for (loop=0; loop<8; ++loop) //around x
					AVertex(0.0, r*sin(loop*vseg), r*cos(loop*vseg));
				for (loop=0; loop<8; ++loop) //around y
					AVertex(r*sin(loop*vseg), 0.0, r*cos(loop*vseg));
				for (loop=0; loop<8; ++loop) //around z
					AVertex(r*sin(loop*vseg), r*cos(loop*vseg), 0.0);

				//indices:
				//circles around sphere
				for (loop=0; loop<7; ++loop) //around x
					Index(loop, loop+1);
				Index(7, 0);
				for (loop=8; loop<15; ++loop) //around y
					Index(loop, loop+1);
				Index(15, 8);
				for (loop=16; loop<23; ++loop) //around z
					Index(loop, loop+1);
				Index(23, 16);

				break;

			case dBoxClass:
				Assure_Memory (8, 12);

				pos = dGeomGetPosition(g);
				rot = dGeomGetRotation(g);
				dGeomBoxGetLengths(g, result);

				//colour based on volume (if needed)
				if (geom_render_level != 5)
					Volume_Colour(result[0]*result[1]*result[2]);

				//half of lengths
				x=result[0]/2.0;
				y=result[1]/2.0;
				z=result[2]/2.0;


				//vertices:
				RVertex(-x, -y, -z);
				RVertex(+x, -y, -z);
				RVertex(+x, -y, +z);
				RVertex(-x, -y, +z);
				RVertex(-x, +y, -z);
				RVertex(+x, +y, -z);
				RVertex(+x, +y, +z);
				RVertex(-x, +y, +z);

				//indices:
				Index(0, 1);
				Index(1, 2);
				Index(2, 3);
				Index(3, 0);

				Index(0, 4);
				Index(1, 5);
				Index(2, 6);
				Index(3, 7);

				Index(4, 5);
				Index(5, 6);
				Index(6, 7);
				Index(7, 4);

				break;

			case dCapsuleClass:
				Assure_Memory (18, 40);

				pos = dGeomGetPosition(g);
				rot = dGeomGetRotation(g);
				dGeomCapsuleGetParams(g, &r, &l);

				//colour based on volume (if needed)
				if (geom_render_level != 5)
					Volume_Colour(r*r*M_PI+r*M_PI*l);

				l/=2.0;

				//vertices:

				//circles for ends
				for (loop=0; loop<8; ++loop)
					RVertex(r*sin(loop*vseg), r*cos(loop*vseg), l);

				for (loop=0; loop<8; ++loop)
					RVertex(r*sin(loop*vseg), r*cos(loop*vseg), -l);

				//attempt on "sphere" capping
				RVertex(0,0, l+r);
				RVertex(0,0, -l-r);

				//indices:
				
				//circles
				for (loop=0; loop<7; ++loop)
					Index(loop, loop+1);
				Index(7, 0);

				for (loop=8; loop<15; ++loop)
					Index(loop, loop+1);
				Index(15, 8);

				//connect circles
				for (loop=0; loop<8; ++loop)
					Index(loop, loop+8);

				//spheres
				for (loop=0; loop<8; ++loop)
					Index(16, loop);

				for (loop=8; loop<16; ++loop)
					Index(17, loop);

				break;

			case dCylinderClass:
				Assure_Memory (18, 40);

				pos = dGeomGetPosition(g);
				rot = dGeomGetRotation(g);
				dGeomCylinderGetParams(g, &r, &l);

				//colour based on volume (if needed)
				if (geom_render_level != 5)
					Volume_Colour(r+l);

				l/=2.0; //for cleaner code, divided here

				//circles
				for (loop=0; loop<8; ++loop)
					RVertex(r*sin(loop*vseg), r*cos(loop*vseg), l);

				for (loop=0; loop<8; ++loop)
					RVertex(r*sin(loop*vseg), r*cos(loop*vseg), -l);

				for (loop=0; loop<7; ++loop)
					Index(loop, loop+1);
				Index(7, 0);

				for (loop=8; loop<15; ++loop)
					Index(loop, loop+1);
				Index(15, 8);

				//centers
				RVertex(0, 0, l);
				RVertex(0, 0, -l);

				//"spokes"
				for (loop=0; loop<8; ++loop)
					Index(16, loop);

				for (loop=8; loop<16; ++loop)
					Index(17, loop);

				//connect circles
				for (loop=0; loop<8; ++loop)
					Index(loop, loop+8);

				break;

			case dTriMeshClass:
				//check if ultimate geom rendering level
				if (geom_render_level == 2 || geom_render_level == 3 || geom_render_level == 5)
				{
					//how many triangles in trimesh
					triangles = dGeomTriMeshGetTriangleCount(g);

					//if in right level, colour triangles based on collision:
					if ( geom_render_level == 5)
					{
						//we will be rendering the colliding triangles twice:
						//count colliding triangles
						collidingtriangles=0;
						for (tloop=0; tloop<triangles; ++tloop)
							if (geom->triangle_colliding[tloop])
								++collidingtriangles;

						//make sure got memory
						Assure_Memory ((triangles+collidingtriangles)*3, (triangles+collidingtriangles)*3);

						//copy vertices (with red colour)
						colour[0]= 1.0;
						colour[1]= 0.0;
						colour[2]= 0.0;

						for (tloop=0; tloop<triangles; ++tloop)
							if (geom->triangle_colliding[tloop])
							{
								dGeomTriMeshGetTriangle(g, tloop, &v0, &v1, &v2);
								AAVertex(v0[0], v0[1], v0[2]);
								AAVertex(v1[0], v1[1], v1[2]);
								AAVertex(v2[0], v2[1], v2[2]);
							}

						//create indices (for simple triangle array rendering):
						for (tloop=0; tloop<collidingtriangles; ++tloop)
						{
							Index(tloop*3+0, tloop*3+1);
							Index(tloop*3+1, tloop*3+2);
							Index(tloop*3+2, tloop*3+0);
						}

						//set normal colour for the rest
						colour[0]= 0.0;
						colour[1]= 1.0;
						colour[2]= 0.0;


						//this way, hopefully, (thanks to depth testing) all red lines will cover the (incorrectly)
						//green ones when rendered again (below and since most lines are shared by two triangles)

						//update usage counters
						vertex_usage+=new_vertices;
						index_usage+=new_indices;
						//... and reset these counters
						new_vertices=0;
						new_indices=0;
					}
					else
					{
						Volume_Colour((float)triangles);
						Assure_Memory (triangles*3, triangles*3);
					}

					//render all triangles
					for (tloop=0; tloop<triangles; ++tloop)
					{
						//vertices (3 per tri):
						dGeomTriMeshGetTriangle(g, tloop, &v0, &v1, &v2);
						AAVertex(v0[0], v0[1], v0[2]);
						AAVertex(v1[0], v1[1], v1[2]);
						AAVertex(v2[0], v2[1], v2[2]);

						//indices (3 per tri):
						Index(tloop*3+0, tloop*3+1);
						Index(tloop*3+1, tloop*3+2);
						Index(tloop*3+2, tloop*3+0);
					}
				}

				break;

			default:
				break;
		}

		//update usage counters
		vertex_usage+=new_vertices;
		index_usage+=new_indices;
	}
	//unlock ode access
	SDL_mutexV(ode_mutex);

	//send and configure data
	//NOTE: using glBufferData to allocate vbo and send data to it in one step.
	//alternatives includes using the same allocated buffer for every frame and
	//use glMapBuffer or glBufferSubData to just send the new data. but this
	//has shown not to give any fps increase on systems I've tried. (most
	//likely the generating and sending of new data is much slower than allocation)

	//vertices+colours:
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO); //bind
	glBufferData(GL_ARRAY_BUFFER, sizeof(geom_vertex)*vertex_usage, vertices, GL_STREAM_DRAW); //alloc+init

	glVertexPointer(3, GL_FLOAT, sizeof(geom_vertex), BUFFER_OFFSET(0)); //strided
	glColorPointer(3, GL_FLOAT, sizeof(geom_vertex), BUFFER_OFFSET(sizeof(float)*3));
	
	//indices:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(geom_index)*index_usage, indices, GL_STREAM_DRAW);
	
	//check if memory problems...
	//TODO: do this after each BufferData?
	if (GLenum error = glGetError())
	{
		//should be a memory issue, but lets take a look
		if (error == GL_OUT_OF_MEMORY)
			printlog(0, "WARNING: insufficient graphics memory, can not render geoms...");
		else
			printlog(0, "ERROR: unexpected opengl error!!! Fix this!");

		//disable rendering and quit before causing any harm (hope gl is still ok)...
		geom_render_level = 0;
		return;
	}

	//configure rendering options:
	glDisable (GL_LIGHTING);
	glShadeModel (GL_FLAT);

	//disable depth testing at level 1, since we need to see everything
	if (geom_render_level == 1)
		glDisable (GL_DEPTH_TEST);
	else
		glEnable (GL_DEPTH_TEST);

	glDisable (GL_CULL_FACE);
	glDisable (GL_FOG);

	//(I wounder if this is deprecated in latest ogl?)
	glLineWidth(2.0); //wide lines

	//render buffer
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glDrawElements(GL_LINES, 2*index_usage, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	//done
}

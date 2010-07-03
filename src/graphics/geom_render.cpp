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

#include "geom_render.hpp"
#include "gl_extensions.hpp"
#include "../shared/geom.hpp"
#include "../shared/racetime_data.hpp"
#include "../shared/printlog.hpp"
#include <limits.h>
#include <math.h>

#define VERTEX_SIZE USHRT_MAX //max index range for ushort
#define INDEX_SIZE 10000 //arbitrary

//only allocate memory and buffers _if_ going to render
//(and then keep the memory until end of race)
bool Got_Buffers = false;
GLuint vertexVBO, indexVBO;

struct geom_vertex {
	float x;
	float y;
	float z;
};
geom_vertex *vertices; //when building
struct geom_index {
	unsigned short a;
	unsigned short b;
};
geom_index *indices; //when building

//keep track, so not overflowing
unsigned int vertex_usage, index_usage;

class Render_Buffers:public Racetime_Data
{
	public:
		//allocate one buffer
		Render_Buffers(): Racetime_Data("geom rendering VBOs tracking class")
		{
			printlog(1, "generating buffers for geom rendering");

			//allocate for building
			vertices = new geom_vertex[VERTEX_SIZE]; //3 floats per vertex
			indices = new geom_index[INDEX_SIZE];

			//create
			glGenBuffers(1, &vertexVBO);
			glGenBuffers(1, &indexVBO);

			//ok!
			Got_Buffers = true;
		}

		~Render_Buffers()
		{
			//delete
			glDeleteBuffers(1, &vertexVBO);
			glDeleteBuffers(1, &indexVBO);

			//delete building arrays
			delete[] vertices;
			delete[] indices;

			//just making sure not fooled in next race
			Got_Buffers = false;
		}
};

//render geoms
//NOTE: when binding this buffer, will have to rebind the proper model boffer again...
void Geom_Render()
{
	//check if rendering
	if (!Got_Buffers)
		new Render_Buffers();

	//build data
	vertex_usage=0;
	index_usage=0;
	

	//geom
	Geom *geom;
	dGeomID g;

	//index/vertex looping pointers
	geom_vertex *v = vertices;
	geom_index *i = indices;

	//position/rotation of geom
	const dReal *pos;
	const dReal *rot;

	//different variables for sizes
	dVector3 result;
	dReal x,y,z,l,r;

	//misc
	unsigned int loop, new_indices, new_vertices;
	float vseg = 2.0*M_PI/8.0;

	//macros to reduce repetitive typing...
	//vertex (absolute rotation)
#define AVertex(X,Y,Z){ \
	(v->x)=(pos[0]+(X)); \
	(v->y)=(pos[1]+(Y)); \
	(v->z)=(pos[2]+(Z)); \
	++v; ++new_vertices;}

	//vertex (relative rotation)
#define RVertex(X,Y,Z){ \
	(v->x)=(pos[0]+((X)*rot[0]+(Y)*rot[1]+(Z)*rot[2])); \
	(v->y)=(pos[1]+((X)*rot[4]+(Y)*rot[5]+(Z)*rot[6])); \
	(v->z)=(pos[2]+((X)*rot[8]+(Y)*rot[9]+(Z)*rot[10])); \
	++v; ++new_vertices;}

	//index
#define Index(A,B){ \
	(i->a)=(vertex_usage+(A)); \
	(i->b)=(vertex_usage+(B)); \
	++i; ++new_indices;}

	for (geom=Geom::head; geom; geom=geom->next)
	{
		g = geom->geom_id;
		new_vertices=0;
		new_indices=0;

		switch (dGeomGetClass(g))
		{
			case dSphereClass:
				pos = dGeomGetPosition(g);
				r = dGeomSphereGetRadius(g);

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
				pos = dGeomGetPosition(g);
				rot = dGeomGetRotation(g);
				dGeomBoxGetLengths(g, result);
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
				pos = dGeomGetPosition(g);
				rot = dGeomGetRotation(g);
				dGeomCapsuleGetParams(g, &r, &l);

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
				pos = dGeomGetPosition(g);
				rot = dGeomGetRotation(g);
				dGeomCylinderGetParams(g, &r, &l);

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

			default:
				break;
		}

		vertex_usage+=new_vertices;
		index_usage+=new_indices;

		//check so not overflowing (enough room for any of above models next render)
		//(assumes no model needs more than 64 vertices and indices)
		if ( (vertex_usage+64) >VERTEX_SIZE || (index_usage+64) >INDEX_SIZE )
			break;
	}

	//send and configure data
	//NOTE: using glBufferData to allocate vbo and send data to it in one step.
	//alternatives includes using the same allocated buffer for every frame and
	//use glMapBuffer or glBufferSubData to just send the new data. but this
	//has shown not to give any fps increase on systems I've tried. (most
	//likely the generating and sending of new data is much slower than sending)
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO); //bind
	glBufferData(GL_ARRAY_BUFFER, sizeof(geom_vertex)*vertex_usage, vertices, GL_STREAM_DRAW); //alloc+init
	glVertexPointer(3, GL_FLOAT, 0, 0); //(no) array striding
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(geom_index)*index_usage, indices, GL_STREAM_DRAW);
	
	//configure rendering options:
	glDisable (GL_LIGHTING);
	glShadeModel (GL_FLAT);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);


	//render buffer
	glEnableClientState(GL_VERTEX_ARRAY);

	glDrawElements(GL_LINES, 2*index_usage, GL_UNSIGNED_SHORT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);

	//done
}

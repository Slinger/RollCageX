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

	//macros to reduce repetitive typing...
#define Vertex(X,Y,Z) (v->x)=(pos[0]+(X)); (v->y)=(pos[1]+(Y)); (v->z)=(pos[2]+(Z)); ++v;
#define Index(A,B) (i->a)=(vertex_usage+(A)); (i->b)=(vertex_usage+(B)); ++i;

	for (geom=Geom::head; geom; geom=geom->next)
	{
		g = geom->geom_id;

		switch (dGeomGetClass(g))
		{
			case dSphereClass:
				break;

			case dBoxClass:
				pos = dGeomGetPosition(g);
				rot = dGeomGetRotation(g);
				dGeomBoxGetLengths(g, result);
				x=result[0]/2.0;
				y=result[1]/2.0;
				z=result[2]/2.0;

				//vertices:
				Vertex(-x, -y, -z);
				Vertex(+x, -y, -z);
				Vertex(+x, -y, +z);
				Vertex(-x, -y, +z);
				Vertex(-x, +y, -z);
				Vertex(+x, +y, -z);
				Vertex(+x, +y, +z);
				Vertex(-x, +y, +z);

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

				//increase counters
				vertex_usage+=8;
				index_usage+=12;

				break;

			default:
				break;
		}

		//check so not overflowing
		if ( (vertex_usage+8) >VERTEX_SIZE || (index_usage+12) >INDEX_SIZE )
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

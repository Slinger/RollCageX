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
#include "../shared/racetime_data.hpp"
#include "../shared/printlog.hpp"
#include <limits.h>

#define VERTEX_SIZE USHRT_MAX //max index range for ushort
#define INDEX_SIZE 10000 //arbitrary

//only allocate memory and buffers _if_ going to render
//(and then keep the memory until end of race)
bool Got_Buffers = false;
GLuint vertexVBO, indexVBO;

struct vertex {
	float x;
	float y;
	float z;
};
vertex *vertices; //when building
unsigned short *indices; //when building

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
			vertices = new vertex[VERTEX_SIZE]; //3 floats per vertex
			indices = new unsigned short[INDEX_SIZE];

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
	

	//TMP: just a triangle
	vertices[0].x=0; vertices[0].y=0; vertices[0].z=0; 
	vertices[1].x=5; vertices[1].y=0; vertices[1].z=5; 
	vertices[2].x=0; vertices[2].y=0; vertices[2].z=5; 
	vertex_usage+=3;

	indices[0]=0; indices[1]=1; indices[2]=2;
	index_usage+=3;
	//END

	//send and configure data
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vertex_usage, vertices, GL_STREAM_DRAW);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*index_usage, indices, GL_STREAM_DRAW);
	
	//configure rendering options:
	glDisable (GL_LIGHTING);
	glShadeModel (GL_FLAT);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);


	//render buffer
	glEnableClientState(GL_VERTEX_ARRAY);

	glDrawElements(GL_TRIANGLES, index_usage, GL_UNSIGNED_SHORT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);

	//done
}

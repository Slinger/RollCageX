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

#include "graphic_list.hpp"
#include "gl_extensions.hpp"

#include "../shared/internal.hpp"
#include "../shared/printlog.hpp"
#include "../shared/trimesh.hpp"
#include "../shared/geom.hpp"
#include "../shared/body.hpp"
#include "../shared/camera.hpp"

#include <stdlib.h>
#include <SDL/SDL_opengl.h>
#include <ode/ode.h>

//offset for vbo
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

//
#define v_length(x, y, z) (dSqrt( (x)*(x) + (y)*(y) + (z)*(z) ))

//just normal (component) list for now:

//consists of two buffers:
//each element in buffer:
struct list_element
{
	GLfloat matrix[16]; //4x4
	Trimesh_3D *model; //model to render
};

//keeps track of a buffer of elements:
struct list_buffer
{
	bool filled;
	size_t count;
	list_element *list;
};

size_t buffer_size = INITIAL_GRAPHIC_LIST_BUFFER_SIZE;

//buffers
list_buffer buffer1 = {0, false, (list_element*)malloc(sizeof(list_element)*buffer_size)};
list_buffer buffer2 = {0, false, (list_element*)malloc(sizeof(list_element)*buffer_size)};

//pointers at buffers
list_buffer *buffer_in = &buffer1; //filled with data
list_buffer *buffer_out = &buffer2; //rendered



void Graphic_List_Update()
{
	//pointers:
	list_buffer *tmp=buffer_in;

	size_t *count=&(tmp->count);
	list_element *list=tmp->list;

	tmp->filled=false; //indicate buffer is now updating (incomplete)
	*count=0; //set to zero (empty)

	//variables
	const dReal *pos, *rot;
	GLfloat *matrix;

	for (Geom *g=Geom::head; g; g=g->next)
	{
		if (g->model)
		{
			pos = dGeomGetPosition(g->geom_id);
			rot = dGeomGetRotation(g->geom_id);
			matrix = list[*count].matrix;

			//set matrix
			matrix[0]=rot[0];
			matrix[1]=rot[4];
			matrix[2]=rot[8];
			matrix[3]=0;
			matrix[4]=rot[1];
			matrix[5]=rot[5];
			matrix[6]=rot[9];
			matrix[7]=0;
			matrix[8]=rot[2];
			matrix[9]=rot[6];
			matrix[10]=rot[10];
			matrix[11]=0;
			matrix[12]=pos[0];
			matrix[13]=pos[1];
			matrix[14]=pos[2];
			matrix[15]=1;

			//set what to render
			list[*count].model = g->model;

			//if buffer full...
			if (++(*count) == buffer_size)
			{
				printlog(1, "Note: Graphic_List buffers were too small, resizing");

				buffer_size+=INITIAL_GRAPHIC_LIST_BUFFER_SIZE;
				buffer1.list = (list_element*) realloc(buffer1.list, sizeof(list_element)*buffer_size);
				buffer2.list = (list_element*) realloc(buffer2.list, sizeof(list_element)*buffer_size);
			}
		}
	}

	//same as above, but for bodies
	for (Body *b=Body::head; b; b=b->next)
	{
		if (b->model)
		{
			pos = dBodyGetPosition(b->body_id);
			rot = dBodyGetRotation(b->body_id);
			matrix = list[*count].matrix;

			//set matrix
			matrix[0]=rot[0];
			matrix[1]=rot[4];
			matrix[2]=rot[8];
			matrix[3]=0;
			matrix[4]=rot[1];
			matrix[5]=rot[5];
			matrix[6]=rot[9];
			matrix[7]=0;
			matrix[8]=rot[2];
			matrix[9]=rot[6];
			matrix[10]=rot[10];
			matrix[11]=0;
			matrix[12]=pos[0];
			matrix[13]=pos[1];
			matrix[14]=pos[2];
			matrix[15]=1;

			//set what to render
			list[*count].model = b->model;

			//if buffer full...
			if (++(*count) == buffer_size)
			{
				printlog(1, "Note: Graphic_List buffers were too small, resizing");

				buffer_size+=INITIAL_GRAPHIC_LIST_BUFFER_SIZE;
				buffer1.list = (list_element*) realloc(buffer1.list, sizeof(list_element)*buffer_size);
				buffer2.list = (list_element*) realloc(buffer2.list, sizeof(list_element)*buffer_size);
			}
		}
	}

	tmp->filled = true; //this buffer is now filled with new data
}

void Graphic_List_Render()
{
	//see if in buffer got complete set of new data, if so switch
	if (buffer_in->filled) //got new stuff to render
	{
		list_buffer *tmp=buffer_out;
		buffer_out=buffer_in;
		buffer_in = tmp;

		buffer_in->filled=false; //make sure we don't switch back to this until really new
	}

	//copy needed data
	size_t *count=&(buffer_out->count);
	list_element *list=buffer_out->list;

	//variables
	unsigned int m_loop;
	Trimesh_3D *model;
	GLuint bound_vbo = 0; //keep track of which vbo is bound


	//configure rendering options:
	//(currently pretty redundant, but good when adding other stuff like menus)

	//enable lighting
	glEnable (GL_LIGHT0);
	glEnable (GL_LIGHTING);

	glShadeModel (GL_SMOOTH); //by default, can be changed

	//glClearDepth (1.0); pointless to define this?

	//depth testing (proper overlapping)
	glDepthFunc (GL_LESS);
	glEnable (GL_DEPTH_TEST);

	//culling of backs
	if (internal.culling)
		glEnable(GL_CULL_FACE);



	//enable rendering of vertices and normals
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	//NOTE: new opengl vbo rendering commands (2.0 I think). For compatibility lets stick to 1.5 instead
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Trimesh_3D::Vertex), (BUFFER_OFFSET(0)));
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Trimesh_3D::Vertex), BUFFER_OFFSET(sizeof(float)*3));

	for (size_t i=0; i<(*count); ++i)
	{
		model = list[i].model; //point at model (cleaner code)

		//will be checking if objects are visible from camera (currently only one)
		//dot product between camera direction and position
		//(position is relative to camera. useing values from matrix)
		float projection=	camera.rotation[1]*(list[i].matrix[12]-camera.pos[0])+
					camera.rotation[4]*(list[i].matrix[13]-camera.pos[1])+
					camera.rotation[7]*(list[i].matrix[14]-camera.pos[2]);
		//(dir is second column in rotation matrix)

		//if projection is behind camera, and no chance of model still reaching into view, ignore this model
		//TODO: can rule out more through view angle... but maybe takes more processing than gives?
		if ( (projection + model->radius) < 0.0 )
			continue;
		//
		

		glPushMatrix();

			glMultMatrixf (list[i].matrix);

			if (model->vbo_id != bound_vbo)
			{
				//bind and configure the new vbo
				glBindBuffer(GL_ARRAY_BUFFER, model->vbo_id);
				glVertexPointer(3, GL_FLOAT, sizeof(Trimesh_3D::Vertex), BUFFER_OFFSET(0));
				glNormalPointer(GL_FLOAT, sizeof(Trimesh_3D::Vertex), BUFFER_OFFSET(sizeof(float)*3));

				//indicate this is used now
				bound_vbo = model->vbo_id;
			}

			//loop through materials, and draw section(s) of model with this material
			for (m_loop=0; m_loop< (model->material_count); ++m_loop)
			{
				glMaterialfv(GL_FRONT, GL_AMBIENT, model->materials[m_loop].ambient);
				glMaterialfv(GL_FRONT, GL_DIFFUSE, model->materials[m_loop].diffuse);
				glMaterialfv(GL_FRONT, GL_SPECULAR, model->materials[m_loop].specular);
				glMaterialfv(GL_FRONT, GL_EMISSION, model->materials[m_loop].emission);
				glMaterialf (GL_FRONT, GL_SHININESS, model->materials[m_loop].shininess);

				//draw
				glDrawArrays(GL_TRIANGLES, model->materials[m_loop].start, model->materials[m_loop].size);
			}

		glPopMatrix();
	}

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	//new/not used (see above)
	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
}

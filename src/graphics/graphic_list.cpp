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


//just normal (component) list for now:

//consists of two buffers:
//each element in buffer:
struct list_element
{
	GLfloat matrix[16]; //4x4
	Trimesh_3D *model; //model to render
	Object *object; //object to which this component belongs
};

//keeps track of a buffer of elements:
struct list_buffer
{
	bool filled;
	size_t count;
	list_element *list;
};

//the buffers are initially set at this size, and increased when needed
//(but never decreased. but since relatively smal, no out-of-memory problems?)
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

			//set object owning this component:
			list[*count].object = g->object_parent;

			//if buffer full...
			if (++(*count) == buffer_size)
			{
				printlog(1, "Note: Graphic_List buffers were too small, resizing");

				buffer_size+=INITIAL_GRAPHIC_LIST_BUFFER_SIZE;
				buffer1.list = (list_element*) realloc(buffer1.list, sizeof(list_element)*buffer_size);
				buffer2.list = (list_element*) realloc(buffer2.list, sizeof(list_element)*buffer_size);
				list=tmp->list;
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

			//set object owning this component:
			list[*count].object = b->object_parent;

			//if buffer full...
			if (++(*count) == buffer_size)
			{
				printlog(1, "Note: Graphic_List buffers were too small, resizing");

				buffer_size+=INITIAL_GRAPHIC_LIST_BUFFER_SIZE;
				buffer1.list = (list_element*) realloc(buffer1.list, sizeof(list_element)*buffer_size);
				buffer2.list = (list_element*) realloc(buffer2.list, sizeof(list_element)*buffer_size);
				list=tmp->list;
			}
		}
	}

	tmp->filled = true; //this buffer is now filled with new data
}

//updated on resizing, needed here:
extern float view_angle_rate_x, view_angle_rate_y;

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
	float *matrix;
	Trimesh_3D::Material *materials;
	unsigned int material_count;
	float radius;

	GLuint bound_vbo = 0; //keep track of which vbo is bound

	//data needed to eliminate models not visible by camera:
	float dir_proj, up_proj, right_proj; //model position relative to camera
	float dir_min, dir_max, up_max, right_max; //limit for what camera can render
	float pos[3]; //relative pos

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
		//for cleaner code, set pointers:
		model = list[i].model;
		matrix = list[i].matrix;
		materials = model->materials;
		material_count = model->material_count;
		radius = model->radius;

		//check if object is not visible from current camera:
		//model pos relative to camera
		pos[0] = matrix[12]-camera.pos[0];
		pos[1] = matrix[13]-camera.pos[1];
		pos[2] = matrix[14]-camera.pos[2];

		//position of camera relative to camera/screen
		//this is really just a matrix multiplication, but we separate each line into a variable
		right_proj = pos[0]*camera.rotation[0]+pos[1]*camera.rotation[3]+pos[2]*camera.rotation[6];
		dir_proj = pos[0]*camera.rotation[1]+pos[1]*camera.rotation[4]+pos[2]*camera.rotation[7];
		up_proj = pos[0]*camera.rotation[2]+pos[1]*camera.rotation[5]+pos[2]*camera.rotation[8];

		//limit of what range is rendered (compensates for "radius" of model that might still be seen)
		dir_min = internal.clipping[0] - radius; //behind close clipping
		dir_max = internal.clipping[1] + radius; //beyound far clipping
		right_max = view_angle_rate_x*(dir_proj+radius) + radius; //right/left
		up_max = view_angle_rate_y*(dir_proj+radius) + radius; //above/below

		//check if visible (or hidden for this camera):
		if (	(list[i].object == camera.hide)				||
			(dir_proj > dir_max)	|| (dir_proj < dir_min)		||
			(right_proj > right_max)|| (-right_proj > right_max)	||
			(up_proj > up_max)	|| (-up_proj > up_max)		)
			continue;
		//
		
		glPushMatrix();

			glMultMatrixf (matrix);

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
			for (m_loop=0; m_loop< (material_count); ++m_loop)
			{
				glMaterialfv(GL_FRONT, GL_AMBIENT, materials[m_loop].ambient);
				glMaterialfv(GL_FRONT, GL_DIFFUSE, materials[m_loop].diffuse);
				glMaterialfv(GL_FRONT, GL_SPECULAR, materials[m_loop].specular);
				glMaterialfv(GL_FRONT, GL_EMISSION, materials[m_loop].emission);
				glMaterialf (GL_FRONT, GL_SHININESS, materials[m_loop].shininess);

				//draw
				glDrawArrays(GL_TRIANGLES, materials[m_loop].start, materials[m_loop].size);
			}

		glPopMatrix();
	}

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	//new/not used (see above)
	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
}

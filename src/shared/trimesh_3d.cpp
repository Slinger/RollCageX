/*
 * RCX  Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger")
 *
 * This program comes with ABSOLUTELY NO WARRANTY!
 *
 * This is free software, and you are welcome to
 * redistribute it under certain conditions.
 *
 * See license.txt and README for more info
 */

//
//for vbo 3d rendering trimesh:
//
#include "internal.hpp"
#include "trimesh.hpp"
#include "../interface/gl_extensions.hpp"
#include "../loaders/conf.hpp"
#include "printlog.hpp"

//length of vector
#define v_length(x, y, z) (sqrt( (x)*(x) + (y)*(y) + (z)*(z) ))


//keep track of VBOs (new generated if not enough room in already existing)
class VBO: public Racetime_Data
{
	public:
		//find a vbo with enough room, if not create a new one
		static VBO *Find_Enough_Room(unsigned int needed)
		{
			printlog(2, "Locating vbo to hold %u bytes of data", needed);

			//check so enough space in even a new vbo:
			if (needed > (unsigned int) internal.vbo_size)
			{
				printlog(0, "ERROR: needed more room than max vbo size (%uB needed, %iB is specified)!", needed, internal.vbo_size);
				return NULL;
			}

			//so if already exists
			for (VBO *p=head; p; p=p->next)
				if ( (p->usage)+needed <= (unsigned int) internal.vbo_size ) //enough to hold
					return p;

			//else, did not find enough room, create
			printlog(1, "creating new vbo, %u bytes of size", internal.vbo_size);

			//create and bind vbo:
			GLuint target;
			glGenBuffers(1, &target);
			glBindBuffer(GL_ARRAY_BUFFER, target);
			glBufferData(GL_ARRAY_BUFFER, internal.vbo_size, NULL, GL_STATIC_DRAW);
			//

			//check if allocated ok:
			if (GLenum error = glGetError()) //if got error...
			{
				//...should be a memory issue...
				if (error == GL_OUT_OF_MEMORY)
					printlog(0, "WARNING: insufficient graphics memory, can not store rendering models...");
				else //...but might be a coding error
					printlog(0, "ERROR: unexpected opengl error!!! Fix this!");

				//anyway, we return NULL to indicate failure
				return NULL;
			}

			//ok, so create a class to track it (until not needed anymore)
			return new VBO(target);
		}

		GLuint id; //size of buffer (for mapping)
		GLsizei usage; //how much of buffer is used (possibly GLint instead?)

	private:
		//normally, Racetime_Data is only for tracking loaded data, one class for each loaded...
		//but this is slightly different: one vbo class can store several model sets
		//(making it a Racetime_Data makes sure all VBOs gets deleted at the same time as models)
		VBO(GLuint target): Racetime_Data("VBO tracking class") //name all vbo classes this...
		{
			//place on top of list
			next=head;
			head=this;
			id=target;
			usage=0; //no data yet
		}
		~VBO()
		{
			glDeleteBuffers(1, &id);
			//VBOs only removed on end of race (are racetime_data), all of them, so can safely just destroy old list
			head = NULL;
		}

		static VBO *head;
		VBO *next;
};

VBO *VBO::head=NULL;

//
//Trimesh_3D stuff:
//

//constructor
Trimesh_3D::Trimesh_3D(const char *name, float r, GLuint vbo, Material *mpointer, unsigned int mcount):
	Racetime_Data(name), materials(mpointer), material_count(mcount), radius(r), vbo_id(vbo)
{
}

//only called together with all other racetime_data destruction (at end of race)
Trimesh_3D::~Trimesh_3D()
{
	printlog(2, "Removing rendering trimesh");

	//remove local data:
	delete[] materials;
}

//loading of a 3d model directly
Trimesh_3D *Trimesh_3D::Quick_Load(const char *name, float resize,
		float rotx, float roty, float rotz,
		float offx, float offy, float offz)
{
	//check if already exists
	if (Trimesh_3D *tmp=Racetime_Data::Find<Trimesh_3D>(name))
		return tmp;

	//no, load
	Trimesh mesh;

	//failure to load
	if (!mesh.Load(name))
		return NULL;

	//pass modification requests (will be ignored if defaults)
	mesh.Resize(resize);
	mesh.Rotate(rotx, roty, rotz);
	mesh.Offset(offx, offy, offz);

	//create a geom from this and return it
	return mesh.Create_3D();
}

//simplified
Trimesh_3D *Trimesh_3D::Quick_Load(const char *name)
{
	//check if already exists
	if (Trimesh_3D *tmp=Racetime_Data::Find<Trimesh_3D>(name))
		return tmp;

	//no, load
	Trimesh mesh;

	//failure to load
	if (!mesh.Load(name))
		return NULL;

	//create a geom from this and return it
	return mesh.Create_3D();
}

//even simpler: all data grabbed from conf file...
Trimesh_3D *Trimesh_3D::Quick_Load_Conf(const char *path, const char *file)
{
	//small conf for filename and model manipulation stuff
	struct mconf {
		Conf_String model;
		float resize, rotate[3], offset[3];
	} modelconf = { //defaults
		"",
		1.0, {0.0,0.0,0.0}, {0.0,0.0,0.0}
	};
	//index (for loading)
	const struct Conf_Index modelconfindex[] = {
	{"model",		's',1, offsetof(mconf, model)},
	{"resize",		'f',1, offsetof(mconf, resize)},
	{"rotate",		'f',3, offsetof(mconf, rotate)},
	{"offset",		'f',3, offsetof(mconf, offset)},
	{"",0,0}};

	//build path+file string for loading conf
	char conf[strlen(path)+1+strlen(file)+1];
	strcpy(conf, path);
	strcat(conf, "/");
	strcat(conf, file);

	//load conf
	if (!load_conf(conf, (char*)&modelconf, modelconfindex))
		return NULL;

	//if we got no filename from the conf, nothing more to do
	if (!modelconf.model)
	{
		printlog(1, "WARNING: could not find model filename in conf \"%s\"", conf);
		return NULL;
	}

	//build path+file for model
	char model[strlen(path)+1+strlen(modelconf.model)+1];
	strcpy(model, path);
	strcat(model, "/");
	strcat(model, modelconf.model);

	//load
	return Trimesh_3D::Quick_Load(model, modelconf.resize,
			modelconf.rotate[0], modelconf.rotate[1], modelconf.rotate[2],
			modelconf.offset[0], modelconf.offset[1], modelconf.offset[2]);
}

//method for creating a Trimesh_3D from Trimesh
Trimesh_3D *Trimesh::Create_3D()
{
	printlog(2, "Creating rendering trimesh from class");

	//already uploaded?
	if (Trimesh_3D *tmp = Racetime_Data::Find<Trimesh_3D>(name.c_str()))
		return tmp;

	//check how many vertices (if any)
	unsigned int vcount=0; //how many vertices
	unsigned int mcount=0; //how many materials
	size_t material_count=materials.size();
	size_t tmp;
	for (unsigned int mat=0; mat<material_count; ++mat)
	{
		//vertices (3 per triangle)
		tmp = 3*materials[mat].triangles.size();
		vcount+=tmp;

		//but if this material got 0 triangles, don't use it
		if (tmp)
			++mcount;
	}

	if (!vcount)
	{
		printlog(0, "ERROR: trimesh is empty (at least no triangles)");
		return NULL;
	}
	//mcount is always secured

	//each triangle requires 3 vertices - vertex defined as "Vertex" in "Trimesh_3D"
	unsigned int needed_vbo_size = sizeof(Trimesh_3D::Vertex)*(vcount);
	VBO *vbo = VBO::Find_Enough_Room(needed_vbo_size);

	if (!vbo)
		return NULL;

	//
	//ok, ready to go!
	//
	
	printlog(2, "number of vertices: %u", vcount);

	//quickly find furthest vertex of obj, so can determine radius
	float radius = Find_Longest_Distance();
	
	//build a tmp list of all vertices sorted by material to minimize calls to be copied
	//to vbo (graphics memory), and list of materials (with no duplicates or unused)

	//first: how big should vertex list be?
	Trimesh_3D::Vertex *vertex_list = new Trimesh_3D::Vertex[vcount];

	//make material list as big as the number of materials
	Trimesh_3D::Material *material_list = new Trimesh_3D::Material[mcount];


	//some values needed:
	unsigned int m,t; //looping of Material and Triangle
	size_t m_size=materials.size();
	size_t t_size;
	mcount=0; //reset to 0 (will need to count again)
	vcount=0; //the same here
	unsigned int vcount_old=0; //keep track of last block of vertices

	//points at current indices:
	unsigned int *vertexi, *normali;

	//loop through all materials, and for each used, loop through all triangles
	//(removes indedexing -make copies- and interleaves the vertices+normals)
	for (m=0; m<m_size; ++m)
	{
		//if this material is used for some triangle:
		if ((t_size = materials[m].triangles.size()))
		{
			//
			//copy vertices data:
			//
			for (t=0; t<t_size; ++t)
			{
				//store indices:
				vertexi = materials[m].triangles[t].vertex;
				normali = materials[m].triangles[t].normal;

				//vertex
				vertex_list[vcount].x = vertices[vertexi[0]].x;
				vertex_list[vcount].y = vertices[vertexi[0]].y;
				vertex_list[vcount].z = vertices[vertexi[0]].z;

				//normal
				vertex_list[vcount].nx = normals[normali[0]].x;
				vertex_list[vcount].ny = normals[normali[0]].y;
				vertex_list[vcount].nz = normals[normali[0]].z;

				//jump to next
				++vcount;

				//vertex
				vertex_list[vcount].x = vertices[vertexi[1]].x;
				vertex_list[vcount].y = vertices[vertexi[1]].y;
				vertex_list[vcount].z = vertices[vertexi[1]].z;

				//normal
				vertex_list[vcount].nx = normals[normali[1]].x;
				vertex_list[vcount].ny = normals[normali[1]].y;
				vertex_list[vcount].nz = normals[normali[1]].z;

				//jump to next
				++vcount;

				//vertex
				vertex_list[vcount].x = vertices[vertexi[2]].x;
				vertex_list[vcount].y = vertices[vertexi[2]].y;
				vertex_list[vcount].z = vertices[vertexi[2]].z;

				//normal
				vertex_list[vcount].nx = normals[normali[2]].x;
				vertex_list[vcount].ny = normals[normali[2]].y;
				vertex_list[vcount].nz = normals[normali[2]].z;

				//jump to next
				++vcount;
			}
			//
			//copy material data:
			//
			//boooooooorrrriiiiinnnngggg....
			material_list[mcount].ambient[0] = materials[m].ambient[0];
			material_list[mcount].ambient[1] = materials[m].ambient[1];
			material_list[mcount].ambient[2] = materials[m].ambient[2];
			material_list[mcount].ambient[3] = materials[m].ambient[3];
			material_list[mcount].diffuse[0] = materials[m].diffuse[0];
			material_list[mcount].diffuse[1] = materials[m].diffuse[1];
			material_list[mcount].diffuse[2] = materials[m].diffuse[2];
			material_list[mcount].diffuse[3] = materials[m].diffuse[3];
			material_list[mcount].specular[0] = materials[m].specular[0];
			material_list[mcount].specular[1] = materials[m].specular[1];
			material_list[mcount].specular[2] = materials[m].specular[2];
			material_list[mcount].specular[3] = materials[m].specular[3];
			material_list[mcount].emission[0] = materials[m].emission[0];
			material_list[mcount].emission[1] = materials[m].emission[1];
			material_list[mcount].emission[2] = materials[m].emission[2];
			material_list[mcount].emission[3] = materials[m].emission[3];
			material_list[mcount].shininess = materials[m].shininess;

			//set up rendering tracking:
			material_list[mcount].start=vcount_old;
			material_list[mcount].size=(vcount-vcount_old);

			//actually, the start should be offsetted by the current usage of vbo
			//(since this new data will be placed after the last model)
			//NOTE: instead of counting in bytes, this is counting in "vertices"
			material_list[mcount].start += (vbo->usage)/sizeof(Trimesh_3D::Vertex);

			//next time, this count will be needed
			vcount_old=vcount;

			//increase material counter
			++mcount;
		}
	}

	printlog(2, "number of (used) materials: %u", mcount);

	//create Trimesh_3D class from this data:
	//set the name. NOTE: both Trimesh_3D and Trimesh_Geom will have the same name
	//this is not a problem since they are different classes and Racetime_Data::Find will notice that
	Trimesh_3D *mesh = new Trimesh_3D(name.c_str(), radius, vbo->id, material_list, mcount);

	//assume this vbo is not bound
	glBindBuffer(GL_ARRAY_BUFFER, vbo->id);

	//transfer data to vbo...
	glBufferSubData(GL_ARRAY_BUFFER, vbo->usage, needed_vbo_size, vertex_list);

	//increase vbo usage counter
	vbo->usage+=needed_vbo_size;

	//free tmp memory
	delete[] vertex_list;

	//ok, done
	return mesh;
}



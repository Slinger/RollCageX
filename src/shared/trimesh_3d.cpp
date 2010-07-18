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

//
//for vbo 3d rendering trimesh:
//
#include "trimesh.hpp"
#include "../graphics/gl_extensions.hpp"
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
			if (needed > VBO_SIZE)
			{
				printlog(0, "ERROR: needed more room than max vbo size (%i)!", VBO_SIZE);
				return NULL;
			}

			//so if already exists
			for (VBO *p=head; p; p=p->next)
				if ( (p->usage)+needed <= VBO_SIZE ) //enough to hold
					return p;

			//else, did not find enough room, create
			return new VBO;
		}

		GLuint id; //size of buffer (for mapping)
		GLsizei usage; //how much of buffer is used (possibly GLint instead?)

	private:
		VBO(): Racetime_Data("VBO tracking class") //name all vbo classes this...
		{
			printlog(1, "creating new vbo, %u bytes of size", VBO_SIZE);

			//place on top of list
			next=head;
			head=this;

			//create and bind vbo:
			glGenBuffers(1, &id);
			glBindBuffer(GL_ARRAY_BUFFER, id);
			glBufferData(GL_ARRAY_BUFFER, VBO_SIZE, NULL, GL_STATIC_DRAW);
			//
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

//method for creating a Trimesh_3D from Trimesh
Trimesh_3D *Trimesh::Create_3D()
{
	printlog(2, "Creating rendering trimesh from class");

	//already uploaded?
	if (Trimesh_3D *tmp = Racetime_Data::Find<Trimesh_3D>(name.c_str()))
		return tmp;

	//check that we got any data
	if (triangles.empty())
	{
		printlog(0, "ERROR: trimesh is empty (at least no useful data)");
		return NULL;
	}

	//calculate size and get vbo for storing
	//each triangle requires 3 vertices - vertex defined as "Vertex" in "Trimesh_3D"
	unsigned int needed_vbo_size = sizeof(Trimesh_3D::Vertex)*(triangles.size()*3);
	VBO *vbo = VBO::Find_Enough_Room(needed_vbo_size);
	
	if (!vbo)
		return NULL;

	//make sure got at least one material (if not create a default)
	if (materials.empty())
	{
		printlog(0, "ERROR: Trimesh did not have any materials! Creating default");
		materials.push_back(Material_Default);
	}

	if (material_indices.empty())
	{
		printlog(0, "ERROR: Trimesh does not select any materials! Selecting first");
		Material_Index tmp = {0,0}; //use material 0 starting at first triangle
		material_indices.push_back(tmp);
	}

	//
	//ok, ready to go!
	//
	
	//quickly find furthest vertex of obj, so can determine radius
	float radius = Find_Longest_Distance();
	
	//build a tmp list of all vertices sorted by material to minimize calls
	//(and list with no unused materials nor duplicates)

	//first: how big should vertex list be?
	unsigned int vcount=3*triangles.size(); //3 vertices per triangle
	Trimesh_3D::Vertex *vertex_list = new Trimesh_3D::Vertex[vcount];

	//make material list as big as the number of materials (might be bigger than needed, but safe+easy)
	unsigned int mcount=0;
	Trimesh_3D::Material *material_list = new Trimesh_3D::Material[materials.size()];

	printlog(2, "number of vertices: %u", vcount);

	//some values needed:
	unsigned int start, stop; //keeps track of data to copy from old triangle list
	unsigned int position=0, position_old=0; //keep track of position in new vertex list
	bool material_is_used; //keep track if material is even used


	unsigned int m,i; //looping of Material and material Index
	size_t m_size=materials.size();
	size_t i_size=material_indices.size();
	for (m=0; m<m_size; ++m) //material
	{
		material_is_used=false; //so far: not used, no

		for (i=0; i<i_size; ++i) //material index
		{
			if (material_indices[i].material == m) //matches
			{
				//indicate that we have valid match
				material_is_used=true;

				//variables for copying:
				//triangle start
				if (i==0) //first material, _always_ set start to 0
					start=0;
					//material_list[mcount].start =0;
				else //else, start as specified
					start=material_indices[i].start_at;
					//material_list[mcount].stat = material_indices[i].start_at;

				//triangle stop
				if ((i+1)==i_size) //last index, _always_ stop at end
					stop=triangles.size();
					//material_list[mcount].stop = vertices.size();
				else //else, stop at start of next material index
					stop=material_indices[i+1].start_at;

				//copy all specified materials:
				for (size_t triangle=start; triangle<stop; ++triangle)
				{
					//vertex
					vertex_list[position].x = vertices[triangles[triangle].vertex[0]].x;
					vertex_list[position].y = vertices[triangles[triangle].vertex[0]].y;
					vertex_list[position].z = vertices[triangles[triangle].vertex[0]].z;

					//normal
					vertex_list[position].nx = normals[triangles[triangle].normal[0]].x;
					vertex_list[position].ny = normals[triangles[triangle].normal[0]].y;
					vertex_list[position].nz = normals[triangles[triangle].normal[0]].z;

					++position;

					//vertex
					vertex_list[position].x = vertices[triangles[triangle].vertex[1]].x;
					vertex_list[position].y = vertices[triangles[triangle].vertex[1]].y;
					vertex_list[position].z = vertices[triangles[triangle].vertex[1]].z;

					//normal
					vertex_list[position].nx = normals[triangles[triangle].normal[1]].x;
					vertex_list[position].ny = normals[triangles[triangle].normal[1]].y;
					vertex_list[position].nz = normals[triangles[triangle].normal[1]].z;

					++position;

					//vertex
					vertex_list[position].x = vertices[triangles[triangle].vertex[2]].x;
					vertex_list[position].y = vertices[triangles[triangle].vertex[2]].y;
					vertex_list[position].z = vertices[triangles[triangle].vertex[2]].z;

					//normal
					vertex_list[position].nx = normals[triangles[triangle].normal[2]].x;
					vertex_list[position].ny = normals[triangles[triangle].normal[2]].y;
					vertex_list[position].nz = normals[triangles[triangle].normal[2]].z;

					++position;
				}
			}
		}

		//ok, this is a used material
		if (material_is_used)
		{
			//copy material data:
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
			material_list[mcount].start=position_old;
			material_list[mcount].size=(position-position_old);

			//actually, the start should be offsetted by the current usage of vbo
			//(since this new data will be placed after the last model)
			//NOTE: instead of counting in bytes, this is counting in "vertices"
			material_list[mcount].start += (vbo->usage)/sizeof(Trimesh_3D::Vertex);

			//next time, this position will be position_old
			position_old=position;

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



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

#include "trimesh.hpp"
#include "geom.hpp"
#include "printlog.hpp"
#include "../graphics/gl_extensions.hpp"
#include <limits.h>

//length of vector
#define v_length(x, y, z) (dSqrt( (x)*(x) + (y)*(y) + (z)*(z) ))

//default values for material
const Trimesh::Material Trimesh::Material_Default = 
{
	"unknown material",
	{0.2, 0.2, 0.2, 1.0},
	{0.8, 0.8, 0.8, 1.0},
	{0.0, 0.0, 0.0, 1.0},
	{0.0, 0.0, 0.0, 1.0},
	0.0
};

//
//for vbo 3d rendering trimesh:
//

//keep track of VBOs (new generated if not enough room in already existing)
class VBO: Racetime_Data
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
		VBO(): Racetime_Data("internal_VBO_tracking_class") //name all vbo classes this...
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
			printlog(2, "removing vbo\n");
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
GLuint Trimesh_3D::current_vbo = 0;

//constructor
Trimesh_3D::Trimesh_3D(const char *name, GLuint vbo, Material *mpointer, unsigned int mcount): Racetime_Data(name)
{
	//set the vbo id for this trimesh_3d
	vbo_id = vbo;

	//pointer to materials
	materials=mpointer;
	material_count=mcount;

	//if this vbo is not bound
	if (vbo != current_vbo)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		current_vbo=vbo;
	}
}

//only called together with all other racetime_data destruction (at end of race)
Trimesh_3D::~Trimesh_3D()
{
	printlog(2, "Removing rendering trimesh");

	//remove local data:
	delete[] materials;

	//make sure next time creating vbos not fooled by this old value
	current_vbo=0;
}

//
//lots of methods for Trimesh class:
//

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
	Trimesh_3D *mesh = new Trimesh_3D(name.c_str(), vbo->id, material_list, mcount);

	//transfer data to vbo...
	glBufferSubData(GL_ARRAY_BUFFER, vbo->usage, needed_vbo_size, vertex_list);

	//increase vbo usage counter
	vbo->usage+=needed_vbo_size;

	//free tmp memory
	delete[] vertex_list;

	//ok, done
	return mesh;
}

//
//for geom 3d collision detection trimesh:
//
Trimesh_Geom::Trimesh_Geom(const char *name,
		Vector_Float *v, unsigned int vcount,
		unsigned int *i, unsigned int icount,
		Vector_Float *n): Racetime_Data(name), vertices(v), indices(i), normals(n) //set name and values
{
	//just tell ode to create trimesh from this data
	data = dGeomTriMeshDataCreate();

	dGeomTriMeshDataBuildSingle1 (data,
			vertices, sizeof(Vector_Float), vcount,
			indices, icount, 3*sizeof(unsigned int),
			normals);
}

Geom *Trimesh_Geom::Create_Geom(Object *obj)
{
	//create geom without space, callback, array callback or ray callback
	dGeomID g = dCreateTriMesh(0, data, 0, 0, 0);
	Geom *geom = new Geom(g, obj);

	return geom;
}

Trimesh_Geom::~Trimesh_Geom()
{
	printlog(2, "Removing collision trimesh");

	delete[] vertices;
	delete[] indices;
	delete[] normals;

	dGeomTriMeshDataDestroy(data);
}

//method Trimesh_Geom from Trimesh
Trimesh_Geom *Trimesh::Create_Geom()
{
	printlog(2, "Creating collision trimesh from class");

	//already created?
	if (Trimesh_Geom *tmp = Racetime_Data::Find<Trimesh_Geom>(name.c_str()))
		return tmp;

	//check that we got any data
	if (triangles.empty())
	{
		printlog(0, "ERROR: trimesh is empty (at least no useful data)");
		return NULL;
	}

	//sizes
	unsigned int verts=vertices.size();
	unsigned int tris=triangles.size();

	printlog(2, "Number of vertices: %u, number of triangles: %u", verts, tris);

	//check (vertice and indix count can't exceed int limit)
	if (verts>INT_MAX || (tris*3)>INT_MAX)
	{
		printlog(0, "ERROR: trimesh is too big for ode collision trimesh");
		return NULL;
	}

	//lets start!
	//allocate
	Vector_Float *v, *n;
	unsigned int *i;

	v = new Vector_Float[verts]; //one vertex per vertex
	i = new unsigned int[tris*3]; //3 indices per triangle
	n = new Vector_Float[tris]; //one normal per triangle

	//copy
	unsigned int loop;
	//vertices
	for (loop=0; loop<verts; ++loop)
	{
		v[loop] = vertices[loop];
	}
	//indices
	for (loop=0; loop<tris; ++loop)
	{
		i[loop*3+0] = triangles[loop].vertex[0];
		i[loop*3+1] = triangles[loop].vertex[1];
		i[loop*3+2] = triangles[loop].vertex[2];
	}
	//normals
	unsigned int new_normals=0;
	for (loop=0; loop<tris; ++loop)
	{
		//NOTE: ode uses one, not indexed, normal per triangle,
		//we store them as 3 indexed nomrals per triangle - translate

		//all 3 normals are the same, use it
		if (	(triangles[loop].normal[0]) == (triangles[loop].normal[1]) &&
			(triangles[loop].normal[0]) == (triangles[loop].normal[2])	)
		{
			n[loop] = normals[triangles[loop].normal[0]]; //copy first specified normal
		}
		else //trouble, smooth surface normals? calculate a new normal!
		{
			++new_normals;
			//the same as in Generate_Missing_Normals

			//get vertices
			Vector_Float v1 = vertices[triangles[loop].vertex[0]];
			Vector_Float v2 = vertices[triangles[loop].vertex[1]];
			Vector_Float v3 = vertices[triangles[loop].vertex[2]];

			//create two vectors (a and b)
			float ax = (v2.x-v1.x);
			float ay = (v2.y-v1.y);
			float az = (v2.z-v1.z);

			float bx = (v3.x-v1.x);
			float by = (v3.y-v1.y);
			float bz = (v3.z-v1.z);

			//cross product gives normal:
			float x = (ay*bz)-(az*by);
			float y = (az*bx)-(ax*bz);
			float z = (ax*by)-(ay*bx);
			
			//set and make unit:
			float l = v_length(x, y, z);

			n[loop].x = x/l;
			n[loop].y = y/l;
			n[loop].z = z/l;
		}
	}

	printlog(2, "How many normals recalculated (from smooth): %u", new_normals);

	//create
	return new Trimesh_Geom(name.c_str(),
			v, verts,
			i, tris*3,
			n);
}

//
//for trimesh file loading
//

//wrapper for loading
bool Trimesh::Load(const char *file)
{
	printlog(2, "Loading trimesh from file \"%s\", determining file type", file);

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		printlog(0, "ERROR: no suffix for file \"%s\"", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".obj"))
		return Load_OBJ(file);
	//else if (!strcasecmp(suffix, ".3ds"))
		//return Load_3DS(file);
	
	//else, no match
	printlog(0, "ERROR: unknown 3D file suffix for \"%s\"", file);
	return false;
}

unsigned int Trimesh::Find_Material(const char *name)
{
	size_t end = materials.size();

	for (size_t i=0; i<end; ++i)
	{
		if (materials[i].name == name)
			return i;
	}

	//failure
	printlog(0, "ERROR: could not find trimesh material %s", name);
	return INDEX_ERROR;
}

bool Trimesh::Compare_Name(const char *n)
{
	if (!name.compare(n))
		return true;
	return false;
}

//makes sure all normals are unit
void Trimesh::Normalize_Normals()
{
	printlog(2, "Making sure all normals are unit for trimesh");

	size_t end = normals.size();
	float l;

	for (size_t i=0; i<end; ++i)
	{
		l=v_length(normals[i].x, normals[i].y, normals[i].z);

		if (l > 1.000001 || l < 0.999999) //TODO: consider remove this checking?
		{
			normals[i].x /=l;
			normals[i].y /=l;
			normals[i].z /=l;
		}
	}
}

//creates missing normals (if needed)
//counter-clockwise order of triangles assumed
void Trimesh::Generate_Missing_Normals()
{
	printlog(2, "Generating missing normals for trimesh");

	size_t end = triangles.size();
	unsigned int *nindex;
	Vector_Float v1, v2, v3;

	float ax,ay,az, bx,by,bz, l;
	Vector_Float new_normal;
	unsigned new_normal_number;


	for (size_t i=0; i<end; ++i)
	{
		nindex=triangles[i].normal;

		//one or more indices are unspecified
		if (nindex[0] == INDEX_ERROR || nindex[1] == INDEX_ERROR || nindex[2] == INDEX_ERROR)
		{
			//copy vertices:
			v1=vertices[triangles[i].vertex[0]];
			v2=vertices[triangles[i].vertex[1]];
			v3=vertices[triangles[i].vertex[2]];

			//create two vectors (a and b) from the first point to the two others:
			ax = (v2.x-v1.x);
			ay = (v2.y-v1.y);
			az = (v2.z-v1.z);

			bx = (v3.x-v1.x);
			by = (v3.y-v1.y);
			bz = (v3.z-v1.z);

			//cross product gives normal:
			new_normal.x = (ay*bz)-(az*by);
			new_normal.y = (az*bx)-(ax*bz);
			new_normal.z = (ax*by)-(ay*bx);
			
			//make unit:
			l = v_length(new_normal.x,new_normal.y,new_normal.z);
			new_normal.x /= l;
			new_normal.y /= l;
			new_normal.z /= l;

			//store it:
			//note: since indexing the normal array isn't needed for any later stage
			//(will be "unindexed"), don't bother about duplicates
			normals.push_back(new_normal);

			//set up indices:
			new_normal_number = normals.size()-1;
			nindex[0] = new_normal_number;
			nindex[1] = new_normal_number;
			nindex[2] = new_normal_number;
		}
	}
}

//resize, rotate, change offset stuff:
void Trimesh::Resize(float r)
{
	if (r == 1.0) //no need
		return;

	if (r == 0.0) //easy mistake
	{
		printlog(0, "You've made a typo: resize is 1.0, not 0.0 - Ignoring...");
		return;
	}

	printlog(2, "Resizing trimesh");

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		vertices[i].x *= r;
		vertices[i].y *= r;
		vertices[i].z *= r;
	}
}

void Trimesh::Rotate(float x, float y, float z)
{
	if (x==0 && y==0 && z==0)
		return;

	printlog(2, "Rotating trimesh");

	//rotation matrix:
	dMatrix3 rot;
	dRFromEulerAngles (rot, x*(M_PI/180), y*(M_PI/180), z*(M_PI/180));

	Vector_Float v, rotated;

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		v=vertices[i];
		rotated.x = v.x*rot[0]+v.y*rot[4]+v.z*rot[8];
		rotated.y = v.x*rot[1]+v.y*rot[5]+v.z*rot[9];
		rotated.z = v.x*rot[2]+v.y*rot[6]+v.z*rot[10];

		vertices[i]=rotated;
	}

	end = normals.size();

	for (i=0; i != end; ++i)
	{
		v=normals[i];
		rotated.x = v.x*rot[0]+v.y*rot[4]+v.z*rot[8];
		rotated.y = v.x*rot[1]+v.y*rot[5]+v.z*rot[9];
		rotated.z = v.x*rot[2]+v.y*rot[6]+v.z*rot[10];

		normals[i]=rotated;
	}
}

void Trimesh::Offset(float x, float y, float z)
{
	if (x==0 && y==0 && z==0)
		return;

	printlog(2, "Changing offset of trimesh");

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		vertices[i].x += x;
		vertices[i].y += y;
		vertices[i].z += z;
	}
}

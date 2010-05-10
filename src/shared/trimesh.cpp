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
#include "printlog.hpp"

#include <GL/gl.h>
#include <GL/glext.h> //might be needed for vbo definitions

//TMP:
void Trimesh::TMP_printinfo()
{
	size_t i;
	printf("printing contents of trimesh:\n");
	printf("number of materials: %i\n", materials.size());
	for (i=0; i!=materials.size(); ++i)
		printf("> %s\n", materials[i].name.c_str());

	printf("number of material indices: %i\n", material_indices.size());
	for (i=0; i!=material_indices.size(); ++i)
		printf("> %u\n", material_indices[i].material);

	printf("number of vertices: %i\n", vertices.size());
	for (i=0; i!=vertices.size(); ++i)
		printf("> %f %f %f\n", vertices[i].x, vertices[i].y, vertices[i].z);

	printf("number of normals: %i\n", normals.size());
	for (i=0; i!=normals.size(); ++i)
		printf("> %f %f %f\n", normals[i].x, normals[i].y, normals[i].z);

	printf("number of triangles: %i\n", triangles.size());
	for (i=0; i!=triangles.size(); ++i)
		printf("> %u %u %u - %u %u %u\n", triangles[i].vertex[0], triangles[i].vertex[1], triangles[i].vertex[2], triangles[i].normal[0], triangles[i].normal[1], triangles[i].normal[2]);
}


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

//keep track of VBOs (new generated if not enough room in already existing)
class VBO: Racetime_Data
{
	public:
		//find a vbo with enough room, if not create a new one
		VBO *Select_With_Enough_Room(unsigned int needed)
		{
			return NULL; //change
		}

		GLuint id; //size of buffer (for mapping)
		GLsizei usage; //how much of buffer is used (possibly GLint instead?)

	private:
		VBO(): Racetime_Data("internal_VBO_tracking_class") //name all vbo classes this...
		{
			//glBufferCreate
		}
		~VBO()
		{
			//glBufferDestroy
		}

		static VBO *head;
		VBO *next;
};

//wrapper for loading
bool Trimesh::Load(const char *file)
{
	const char *suffix = strrchr(file, '.');

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

void Trimesh::Resize(float r)
{
	if (r == 1.0) //no need
		return;

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
	printf("TODO: check if dMatrix3 is column on line based\n");

	Vector_Float v, rotated;

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		v=vertices[i];
		rotated.x = v.x*rot[0]+v.y*rot[1]+v.z*rot[2];
		rotated.y = v.x*rot[3]+v.y*rot[4]+v.z*rot[5];
		rotated.z = v.x*rot[6]+v.y*rot[7]+v.z*rot[8];

		vertices[i]=rotated;
	}

	end = normals.size();

	for (i=0; i != end; ++i)
	{
		v=normals[i];
		rotated.x = v.x*rot[0]+v.y*rot[1]+v.z*rot[2];
		rotated.y = v.x*rot[3]+v.y*rot[4]+v.z*rot[5];
		rotated.z = v.x*rot[6]+v.y*rot[7]+v.z*rot[8];

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

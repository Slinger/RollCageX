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

/*Trimesh::Trimesh()
{
	name=NULL;
}

Trimesh::~Trimesh()
{
	if (name)
		delete name;
}*/

		//keep track of VBOs (new generated if not enough room in already existing)
		struct VBO
		{
			GLuint id; //size of buffer (for mapping)
			GLsizeiptr usage; //how much of buffer is used (possibly GLint instead?)
		}

		static std::vector vbo<VBO>;


void Trimesh::Set_Name(const char *n)
{
	//unlikely, but anyway
	if (name)
		delete[] name;


	name = new char[strlen(n)+1]; //alloc
	strcpy(name, n);
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
		vertices[i][0] *= r;
		vertices[i][1] *= r;
		vertices[i][2] *= r;
	}
}

void Trimesh::Rotate(float x, float y, float z)
{
	if (x==y==z==0)
		return;

	printlog(2, "Rotating trimesh");

	//rotation matrix:
	dMatrix3 rot;
	float *origin;
	float rotated[3];
	dRFromEulerAngles (rot, x*(M_PI/180), y*(M_PI/180), z*(M_PI/180));
	printf("TODO: check if dMatrix3 is column on line based\n");

	size_t end = vectors.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		v=vectors[i];
		rotated[0] = v[0]*rot[0]+v[1]*rot[1]+v[2]*rot[2];
		rotated[1] = v[0]*rot[3]+v[1]*rot[4]+v[2]*rot[5];
		rotated[2] = v[0]*rot[6]+v[1]*rot[7]+v[2]*rot[8];

		vectors[i]=rotated;
	}

	end = normals.size();

	for (i=0; i != end; ++i)
	{
		normals[i][0] *= r;
		normals[i][1] *= r;
		normals[i][2] *= r;
	}
}

void Trimesh::Offset(float x, float y, float z)
{
	if (x==y==z==0)
		return;

	printlog(2, "Changing offset of trimesh");

	size_t end = vertices.size();
	size_t i;

	for (i=0; i != end; ++i)
	{
		vertices[i][0] += x;
		vertices[i][1] += y;
		vertices[i][2] += z;
	}
}

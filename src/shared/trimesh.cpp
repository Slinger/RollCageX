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
#include <limits.h>

//length of vector
#define v_length(x, y, z) (sqrt( (x)*(x) + (y)*(y) + (z)*(z) ))

//default values for material
const Trimesh::Material Trimesh::Material_Default = 
{
	"unknown material",
	{0.2, 0.2, 0.2, 1.0},
	{0.8, 0.8, 0.8, 1.0},
	{0.0, 0.0, 0.0, 1.0},
	{0.0, 0.0, 0.0, 1.0},
	0.0
	//no value for vector of triangles
};


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

	unsigned int *nindex, *vindex;;
	Vector_Float v1, v2, v3;

	float ax,ay,az, bx,by,bz, l;
	Vector_Float new_normal;
	unsigned new_normal_number;


	size_t m, mend = materials.size();
	size_t t, tend;

	//all triangles in all materials
	for (m=0; m<mend; ++m)
		if ((tend = materials[m].triangles.size()))
			for (t=0; t<tend; ++t)
			{
				//normal indices
				nindex=materials[m].triangles[t].normal;

				//one or more indices are unspecified
				if (nindex[0] == INDEX_ERROR || nindex[1] == INDEX_ERROR || nindex[2] == INDEX_ERROR)
				{
					//vertex indices
					vindex=materials[m].triangles[t].vertex;

					//copy vertices:
					v1=vertices[vindex[0]];
					v2=vertices[vindex[1]];
					v3=vertices[vindex[2]];

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

float Trimesh::Find_Longest_Distance()
{
	printlog(2, "Finding longest distance vertex in trimesh (\"radius\")");
	size_t end = vertices.size();
	size_t i;
	float biggest=0.0, length;

	//for optimum performance, no sqrt or similar, just store the one biggest axis
	for (i=0; i<end; ++i)
	{
		length = v_length(vertices[i].x, vertices[i].y, vertices[i].z);

		if (length > biggest)
			biggest=length;
	}

	printlog(2, "longest distance in trimesh: %f", biggest);

	return biggest;
}

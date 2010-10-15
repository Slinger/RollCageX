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

#include "internal.hpp"
#include "trimesh.hpp"
#include "geom.hpp"
#include "printlog.hpp"

//length of vector
#define v_length(x, y, z) (sqrt( (x)*(x) + (y)*(y) + (z)*(z) ))

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

	//perhaps use dGeomTriMeshDataPreprocess here, but it takes a long time to complete...
}

Trimesh_Geom *Trimesh_Geom::Quick_Load(const char *name, float resize,
		float rotx, float roty, float rotz,
		float offx, float offy, float offz)
{
	//check if already exists
	if (Trimesh_Geom *tmp=Racetime_Data::Find<Trimesh_Geom>(name))
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
	return mesh.Create_Geom();
}

Trimesh_Geom *Trimesh_Geom::Quick_Load(const char *name)
{
	//check if already exists
	if (Trimesh_Geom *tmp=Racetime_Data::Find<Trimesh_Geom>(name))
		return tmp;

	//no, load
	Trimesh mesh;

	//failure to load
	if (!mesh.Load(name))
		return NULL;

	//create a geom from this and return it
	return mesh.Create_Geom();
}

Geom *Trimesh_Geom::Create_Geom(Object *obj)
{
	//create geom without space, callback, array callback or ray callback
	dGeomID g = dCreateTriMesh(0, data, 0, 0, 0);
	Geom *geom = new Geom(g, obj);

	//see if should enable temporal coherence
	if (internal.temporal_coherence)
	{
		dGeomTriMeshEnableTC(g, dSphereClass, 1);
		dGeomTriMeshEnableTC(g, dBoxClass, 1);
		dGeomTriMeshEnableTC(g, dCapsuleClass, 1);

		//following are not supporting in ode right now:
		dGeomTriMeshEnableTC(g, dCylinderClass, 1); //not working yet
		dGeomTriMeshEnableTC(g, dPlaneClass, 1); //not working yet
		dGeomTriMeshEnableTC(g, dRayClass, 1); //not working yet
		dGeomTriMeshEnableTC(g, dTriMeshClass, 1); //not working yet
	}

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



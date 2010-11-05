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

//override index when merging of contacts (tmp solution until next version of ode!)
int mergecallback(dGeomID geom, int index1, int index2)
{
	//probably needs better decicion on which to use... but hey, this is hackish anyway!
	return index1; //set index to first
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
	//create a trimesh geom (with usual global space for now)
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

	//enable per-triangle collision detection (for different materials)
	//always used to display which triangle is colliding:
	geom->triangle_count = triangle_count;
	geom->triangle_colliding = new bool[triangle_count];
	//(optional) used for assigning different surfaces for different materials:
	geom->material_count = material_count;
	geom->parent_materials = materials; //points at out simple material list

	//opcode merges several contacts into one (currently only trimesh-sphere)
	//since we  need the triangle index contacts, this causes problems...
	//TODO: use dGeomLowLevelControl to disable merging of contacts!
	//(is only available in the next version of ode which few distros got)
	//
	//right now, lets just override the index=-1 by defining a callback:
	dGeomTriMeshSetTriMergeCallback(g, &mergecallback);

	return geom;
}

Trimesh_Geom::~Trimesh_Geom()
{
	printlog(2, "Removing collision trimesh");

	delete[] vertices;
	delete[] indices;
	delete[] normals;

	//delete materials (including names)
	for (int i=0; i<material_count; ++i)
		delete[] materials[i].name;
	delete[] materials;

	dGeomTriMeshDataDestroy(data);
}

//method Trimesh_Geom from Trimesh
Trimesh_Geom *Trimesh::Create_Geom()
{
	printlog(2, "Creating collision trimesh from class");

	//already created?
	if (Trimesh_Geom *tmp = Racetime_Data::Find<Trimesh_Geom>(name.c_str()))
		return tmp;

	//check that we got any data (and how much?)
	unsigned int tris=0, mats=0, tmp;;
	size_t material_count=materials.size();
	for (unsigned int mat=0; mat<material_count; ++mat)
		if ((tmp = materials[mat].triangles.size()))
		{
			tris += tmp;
			++mats;
		}

	if (!tris)
	{
		printlog(0, "ERROR: trimesh is empty (at least no triangles)");
		return NULL;
	}

	//assumed to be non-empty
	size_t verts = vertices.size();

	printlog(2, "number of vertices: %u, number of triangles: %u", verts, tris);

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
	Trimesh_Geom::Material *m;

	v = new Vector_Float[verts]; //one vertex per vertex
	i = new unsigned int[tris*3]; //3 indices per triangle
	n = new Vector_Float[tris]; //one normal per triangle
	m = new Trimesh_Geom::Material[mats]; //one material per material

	//copy
	//vertices
	for (unsigned int loop=0; loop<verts; ++loop)
		v[loop] = vertices[loop];

	//triangles (using indexed vertices and an array of normals)
	//by material:
	unsigned int tcount=0, mcount=0; //total counters (for where to fill in data)
	unsigned int *vp, *np; //vertex and normal index pointers
	unsigned int mloop=0, tloop=0; //material/triangle looping
	float ax,ay,az,bx,by,bz,x,y,z,l; //(fooling myself these declarations will increase speed)
	unsigned int new_normals=0; //just interesting...
	unsigned int mtris; //how many triangles per material
	for (mloop=0; mloop<material_count; ++mloop)
		if ((mtris = materials[mloop].triangles.size()))
		{
			//copy triangles
			for (tloop=0; tloop<mtris; ++tloop)
			{
				vp = materials[mloop].triangles[tloop].vertex;
				np = materials[mloop].triangles[tloop].normal;

				//indices
				i[3*tcount] = vp[0];
				i[3*tcount+1] = vp[1];
				i[3*tcount+2] = vp[2];

				//normals
				//NOTE: ode uses one, not indexed, normal per triangle,
				//but we have 3 indexed normals per triangle (find compromise)

				//all 3 normals are the same, use it
				if (	(np[0]) == (np[1])	&&
					(np[0]) == (np[2])	)
				{
					n[tcount] = normals[np[0]]; //copy first specified normal
				}
				else //trouble, smooth surface normals? calculate a new normal!
				{
					++new_normals;
					//the same as in Generate_Missing_Normals

					//get vertices
					Vector_Float v1 = vertices[vp[0]];
					Vector_Float v2 = vertices[vp[1]];
					Vector_Float v3 = vertices[vp[2]];

					//create two vectors (a and b)
					ax = (v2.x-v1.x);
					ay = (v2.y-v1.y);
					az = (v2.z-v1.z);

					bx = (v3.x-v1.x);
					by = (v3.y-v1.y);
					bz = (v3.z-v1.z);

					//cross product gives normal:
					x = (ay*bz)-(az*by);
					y = (az*bx)-(ax*bz);
					z = (ax*by)-(ay*bx);
					
					//set and make unit:
					l = v_length(x, y, z);

					n[tcount].x = x/l;
					n[tcount].y = y/l;
					n[tcount].z = z/l;
				}
				++tcount;
			}

			//"copy" material (name+triangle index):
			m[mcount].end = tcount;
			//allocate for name
			m[mcount].name = new char[materials[mloop].name.size()+1];
			//copy name
			strcpy(m[mcount].name, materials[mloop].name.c_str());

			//increase material counter
			++mcount;
		}

	printlog(2, "How many normals recalculated (from smooth): %u", new_normals);

	//create
	Trimesh_Geom *result = new Trimesh_Geom(name.c_str(),
			v, verts,
			i, 3*tris,
			n);

	//needs triangle count...
	result->triangle_count = tris;
	//and materials:
	result->material_count = mats;
	result->materials = m;

	return result;
}



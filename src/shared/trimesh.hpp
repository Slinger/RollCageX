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

#ifndef _RCX_TRIMESH_H
#define _RCX_TRIMESH_H

#include <vector>
#include <inttypes.h>

#include <GL/gl.h>
#include <ode/ode.h>

//definitions:
//for keeping track for overflowing:
#define INDEX_MAX uintmax_t
//how big each VBO should be:
#define VBO_MAX 8388608 //8MiB - or should it be MB? need to test...
//end of definitions



//specialized trimesh classes:

//optimized trimesh rendering
class Trimesh_3D: public Racetime_Data
{
	public:
		GLint Get_ID();

	private:
		Trimesh_3D(const char*); //sends name to Racetime_Data
		friend class Trimesh; //only Trimesh is allowed to create this...

		GLint id;

		//element (vertex+normal) for interleaved array
		struct Vertex
		{
			float x,y,z;
			float nx,ny,nz;
			float padding[2]; //ATI recommends "vertex" size divideable by 32...
		};

		//VBO and position in VBO of array:


		//keep track of VBOs (new generated if not enough room in already existing)
		struct VBO
		{
			GLuint id; //size of buffer (for mapping)
			GLsizeiptr usage; //how much of buffer is used (possibly GLint instead?)
		}

		static std::vector vbo<VBO>;
};

//for collision detection (geom) generation
class Trimesh_Geom_Data: public Racetime_Data
{
	public:
		Create_Geom(); //creates geom from trimesh
		//TODO: ode supports callbacks for trimeshes, could be usefull...

	private:
		Trimesh_Geom_Data(const char*); //sends name to Racetime_Data
		friend class Trimesh; //only Trimesh is allowed to create this...

		//data for trimesh:
		dTriMeshDataID data; //collected, pointers
		dVector3 *vertices;
		unsigned int *indices;
		dVector3 *normals; //not needed, but already calculating gives extra performance
};



//trimesh storage class - not used during race, only while loading/processing
class Trimesh
{
	public:
		Trimesh();
		~Trimesh();

		//wrapper that decides loading function by file suffix:
		bool Load(const char*);
		//obj files
		bool Load_OBJ(const char *);
		//3ds files
		bool Load_3DS(const char *);

		//void Generate... - create trimesh, not supported/nneded yet

		//create "dedicated" (used during race) timeshes from this one:
		Trimesh_3D *Create_3D();
		Trimesh_Geom *Create_Geom();

		//tools/helpers:
		void Generate_Missing_Normals(); //if loaded incomplete normals
		void Rotate(float,float,float);
		void Reize(float);
		void Move(float, float, float);

	private:
		char *name; //just for the other trimesh classes (for Racetime_Data)

		void Normalize_Normals(); //make sure normals are unit (for some loaders maybe not...)

		std::vector vertices<dReal[3]>;
		std::vector normals<dReal[3]>;

		//each triangle is 6 indices: first 3=vertices, last 3=normals
		std::vector triangles<unsigned int[6]>;
};

#endif

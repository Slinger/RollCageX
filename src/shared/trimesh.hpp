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
#include <string>
#include <limits.h>

#include <GL/gl.h>
#include <ode/ode.h>

#include "racetime_data.hpp"

//definitions:
//for keeping track for overflowing:
//#define INDEX_MAX UINT_MAX - dont care for now

//for indicating missing data
#define INDEX_ERROR UINT_MAX

//how big each VBO should be:
#define VBO_SIZE 8388608 //8MiB - or should it be MB? need to test...
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

		//
		//data to store:
		//

		//element (vertex+normal) for interleaved array
		struct Vertex
		{
			float x,y,z;
			float nx,ny,nz;
			float padding[2]; //ATI recommends "vertex" size divideable by 32...
		};

		//material (all lements are grouped by materials for performance)
		struct Material
		{
			GLuint start, stop; //where in vbo this material is used

			GLfloat ambient[4];
			GLfloat diffuse[4];
			GLfloat specular[4];
			GLfloat emission[4];
			GLfloat shininess;
		};

		//everything needed to render:
		Material *materials;
		unsigned int material_count;


		//VBO and position in VBO of array:
		GLuint vbo_id; //which vbo got this model
		static GLuint current_vbo; //which vbo is active

		//only graphics list rendering can access this stuff
		friend void Graphic_List_Render();
};

//for collision detection (geom) generation
class Trimesh_Geom_Data: public Racetime_Data
{
	public:
		class Geom *Create_Geom(); //creates geom from trimesh
		//TODO: ode supports callbacks for trimeshes, could be usefull...

	private:
		Trimesh_Geom_Data(const char*); //sends name to Racetime_Data
		friend class Trimesh; //only Trimesh is allowed to create this...

		//
		//data for trimesh:
		//

		dTriMeshDataID data; //collected, pointers
		dVector3 *vertices;
		unsigned int *indices;
		dVector3 *normals; //not needed, but already calculating gives extra performance
};



//trimesh storage class - not used during race, only while loading/processing
class Trimesh
{
	public:
		//Trimesh();
		//~Trimesh();

		//wrapper that decides loading function by file suffix:
		bool Load(const char*);

		//void Generate(const char*); - create trimesh by description, not supported/needed yet

		//create "dedicated" (used during race) timeshes from this one:
		Trimesh_3D *Create_3D();
		Trimesh_Geom_Data *Create_Geom_Data();

		//tools:
		void Resize(float);
		void Rotate(float,float,float);
		void Offset(float, float, float);

		void TMP_printinfo();
	private:
		//Set_Name(const char*);
		//char *name; //just for the other trimesh classes (for Racetime_Data name)
		std::string name;

		//tools:
		void Normalize_Normals(); //make sure normals are unit (for some loaders, like obj, maybe not...)
		void Generate_Missing_Normals(); //if loaded incomplete normals, solve
		unsigned int Find_Material(const char *); //find first matching material by name

		//functions for loading 3d files:
		//obj files (obj.cpp)
		bool Load_OBJ(const char *);
		bool Load_MTL(const char *); //used by obj loader
		//3ds files (3ds.cpp)
		bool Load_3DS(const char *);

		//
		//actual data to store:
		//

		struct Vector_Float{
			float x, y, z;
		};

		std::vector<Vector_Float> vertices;
		std::vector<Vector_Float> normals;

		//materials:
		struct Material
		{
			std::string name;

			GLfloat ambient[4];
			GLfloat diffuse[4];
			GLfloat specular[4];
			GLfloat emission[4];
			GLfloat shininess;
		};
		static const Material Material_Default;

		std::vector<Material> materials; //store materials

		//
		//indices:
		//

		//each triangle is 6 indices: 3=vertices, 3=normals
		struct Triangle_Index{
			unsigned int vertex[3];
			//texture coords
			unsigned int normal[3];
		};

		std::vector<Triangle_Index> triangles;

		struct Material_Index
		{
			unsigned int material; //number in list
			unsigned int start_at; //before switching to this material
		};

		std::vector<Material_Index> material_indices; //swith of materials
};

#endif

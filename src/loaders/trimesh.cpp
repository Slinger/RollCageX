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

#include "../shared/trimesh.hpp"

#include "../shared/printlog.hpp"

#include "text_file.hpp"

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

bool Trimesh::Load_OBJ(const char *f)
{
	printlog(1, "Loading trimesh from OBJ file %s", f);
	name = f;

	Text_File file;

	//check if ok...
	if (!file.Open(f))
	{
		printlog(0, "ERROR: could not open file!");
		return false;
	}
	
	//set name to filename
	name=f;

	//ok, start processing
	Vector_Float vector;
	Triangle_Index triangle;
	int i;
	unsigned int vi, ni;
	int count;
	//char *end;

	while (file.Read_Line())
	{
		// "v" vertex and 4 words
		if (file.words[0][0]=='v' && file.words[0][1]=='\0' && file.word_count==4)
		{
			vector.x=atof(file.words[1]);
			vector.y=atof(file.words[2]);
			vector.z=atof(file.words[3]);

			vertices.push_back(vector); //add to list
		} // "vn" normal and 4 words
		else if (file.words[0][0]=='v' && file.words[0][1]=='n' && file.words[0][2]=='\0' && file.word_count==4)
		{
			vector.x=atof(file.words[1]);
			vector.y=atof(file.words[2]);
			vector.z=atof(file.words[3]);

			normals.push_back(vector); //add to list
		} // "f" index and more than 3 words (needs at least 3 indices)
		else if (file.words[0][0]=='f' && file.words[0][1]=='\0' && file.word_count>3)
		{
			for (i=1; i<file.word_count; ++i)
			{
				// - format: v/(t)/(n) - vertex, texture, normal
				// only v is absolutely needed, and not optional
				// t ignored for now
				ni = INDEX_ERROR;
				count = sscanf(file.words[i], "%u/%*u/%u", &vi, &ni);

				if (count == 0) //nothing read
				{
					printlog(0, "ERROR: obj file got malformated index, ignoring");
					break;
				}
				else //at least v read
				{
					--vi; //obj starts count on 1, change to 0

					if (count == 2) //read everything - v,t,n
					{
						--ni; //1->0
					}
					else if (count == 1) //only v, not t (not provided), parhaps n is stil there
					{
						if (sscanf(file.words[i], "%*u//%u", &ni) == 1)
							--ni; //1->0
					}
					else
						printf("CODING ERROR CHECK: this should not be printed\n");
				}

				//now we got indices, see what to do with them
				//note: this checking is ugly from performance point of view, but helps keep the code clean
				//the first two times, just store indices, then start build triangles for each new index
				if (i>2) //time to build
				{
					//move latest to second latest
					triangle.vertex[1]=triangle.vertex[2]; //vertex
					triangle.normal[1]=triangle.normal[2]; //normal

					//add new
					triangle.vertex[2]=vi; //vertex
					triangle.normal[2]=ni; //normal

					//store
					triangles.push_back(triangle);
				}
				else if (i==1) //second time
				{
					triangle.vertex[2]=vi;
					triangle.normal[2]=ni;
				}
				else //first time
				{
					triangle.vertex[0]=vi;
					triangle.normal[0]=ni;
				}
			}
		}
		else if (!strcmp(file.words[0], "usemt") && file.word_count==2)
		{
			Material_Index mat;
			mat.material = Find_Material(file.words[1]);
			mat.start_at = triangles.size();

			if (mat.material == INDEX_ERROR)
				return false;

			//else, we now have material switch for next triangles
			//material_indices.push_back(mat);
		}
		else if (!strcmp(file.words[0], "mtllib") && file.word_count==2)
		{
			char filename[strlen(f)+strlen(file.words[1])]; //enough to hold both obj and mtl path+filename
			strcpy(filename, f); //copy obj path+filename
			const char *last = strrchr(filename, '/'); //last slash in obj filename

			if (last) //if obj file had a path before filename (most likely):
			{
				last=0; //indicate end at end of path
				strcat(filename, file.words[1]); //add mtl filename/path to obj path
			}
			else //just what's requested
			{
				strcpy(filename, file.words[1]); //overwrite with mtl filename
			}
				
			if (!Load_MTL(filename))
				return false;
		}
	}


	return true;
}


//bool Trimesh::Load_3DS(const char *file)
//{
	//Set_Name(file);


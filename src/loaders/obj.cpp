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


bool Trimesh::Load_OBJ(const char *f)
{
	printlog(1, "Loading trimesh from OBJ file %s", f);

	Text_File file;

	//check if ok...
	if (!file.Open(f))
		return false;
	
	//set name to filename
	name=f;

	//empty old data (if any)
	triangles.clear();
	vertices.clear();
	normals.clear();
	materials.clear();
	material_indices.clear();

	//
	//ok, start processing
	//
	Vector_Float vector;
	Triangle_Index triangle;
	int i;
	unsigned int vi, ni;
	int count;

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
						printf("CODING ERROR CHECK: this should not be printed\n"); //TODO: remove
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
				else if (i==2) //second time
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
		else if (!strcmp(file.words[0], "usemtl") && file.word_count==2)
		{
			Material_Index mat;
			mat.material = Find_Material(file.words[1]);
			mat.start_at = triangles.size();

			if (mat.material == INDEX_ERROR)
			{
				printlog(0, "Ignoring change of material (things will probably look wrong)");
				continue; //just ignore material change
			}

			//else, we now have material switch for next triangles
			material_indices.push_back(mat);
		}
		else if (!strcmp(file.words[0], "mtllib") && file.word_count==2)
		{
			char filename[strlen(f)+strlen(file.words[1])]; //enough to hold both obj and mtl path+filename
			strcpy(filename, f); //copy obj path+filename
			char *last = strrchr(filename, '/'); //last slash in obj filename

			if (last) //if obj file had a path before filename (most likely):
			{
				last[1]='\0'; //indicate end at end of path (after /)
				strcat(filename, file.words[1]); //add mtl filename/path to obj path
			}
			else //just what's requested
			{
				strcpy(filename, file.words[1]); //overwrite with mtl filename
			}
				
			Load_MTL(filename); //if not succesfull, continue - might not need any materials anyway?
		}
	}

	//check that at least something got loaded:
	if (triangles.empty() || vertices.empty())
	{
		printlog(0, "ERROR: obj seems to exist, but empty?!");
		return false;
	}

	//ok, lets just make sure all data is good:
	Normalize_Normals();
	//Generate_Missing_Normals(); //writes unit normals, don't need normalizing
	//

	//
	//TODO: perhaps check that all indices are within valid range? don't bother now, but prevents errors from malformed obj files...
	//
	return true;
}

bool Trimesh::Load_MTL(const char *f)
{
	printlog(1, "Loading trimesh material(s) from MTL file %s", f);

	Text_File file;

	//check if ok...
	if (!file.Open(f))
		return false;
	
	//
	//start processing
	//
	unsigned int mat_nr=INDEX_ERROR;

	while (file.Read_Line())
	{
		if ( (!strcmp(file.words[0], "newmtl")) && file.word_count == 2 )
		{
			mat_nr = materials.size(); //how much used, which number to give this material
			materials.push_back(Material_Default); //add new material (with defaults)
			materials[mat_nr].name = file.words[1]; //set name
		}
		else if (mat_nr == INDEX_ERROR)
		{
			printlog(0, "ERROR: mtl wants to specify material properties for unnamed material?! ignoring");
		}
		else
		{
			//material properties:
			if (file.words[0][0] == 'K') //colours?
			{
				if (file.words[0][1] == 'a' && file.word_count == 4) //ambient
				{
					materials[mat_nr].ambient[0] = atof(file.words[1]);
					materials[mat_nr].ambient[1] = atof(file.words[2]);
					materials[mat_nr].ambient[2] = atof(file.words[3]);
				}
				else if (file.words[0][1] == 'd' && file.word_count == 4) //diffuse
				{
					materials[mat_nr].diffuse[0] = atof(file.words[1]);
					materials[mat_nr].diffuse[1] = atof(file.words[2]);
					materials[mat_nr].diffuse[2] = atof(file.words[3]);
				}
				else if (file.words[0][1] == 's' && file.word_count == 4) //specular
				{
					materials[mat_nr].specular[0] = atof(file.words[1]);
					materials[mat_nr].specular[1] = atof(file.words[2]);
					materials[mat_nr].specular[2] = atof(file.words[3]);
				}

				//the following seems to be an unofficial extension of the mtl format (which us usefull):
				else if (file.words[0][1] == 'e' && file.word_count == 4) //emission
				{
					materials[mat_nr].emission[0] = atof(file.words[1]);
					materials[mat_nr].emission[1] = atof(file.words[2]);
					materials[mat_nr].emission[2] = atof(file.words[3]);
				}
			}
			else if (file.words[0][0] == 'N') //some other stuff?
			{
				//only one of these are used:
				if (file.words[0][1] == 's' && file.word_count == 4) //shininess
				{
					//usually, this vary between 0 to 1000 for obj, since opengl uses 0 to 128 translate
					materials[mat_nr].shininess = (atof(file.words[1])*(128.0/1000.0));
				}
			}


		}
	}

	//check if we got any data:
	if (mat_nr == INDEX_ERROR)
	{
		printlog(0, "ERROR: mtl existed, but was empty?!");
		return false;
	}

	//else, ok
	return true;
}


/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of ReCaged.
 *
 * ReCaged is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReCaged is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
 */ 

//#include "../shared/shared.hpp"
#include "../shared/track.hpp"

#include "../shared/internal.hpp"
#include "../shared/camera.hpp"
#include "../shared/geom.hpp"
#include "../shared/printlog.hpp"
#include "../shared/object.hpp"

#include "text_file.hpp"

#include <stdlib.h>

//
//keep track of all loaded models (cleared between loading)
//
std::vector<Trimesh*> trimeshes;

//helper function for finding or loading model files
Trimesh *FindOrLoadMesh(const char *path, const char *name)
{
	//merge path and name (path+/+name+\0)...
	char file[strlen(path)+1+strlen(name)+1];
	strcpy(file, path);
	strcat(file, "/");
	strcat(file, name);

	//already loaded?
	for (size_t i=0; i!=trimeshes.size(); ++i)
		if (trimeshes[i]->Compare_Name(file))
		{
			printlog(2, "model already loaded");
			return trimeshes[i];
		}

	//else, try loading...
	Trimesh *mesh = new Trimesh();
	if (mesh->Load(file))
	{
		trimeshes.push_back(mesh);
		return mesh; //ok, worked
	}
	//else¸ failure
	delete mesh;
	return NULL;
}

//remove all loading meshes
void RemoveMeshes()
{
	for (size_t i=0; i!=trimeshes.size(); ++i)
		delete trimeshes[i];
	trimeshes.clear();
}
//
//


bool load_track (const char *path)
{
	printlog(1, "Loading track: %s", path);

	//
	//conf
	//
	char conf[strlen(path)+11+1];
	strcpy (conf,path);
	strcat (conf,"/track.conf");

	load_conf(conf, (char *)&track, track_index);

	//set camera default values, some from track specs
	camera.Set_Pos(track.cam_start[0], track.cam_start[1], track.cam_start[2],
			track.focus_start[0], track.focus_start[1], track.focus_start[2]);

	//all data loaded, start building
	glClearColor (track.sky[0],track.sky[1],track.sky[2],track.sky[3]); //background
	glFogfv(GL_FOG_COLOR, track.sky); //"fog"
	
	//sun position and colour
	glLightfv (GL_LIGHT0, GL_AMBIENT, track.ambient);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, track.diffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, track.specular);
	glLightfv (GL_LIGHT0, GL_POSITION, track.position);

	//set track specific global ode params:
	dWorldSetGravity (world,0,0,-track.gravity);

	//
	//geoms
	//
	//using trimesh geoms and one plane for describing world, store in track object
	track.object = new Object();
	track.space = new Space (track.object);

	//loading of model files
	char glist[strlen(path)+10+1];
	strcpy (glist,path);
	strcat (glist,"/geoms.lst");

	printlog(1, "Loading track geom list: %s", glist);
	Text_File file;

	if (file.Open(glist))
	{
		//store default global surface properties for all geoms
		Surface global;
		//keep track of latest geom spawned
		Geom *latestgeom=NULL;

		while (file.Read_Line())
		{
			//if requesting optional stuff
			if (!strcmp(file.words[0], ">") && file.word_count >= 2)
			{
				//model manipulation
				if (!strcmp(file.words[1], "modify"))
				{
					printlog(2, "overriding model properties");

					Trimesh *mesh = FindOrLoadMesh(path, file.words[2]);

					if (!mesh)
					{
						RemoveMeshes();
						delete track.object;
						return false;
					}

					//now process the rest for extra options
					int pos = 3;
					while (pos < file.word_count)
					{
						int left=file.word_count-pos;

						//resize, takes the word resize and one value
						if (!strcmp(file.words[pos], "resize") && left >= 2)
						{
							mesh->Resize(atof(file.words[pos+1]));
							pos+=2;
						}
						//rotate, takes the word rotate and 3 values
						else if (!strcmp(file.words[pos], "rotate") && left >= 4)
						{
							mesh->Rotate(atof(file.words[pos+1]), //x
									atof(file.words[pos+2]), //y
									atof(file.words[pos+3])); //z
							pos+=4;
						}
						//offset, takes the word offset and 3 values
						else if (!strcmp(file.words[pos], "offset") && left >= 4)
						{
							mesh->Offset(atof(file.words[pos+1]), //x
									atof(file.words[pos+2]), //y
									atof(file.words[pos+3])); //z
							pos+=4;
						}
						else
						{
							printlog(0, "WARNING: trimesh loading option \"%s\" not known", file.words[pos]);
							++pos;
						}
					}
				}
				//surface manipulation (of latest geom)
				else if (!strcmp(file.words[1], "surface") && file.word_count >= 3)
				{
					printlog(2, "changing surface properties");
					Surface *surface=NULL;
					int pos=0;

					if (!strcmp(file.words[2], "global"))
					{
						surface = &global;
						pos = 3;
					}
					else if (!strcmp(file.words[2], "material") && file.word_count >= 4)
					{
						//locate surface named as specified
						surface = latestgeom->Find_Material_Surface(file.words[3]);
						pos = 4;
					}
					else
						printlog(0, "WARNING: surface type must be either global or material");

					if (!surface)
					{
						printlog(0, "WARNING: could not find specified surface");
						continue;
					}

					//as long as there are two words left (option name and value)
					while ( (file.word_count-pos) >= 2)
					{
						//note: mu and damping uses strtod instead of atof.
						//as usuall, this it's because losedows not being standardized
						//and failing to support infinity support for atof... :-p
						//only added for spring and mu - the only ones supporting it
						if (!strcmp(file.words[pos], "mu"))
							surface->mu = strtod(file.words[++pos], (char**)NULL);
						else if (!strcmp(file.words[pos], "bounce"))
							surface->bounce = atof(file.words[++pos]);
						else if (!strcmp(file.words[pos], "spring"))
							surface->spring = strtod(file.words[++pos], (char**)NULL);
						else if (!strcmp(file.words[pos], "damping"))
							surface->damping = atof(file.words[++pos]);
						else if (!strcmp(file.words[pos], "position"))
							surface->tyre_pos_scale = atof(file.words[++pos]);
						else if (!strcmp(file.words[pos], "sharpness"))
							surface->tyre_sharp_scale = atof(file.words[++pos]);
						else if (!strcmp(file.words[pos], "rollingres"))
							surface->tyre_rollres_scale = atof(file.words[++pos]);
						else
							printlog(0, "WARNING: trimesh surface option \"%s\" unknown", file.words[pos]);

						//one step forward
						pos+=1;
					}

				}
				else
					printlog(0, "WARNING: optional line in geoms.lst malformed");

			}
			//geom to create
			else if (file.word_count == 8 || file.word_count == 7)
			{
				Trimesh *mesh1, *mesh2;
				Trimesh_3D *model;
				Trimesh_Geom *geom;
				float x,y,z;

				//no alternative render model
				if ( file.word_count == 7)
				{
					if (	!(mesh1 = FindOrLoadMesh(path, file.words[6])) ||
						!(geom = mesh1->Create_Geom()) ||
						!(model = mesh1->Create_3D()) )
					{
						RemoveMeshes();
						delete track.object;
						return false;
					}
				}
				//one collision and one render model
				else if (file.word_count == 8)
				{
					if (	!(mesh1 = FindOrLoadMesh(path, file.words[6])) ||
						!(mesh2 = FindOrLoadMesh(path, file.words[7])) ||
						!(geom = mesh1->Create_Geom()) ||
						!(model = mesh2->Create_3D()) )
					{
						RemoveMeshes();
						delete track.object;
						return false;
					}
				}
				//in case failure to load
				else
				{
					continue; //go to next line
				}

				//ok, now geom and model should contain useful data...
				latestgeom = geom->Create_Geom(track.object); //create geom from geom-trimesh
				
				//configure geom
				latestgeom->model = model; //render geom with model
				latestgeom->surface = global; //start by using global specified properties

				//position
				x = atof(file.words[0]);
				y = atof(file.words[1]);
				z = atof(file.words[2]);
				dGeomSetPosition(latestgeom->geom_id, x,y,z);
				
				//rotation
				dMatrix3 rot;
				x = atof(file.words[3])*M_PI/180.0;
				y = atof(file.words[4])*M_PI/180.0;
				z = atof(file.words[5])*M_PI/180.0;

				dRFromEulerAngles(rot, x,y,z);
				dGeomSetRotation(latestgeom->geom_id, rot);
			}
			else
			{
				printlog(0, "WARNING: did not understand line in geom list...");
				continue;
			}
		}
	}
	else
	{
		printlog(0, "ERROR: no geom list for track! can not create any terrain...");
		RemoveMeshes();
		delete track.object;
		return false;
	}

	RemoveMeshes();

	//
	//objects
	//
	char olist[strlen(path)+12+1];
	strcpy (olist,path);
	strcat (olist,"/objects.lst");

	printlog(1, "Loading track object list: %s", olist);

	//each object is loaded/selected at a time (NULL if none loaded so far)
	Object_Template *obj = NULL;

	//don't fail if can't find file, maybe there is no need for it anyway
	if (file.Open(olist))
	{
		while (file.Read_Line())
		{
			//object load request
			if (file.word_count==2 && !strcmp(file.words[0], ">"))
			{
				printlog(2, "object load request: %s", file.words[1]);
				char obj_name[13+strlen(file.words[1])+1];
				strcpy (obj_name, "objects/");
				strcat (obj_name, file.words[1]);

				if (!(obj = Object_Template::Load(obj_name))) //NULL if failure
				{
					printlog(0, "ERROR: could not load object \"%s\"", obj_name);
					delete track.object;
					return false;
				}
			}
			//three words (x, y and z coord for spawning): spawning
			else if (file.word_count == 3)
			{
				printlog(2, "object spawn request");
				//in case no object has been loaded yet
				if (!obj)
				{
					printlog(0, "ERROR: trying to spawn object without specifying what object!");
					continue; //go to next
				}

				//translate words to values
				float x,y,z;

				//assume conversion is succesfully (not reliable, but it shouldn't be a problem)
				x = atof(file.words[0]);
				y = atof(file.words[1]);
				z = atof(file.words[2]);

				obj->Spawn(x, y, z);
			}
			else
			{
				printlog(0, "ERROR: unknown line in object list!");
				break;
			}
		}
	}
	else
		printlog(1, "WARNING: no object list for track, no default objects spawned");

	//that's it!
	return true;
}


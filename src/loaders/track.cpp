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

//#include "../shared/shared.hpp"
#include "../shared/track.hpp"

#include "../shared/camera.hpp"
#include "../shared/geom.hpp"
#include "../shared/printlog.hpp"
#include "../shared/object.hpp"

#include "colours.hpp"
#include "text_file.hpp"


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
	camera.Set_Pos(track.cam_start, track.focus_start);

	//append forced data
	track.position[3] = 0.0f; //directional
	track.ambient[3] = 1.0f; //a = 1.0f
	track.diffuse[3] = 1.0f; //-''-
	track.specular[3] = 1.0f; //-''-

	//all data loaded, start building
	glClearColor (track.sky[0],track.sky[1],track.sky[2],1.0f); //background
	
	//sun position and colour
	glLightfv (GL_LIGHT0, GL_AMBIENT, track.ambient);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, track.diffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, track.specular);
	glLightfv (GL_LIGHT0, GL_POSITION, track.position);

	//gravity
	dWorldSetGravity (world,0,0,-track.gravity);

	//
	//geoms
	//
	//using trimesh geoms and one plane for describing world, store in track object
	track.object = new Object();
	track.space = new Space (track.object);

	//tmp vars
	dGeomID geom;
	Geom *data;

	//tmp plane until respawning implemented
	geom = dCreatePlane (0, 0,0,1,track.respawn);
	data = new Geom(geom, track.object);
	data->mu = track.mu;
	data->slip = track.slip;
	data->erp = track.erp;
	data->cfm = track.cfm;

	//loading of model files
	char glist[strlen(path)+10+1];
	strcpy (glist,path);
	strcat (glist,"/geoms.lst");

	printlog(1, "Loading track geom list: %s", glist);
	Text_File file;

	if (file.Open(glist))
	{
		while (file.Read_Line())
		{
			//if requesting loading of file
			if (!strcmp(file.words[0], ">") && file.word_count >= 2)
			{
				Trimesh *mesh = FindOrLoadMesh(path, file.words[1]);

				if (!mesh)
					continue; //failure to load, skip this line...


				//now process the rest for extra options
				int pos = 2;
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
			//geom to create
			else if (file.word_count == 8 || file.word_count == 7)
			{
				Trimesh *mesh1, *mesh2;
				Trimesh_3D *model;
				Trimesh_Geom *geom;
				Geom *data;
				float x,y,z;

				//no alternative render model
				if ( (file.word_count == 7) &&
						(mesh1 = FindOrLoadMesh(path, file.words[6])) &&
						(geom = mesh1->Create_Geom()) &&
						(model = mesh1->Create_3D()) );

				//one collision and one render model
				else if ( (file.word_count == 8) &&
						(mesh1 = FindOrLoadMesh(path, file.words[6])) &&
						(mesh2 = FindOrLoadMesh(path, file.words[7])) &&
						(geom = mesh1->Create_Geom()) &&
						(model = mesh2->Create_3D()) );
				//in case failure to load
				else
				{
					continue; //go to next line
				}

				//ok, now geom and model should contain useful data...
				data = geom->Create_Geom(track.object); //create geom from geom-trimesh
				
				//configure geom
				data->model = model; //render geom with model
				data->mu = track.mu;
				data->slip = track.slip;
				data->erp = track.erp;
				data->cfm = track.cfm;

				//position
				x = atof(file.words[0]);
				y = atof(file.words[1]);
				z = atof(file.words[2]);
				dGeomSetPosition(data->geom_id, x,y,z);
				
				//rotation
				dMatrix3 rot;
				x = atof(file.words[3])*M_PI/180.0;
				y = atof(file.words[4])*M_PI/180.0;
				z = atof(file.words[5])*M_PI/180.0;

				dRFromEulerAngles(rot, x,y,z);
				dGeomSetRotation(data->geom_id, rot);
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
		printlog(0, "SERIOUS WARNING: no geom list for track! can not create any terrain...");
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
				strcpy (obj_name, "data/objects/");
				strcat (obj_name, file.words[1]);

				obj = Object_Template::Load(obj_name); //NULL if failure
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


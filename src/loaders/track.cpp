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


//keep track of all loaded models (cleared between loading)
std::vector<Trimesh> trimeshes;

//helper function for finding or loading model files
Trimesh *FindOrLoad(const char *path, const char *name)
{
	//merge path and name (path+/+name+\0)...
	char file[strlen(path)+1+strlen(name)+1];
	strcpy(file, path);
	strcat(file, "/");
	strcat(file, name);

	//already loaded?
	for (size_t i=0; i!=trimeshes.size(); ++i)
		if (trimeshes[i].Compare_Name(file))
		{
			printlog(2, "model already loaded");
			return &trimeshes[i];
		}

	//else, try loading...
	Trimesh mesh; //copy needed for some stupid reason
	trimeshes.push_back(mesh);
	if (trimeshes.back().Load(file)) //try loading
		return &trimeshes.back(); //ok, worked
	else //did not load
	{
		trimeshes.pop_back(); //remove new trimesh (not useful)
		return NULL; //return failure
	}
}


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
	camera.Set_Pos(track.cam_start, track.target_start);

	//append forced data
	track.position[3] = 0.0f; //directional
	track.ambient[3] = 1.0f; //a = 1.0f
	track.diffuse[3] = 1.0f; //-''-
	track.specular[3] = 1.0f; //-''-

	//all data loaded, start building
	glClearColor (track.sky[0],track.sky[1],track.sky[2],1.0f); //background
	
	glLightfv (GL_LIGHT0, GL_AMBIENT, track.ambient);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, track.diffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, track.specular);
	glLightfv (GL_LIGHT0, GL_POSITION, track.position);
	glEnable (GL_LIGHT0);
	glEnable (GL_LIGHTING);

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
				Trimesh *mesh = FindOrLoad(path, file.words[1]);

				//now process the rest for extra options
				printf("TODO!\n");
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
						(mesh1 = FindOrLoad(path, file.words[6])) &&
						(model = mesh1->Create_3D()) &&
						(geom = mesh1->Create_Geom()) );

				//one collision and one render model
				else if ( (file.word_count == 8) &&
						(mesh1 = FindOrLoad(path, file.words[6])) &&
						(mesh2 = FindOrLoad(path, file.words[7])) &&
						(model = mesh1->Create_3D()) &&
						(geom = mesh2->Create_Geom()) );
				//in case failure to load
				else
				{
					continue; //go to next line
				}

				//ok, now geom and model should contain useful data...
				data = geom->Create_Geom(track.object); //create geom from geom-trimesh
				data->model = model; //render geom with model

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

	trimeshes.clear();

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

				obj = Object_Template::Load(obj_name);

				//failure to load object
				if (!obj)
					break;
			}
			//three words (x, y and z coord for spawning): spawning
			else if (file.word_count == 3)
			{
				printlog(2, "object spawn request");
				//in case no object has been loaded yet
				if (!obj)
				{
					printlog(0, "ERROR: trying to spawn object without specifying what object!");
					break;
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
	{
		printlog(0, "WARNING: no object list for track, no default objects spawned");
	}

	//that's it!
	return true;
}


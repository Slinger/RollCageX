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

bool load_track (const char *path)
{
	printlog(1, "Loading track: %s", path);

	//conf
	char conf[strlen(path)+11+1];
	strcpy (conf,path);
	strcat (conf,"/track.conf");

	load_conf(conf, (char *)&track, track_index);

	//obj (tmp)
	Trimesh mesh;
	mesh.Load("data/worlds/Sandbox/tracks/Box/Box.obj"); //assume loading fine
	Trimesh_3D *mesh3d = mesh.Create_3D();

	if (!mesh3d) //fail
		return false;

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

	//(for now, use geoms to describe world)
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

	geom = dCreateBox (0,1,1,1); //tmp for rendering
	dGeomSetPosition (geom, 0,0,0);
	data = new Geom(geom, track.object);
	data->model = mesh3d;


	//now lets load some objects!
	char list[strlen(path)+12+1];
	strcpy (list,path);
	strcat (list,"/objects.lst");

	printlog(1, "Loading track object list: %s", path);
	Text_File file;

	//each object is loaded/selected at a time (NULL if none loaded so far)
	Object_Template *obj = NULL;

	//don't fail if can't find file, maybe there is no need for it anyway
	if (file.Open(list))
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

	//that's it!
	return true;
}


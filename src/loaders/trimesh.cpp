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

//
//for trimesh file loading
//

//wrapper for loading
bool Trimesh::Load(const char *file)
{
	printlog(1, "Loading trimesh from file \"%s\"", file);
	printlog(2, "determining file type from suffix");

	if (file == NULL)
	{
		printlog(0, "WARNING: empty file path+name for trimesh");
		return false;
	}

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		printlog(0, "ERROR: no suffix for file \"%s\"", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".obj"))
		return Load_OBJ(file);
	else if (!strcasecmp(suffix, ".road"))
		return Load_Road(file);
	//else if (!strcasecmp(suffix, ".3ds"))
		//return Load_3DS(file);
	
	//else, no match
	printlog(0, "ERROR: unknown 3D file suffix for \"%s\"", file);
	return false;
}

//for materials
bool Trimesh::Load_Material(const char *file)
{
	printlog(1, "Loading material from file \"%s\"", file);
	printlog(2, "determining file type from suffix");

	if (file == NULL)
	{
		printlog(0, "WARNING: empty file path+name for material");
		return false;
	}

	const char *suffix = strrchr(file, '.');

	//in case something really wrong
	if (!suffix)
	{
		printlog(0, "ERROR: no suffix for file \"%s\"", file);
		return false;
	}

	//see if match:
	if (!strcasecmp(suffix, ".mtl"))
		return Load_MTL(file);

	//else, no match
	printlog(0, "ERROR: unknown 3D file suffix for \"%s\"", file);
	return false;
}

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

bool Trimesh::Load(const char *file)
{
	char *suffix = strrchr(file, '.');

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
	Set_Name(f);

	Text_File file;

	//check if ok...
	if (!file.Open(file))
	{
		printlog(0, "ERROR: could not open file!");
		return false;
	}

	//ok, start processing
	float vector[3];
	unsigned int index[6];

	while (file.Read_Line())
	{
		if (file.words[0][0]=='v' && file.words[0][1]=='\0') //vertex
		{
		}
		else if (file.words
	}

}


//bool Trimesh::Load_3DS(const char *file)
//{
	//Set_Name(file);


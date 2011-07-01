/*
 * RollCageX - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of RollCageX.
 *
 * RollCageX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RollCageX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RollCageX.  If not, see <http://www.gnu.org/licenses/>.
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

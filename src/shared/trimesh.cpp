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

#include "trimesh.hpp"

/*Trimesh::Trimesh()
{
	name=NULL;
}

Trimesh::~Trimesh()
{
	if (name)
		delete name;
}*/

void Trimesh::Set_Name(const char *n)
{
	//unlikely, but anyway
	if (name)
		delete[] name;


	name = new char[strlen(n)+1]; //alloc
	strcpy(name, n);
}

void Trimesh::Resize(float r)
{
	if (r == 1.0) //no need
		return;

	printlog(2, "Resizing trimesh");
	???
}

void Trimesh::Rotate(float x, float y, float z)
{
	if (x==y==z==0)
		return;

	printlog(2, "Rotating trimesh");
	???
}

void Trimesh::Offset(float x, float y, float z)
{
	if (x==y==z==0)
		return;

	printlog(2, "Changing offset of trimesh");
	???
}

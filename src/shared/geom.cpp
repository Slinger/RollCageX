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

#include <ode/ode.h>
#include "geom.hpp"
#include "printlog.hpp"
#include "track.hpp"
#include "internal.hpp"
#include "../simulation/event_buffers.hpp"
#include "../loaders/conf.hpp"

Geom *Geom::head = NULL;

//allocates a new geom data, returns its pointer (and uppdate its object's count),
//ads it to the component list, and ads the data to specified geom (assumed)
Geom::Geom (dGeomID geom, Object *obj): Component(obj) //pass object argument to base class constructor
{
	printlog(2, "configuring Geom class");

	//increase object activity counter
	object_parent->Increase_Activity();

	//parent object
	if (obj->selected_space)
	{
		printlog(2, "(adding to selected space)");
		dSpaceAdd (obj->selected_space, geom);
	}
	else //add geom to global space
	{
		dSpaceAdd (space, geom);
	}

	//add it to the geom list
	next = Geom::head;
	Geom::head = this;
	prev = NULL;

	//one more geom after this
	if (next)
		next->prev = this;
	else
		printlog(2, "(first registered)");

	//add it to the geom
	dGeomSetData (geom, (void*)(Geom*)(this));
	geom_id = geom;

	//now lets set some default values...
	//event processing (triggering):
	colliding = false; //no collision event yet
	model = NULL; //default: don't render

	//special geom indicators
	wheel = NULL; //not a wheel
	triangle_count = 0; //no "triangles"
	material_count = 0; //no "materials"
	triangle_colliding = NULL;
	material_surfaces = NULL;

	//events:
	//for force handling (disable)
	buffer_event=false;
	force_to_body=NULL; //when true, points at wanted body
	sensor_event=false;

	//debug variables
	flipper_geom = 0;
	TMP_pillar_geom =false; //not a demo pillar geom
}
//destroys a geom, and removes it from the list
Geom::~Geom ()
{
	//lets just hope the given pointer is ok...
	printlog(2, "clearing Geom class");

	//remove all events
	Event_Buffer_Remove_All(this);

	//1: remove it from the list
	if (!prev) //head in list, change head pointer
		Geom::head = next;
	else //not head in list, got a previous link to update
		prev->next = next;

	if (next) //not last link in list
		next->prev = prev;

	dGeomDestroy(geom_id);

	//decrease activity and check if 0
	object_parent->Decrease_Activity();

	//clear possible collision checking and material-based-surfaces
	delete[] triangle_colliding;
	delete[] material_surfaces;
}

Surface *Geom::Find_Material_Surface(const char *name)
{
	//is possible at all?
	if (!material_count)
	{
		printlog(0, "WARNING: tried to use per-material surfaces for non-trimesh geom");
		return NULL;
	}

	//firtst of all, check if enabled?
	if (!material_surfaces)
	{
		printlog(2, "enabling per-material surfaces");
		material_surfaces = new Surface[material_count];

		//set default (set to out global surface)
		for (int i=0; i<material_count; ++i)
			material_surfaces[i] = surface;
	}

	//ok 
	int i;
	for (i=0; i<material_count && strcmp(name, parent_materials[i].name); ++i);

	if (i==material_count)
	{
		printlog(0, "WARNING: could not find material \"%s\" for trimesh", name);
		return NULL;
	}
	else
		return &material_surfaces[i];
}

//set defaults:
Surface::Surface()
{
	//collision contactpoint data
	mu = 0.0;
	spring = dInfinity; //infinite spring constant (disabled)
	damping = 0.0; //no collision damping (only with spring)
	bounce = 0.0; //no bouncyness

	//normal friction scaling for tyre
	tyre_pos_scale = 1.0;
	tyre_sharp_scale = 1.0;
	tyre_rollres_scale = 1.0;
}


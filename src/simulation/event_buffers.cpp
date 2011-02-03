/*
 * RCX  Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger")
 *
 * This program comes with ABSOLUTELY NO WARRANTY!
 *
 * This is free software, and you are welcome to
 * redistribute it under certain conditions.
 *
 * See license.txt and README for more info
 */

#include "../shared/track.hpp"
#include "event_buffers.hpp"
#include "timers.hpp"


//
//component buffers:
//

//standard single linked-list approach for storing:
struct Link
{
	void *p;
	Link *next;
};

//heads (lists/buffers used)
Link *geom_depleted=NULL; //=damaged
Link *geom_triggered=NULL; //=sensor triggered
//Link *geom_radar=NULL; somehow keep track of all geoms
Link *body_depleted=NULL; //=damaged
Link *joint_depleted=NULL; //=damaged
Link *object_inactive=NULL; //=done

//
//universal buffer processing functions (used through wrappers)
//
//add to buffer
void Push(Link **buffer, void *p)
{
	Link *tmp = new Link;
	tmp->p = p;
	tmp->next = *buffer;
	*buffer = tmp;
}
//get top and remove from buffer
void *Pop(Link **buffer)
{
	//end of list?
	if (!*buffer)
		return NULL;

	//remove this link
	Link *tmp = *buffer;
	*buffer = tmp->next;

	//return data and remove
	void *p = tmp->p;
	delete (tmp);

	//ok
	return p;
}
//remove all matching from buffer
void RemoveAll(Link **buffer, void *target)
{
	Link *p=*buffer; //points at current event in list
	Link **pp=&(*buffer); //points at pointer ("next") pointing at current event

	while (p)
	{
		if (p->p == target) //remove
		{
			*pp = p->next; //change last 'next' pointer
			delete p; //delete this block
			p = *pp; //point at next block
		}
		else //keep, jump over
		{
			pp = &p->next; //change pointer-pointer to next 'next' pointer... ;-)
			p = p->next; //change pointer to next block
		}
	}
}

//
//wrappers (for use of above functions)
//
//adding functions
void Event_Buffer_Add_Depleted(Geom *geom)
{
	printlog(2, "Geom depleted event registered");
	Push(&geom_depleted, (void*)geom);
}

void Event_Buffer_Add_Triggered(Geom *geom)
{
	printlog(2, "Geom sensor event registered");
	Push(&geom_triggered, (void*)geom);
}

void Event_Buffer_Add_Depleted(Body *body)
{
	printlog(2, "Body depleted event registered");
	Push(&body_depleted, (void*)body);
}

void Event_Buffer_Add_Depleted(Joint *joint)
{
	printlog(2, "Joint depleted event registered");
	Push(&joint_depleted, (void*)joint);
}

void Event_Buffer_Add_Inactive(Object *object)
{
	printlog(2, "Object inactive event registered");
	Push(&object_inactive, (void*)object);
}


//removing functions (when needing to remove all events for one thing)
void Event_Buffer_Remove_All(Geom *geom)
{
	RemoveAll(&geom_depleted, geom); //damage
	RemoveAll(&geom_triggered, geom); //sensor
}

void Event_Buffer_Remove_All(Body *body)
{
	RemoveAll(&body_depleted, body);
}

void Event_Buffer_Remove_All(Joint *joint)
{
	RemoveAll(&joint_depleted, joint);
}

void Event_Buffer_Remove_All(Object *object)
{
	RemoveAll(&object_inactive, object);
}

//function for parsing all buffered events

//process all events:
void Event_Buffers_Process(dReal step)
{
	Geom *geom;
	Body *body;
	Joint *joint;
	Object *object;

	//geom buffer:
	while ((geom = (Geom*)Pop(&geom_depleted)))
	{
		dBodyID bodyid = dGeomGetBody(geom->geom_id);

		//if has body, remove body and this geom
		if (bodyid)
		{
			//break into two pieces
			if (geom->TMP_pillar_geom)
			{
				const dReal *rot = dBodyGetRotation(bodyid);
				dVector3 pos1, pos2;
				dBodyGetRelPointPos(bodyid, 0,0,5.0/4.0, pos1);
				dBodyGetRelPointPos(bodyid, 0,0,-5.0/4.0, pos2);

				//geom1
				dGeomID g = dCreateBox(0, 2,2,5.0/2.0);
				Geom *gd = new Geom(g, geom->object_parent);
				gd->surface.mu = 1.0;
				gd->Set_Buffer_Event(150000, 1000, (Script*)1337);

				//body1
				dBodyID b = dBodyCreate(world);
				dMass m;
				dMassSetBoxTotal (&m, 100, 2,2,5.0/2.0);
				dBodySetMass(b, &m);
				new Body(b, geom->object_parent);
				dBodySetPosition(b, pos1[0], pos1[1], pos1[2]);
				dBodySetRotation(b, rot);
				dGeomSetBody(g,b);

				gd->model = geom->TMP_pillar_graphics;

				//geom2
				g = dCreateBox(0, 2,2,5.0/2.0);
				gd = new Geom(g, geom->object_parent);
				gd->surface.mu = 1.0;
				gd->Set_Buffer_Event(150000, 1000, (Script*)1337);

				//body2
				b = dBodyCreate(world);
				dMassSetBoxTotal (&m, 100, 2,2,5.0/2.0);
				dBodySetMass(b, &m);
				new Body(b, geom->object_parent);
				dBodySetPosition(b, pos2[0], pos2[1], pos2[2]);
				dBodySetRotation(b, rot);
				dGeomSetBody(g,b);

				gd->model = geom->TMP_pillar_graphics;
			}
			Body *body = (Body*)dBodyGetData(bodyid);

			delete geom;
			delete body;
		}
		else //static geom, hmm...
		{
			if (geom->TMP_pillar_geom)
			{
				//pillar that should be getting a body (to detach), and buffer refill
				dBodyID body = dBodyCreate(world);

				//mass
				dMass m;
				dMassSetBoxTotal (&m, 200, 2,2,5);
				dBodySetMass(body, &m);

				new Body(body, geom->object_parent);
				//position
				const dReal *pos = dGeomGetPosition(geom->geom_id);
				dBodySetPosition(body, pos[0], pos[1], pos[2]);

				//attach
				dGeomSetBody(geom->geom_id, body);


				//reset buffer
				geom->Increase_Buffer(8000);
			}
		}
	}

	//geom sensor:
	while ((geom = (Geom*)Pop(&geom_triggered)))
	{
		if (geom->flipper_geom)
		{
			//this geom (the sensor) is connected to the flipper surface geom ("flipper_geom") which is moved
			if (geom->sensor_last_state == true) //triggered
			{
				//run script for each step with a value going from <z> to <z+2> over 0.1 seconds
				const dReal *pos;
				pos = dGeomGetPosition(geom->flipper_geom); //get position (need z)
				new Animation_Timer(geom->object_parent, (Script*)geom->flipper_geom, pos[2], pos[2]+2.0, 0.1);
				//note: Animation_Timer expects a script, but pass flipper geom instead...
			}
		}
		else
			printlog(0, "WARNING: tmp unidentified geom got configured as sensor?! - ignoring...");
	}

	//body buffer:
	Geom *next;
	while ((body = (Body*)Pop(&body_depleted)))
	{
		//first of all, remove all connected (to this body) geoms:
		//ok, this is _really_ uggly...
		//ode lacks a "dBodyGetGeom" routine (why?! it's easy to implement!)...
		//solution: loop through all geoms remove all with "force_to_body"==this body
		for (geom=Geom::head; geom; geom=next)
		{
			next=geom->next; //needs this after possibly destroying the geom (avoid segfault)
			if (geom->force_to_body == body)
				delete geom;
		}

		delete body;
	}

	//joints buffer:
	while ((joint = (Joint*)Pop(&joint_depleted)))
		delete joint;

	//objects buffer:
	while ((object = (Object*)Pop(&object_inactive)))
		delete object;
}


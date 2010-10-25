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

#include "../shared/track.hpp"
#include "event_buffers.hpp"
#include "timers.hpp"


//
//component buffers:
//

//make sure header pointers are NULL for the wanted lists
Buffer_Event_List *Buffer_Event_List::geom_head = NULL;
Buffer_Event_List *Buffer_Event_List::body_head = NULL;
Buffer_Event_List *Buffer_Event_List::joint_head = NULL;

//for adding to lists
Buffer_Event_List::Buffer_Event_List(Geom *geom): component(geom)
{
	next = geom_head;
	geom_head = this;
}

Buffer_Event_List::Buffer_Event_List(Body *body): component(body)
{
	next = body_head;
	body_head = this;
}

Buffer_Event_List::Buffer_Event_List(Joint *joint): component(joint)
{
	next = joint_head;
	joint_head = this;
}

//for getting
bool Buffer_Event_List::Get_Event(Geom **geom)
{
	//if end of list
	if (!geom_head)
		return false;

	//remove from list
	Buffer_Event_List *tmp = geom_head;
	geom_head = tmp->next;

	//set wanted data and delete
	*geom = (Geom*)tmp->component; //safe to assume this list consists of wanted component
	delete (tmp);

	return true;
}

bool Buffer_Event_List::Get_Event(Body **body)
{
	if (!body_head)
		return false;

	Buffer_Event_List *tmp = body_head;
	body_head = tmp->next;

	*body = (Body*)tmp->component;
	delete (tmp);

	return true;
}

bool Buffer_Event_List::Get_Event(Joint **joint)
{
	if (!joint_head)
		return false;

	Buffer_Event_List *tmp = joint_head;
	joint_head = tmp->next;

	*joint = (Joint*)tmp->component;
	delete (tmp);

	return true;
}

//removing
void Buffer_Event_List::Remove(Geom *comp)
{
	printlog(2, "seeking and removing all events for specified Geom");

	Buffer_Event_List *p = geom_head; //points at current event in list
	Buffer_Event_List **pp = &geom_head; //points at pointer ("next") pointing at current event

	while (p)
	{
		if (p->component == comp) //remove
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

void Buffer_Event_List::Remove(Body *comp)
{
	printlog(2, "seeking and removing all events for specified Body");
	Buffer_Event_List *p = body_head;
	Buffer_Event_List **pp = &body_head;
	while (p)
	{
		if (p->component == comp)
		{
			*pp = p->next;
			delete p;
			p = *pp;
		}
		else
		{
			pp = &p->next;
			p = p->next;
		}
	}
}

void Buffer_Event_List::Remove(Joint *comp)
{
	printlog(2, "seeking and removing all events for specified Joint");

	Buffer_Event_List *p = joint_head;
	Buffer_Event_List **pp = &joint_head;
	while (p)
	{
		if (p->component == comp)
		{
			*pp = p->next;
			delete p;
			p = *pp;
		}
		else
		{
			pp = &p->next;
			p = p->next;
		}
	}
}

//
//sensor triggering/untriggering
//

Sensor_Event_List *Sensor_Event_List::head = NULL;

Sensor_Event_List::Sensor_Event_List(Geom *g): geom(g)
{
	next = head;
	head = this;
}

bool Sensor_Event_List::Get_Event(Geom **g)
{
	if (!head)
		return false;

	Sensor_Event_List *tmp = head;
	head = tmp->next;

	*g = tmp->geom;
	delete (tmp);

	return true;
}

void Sensor_Event_List::Remove(Geom *geom)
{
	printlog(2, "removing all events for specified Sensor");

	Sensor_Event_List *p = head; //points at first event
	Sensor_Event_List **pp = &head; //points at pointer for first event

	while (p)
	{
		if (p->geom == geom)
		{
			*pp = p->next;
			delete p;
			p = *pp;
		}
		else
		{
			pp = &p->next;
			p = p->next;
		}
	}
}

//
//object inactivity:
//
Object_Event_List *Object_Event_List::head = NULL;

Object_Event_List::Object_Event_List(Object *obj): object(obj)
{
	next = head;
	head = this;
}

bool Object_Event_List::Get_Event(Object **obj)
{
	if (!head)
		return false;

	*obj = head->object;

	Object_Event_List *tmp = head;
	head = head->next;

	delete tmp;
	return true;
}

void Object_Event_List::Remove(Object *obj)
{
	printlog(2, "removing all events for specified Object");

	Object_Event_List *p = head;
	Object_Event_List **pp = &head;
	while (p)
	{
		if (p->object == obj)
		{
			*pp = p->next;
			delete p;
			p = *pp;
		}
		else
		{
			pp = &p->next;
			p = p->next;
		}
	}
}


//process all events:
void Event_Buffers_Process(dReal step)
{
	Geom *geom;
	Body *body;
	Joint *joint;

	//buffer:
	while (Buffer_Event_List::Get_Event(&geom))
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
				gd->mu = 1.0;
				gd->Set_Buffer_Event(150000, 1000, (Script*)1337);

				//body1
				dBodyID b = dBodyCreate(world);
				dMass m;
				dMassSetBox (&m, 1, 2,2,5.0/2.0);
				dMassAdjust (&m, 100); //200kg
				dBodySetMass(b, &m);
				new Body(b, geom->object_parent);
				dBodySetPosition(b, pos1[0], pos1[1], pos1[2]);
				dBodySetRotation(b, rot);
				dGeomSetBody(g,b);

				gd->model = geom->TMP_pillar_graphics;

				//geom2
				g = dCreateBox(0, 2,2,5.0/2.0);
				gd = new Geom(g, geom->object_parent);
				gd->mu = 1.0;
				gd->Set_Buffer_Event(150000, 1000, (Script*)1337);

				//body2
				b = dBodyCreate(world);
				dMassSetBox (&m, 1, 2,2,5.0/2.0);
				dMassAdjust (&m, 100); //200kg
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
				dMassSetBox (&m, 1, 2,2,5);
				dMassAdjust (&m, 200); //200kg
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

	Geom *next;
	while (Buffer_Event_List::Get_Event(&body))
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

	//loop joints
	while (Buffer_Event_List::Get_Event(&joint))
	{
		//assume the joint should be destroyed
		delete joint;
	}

	//sensors:
	while (Sensor_Event_List::Get_Event(&geom))
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
			printlog(0, "WARNING: unidentified geom got configured as sensor?! - ignoring...");
	}

	//objects (depleted activity)
	Object *obj;
	while (Object_Event_List::Get_Event(&obj))
		delete obj;
}

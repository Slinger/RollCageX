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

#include "../shared/body.hpp"
#include "../shared/car.hpp"
#include "../shared/printlog.hpp"
#include "../shared/track.hpp"
#include "../shared/internal.hpp"
#include "event_buffers.hpp"

#define v_length(x, y, z) (sqrt( (x)*(x) + (y)*(y) + (z)*(z) ))
//functions for body drag

void Body::Update_Mass()
{
	printlog(2, "storing body mass for drag calculations");

	dMass dmass;

	//TODO: use the body's inertia tensor instead...?
	dBodyGetMass (body_id, &dmass);

	mass = dmass.mass;
}

//NOTE: modifying specified drag to the current mass (rice-burning optimization, or actually good idea?)
//(this way the body mass doesn't need to be requested and used in every calculation)
void Body::Set_Linear_Drag (dReal drag)
{
	printlog(2, "setting body linear drag");

	linear_drag = drag;
	use_advanced_linear_drag = false;
}

void Body::Set_Advanced_Linear_Drag (dReal drag_x, dReal drag_y, dReal drag_z)
{
	printlog(2, "setting body advanced linear drag");

	advanced_linear_drag[0] = drag_x;
	advanced_linear_drag[1] = drag_y;
	advanced_linear_drag[2] = drag_z;

	use_advanced_linear_drag = true;
}

void Body::Set_Angular_Drag (dReal drag)
{
	printlog(2, "setting body angular drag");
	angular_drag = drag;
}


//simulation of drag
//
//not to self: if implementing different density areas, this is where density should be chosen
void Body::Linear_Drag (dReal step)
{
	const dReal *abs_vel; //absolute vel
	abs_vel = dBodyGetLinearVel (body_id);
	dReal vel[3] = {abs_vel[0]-track.wind[0], abs_vel[1]-track.wind[1], abs_vel[2]-track.wind[2]}; //relative to wind
	dReal total_vel = v_length(vel[0], vel[1], vel[2]);

	//how much of original velocity is left after breaking by air/liquid drag
	dReal remain = 1-(total_vel*(track.density)*(linear_drag/mass)*(step));

	if (remain < 0) //in case breaking is so extreme it will reverse movement, just change velocity to 0
		remain = 0;

	//change velocity
	vel[0]*=remain;
	vel[1]*=remain;
	vel[2]*=remain;

	//make absolute
	vel[0]+=track.wind[0];
	vel[1]+=track.wind[1];
	vel[2]+=track.wind[2];

	//set velocity
	dBodySetLinearVel(body_id, vel[0], vel[1], vel[2]);
}

//similar to linear_drag, but different drag for different directions
void Body::Advanced_Linear_Drag (dReal step)
{
	//absolute velocity
	const dReal *abs_vel;
	abs_vel = dBodyGetLinearVel (body_id);

	//translate movement to relative to car (and to wind)
	dVector3 vel;
	dBodyVectorFromWorld (body_id, (abs_vel[0]-track.wind[0]), (abs_vel[1]-track.wind[1]), (abs_vel[2]-track.wind[2]), vel);
	dReal total_vel = v_length(vel[0], vel[1], vel[2]);

	//how much of original velocities is left after breaking by air/liquid drag
	dReal remain;
	int i;
	for (i=0; i<3; ++i)
	{
		//how much of original velocity remains after drag?
		remain = 1-(total_vel*(track.density)*(advanced_linear_drag[i]/mass)*(step));

		//check so not going negative
		if (remain < 0)
			remain = 0;

		//change velocity
		vel[i]*=remain;
	}

	//make absolute
	dVector3 vel_result;
	dBodyVectorToWorld (body_id, vel[0], vel[1], vel[2], vel_result);

	//add wind
	vel_result[0]+=track.wind[0];
	vel_result[1]+=track.wind[1];
	vel_result[2]+=track.wind[2];

	//set velocity
	dBodySetLinearVel(body_id, vel_result[0], vel_result[1], vel_result[2]);
}

void Body::Angular_Drag (dReal step)
{
	const dReal *vel; //rotation velocity
	vel = dBodyGetAngularVel (body_id);
	dReal total_vel = v_length(vel[0], vel[1], vel[2]);

	//how much of original velocity is left after breaking by air/liquid drag
	dReal remain = 1-(total_vel*(track.density)*(angular_drag/mass)*(step));

	if (remain < 0) //in case breaking is so extreme it will reverse movement, just change velocity to 0
		remain = 0;

	//set velocity with change
	dBodySetAngularVel(body_id, vel[0]*remain, vel[1]*remain, vel[2]*remain);
}


void Body::Set_Buffer_Event(dReal thres, dReal buff, Script *scr)
{
	if (thres > 0 && buff > 0 && scr)
	{
		printlog(2, "setting Body event");

		threshold=thres;
		buffer=buff;
		buffer_script=scr;

		//make sure no old event is left
		Event_Buffer_Remove_All(this);
		buffer_event=true;
	}
	else
	{
		printlog(2, "disabling Body event");

		Event_Buffer_Remove_All(this);

		//disable
		buffer_event=false;
	}
}

void Body::Damage_Buffer(dReal force, dReal step)
{
	//if not processing forces or not high enough force, no point continuing
	if (!buffer_event || (force<threshold))
		return;

	//buffer still got health
	if (buffer > 0)
	{
		buffer -= force*step;

		//now it's negative, issue event
		if (buffer < 0)
			Event_Buffer_Add_Depleted(this);
	}
	else //just damage buffer even more
		buffer -= force*step;
}

void Body::Physics_Step (dReal step)
{
	Body *body, *bnext = Body::head;

	Geom *geom, *gnext;

	while ((body = bnext))
	{
		//drag
		if (body->use_advanced_linear_drag)
			body->Advanced_Linear_Drag(step);
		else //simple drag instead
			body->Linear_Drag(step);

		//angular
		body->Angular_Drag(step);


		//check if at "respawn depth"

		//store pointer to next (if removing below)
		bnext = body->next;

		const dReal *pos = dBodyGetPosition(body->body_id); //get position
		if (pos[2] < track.respawn) //under respawn height
		{
			Car *car = dynamic_cast<Car*>(body->object_parent);

			//this is part of a car, it can be respawned
			if (car)
				car->Respawn(track.start[0], track.start[1], track.start[2]);
			//else, this is part of an object, destroy it (and any attached geom)
			else
			{
				//find all geoms that are attached to this body (proper cleanup)
				//ode lacks a "dBodyGetGeom" routine (why?! it's easy to implement!)...
				gnext = Geom::head;
				while ((geom = gnext))
				{
					gnext=geom->next; //needs this after possibly destroying the geom (avoid segfault)
					if (dGeomGetBody(geom->geom_id) == body->body_id)
						delete geom;
				}

				//and remove the body
				delete body;
			}
		}
	}
}


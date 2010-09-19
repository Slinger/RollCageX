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

#include "../shared/geom.hpp"

#include "../shared/internal.hpp"
#include "../shared/track.hpp"
#include "../shared/printlog.hpp"

#include "collision_feedback.hpp"

#include "../events/event_lists.hpp"

#include <ode/ode.h>

//when two geoms might intersect
void Geom::Collision_Callback (void *data, dGeomID o1, dGeomID o2)
{
	//check if one (or both) geom is space
	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2 (o1,o2, data, &Collision_Callback);
		return;
	}

	//both geoms are geoms, get component_data from geoms
	Geom *geom1, *geom2;
	geom1 = (Geom*) dGeomGetData (o1);
	geom2 = (Geom*) dGeomGetData (o2);

	//get attached bodies
	dBodyID b1, b2;
	b1 = dGeomGetBody(o1);
	b2 = dGeomGetBody(o2);

	//the stepsize (supplied as the "collision data")
	dReal stepsize = *((dReal*)data);

	//none connected to bodies
	if (!b1 && !b2)
		return;

	//none wants to create collisions..
	if (!geom1->spring&&!geom2->spring)
	{
		printlog(1, "not collideable, TODO: bitfield solution");
		return;
	}

	dContact contact[internal.contact_points];
	int count = dCollide (o1,o2,internal.contact_points, &contact[0].geom, sizeof(dContact));

	if (count == 0)
		return;

	//does both components want to collide for real? (not "ghosts"/"sensors")
	if (geom1->spring&&geom2->spring)
	{
		//default+optional data:
		dSurfaceParameters surface_base;
		surface_base.mode = dContactApprox1; //only nice friction enabled yet
		surface_base.mu = (geom1->mu)*(geom2->mu); //friction

		//all other parameters are optional... (here set to 0 to prevent compile warnings)
		surface_base.bounce = 0.0;
		surface_base.soft_erp = 0.0;
		surface_base.soft_cfm = 0.0;


		bool feedback = false;
		//if any of the geoms responds to forces or got a body that responds to force, enable force feedback
		if (geom1->buffer_event || geom2->buffer_event || geom1->force_to_body || geom2->force_to_body)
			feedback = true;

		//
		//optional features:
		//
		//optional bouncyness (good for wheels?)
		if (geom1->bounce != 0.0 || geom2->bounce != 0.0)
		{
			//enable bouncyness
			surface_base.mode |= dContactBounce;

			//use sum
			surface_base.bounce = (geom1->bounce)+(geom2->bounce);
		}

		//optional spring+damping erp+cfm override
		if (geom1->spring != dInfinity || geom2->spring != dInfinity)
		{
			//enable erp+cfm overriding
			surface_base.mode |= dContactSoftERP | dContactSoftCFM;

			//should be good
			dReal spring = 1/( 1/(geom1->spring) + 1/(geom2->spring) );
			//sum
			dReal damping = geom1->damping + geom2->damping;

			//calculate erp+cfm from stepsize, spring and damping values:
			surface_base.soft_erp = (stepsize*spring)/(stepsize*spring +damping);
			surface_base.soft_cfm = 1.0/(stepsize*spring +damping);
		}
		//end of optional features

		//set all contacts to these settings:
		for (int i=0; i<count; ++i)
			contact[i].surface = surface_base;


		//
		//simulation of wheel or normal?
		//
		//determine if _one_of the geoms is a wheel
		if (geom1->wheel&&!geom2->wheel)
			geom1->wheel->Set_Contacts(b1, b2, geom2, true, contact, count, stepsize);
		else if (!geom1->wheel&&geom2->wheel)
			geom2->wheel->Set_Contacts(b2, b1, geom1, false, contact, count, stepsize);

		//just a reminder to myself
		if (geom1->wheel&&geom2->wheel)
			printlog(1, "TODO: haven't looked at wheel*wheel collision simulation! (will only be rim_mu*rim_mu and no tyre right now)");

		//
		//
		//

		//finally, we create contactjoints
		for (int i=0; i<count; ++i)
		{

			dJointID c = dJointCreateContact (world,contactgroup,&contact[i]);
			dJointAttach (c,b1,b2);

			if (feedback)
				new Collision_Feedback(c, geom1, geom2);
		}
	}
	
	//with physical contact or not, might respond to collision events
	if (geom1->spring) //geom1 would have generated collision
		geom2->colliding = true; //thus geom2 is colliding

	if (geom2->spring) //geom2 would have generated collision
		geom1->colliding = true; //thus geom2 is colliding
}

//
//set events:
//

//buffer
void Geom::Set_Buffer_Event(dReal thres, dReal buff, Script *scr)
{
	if (thres > 0 && buff > 0 && scr)
	{
		printlog(2, "setting Geom event");

		threshold=thres;
		buffer=buff;
		buffer_script=scr;

		//make sure no old event is left
		Buffer_Event_List::Remove(this);

		buffer_event=true;
	}
	else
	{
		printlog(2, "disabling Geom event");
		buffer_event=false;
		Buffer_Event_List::Remove(this);
	}
}

void Geom::Set_Buffer_Body(Body *b)
{
	force_to_body = b;
}

void Geom::Damage_Buffer(dReal force, dReal step)
{
	if (force_to_body)
	{
		force_to_body->Damage_Buffer(force, step);
		return;
	}

	//if not configured or force not exceeds threshold, stop
	if (!buffer_event || (force<threshold))
		return;

	//buffer still got health
	if (buffer > 0)
	{
		buffer -= force*step;

		//now it's negative, issue event
		if (buffer < 0)
		{
			printlog(2, "Geom buffer depleted, generating event");
			new Buffer_Event_List(this);
		}
	}
	else //just damage buffer even more
		buffer -= force*step;
}

void Geom::Increase_Buffer(dReal buff)
{
	buffer+=buff;

	if (buffer < 0) //still depleted, regenerate event
		new Buffer_Event_List(this);
}

//sensor
void Geom::Set_Sensor_Event(Script *s1, Script *s2)
{
	if (s1 || s2) //enable
	{
		sensor_triggered_script=s1;
		sensor_untriggered_script=s2;
		sensor_last_state=false;
		sensor_event=true;
	}
	else //disable
		sensor_event=false;
}

//physics step
void Geom::Physics_Step()
{
	Geom *geom;
	for (geom=head; geom; geom=geom->next)
	{
		if (geom->sensor_event)
		{
			//triggered/untriggered
			if (geom->colliding != geom->sensor_last_state)
			{
				geom->sensor_last_state=geom->colliding;

				new Sensor_Event_List(geom);
			}

			//always reset collision status for these geoms - allows update for each step
			geom->colliding=false;
		}

		//if (geom->radar_event)... - TODO
	}
}

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

#ifndef _RCX_GEOM_H
#define _RCX_GEOM_H
#include "component.hpp"
#include "body.hpp"
#include "object.hpp"
#include "trimesh.hpp"
#include "script.hpp"
#include "../simulation/wheel.hpp"
#include <SDL/SDL_stdinc.h> //definition for Uint32

//Geom: (meta)data for geometrical shape (for collision detection), for: 
//contactpoint generation (friction and saftness/hardness). also contains
//rendering data for geom
//
//(contains boolean variable indicating collision - for triggering event script)
//
//>Dynamically allocated
class Geom: public Component
{
	public:
		//methods for creating/destroying/processing Geoms
		Geom (dGeomID geom, Object *obj);
		~Geom();

		//methods for steps/simulations:
		static void Clear_Collisions();
		static void Physics_Step();

		static void Collision_Callback(void *, dGeomID, dGeomID);

		//end of methods, variables:
		//geom data bellongs to
		dGeomID geom_id;

		//Physics data:
		//placeholder for more physics data
		dReal spring, damping; //to make surfaces properly soft
		dReal mu, bounce;
		//note: spring+dmaping should give bouncyness, so no need to use both
		//scaling values for tyres:
		dReal tyre_pos_scale, tyre_sharp_scale, tyre_rollres_scale;

		//register if geom is colliding
		bool colliding; //set after each collision


		//special kind of geoms:
		//wheel: points at a wheel simulation class, or NULL if not a wheel
		Wheel *wheel;

		//trimesh: how many triangles (0 if not trimesh/disabled) and which colliding:
		int triangle_count;
		bool *triangle_colliding;
		//TODO: material-specific-surface-parameters
		//end of special geoms


		//End of physics data
		
		Trimesh_3D *model; //points at model

		//debug variables
		dGeomID flipper_geom;

		bool TMP_pillar_geom;
		Trimesh_3D *TMP_pillar_graphics; //TMP

		//for buffer events
		void Set_Buffer_Event(dReal thresh, dReal buff, Script *scr);
		void Increase_Buffer(dReal add);
		void Set_Buffer_Body(Body*); //send damage to body instead
		void Damage_Buffer(dReal force, dReal step); //"damage" geom with specified force

		//sensor events
		void Set_Sensor_Event(Script *s1, Script *s2);

	private:
		//events:
		bool buffer_event;
		bool sensor_event;
		//bool radar_event; - TODO

		//sensor events:
		bool sensor_last_state; //last state of sensor: enabled or disabled
		Script *sensor_triggered_script, *sensor_untriggered_script;

		//buffer events:
		Body *force_to_body; //send forces to this body instead

		//normal buffer handling
		dReal threshold;
		dReal buffer;
		Script *buffer_script; //script to execute when colliding (NULL if not used)


		//used to find next/prev geom in list of all geoms
		//set next to null in last link in chain (prev = NULL in first)
		static Geom *head; // = NULL;
		Geom *prev;
		Geom *next;

		friend void Graphic_List_Update(); //to allow loop through geoms
		friend void Event_Buffers_Process(dReal); //to allow looping
		friend void Body::Physics_Step (dReal step); //dito
		friend void Geom_Render(); //same as above, for debug collision render
};

#endif

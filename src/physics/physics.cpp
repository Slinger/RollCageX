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

//handles physics simulation (mostly rigid body dynamics)

#include <SDL/SDL_timer.h>
#include <SDL/SDL_mutex.h>

#include "../shared/threads.hpp"
#include "../shared/internal.hpp"
#include "../shared/runlevel.hpp"
#include "../shared/track.hpp"
#include "../shared/printlog.hpp"
#include "../shared/body.hpp"
#include "../shared/geom.hpp"
#include "../shared/camera.hpp"
#include "../shared/car.hpp"
#include "../shared/joint.hpp"

#include "collision_feedback.hpp"

#include "../graphics/graphic_list.hpp"

unsigned int physics_lag = 0;
unsigned int physics_count = 0;
Uint32 physics_time = 0;

bool physics_init(void)
{
	printlog(0, "Initiating physics");
	dInitODE2(0);
	dAllocateODEDataForThread(dAllocateFlagBasicData | dAllocateFlagCollisionData);

	world = dWorldCreate();

	//set global ode parameters (except those specific to track)

	space = dHashSpaceCreate(0);
	dHashSpaceSetLevels(space, internal.hash_levels[0], internal.hash_levels[1]);

	contactgroup = dJointGroupCreate(0);

	dWorldSetQuickStepNumIterations (world, internal.iterations);

	//autodisable
	dWorldSetAutoDisableFlag (world, 1);
	dWorldSetAutoDisableLinearThreshold (world, internal.dis_linear);
	dWorldSetAutoDisableAngularThreshold (world, internal.dis_angular);
	dWorldSetAutoDisableSteps (world, internal.dis_steps);
	dWorldSetAutoDisableTime (world, internal.dis_time);

	//joint softness (collisions are specified in physics/geom.cpp)
	dWorldSetERP (world, internal.joint_erp);
	dWorldSetCFM (world, internal.joint_cfm);

	return true;
}


int physics_loop (void *d)
{
	printlog(1, "Starting physics loop");

	physics_time = SDL_GetTicks(); //set simulated time to realtime
	Uint32 realtime; //real time (with possible delay since last update)
	Uint32 stepsize_ms = (Uint32) (internal.stepsize*1000.0+0.0001);
	dReal divided_stepsize = internal.stepsize/internal.multiplier;

	//keep running until done
	while (runlevel != done)
	{
		//only if in active mode do we simulate
		if (runlevel == running)
		{
			//technically, collision detection doesn't need this, but this is easier
			SDL_mutexP(ode_mutex);

			for (int i=0; i<internal.multiplier; ++i)
			{
				Car::Physics_Step(divided_stepsize); //control, antigrav...
				Body::Physics_Step(divided_stepsize); //drag (air/liquid "friction")

				Geom::Clear_Collisions(); //set all collision flags to false

				dSpaceCollide (space, (void*)(&divided_stepsize), &Geom::Collision_Callback);

				dWorldQuickStep (world, divided_stepsize);
				dJointGroupEmpty (contactgroup);

				Joint::Physics_Step(divided_stepsize); //joint forces
				Collision_Feedback::Physics_Step(divided_stepsize); //forces from collisions
			}

			Geom::Physics_Step(); //sensor/radar handling
			camera.Physics_Step(internal.stepsize); //move camera to wanted postion

			//done with ode
			SDL_mutexV(ode_mutex);
			
			Graphic_List_Update(); //make copy of position/rotation for rendering
		}

		//broadcast to wake up sleeping threads
		if (internal.sync_events || internal.sync_graphics)
		{
			SDL_mutexP(sync_mutex);
			SDL_CondBroadcast (sync_cond);
			SDL_mutexV(sync_mutex);
		}

		physics_time += stepsize_ms;

		//sync physics with realtime
		if (internal.sync_physics)
		{
			//get some time to while away?
			realtime = SDL_GetTicks();
			if (physics_time > realtime)
			{
				//busy-waiting:
				if (internal.physics_spinning)
					while (physics_time > SDL_GetTicks());
				//sleep:
				else
					SDL_Delay (physics_time - realtime);
			}
			else
				++physics_lag;
		}

		//count how many steps
		++physics_count;
	}
	return 0;
}

void physics_quit (void)
{
	printlog(1, "Quit physics");
	dJointGroupDestroy (contactgroup);
	dSpaceDestroy (space);
	dWorldDestroy (world);
	dCloseODE();
}


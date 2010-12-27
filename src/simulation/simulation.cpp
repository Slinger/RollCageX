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
#include "event_buffers.hpp"
#include "timers.hpp"

#include "../interface/render_lists.hpp"


unsigned int simulation_lag = 0;
unsigned int simulation_count = 0;
Uint32 simulation_time = 0;

bool Simulation_Init(void)
{
	printlog(0, "Initiating simulation");
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

	//joint softness
	dWorldSetERP (world, internal.erp);
	dWorldSetCFM (world, internal.cfm);

	return true;
}


int Simulation_Loop (void *d)
{
	printlog(1, "Starting simulation loop");

	simulation_time = SDL_GetTicks(); //set simulated time to realtime
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
				Body::Physics_Step(divided_stepsize); //drag (air/liquid "friction") and respawning

				Geom::Clear_Collisions(); //set all collision flags to false

				dSpaceCollide (space, (void*)(&divided_stepsize), &Geom::Collision_Callback);

				Geom::Physics_Step(); //sensor/radar handling

				dWorldQuickStep (world, divided_stepsize);
				dJointGroupEmpty (contactgroup);

				Joint::Physics_Step(divided_stepsize); //joint forces
				Collision_Feedback::Physics_Step(divided_stepsize); //forces from collisions
			}

			camera.Physics_Step(internal.stepsize); //move camera to wanted postion

			//done with ode
			SDL_mutexV(ode_mutex);
			
			Render_List_Update(); //make copy of position/rotation for rendering
		}

		//previous simulations might have caused events (to be processed by scripts)...
		Event_Buffers_Process(internal.stepsize);

		//process timers:
		Animation_Timer::Events_Step(internal.stepsize);


		//broadcast to wake up sleeping threads
		if (internal.sync_interface)
		{
			SDL_mutexP(sync_mutex);
			SDL_CondBroadcast (sync_cond);
			SDL_mutexV(sync_mutex);
		}

		simulation_time += stepsize_ms;

		//sync simulation with realtime
		if (internal.sync_simulation)
		{
			//get some time to while away?
			realtime = SDL_GetTicks();
			if (simulation_time > realtime)
			{
				//busy-waiting:
				if (internal.spinning)
					while (simulation_time > SDL_GetTicks());
				//sleep:
				else
					SDL_Delay (simulation_time - realtime);
			}
			else
				++simulation_lag;
		}

		//count how many steps
		++simulation_count;
	}
	return 0;
}

void Simulation_Quit (void)
{
	printlog(1, "Quit simulation");
	dJointGroupDestroy (contactgroup);
	dSpaceDestroy (space);
	dWorldDestroy (world);
	dCloseODE();
}


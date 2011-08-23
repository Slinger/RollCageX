/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of ReCaged.
 *
 * ReCaged is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReCaged is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
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

#include "../interface/render_list.hpp"


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

//tmp hack!
#include "../interface/hud.hpp"

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

			pbcount=0;
			for (int i=0; i<internal.multiplier; ++i)
			{
				Car::Physics_Step(divided_stepsize); //control, antigrav...
				Body::Physics_Step(divided_stepsize); //drag (air/liquid "friction") and respawning
				camera.Physics_Step(divided_stepsize); //calculate velocity and move

				Geom::Clear_Collisions(); //set all collision flags to false

				//perform collision detection
				dSpaceCollide (space, (void*)(&divided_stepsize), &Geom::Collision_Callback);
				Wheel::Generate_Contacts(divided_stepsize); //add tyre contact points

				Geom::Physics_Step(); //sensor/radar handling

				dWorldQuickStep (world, divided_stepsize);
				dJointGroupEmpty (contactgroup);

				Joint::Physics_Step(divided_stepsize); //joint forces
				Collision_Feedback::Physics_Step(divided_stepsize); //forces from collisions
			}
			pcount=pbcount;
			memcpy(point, pointbuild, 100*sizeof(POINT));

			//previous simulations might have caused events (to be processed by scripts)...
			Event_Buffers_Process(internal.stepsize);

			//process timers:
			Animation_Timer::Events_Step(internal.stepsize);

			//done with ode
			SDL_mutexV(ode_mutex);

			//opdate for interface:
			Render_List_Update(); //make copy of position/rotation for rendering
			camera.Generate_Matrix(); //matrix based on new position/rotation
			Render_List_Finish(); //move new list to render
		}
		else
			camera.Generate_Matrix(); //still update camera position (if manually moving)

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

	//during simulation, memory might be allocated, remove
	Wheel::Clear_List();

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


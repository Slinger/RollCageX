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

#include "../shared/profile.hpp"

void Profile_Events_Step(Uint32 step)
{
	Profile *prof;
	Uint8 *keys;
	for (prof=profile_head; prof; prof=prof->next)
	{
		//get keys pressed
		keys = SDL_GetKeyState(NULL);


		//set camera settings
		if (keys[prof->cam1])
			camera.Set_Settings (&prof->cam[0]);
		else if (keys[prof->cam2])
			camera.Set_Settings (&prof->cam[1]);
		else if (keys[prof->cam3])
			camera.Set_Settings (&prof->cam[2]);
		else if (keys[prof->cam4])
			camera.Set_Settings (&prof->cam[3]);


		//if selected car, read input
		if (prof->car)
		{
			Car *carp = prof->car;
			if (keys[prof->drift_break])
				carp->drift_breaks = true;
			else
				carp->drift_breaks = false;

			dReal t_speed = prof->throttle_speed*step;
			if (keys[prof->up])
				carp->throttle += t_speed*carp->dir;
			else if (keys[prof->down])
				carp->throttle -= t_speed*carp->dir;
			else
			{
				if (fabs(carp->throttle) <= t_speed)
					carp->throttle = 0.0;

				else if (carp->throttle > 0.0)
				    carp->throttle -= t_speed;

				else
				    carp->throttle += t_speed;
			}

			//check if too much
			if (carp->throttle > 1.0)
				carp->throttle = 1.0;
			else if (carp->throttle < -1.0)
				carp->throttle = -1.0;



			t_speed = prof->steer_speed*step;
			if (keys[prof->left]&&!keys[prof->right])
				carp->steering -= t_speed*carp->dir;
			else if (!keys[prof->left]&&keys[prof->right])
				carp->steering += t_speed*carp->dir;
			else //center
			{
				//can center in this step
				if (fabs(carp->steering) <= t_speed)
					carp->steering = 0.0;

				else if (carp->steering > 0.0)
				    carp->steering -= t_speed;

				else
				    carp->steering += t_speed;
			}

			//same kind of check
			if (carp->steering > 1.0)
				carp->steering = 1.0;
			else if (carp->steering < -1.0)
				carp->steering = -1.0;
		}
	}
}

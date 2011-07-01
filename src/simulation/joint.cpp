/*
 * RollCageX - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
 * forum)
 *
 * This file is part of RollCageX.
 *
 * RollCageX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RollCageX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RollCageX.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#include "../shared/joint.hpp"
#include "../shared/internal.hpp"
#include "event_buffers.hpp"

//set event
void Joint::Set_Buffer_Event(dReal thres, dReal buff, Script *scr)
{
	if (thres > 0 && buff > 0 && scr)
	{
		printlog(2, "setting Joint event");
		feedback=new dJointFeedback;
		dJointSetFeedback (joint_id, feedback);

		threshold=thres;
		buffer=buff;
		buffer_script=scr;

		//make sure no old event is left
		Event_Buffer_Remove_All(this);

		buffer_event=true;
	}
	else
	{
		printlog(2, "disabling Joint event");
		buffer_event=false;
		//remove feedback data
		if (feedback)
		{
			delete feedback;
			feedback=NULL;
		}
		Event_Buffer_Remove_All(this);
		//disable
		dJointSetFeedback(joint_id, 0);
	}
}



//check for joint triggering
void Joint::Physics_Step (dReal step)
{
	Joint *d = Joint::head;
	dReal delt1, delt2, delt;

	while (d)
	{
		if (d->buffer_event)
		{
			//TODO: check torque also?
			delt1 = dLENGTH(d->feedback->f1);
			delt2 = dLENGTH(d->feedback->f2);

			if (delt1>delt2)
				delt = delt1;
			else
				delt = delt2;

			if (delt > d->threshold)
			{
				if (d->buffer < 0) //already depleted, just damage more
					d->buffer -= delt*step;
				else
				{
					d->buffer -= delt*step;
					if (d->buffer < 0)
						Event_Buffer_Add_Depleted(d);
				}
			}
		}

		d = d->next;
	}
}

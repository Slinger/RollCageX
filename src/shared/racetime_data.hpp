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

#ifndef _RCX_RACETIME_DATA_H
#define _RCX_RACETIME_DATA_H

#include <typeinfo>
#include <string.h>
#include "printlog.hpp"

class Racetime_Data
{
	public:
		static void Destroy_All();

		//find data that matches specified name and type
		//NOTE: actual function template declared in header, since each object that uses it must
		//instantiate needed function (this follows the "Borland model", which is supported by g++)
		template<typename T>
		static T *Find(const char *name)
		{
			Racetime_Data *tmp;
			T *casted;

			for (tmp=head; tmp; tmp=tmp->next) //loop
			{
				//type conversion+casting ok
				if ((!strcmp(tmp->name, name)) && (casted=dynamic_cast<T*>(tmp)))
				{
					printlog(1, "racetime data already existed for \"%s\"", name);
					return casted;
				}
			}

			return NULL; //else
		}

	protected:
		Racetime_Data(const char *name);
		//just make sure the subclass destructor gets called
		virtual ~Racetime_Data();

	private:
		char *name; //name of specific data

		static Racetime_Data *head;
		Racetime_Data *next;
};
#endif

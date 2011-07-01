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

#include "racetime_data.hpp"
#include "printlog.hpp"
#include <stdio.h>
#include <string.h>

Racetime_Data *Racetime_Data::head = NULL;

Racetime_Data::Racetime_Data(const char *n)
{
	name = new char[strlen(n)+1];
	strcpy (name, n);

	next = head;
	head = this;
}

Racetime_Data::~Racetime_Data()
{
	printlog(2, "removing racetime data called \"%s\"", name);
	delete[] name;
}

void Racetime_Data::Destroy_All()
{
	printlog(2, "destroying all racetime data");

	Racetime_Data *tmp, *data = head;
	while (data)
	{
		tmp = data;
		data=data->next;

		delete tmp;
	}

	head = NULL;
}


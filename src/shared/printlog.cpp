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

#include <stdarg.h>
#include "internal.hpp"
#include "printlog.hpp"

//verbosity indicators
const char *indicator[] = {"=> ", " > ", " * "};

//print log message - if it's below or equal to the current verbosity level
void printlog (int level, const char *text, ...)
{
	if (level <= internal.verbosity)
	{
		if (level==0)
			putchar('\n');

		//print verbosity indicator
		fputs(indicator[level], stdout); //puts adds newline, fputs instead

		//print message
		va_list list;
		va_start (list, text);
		vprintf (text, list);
		va_end (list);

		//put newline
		putchar('\n');
	}
}


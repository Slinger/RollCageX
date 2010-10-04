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

#include "conf.hpp"

#include "text_file.hpp"
#include "../shared/printlog.hpp"

#include <string.h>
#include <stdlib.h>

#include <string>

#include <ode/ode.h>

//loads configuration file to memory (using index)
bool load_conf (const char *name, char *memory, const struct Conf_Index index[])
{
	printlog(1, "Loading conf file: %s", name);

	Text_File file;
	if (!file.Open(name))
		return false;


	int i;
	int argnr;
	char *strleft; //text left in word if not completely converted

	//tmp loading variables:
	int tmpi;
	float tmpf;
	double tmpd;
	dReal tmpr;
	while (file.Read_Line())
	{
		//find matching index (loop until end of list or found matching
		for (i=0; ((index[i].type !=0) && (strcmp(index[i].name,file.words[0]) != 0) ); ++i);

		if (index[i].type==0) //not match, got to end
		{
			printlog(0, "WARNING: parameter \"%s\" does not match any parameter name!", file.words[0]);
			continue;
		}
		//else, we have a match

		//see if ammount of args is correct
		//argument name+values == words
		if (index[i].length+1 != file.word_count)
		{
			printlog(0, "WARNING: parameter \"%s\" has wrong ammount of args: expected: %i, got %i!",file.words[0], index[i].length, file.word_count);
			continue;
		}

		//loop through arguments
		for (argnr=0;argnr<index[i].length;++argnr)
		{
			//what type
			switch (index[i].type)
			{
				//float
				case 'f':
					tmpf = strtof(file.words[argnr+1], &strleft);

					if (strleft == file.words[argnr+1])
						printlog(0, "WARNING: \"%s\" is invalid for \"floating point\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else //ok
						*( ((float*)(memory+index[i].offset))+argnr ) = tmpf;
				break;

				//double
				case 'd':
					tmpd = strtod(file.words[argnr+1], &strleft);

					if (strleft == file.words[argnr+1])
						printlog(0, "WARNING: \"%s\" is invalid for \"double precision floating point\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else //ok
						*( ((double*)(memory+index[i].offset))+argnr ) = tmpd;
				break;

				//dReal
				case 'R':
					//there are two alternatives here (depending on how ode is configured): float or double
					//always read values as double, and then cast them to whatever "dReal" might be
					tmpr = strtod(file.words[argnr+1], &strleft);

					if (strleft == file.words[argnr+1])
						printlog(0, "WARNING: \"%s\" is invalid for \"dReal floating point\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else
						*( ((dReal*)(memory+index[i].offset))+argnr ) = tmpr;
				break;

				//bool
				case 'b':
					//"true" if: "true", "on", "yes", "1"
					if (	(!strcasecmp(file.words[argnr+1], "true")) ||
						(!strcasecmp(file.words[argnr+1], "on")) ||
						(!strcasecmp(file.words[argnr+1], "yes")) ||
						(!strcmp(file.words[argnr+1], "1"))	)
						*(((bool*)(memory+index[i].offset))+argnr) = true;
					//false if: "false", "off", "no", "0"
					else if((!strcasecmp(file.words[argnr+1], "false")) ||
						(!strcasecmp(file.words[argnr+1], "off")) ||
						(!strcasecmp(file.words[argnr+1], "no")) ||
						(!strcmp(file.words[argnr+1], "0"))	)
						*(((bool*)(memory+index[i].offset))+argnr) = false;
					else //failure (unknown word)
						printlog(0, "WARNING: \"%s\" is invalid for \"boolean\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
				break;

				//integer
				case 'i':
					tmpi = strtol (file.words[argnr+1], &strleft, 0);

					if (strleft == file.words[argnr+1])
						printlog(0, "WARNING: \"%s\" is invalid for \"integer\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else
						*( ((int*)(memory+index[i].offset))+argnr ) = tmpi;
				break;

				//string
				case 's':
					//check length
					if (strlen(file.words[argnr+1]) >= Conf_String_Size) //equal or bigger than max size
						printlog(0, "WARNING: word in conf file was too big for direct string copy!");
					else //ok, just copy
						strcpy(*( ((Conf_String*)(memory+index[i].offset))+argnr ),    file.words[argnr+1] );
				break;

				//unknown
				default:
					printlog(0, "WARNING: parameter \"%s\" is of unknown type (\"%c\")!", file.words[0], index[i].type);
				break;
			}
		}

	}

	return true;
}

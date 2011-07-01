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

#ifndef _RCX_INFO_H
#define _RCX_INFO_H
/*
   RollCageX is free software: you can redistribute it and/or modify\n\
   it under the terms of the GNU General Public License as published by\n\
   the Free Software Foundation, either version 3 of the License, or\n\
   (at your option) any later version.\n\n\
   RollCageX comes with ABSOLUTELY NO WARRANTY:\n\n\
   RollCageX is distributed in the hope that it will be useful,\n\
   but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
   GNU General Public License for more details.\n\n\
   You should have received a copy of the GNU General Public License\n\
   along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\*/

//general info
const char VERSION[] = "0.07 (98\% done)";

const char TITLE[] = "RollCageX 0.07 (98\% done) (C) 2009, 2010, 2011 Mats Wahlberg (\"Slinger\")"; 

const char ISSUE[] = "\
   RollCageX  Copyright (C) 2009, 2010, 2011 Mats Wahlberg (\"Slinger\" on gorcx.net forum)\n\
\n\
   RollCageX is free software: you can redistribute it and/or modify\n\
   it under the terms of the GNU General Public License as published by\n\
   the Free Software Foundation, either version 3 of the License, or\n\
   (at your option) any later version.\n\
\n\
   RollCageX is distributed in the hope that it will be useful,\n\
   but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
   GNU General Public License for more details.\n\
\n\
   You should have received a copy of the GNU General Public License\n\
   along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\
\n\
				-=[ Credits ]=-\n\n\
    * Mats Wahlberg (\"Slinger\")		-	Creator (coder) + development 3D models\n\
    * \"XEWEASEL\" (\"Codie Morgan\")	-	Extra 3D models\n\
    * \"K.Mac\"				-	Extensive testing, hacks and new ideas\n\
    * \"MeAkaJon\"			-	Creator/maintainer of gorcx.net webpage\n\
    * \"Spontificus\"			-	Testing, hacks and various fixes\n\n\
\n		-=[ Other Projects that made RCX possible ]=\n\n\
    * \"Free Software Foundation\"	-	\"Free Software, Free Society\", supporting the free software movement\n\
    * \"The GNU Project\"			-	For a Free OS. Its fight for computer freedom has changed the world\n\
    * \"Simple DirectMedia Layer\"	-	OS/hardware abstraction library\n\
    * \"Open Dynamics Engine\"		-	Rigid body dynamics and collision detection library\n\n\
   Default key bindings can be found (and changed) in \"data/profiles/default/keys.lst\"\n\
   More keys exists for debug/testing/demo, see README if you are interested.\n\
   - See README for more info -\n\n";
 
const char HELP[] = "\
   Usage: rcx [-d|-p]\n\
     -d <path to data>       override path to directory with data\n\
     -p <path to profile>    override path to directory with user data and settings\n";

#endif

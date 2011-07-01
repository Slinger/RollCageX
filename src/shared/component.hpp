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

#ifndef _RCX_COMPONENT_H
#define _RCX_COMPONENT_H
//base class for creating component classes (geoms, joints, bodies, spaces)
//TODO: make polymorph, this claaa should not be used directly...

//prototype for object class
class Object;

class Component
{
	public:
		virtual ~Component(); //useful outside

	private:
		Component *prev, *next;

	protected: //private for this and subclasses
		Component(Object *obj);
		//keep track of the "owning" object
		Object * object_parent;

};
#endif

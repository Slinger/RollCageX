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

#include "../shared/object.hpp"

#include <ode/ode.h>

#include "../shared/racetime_data.hpp"
#include "../shared/trimesh.hpp"
#include "../shared/printlog.hpp"
#include "../shared/track.hpp"
#include "../shared/joint.hpp"
#include "../shared/geom.hpp"
#include "../shared/body.hpp"

//load data for spawning object (object data), hard-coded debug version
Object_Template *Object_Template::Load(const char *path)
{
	printlog(1, "Loading object: %s", path);

	//see if already loaded
	if (Object_Template *tmp=Racetime_Data::Find<Object_Template>(path))
	{
		return tmp;
	}

	//tmp pointers
	Object_Template *obj;
	
	//currently no scripting, only hard-coded solutions
	if (!strcmp(path,"objects/misc/box"))
	{
		//"load" 3d box
		printlog(2, "(hard-coded box)");

		obj = new Object_Template(path);
		obj->box = true;

		//the debug box will only spawn one component - one "3D file"
		if (!(obj->model[0] = Trimesh_3D::Quick_Load("objects/misc/box/box.obj")))
			return NULL;

	//end of test
	}
	else if (!strcmp(path, "objects/misc/funbox"))
	{
		printlog(2, "(Mac's hard-coded funbox");

		obj = new Object_Template(path);
		obj->funbox = true; //id

		//graphics
		if (!(obj->model[0] = Trimesh_3D::Quick_Load("objects/misc/funbox/box.obj")))
			return NULL;

	}
	else if (!strcmp(path, "objects/misc/flipper"))
	{
		printlog(2, "(hard-coded flipper)");

		obj = new Object_Template(path);
		obj->flipper = true; //id

		//graphics
		if (!(obj->model[0] = Trimesh_3D::Quick_Load("objects/misc/flipper/Flipper.obj")))
			return NULL;

	}
	else if (!strcmp(path, "objects/misc/NH4"))
	{
		printlog(2, "(hard-coded \"molecule\")");

		obj = new Object_Template(path);
		obj->NH4 = true;

		//graphics
		if (	!(obj->model[0] = Trimesh_3D::Quick_Load("objects/misc/NH4/Atom1.obj")) ||
			!(obj->model[1] = Trimesh_3D::Quick_Load("objects/misc/NH4/Atom2.obj"))	)
			return NULL;

	}
	else if (!strcmp(path, "objects/misc/beachball"))
	{
		printlog(2, "(hard-coded beachball)");

		obj = new Object_Template(path);
		obj->sphere = true;
		if (!(obj->model[0] = Trimesh_3D::Quick_Load("objects/misc/beachball/sphere.obj")))
			return NULL;
	}
	else if (!strcmp(path, "objects/misc/building"))
	{
		printlog(2, "(hard-coded building)");

		obj = new Object_Template(path);
		obj->building = true;

		//graphics
		if (	!(obj->model[0] = Trimesh_3D::Quick_Load("objects/misc/building/pillar.obj")) ||
			!(obj->model[1] = Trimesh_3D::Quick_Load("objects/misc/building/roof.obj")) ||
			!(obj->model[2] = Trimesh_3D::Quick_Load("objects/misc/building/wall.obj"))	)
			return NULL;

	}
	else if (!strcmp(path,"objects/misc/pillar"))
	{
		//"load" 3d box
		printlog(2, "(hard-coded pillar)");

		obj = new Object_Template(path);
		obj->pillar = true;

		//graphics
		if (	!(obj->model[0] = Trimesh_3D::Quick_Load("objects/misc/pillar/Pillar.obj")) ||
			!(obj->model[1] = Trimesh_3D::Quick_Load("objects/misc/pillar/Broken.obj"))	)
			return NULL;

	}
	else
	{
		printlog(0, "ERROR: path didn't match any hard-coded object");
		return NULL;
	}

	//if we got here, loading ok
	return obj;
}

//bind two bodies together using fixed joint (simplify connection of many bodies)
void debug_joint_fixed(dBodyID body1, dBodyID body2, Object *obj)
{
	dJointID joint;
	joint = dJointCreateFixed (world, 0);
	Joint *jd = new Joint(joint, obj);
	dJointAttach (joint, body1, body2);
	dJointSetFixed (joint);

	//use feedback
	//set threshold, buffer and dummy script
	jd->Set_Buffer_Event(35000, 15000, (Script*)1337);
}

//spawn a "loaded" (actually hard-coded) object
//TODO: rotation
void Object_Template::Spawn (dReal x, dReal y, dReal z)
{
	printlog(2, "Spawning object at: %f %f %f", x,y,z);
	//prettend to be executing the script... just load debug values
	//
	Object *obj;
	Body *bd;

	if (box)
	{
	printlog(2, "(hard-coded box)");
	//
	//
	//

	obj = new Object();

	dGeomID geom  = dCreateBox (0, 1,1,1); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(100000, 10000, (Script*)1337);
	dBodyID body = dBodyCreate (world);

	dMass m;
	dMassSetBox (&m,1,1,1,1); //sides
	dMassAdjust (&m,400); //mass
	dBodySetMass (body, &m);

	bd = new Body(body, obj); //just for drag
	//bd->Set_Event (100, 10, (script_struct*)1337);

	dGeomSetBody (geom, body);

	dBodySetPosition (body, x, y, z);

	//Next, Graphics
	data->model = model[0];

	//done
	}
	//
	//
	else if (funbox)
	{
	printlog(2, "(Mac's hard-coded funbox)");
	
	obj = new Object();
	new Space(obj);

	//one body to which all geoms are added
	dBodyID body1 = dBodyCreate (world);

	//mass
	dMass m;
	dMassSetBox (&m,1,2.5,2.5,2.5); //sides
	dMassAdjust (&m,100); //mass
	//TODO: create dMass for all geoms, and use dMassTranslate+dMassAdd to add to m
	dBodySetMass (body1, &m);

	//create Body metadata
	Body *bd = new Body (body1, obj);

	//set buffer for body (when destroyed, so are all other geoms)
	bd->Set_Buffer_Event(150000, 10000, (Script*)1337);


	//
	//create geoms
	//

	//geom at (0,0,0)
	dGeomID geom  = dCreateBox (0, 2,2,2); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;

	dGeomSetBody (geom, body1);
	dBodySetPosition (body1, x, y, z);

	data->model = model[0];
	data->Set_Buffer_Body(bd); //send collision forces to body
	data->surface.bounce = 2.0; //about twice the collision force is used to bounce up

	//the outer boxes (different offsets)
	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0.70,0,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,0.70,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,0,0.70); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, -0.70,0,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,-0.70,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,0,-0.70); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);
	
	}
	//
	//
	else if (flipper)
	{
	printlog(2, "(hard-coded flipper)");
	//
	//
	//

	//flipper surface
	obj = new Object();
	new Space(obj);
	
	dGeomID geom  = dCreateBox (0, 8,8,0.5); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetPosition (geom, x, y, z);

	//Graphics
	data->model = model[0];


	//flipper sensor
	dGeomID geom2 = dCreateBox (0, 3,3,2);
	data = new Geom(geom2, obj);
	data->surface.mu = 1.0;
	data->surface.spring = 0.0; //0 spring constanct = no collision forces
	dGeomSetPosition (geom2, x, y, z+0.76);

	data->flipper_geom = geom; //tmp debug solution
	//enable script execution on sensor triggering (but not when untriggered)
	data->Set_Sensor_Event ( ((Script*)1337) , NULL); //(triggered,untriggered)

	//
	}
	//
	else if (NH4)
	{
	printlog(2, "(hard-coded \"molecule\")");
	//
	//
	//

	Object *obj = new Object();
	new Space(obj);

	//center sphere
	dGeomID geom  = dCreateSphere (0, 1); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(100000, 10000, (Script*)1337);

	dBodyID body1 = dBodyCreate (world);

	dMass m;
	dMassSetSphere (&m,1,1); //radius
	dMassAdjust (&m,60); //mass
	dBodySetMass (body1, &m);

	new Body (body1, obj);

	dGeomSetBody (geom, body1);

	dBodySetPosition (body1, x, y, z);

	//Next, Graphics
	data->model = model[0];

	dReal pos[4][3] = {
		{0, 0, 1.052},

		{0, 1, -0.326},
		{0.946,  -0.326, -0.326},
		{-0.946, -0.326, -0.326}};

	dJointID joint;
	dBodyID body;

	int i;
	for (i=0; i<4; ++i) {
	//connected spheres
	geom  = dCreateSphere (0, 0.8); //geom
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(100000, 10000, (Script*)1337);
	body = dBodyCreate (world);

	dMassSetSphere (&m,1,0.5); //radius
	dMassAdjust (&m,30); //mass
	dBodySetMass (body, &m);

	new Body (body, obj);

	dGeomSetBody (geom, body);

	dBodySetPosition (body, x+pos[i][0], y+pos[i][1], z+pos[i][2]);

	//Next, Graphics
	data->model = model[1];

	//connect to main sphere
	
	joint = dJointCreateBall (world, 0);

	Joint *jd = new Joint(joint, obj);
	jd->Set_Buffer_Event(1000, 50000, (Script*)1337);

	dJointAttach (joint, body1, body);
	dJointSetBallAnchor (joint, x+pos[i][0], y+pos[i][1], z+pos[i][2]);
	}
	//done
	//
	//
	}
	else if (sphere)
	{
	printlog(2, "(beachball)");
	//
	//
	//

	Object *obj = new Object();

	//center sphere
	dGeomID geom  = dCreateSphere (0, 1); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(1000, 1500, (Script*)1337);
	dBodyID body1 = dBodyCreate (world);

	dMass m;
	dMassSetSphere (&m,1,1); //radius
	dMassAdjust (&m,20); //mass
	dBodySetMass (body1, &m);

	Body *b = new Body (body1, obj);

	dGeomSetBody (geom, body1);

	dBodySetPosition (body1, x, y, z);

	//set low drag
	b->Set_Linear_Drag(1);
	b->Set_Angular_Drag(1);

	//set spring surface for beachball (should be bouncy)
	data->surface.spring = 5000.0; //springy (soft)
	data->surface.damping = 0.0; //no damping

	//Next, Graphics
	data->model=model[0];
	}
	//
	else if (building)
	{
	printlog(2, "(hard-coded building)");
	//
	//

	Object *obj = new Object();
	new Space(obj);
	dBodyID old_body[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
	dBodyID old_pillar[4] = {0,0,0,0};

	dBodyID body[4];

	int j;
	for (j=0; j<3; ++j)
	{
		int i;
		dBodyID body1[12], body2[9];
		for (i=0; i<12; ++i)
		{
			dGeomID geom  = dCreateBox (0, 4,0.4,2.4); //geom
			Geom *data = new Geom(geom, obj);
			data->surface.mu = 1.0;
			data->Set_Buffer_Event(100000, 100000, (Script*)1337);

			body1[i] = dBodyCreate (world);
			dGeomSetBody (geom, body1[i]);

			dMass m;
			dMassSetBox (&m,1,4,0.4,2.7); //sides
			dMassAdjust (&m,400); //mass
			dBodySetMass (body1[i], &m);

			new Body (body1[i], obj);

			data->model = model[2];
		}
		
		const dReal k = 1.5*4+0.4/2;

		dBodySetPosition (body1[0], x-4, y-k, z+(2.4/2));
		dBodySetPosition (body1[1], x,   y-k, z+(2.4/2));
		dBodySetPosition (body1[2], x+4, y-k, z+(2.4/2));

		dBodySetPosition (body1[6], x+4, y+k, z+(2.4/2));
		dBodySetPosition (body1[7], x,   y+k, z+(2.4/2));
		dBodySetPosition (body1[8], x-4, y+k, z+(2.4/2));

		dMatrix3 rot;
		dRFromAxisAndAngle (rot, 0,0,1, M_PI/2);
		for (i=3; i<6; ++i)
			dBodySetRotation (body1[i], rot);
		for (i=9; i<12; ++i)
			dBodySetRotation (body1[i], rot);

		dBodySetPosition (body1[3], x+k,  y-4, z+(2.4/2));
		dBodySetPosition (body1[4], x+k, y, z+(2.4/2));
		dBodySetPosition (body1[5], x+k, y+4, z+(2.4/2));

		dBodySetPosition (body1[9], x-k, y+4, z+(2.4/2));
		dBodySetPosition (body1[10], x-k, y, z+(2.4/2));
		dBodySetPosition (body1[11], x-k, y-4, z+(2.4/2));

		//connect wall blocks in height
		for (i=0; i<12; ++i)
		{
			debug_joint_fixed(body1[i], old_body[i], obj);
			//move these bodies to list of old bodies
			old_body[i] = body1[i];
		}

		//connect wall blocks in sideway
		for (i=0; i<11; ++i)
			debug_joint_fixed (body1[i], body1[i+1], obj);
		debug_joint_fixed (body1[0], body1[11], obj);

		//walls done, floor/ceiling
		for (i=0; i<9; ++i)
		{
			dGeomID geom  = dCreateBox (0, 4,4,0.2); //geom
			Geom *data = new Geom(geom, obj);
			data->surface.mu = 1.0;
			data->Set_Buffer_Event(100000, 100000, (Script*)1337);

			body2[i] = dBodyCreate (world);
			dGeomSetBody (geom, body2[i]);

			dMass m;
			dMassSetBox (&m,1,4,4,0.2); //sides
			dMassAdjust (&m,400); //mass
			dBodySetMass (body2[i], &m);

			new Body (body2[i], obj);

			data->model = model[1];
		}

		const dReal k2=2.4-0.2/2;

		dBodySetPosition (body2[0], x-4, y-4, z+k2);
		debug_joint_fixed(body2[0], body1[0], obj);
		debug_joint_fixed(body2[0], body1[11], obj);
		dBodySetPosition (body2[1], x,   y-4, z+k2);
		debug_joint_fixed(body2[1], body1[1], obj);
		dBodySetPosition (body2[2], x+4, y-4, z+k2);
		debug_joint_fixed(body2[2], body1[2], obj);
		debug_joint_fixed(body2[2], body1[3], obj);

		dBodySetPosition (body2[3], x-4, y, z+k2);
		debug_joint_fixed(body2[3], body1[10], obj);
		dBodySetPosition (body2[4], x,   y, z+k2);
		dBodySetPosition (body2[5], x+4, y, z+k2);
		debug_joint_fixed(body2[5], body1[4], obj);

		dBodySetPosition (body2[6], x-4, y+4, z+k2);
		debug_joint_fixed(body2[6], body1[9], obj);
		debug_joint_fixed(body2[6], body1[8], obj);
		dBodySetPosition (body2[7], x,   y+4, z+k2);
		debug_joint_fixed(body2[7], body1[7], obj);
		dBodySetPosition (body2[8], x+4, y+4, z+k2);
		debug_joint_fixed(body2[8], body1[6], obj);
		debug_joint_fixed(body2[8], body1[5], obj);

		//join floor blocks
		//1: horisontal
		debug_joint_fixed (body2[0], body2[1], obj);
		debug_joint_fixed (body2[1], body2[2], obj);
		debug_joint_fixed (body2[3], body2[4], obj);
		debug_joint_fixed (body2[4], body2[5], obj);
		debug_joint_fixed (body2[6], body2[7], obj);
		debug_joint_fixed (body2[7], body2[8], obj);
		//2: vertical
		debug_joint_fixed (body2[0], body2[3], obj);
		debug_joint_fixed (body2[3], body2[6], obj);
		debug_joint_fixed (body2[1], body2[4], obj);
		debug_joint_fixed (body2[4], body2[7], obj);
		debug_joint_fixed (body2[2], body2[5], obj);
		debug_joint_fixed (body2[5], body2[8], obj);
	
		//pillars
		dGeomID geom;
		Geom *data;
		for (i=0; i<4; ++i)
		{
			geom  = dCreateCapsule (0, 0.3,1.4); //geom
			data = new Geom(geom, obj);
			data->surface.mu = 1.0;
			data->Set_Buffer_Event(100000, 100000, (Script*)1337);
			body[i] = dBodyCreate (world);
	
			dMass m;
			dMassSetCapsule (&m,1,3,1,0.5); //sides (3=z-axis)
			dMassAdjust (&m,400); //mass
			dBodySetMass (body[i], &m);
	
			new Body (body[i], obj);

			dGeomSetBody (geom, body[i]);
	
			//Next, Graphics
			data->model = model[0];
		}

		dBodySetPosition (body[0], x+2, y+2, z+2.4/2);
		debug_joint_fixed(body[0], body2[8], obj);
		debug_joint_fixed(body[0], body2[7], obj);
		debug_joint_fixed(body[0], body2[5], obj);
		debug_joint_fixed(body[0], body2[4], obj);

		dBodySetPosition (body[1], x+2, y-2, z+2.4/2);
		debug_joint_fixed(body[0], body2[1], obj);
		debug_joint_fixed(body[0], body2[2], obj);
		debug_joint_fixed(body[0], body2[4], obj);
		debug_joint_fixed(body[0], body2[5], obj);

		dBodySetPosition (body[2], x-2, y+2, z+2.4/2);
		debug_joint_fixed(body[0], body2[7], obj);
		debug_joint_fixed(body[0], body2[6], obj);
		debug_joint_fixed(body[0], body2[4], obj);
		debug_joint_fixed(body[0], body2[3], obj);

		dBodySetPosition (body[3], x-2, y-2, z+2.4/2);
		debug_joint_fixed(body[0], body2[0], obj);
		debug_joint_fixed(body[0], body2[1], obj);
		debug_joint_fixed(body[0], body2[3], obj);
		debug_joint_fixed(body[0], body2[4], obj);

		for (i=0; i<4; ++i)
		{
			debug_joint_fixed(body[i], old_pillar[i], obj);
			old_pillar[i] = body[i];
		}

		z+=2.4;
	}
	//
	//
	}
	//
	//
	else if (pillar)
	{
		printlog(2, "(hard-coded pillar)");

		//just one geom in this object
		Geom *g = new Geom(dCreateBox(0, 2,2,5), new Object());
		g->surface.mu = 1.0;

		//position
		dGeomSetPosition(g->geom_id, x,y,(z+2.5));

		//render
		g->model = model[0];

		//identification
		g->TMP_pillar_geom = true;

		//destruction
		g->Set_Buffer_Event(200000, 100000, (Script*)1337);
		g->TMP_pillar_graphics = model[1];
	}
	else
		printlog(0, "ERROR: trying to spawn unidentified object?!");

}


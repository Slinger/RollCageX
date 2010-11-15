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

#include "../shared/internal.hpp"
#include "../shared/racetime_data.hpp"
#include "../shared/car.hpp"
#include "../shared/camera.hpp"
#include "../shared/printlog.hpp"
#include "../shared/track.hpp"
#include "../shared/geom.hpp"
#include "../shared/body.hpp"
#include "../shared/joint.hpp"
#include "text_file.hpp"


Car_Template *Car_Template::Load (const char *path)
{
	printlog(1, "Loading car: %s", path);

	//see if already loaded
	if (Car_Template *tmp=Racetime_Data::Find<Car_Template>(path))
		return tmp;

	//apparently not
	Car_Template *target = new Car_Template(path);

	//car.conf
	char conf[strlen(path)+9+1];//+1 for \0
	strcpy (conf,path);
	strcat (conf,"/car.conf");

	if (!load_conf(conf, (char *)&target->conf, car_conf_index)) //try to load conf
		return NULL;

	//geoms.lst
	char lst[strlen(path)+9+1];
	strcpy (lst, path);
	strcat (lst, "/geoms.lst");

	Text_File file;
	if (file.Open(lst))
	{
		//default surface parameters
		Surface surface;

		while (file.Read_Line())
		{
			if (file.words[0][0] == '>')
			{
				int pos = 1;
				//as long as there are two words left (option name and value)
				while ( (file.word_count-pos) >= 2)
				{
					if (!strcmp(file.words[pos], "mu"))
						surface.mu = strtod(file.words[++pos], (char**)NULL);
					else if (!strcmp(file.words[pos], "bounce"))
						surface.bounce = atof(file.words[++pos]);
					else if (!strcmp(file.words[pos], "spring"))
						surface.spring = strtod(file.words[++pos], (char**)NULL);
					else if (!strcmp(file.words[pos], "damping"))
						surface.damping = atof(file.words[++pos]);
					else if (!strcmp(file.words[pos], "position"))
						surface.tyre_pos_scale = atof(file.words[++pos]);
					else if (!strcmp(file.words[pos], "sharpness"))
						surface.tyre_sharp_scale = atof(file.words[++pos]);
					else if (!strcmp(file.words[pos], "rollingres"))
						surface.tyre_rollres_scale = atof(file.words[++pos]);
					else
					{
						printlog(0, "WARNING: surface option \"%s\" unknown", file.words[pos]);
					}

					//one step forward
					pos+=1;
				}
			}
			else if (!strcmp(file.words[0], "box"))
			{
				struct box tmp_box;
				if (file.word_count == 9) //not wanting rotation?
				{
					//size
					tmp_box.size[0] = atof(file.words[2]);
					tmp_box.size[1] = atof(file.words[3]);
					tmp_box.size[2] = atof(file.words[4]);

					//position
					tmp_box.pos[0] = atof(file.words[6]);
					tmp_box.pos[1] = atof(file.words[7]);
					tmp_box.pos[2] = atof(file.words[8]);

					//rotation (not)
					tmp_box.rot[0]=0.0;
					tmp_box.rot[1]=0.0;
					tmp_box.rot[2]=0.0;

					//surface properties
					tmp_box.surface = surface;
				}
				else if (file.word_count == 13) //also rotate?
				{
					//size
					tmp_box.size[0] = atof(file.words[2]);
					tmp_box.size[1] = atof(file.words[3]);
					tmp_box.size[2] = atof(file.words[4]);

					//position
					tmp_box.pos[0] = atof(file.words[6]);
					tmp_box.pos[1] = atof(file.words[7]);
					tmp_box.pos[2] = atof(file.words[8]);

					//rotation (not)
					tmp_box.rot[0] = atof(file.words[10]);
					tmp_box.rot[1] = atof(file.words[11]);
					tmp_box.rot[2] = atof(file.words[12]);

					//surface properties
					tmp_box.surface = surface;
				}
				else
				{
					printlog(0, "ERROR: box geom in car geom list expects exactly: size, position and (optional) rotation!");
					continue; //don't add
				}

				//store box
				target->boxes.push_back(tmp_box);
			}
			else if (!strcmp(file.words[0], "sphere"))
			{
				if (file.word_count != 7)
				{
					printlog(0, "ERROR: sphere geom in car geom list expects exactly: size (radius) and position!");
					continue; //skip
				}

				struct sphere tmp_sphere;
				tmp_sphere.radius = atof(file.words[2]);
				tmp_sphere.pos[0] = atof(file.words[4]);
				tmp_sphere.pos[1] = atof(file.words[5]);
				tmp_sphere.pos[2] = atof(file.words[6]);

				tmp_sphere.surface = surface;

				//store
				target->spheres.push_back(tmp_sphere);
			}
			else if (!strcmp(file.words[0], "capsule"))
			{
				struct capsule tmp_capsule;
				if (file.word_count == 8) //not wanting rotation?
				{
					//size
					tmp_capsule.size[0] = atof(file.words[2]);
					tmp_capsule.size[1] = atof(file.words[3]);
					//pos
					tmp_capsule.pos[0] = atof(file.words[5]);
					tmp_capsule.pos[1] = atof(file.words[6]);
					tmp_capsule.pos[2] = atof(file.words[7]);
					//rot
					tmp_capsule.rot[0] = 0.0;
					tmp_capsule.rot[1] = 0.0;
					tmp_capsule.rot[2] = 0.0;
					//surface
					tmp_capsule.surface = surface;
				}
				else if (file.word_count == 12) //also rotate?
				{
					//size
					tmp_capsule.size[0] = atof(file.words[2]);
					tmp_capsule.size[1] = atof(file.words[3]);
					//pos
					tmp_capsule.pos[0] = atof(file.words[5]);
					tmp_capsule.pos[1] = atof(file.words[6]);
					tmp_capsule.pos[2] = atof(file.words[7]);
					//rot
					tmp_capsule.rot[0] = atof(file.words[9]);
					tmp_capsule.rot[1] = atof(file.words[10]);
					tmp_capsule.rot[2] = atof(file.words[11]);
					//surface
					tmp_capsule.surface = surface;
				}
				else
				{
					printlog(0, "ERROR: capsule geom in car geom list expects exactly: size (radius and length), position and (optional) rotation!");
					continue; //don't add
				}

				//store
				target->capsules.push_back(tmp_capsule);
			}
			else
				printlog(0, "ERROR: geom \"%s\" in car geom list not recognized!", file.words[0]);
		}
	}
	else
		printlog(0, "WARNING: can not open list of car geoms (%s)!", lst);

	//helper datas:

	//wheel simulation class (friction + some custom stuff):
	target->wheel.rim_angle = target->conf.rim_angle;
	target->wheel.spring = target->conf.wheel_spring;
	target->wheel.damping = target->conf.wheel_damping;
	//cylinder moment of inertia tensor for Z = (mass*r²)/2
	target->wheel.inertia = target->conf.wheel_mass*target->conf.w[0]*target->conf.w[0]/2.0; 
	target->wheel.resistance = target->conf.rollres;
	target->wheel.radius = target->conf.w[0];

	//check and copy data from conf to wheel class
	//x
	target->wheel.xpeak = target->conf.xpeak[0];
	target->wheel.xpeaksch = target->conf.xpeak[1];

	if (target->conf.xshape > 1.0)
		target->wheel.xshape = target->conf.xshape;
	else
		printlog(0, "WARNING: xshape value should be bigger than 1!");

	if (target->conf.xpos[0] > 0.0)
		target->wheel.xpos = target->conf.xpos[0];
	else
		printlog(0, "WARNING: first xpos value should be bigger than 0!");

	target->wheel.xposch= target->conf.xpos[1];

	if (target->conf.xsharp[0] > 0.0)
		target->wheel.xsharp = target->conf.xsharp[0];
	else
		printlog(0, "WARNING: first xsharp value should be bigger than 0!");

	target->wheel.xsharpch = target->conf.xsharp[1];

	//y
	target->wheel.ypeak = target->conf.ypeak[0];
	target->wheel.ypeaksch = target->conf.ypeak[1];

	if (target->conf.yshape > 1.0)
		target->wheel.yshape = target->conf.yshape;
	else
		printlog(0, "WARNING: yshape value should be bigger than 1!");

	if (target->conf.ypos[0] > 0.0)
		target->wheel.ypos = target->conf.ypos[0];
	else
		printlog(0, "WARNING: first ypos value should be bigger than 0!");

	target->wheel.yposch= target->conf.ypos[1];

	if (target->conf.ysharp[0] > 0.0)
		target->wheel.ysharp = target->conf.ysharp[0];
	else
		printlog(0, "WARNING: first ysharp value should be bigger than 0!");

	target->wheel.ysharpch = target->conf.ysharp[1];
	target->wheel.yshift = target->conf.yshift;
	//
	//



	//make sure the values are correct
	//steering distribution
	if (target->conf.dist_steer >1.0 || target->conf.dist_steer <0.0 )
	{
		printlog(0, "ERROR: front/rear steering distribution should be range 0 to 1! (enabling front)");
		target->conf.dist_steer = 1.0;
	}

	//check if neither front or rear drive
	if ( (!target->conf.dist_motor[0]) && (!target->conf.dist_motor[1]) )
	{
		printlog(0, "ERROR: either front and rear motor distribution must be enabled! (enabling 4WD)");
		target->conf.dist_motor[0] = true;
		target->conf.dist_motor[1] = true;
	}

	//breaking distribution
	if (target->conf.dist_break>1.0 || target->conf.dist_break<0.0 )
	{
		printlog(0, "ERROR: front/rear breaking distribution should be range 0 to 1! (enabling rear)");
		target->conf.dist_break = 0.0;
	}


	//load model if specified
	if (target->conf.model[0] == '\0') //empty string
	{
		printlog(1, "WARNING: no car 3D model specified\n");
		target->model=NULL;
	}
	else
	{
		char file[strlen(path)+1+strlen(target->conf.model)+1];
		strcpy(file, path);
		strcat(file, "/");
		strcat(file, target->conf.model);

		if ( !(target->model = Trimesh_3D::Quick_Load(file,
				target->conf.resize,
				target->conf.rotate[0], target->conf.rotate[1], target->conf.rotate[2],
				target->conf.offset[0], target->conf.offset[1], target->conf.offset[2])) )
			return NULL;
	}

	return target;
}


Car *Car_Template::Spawn (dReal x, dReal y, dReal z,  Trimesh_3D *tyre, Trimesh_3D *rim)
{
	printlog(1, "spawning car at: %f %f %f", x,y,z);

	printlog(1, "NOTE: wheels will not collide to other wheels - OPCODE lacks cylinder*cylinder collision");

	printlog(1, "TODO: antigravity and downforce - could be combined");


	//begin copying of needed configuration data
	Car *car = new Car();

	car->wheel = &wheel;

	car->power = conf.power;

	//if electric motor enabled:
	if (conf.electric_torque)
		car->gear_limit = conf.power/conf.electric_torque;
	else //else, direct gear_limit:
		car->gear_limit = conf.gear_limit;


	car->max_break = conf.max_break;
	car->max_steer = conf.max_steer;
	car->steerdecr = conf.steer_decrease;
	car->min_steer = conf.min_steer;
	car->limit_speed = conf.limit_speed;
	car->airtorque = conf.air_torque;

	car->dsteer = conf.dist_steer;
	car->dbreak = conf.dist_break;
	car->fwd = conf.dist_motor[0];
	car->rwd = conf.dist_motor[1];
	car->fwredist = conf.redist[0];
	car->rwredist = conf.redist[1];

	car->diff = conf.diff;
	car->adapt_steer = conf.adapt_steer;
	car->adapt_redist = conf.adapt_redist;
	car->redist_force = conf.redist_force;

	//start building
	new Space(car);

	dMass m;
	car->bodyid = dBodyCreate (world);
	dBodySetAutoDisableFlag (car->bodyid, 0); //never disable main body
	

	//set mass
	dMassSetBox (&m,1,conf.body[0], conf.body[1], conf.body[2]); //sides
	dMassAdjust (&m,conf.body_mass); //mass
	dBodySetMass (car->bodyid, &m);

	//set up air (and liquid) drag for body
	Body *bdata = new Body (car->bodyid, car);
	bdata->Set_Advanced_Linear_Drag (conf.body_linear_drag[0], conf.body_linear_drag[1], conf.body_linear_drag[2]);
	//rotational drag
	bdata->Set_Angular_Drag (conf.body_angular_drag);


	dBodySetPosition (car->bodyid, x, y, z);


	//ok, set rendering model:
	bdata->model = model;

	
	//done, collision geoms:
	dGeomID geom;
	Geom *gdata;

	int i;

	//add geoms, first: boxes
	struct box b;
	dMatrix3 rot;
	for (i=0;i< (int)boxes.size();++i)
	{
		b = boxes[i];
	
		geom = dCreateBox(0,b.size[0],b.size[1],b.size[2]);
		gdata = new Geom (geom, car);

		dGeomSetBody (geom, car->bodyid);

		if (b.pos[0]||b.pos[1]||b.pos[2]) //need offset
			dGeomSetOffsetPosition(geom,b.pos[0],b.pos[1],b.pos[2]);

		if (b.rot[0]||b.rot[1]||b.rot[2]) //need rotation
		{
			dRFromEulerAngles(rot, b.rot[0]*M_PI/180.0, b.rot[1]*M_PI/180.0, b.rot[2]*M_PI/180.0);
			dGeomSetOffsetRotation(geom, rot);
		}

		gdata->surface = b.surface;
	}
	//then: spheres
	struct sphere sphere;
	for (i=0; i<(int)spheres.size(); ++i)
	{
		sphere = spheres[i];

		geom = dCreateSphere(0,sphere.radius);
		gdata = new Geom(geom, car);

		dGeomSetBody (geom, car->bodyid);

		if (sphere.pos[0]||sphere.pos[1]||sphere.pos[2]) //need offset
			dGeomSetOffsetPosition(geom,sphere.pos[0],sphere.pos[1],sphere.pos[2]);


		gdata->surface = b.surface;
	}
	//finally: capsule
	struct capsule capsule;
	for (i=0; i<(int)capsules.size(); ++i)
	{
		capsule = capsules[i];
	
		geom = dCreateCapsule(0,capsule.size[0],capsule.size[1]);
		gdata = new Geom (geom, car);

		dGeomSetBody (geom, car->bodyid);

		if (capsule.pos[0]||capsule.pos[1]||capsule.pos[2]) //need offset
			dGeomSetOffsetPosition(geom,capsule.pos[0],capsule.pos[1],capsule.pos[2]);

		if (capsule.rot[0]||capsule.rot[1]||capsule.rot[2]) //need rotation
		{
			dRFromEulerAngles(rot, capsule.rot[0]*M_PI/180.0, capsule.rot[1]*M_PI/180.0, capsule.rot[2]*M_PI/180.0);
			dGeomSetOffsetRotation(geom, rot);
		}

		gdata->surface = b.surface;
	}

	//side detection sensors:
	dReal *s = conf.s;

	geom = dCreateBox(0,s[0],s[1],s[2]);
	car->sensor1 = new Geom (geom, car);
	car->sensor1->surface.spring = 0.0; //untouchable "ghost" geom - sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,-s[3]);

	geom = dCreateBox(0,s[0],s[1],s[2]);
	car->sensor2 = new Geom (geom, car);
	car->sensor2->surface.spring = 0.0; //sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,s[3]);

	//wheels:
	Geom *wheel_data[4];
	dGeomID wheel_geom;
	dBodyID wheel_body[4];

	//3=z axis of cylinder
	dMassSetCylinder (&m, 1, 3, conf.w[0], conf.w[1]);
	dMassAdjust (&m, conf.wheel_mass);

	for (i=0;i<4;++i)
	{
		//create cylinder
		//(geom)
		wheel_geom = dCreateCylinder (0, conf.w[0], conf.w[1]);

		//(body)
		wheel_body[i] = dBodyCreate (world);

		//never disable wheel body
		dBodySetAutoDisableFlag (wheel_body[i], 0);

		//set mass
		dBodySetMass (wheel_body[i], &m);

		//and connect to body
		dGeomSetBody (wheel_geom, wheel_body[i]);

		//allocate (geom) data
		wheel_data[i] = new Geom(wheel_geom, car);

		//data:
		//note: we set the usual geom parameters as if the wheel was just a
		//normal rigid geom, with the rim mu as the mu. this is good if indeed
		//the rim part of the wheel is colliding. but if it's the tyre part
		//a lot of things will change (erp,cfm,mu,mode). this extra data is
		//generated by the Wheel class, conveniently pointed to from the geom.
		//
		//friction (use rim mu by default until knowing it's tyre)
		wheel_data[i]->surface.mu = conf.rim_mu;
		//points at our wheel simulation class (indicates wheel)
		wheel_data[i]->wheel = &wheel;

		//drag
		bdata = new Body (wheel_body[i], car);
		bdata->Set_Linear_Drag (conf.wheel_linear_drag);
		//rotational drag
		bdata->Set_Angular_Drag (conf.wheel_angular_drag);

		//(we need easy access to wheel body ids
		car->wheel_body[i] = wheel_body[i];
		car->wheel_geom_data[i] = wheel_data[i];

		//graphics.
		//note: it's now possible to render two models for each wheel:
		//one for the geom and one for the body
		//this is great in this case where the wheels got two models (rim+tyre)
		bdata->model = rim;
		wheel_data[i]->model = tyre;
	}

	//place and rotate wheels
	dRFromAxisAndAngle (rot, 0, 1, 0, M_PI/2);
	dBodySetPosition (wheel_body[0], x+conf.wp[0], y+conf.wp[1], z);
	dBodySetRotation (wheel_body[0], rot);
	dBodySetPosition (wheel_body[1], x+conf.wp[0], y-conf.wp[1], z);
	dBodySetRotation (wheel_body[1], rot);

	dRFromAxisAndAngle (rot, 0, 1, 0, -M_PI/2);
	dBodySetPosition (wheel_body[2], x-conf.wp[0], y-conf.wp[1], z);
	dBodySetRotation (wheel_body[2], rot);
	dBodySetPosition (wheel_body[3], x-conf.wp[0], y+conf.wp[1], z);
	dBodySetRotation (wheel_body[3], rot);

	//tmp: might need these later on
	car->wx = conf.wp[0];
	car->wy = conf.wp[1];

	//create joints (hinge2) for wheels
	dReal stepsize = internal.stepsize/internal.multiplier;
	dReal sERP = stepsize*conf.suspension_spring/(stepsize*conf.suspension_spring+conf.suspension_damping);
	dReal sCFM = 1.0/(stepsize*conf.suspension_spring+conf.suspension_damping);
	for (i=0; i<4; ++i)
	{
		car->joint[i]=dJointCreateHinge2 (world, 0);
		new Joint(car->joint[i], car);
		//body is still body of car main body
		dJointAttach (car->joint[i], car->bodyid, wheel_body[i]);
		dJointSetHinge2Axis1 (car->joint[i],0,0,1);
		dJointSetHinge2Axis2 (car->joint[i],1,0,0);

		//setup suspension
		dJointSetHinge2Param (car->joint[i],dParamSuspensionERP,sERP);
		dJointSetHinge2Param (car->joint[i],dParamSuspensionCFM,sCFM);

		//lock steering axis on all wheels
		dJointSetHinge2Param (car->joint[i],dParamLoStop,0);
		dJointSetHinge2Param (car->joint[i],dParamHiStop,0);
	}

	//to make it possible to tweak the hinge2 anchor x position:
	
	dJointSetHinge2Anchor (car->joint[0],x+conf.jx,y+conf.wp[1],z);
	dJointSetHinge2Anchor (car->joint[1],x+conf.jx,y-conf.wp[1],z);
	dJointSetHinge2Anchor (car->joint[2],x-conf.jx,y-conf.wp[1],z);
	dJointSetHinge2Anchor (car->joint[3],x-conf.jx,y+conf.wp[1],z);

	//return
	return car;
}

void Car::Respawn (dReal x, dReal y, dReal z)
{
	printlog(1, "respawning car at: %f %f %f", x,y,z);

	//remember old car position
	const dReal *pos = dBodyGetPosition(bodyid);
	float oldx = pos[0];
	float oldy = pos[1];
	float oldz = pos[2];

	//will use this to reset rotation
	dMatrix3 r;

	//body:
	dRSetIdentity(r); //no rotation
	dBodySetPosition(bodyid, x, y, z);
	dBodySetRotation(bodyid, r);

	//wheels:
	//right side rotation
	dRFromAxisAndAngle (r, 0, 1, 0, M_PI/2);
	dBodySetPosition (wheel_body[0], x+wx, y+wy, z);
	dBodySetRotation (wheel_body[0], r);
	dBodySetPosition (wheel_body[1], x+wx, y-wy, z);
	dBodySetRotation (wheel_body[1], r);

	//left side
	dRFromAxisAndAngle (r, 0, 1, 0, -M_PI/2);
	dBodySetPosition (wheel_body[2], x-wx, y-wy, z);
	dBodySetRotation (wheel_body[2], r);
	dBodySetPosition (wheel_body[3], x-wx, y+wy, z);
	dBodySetRotation (wheel_body[3], r);

	//remove velocities
	dBodySetLinearVel(bodyid, 0.0, 0.0, 0.0);
	dBodySetAngularVel(bodyid, 0.0, 0.0, 0.0);

	dBodySetLinearVel(wheel_body[0], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[0], 0.0, 0.0, 0.0);
	dBodySetLinearVel(wheel_body[1], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[1], 0.0, 0.0, 0.0);
	dBodySetLinearVel(wheel_body[2], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[2], 0.0, 0.0, 0.0);
	dBodySetLinearVel(wheel_body[3], 0.0, 0.0, 0.0);
	dBodySetAngularVel(wheel_body[3], 0.0, 0.0, 0.0);

	//set camera position (move it as much as we moved the car)
	//TODO: in future (with multiple cameras), loop through them all and change those that looks at this car
	//for (Camera camera = Camera::head; camera; camera=camera->next)
	//{
		if (camera.car == this)
		{
			camera.pos[0] += (x-oldx);
			camera.pos[1] += (y-oldy);
			camera.pos[2] += (z-oldz);
		}
	//}
}

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
		while (file.Read_Line())
		{
			if (!strcmp(file.words[0], "box"))
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
				}
				else
				{
					printlog(0, "ERROR: box geom in car geom list expects exactly: size, position and (optional) rotation!");
					continue; //don't add
				}

				//store box
				target->boxes.push_back(tmp_box);

				//create graphics for box:
				//file_3d *f3d = new file_3d();
				//target->box_graphics.push_back(f3d);
				//debug_draw_box(f3d->list, tmp_box.size[0],tmp_box.size[1],tmp_box.size[2], lgreen, gray, 70);
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

				//store
				target->spheres.push_back(tmp_sphere);

				//graphics
				//file_3d *f3d = new file_3d();
				//target->sphere_graphics.push_back(f3d);
				//debug_draw_sphere(f3d->list, tmp_sphere.radius*2, lgreen, gray, 70);
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
				}
				else
				{
					printlog(0, "ERROR: capsule geom in car geom list expects exactly: size (radius and length), position and (optional) rotation!");
					continue; //don't add
				}

				//store
				target->capsules.push_back(tmp_capsule);

				//graphics
				//file_3d *f3d = new file_3d();
				//target->capsule_graphics.push_back(f3d);
				//debug_draw_capsule(f3d->list, tmp_capsule.size[0], tmp_capsule.size[1], lgreen, gray, 70);
			}
			else
				printlog(0, "ERROR: geom \"%s\" in car geom list not recognized!", file.words[0]);
		}
	}
	else
		printlog(0, "WARNING: can not open list of car geoms (%s)!", lst);

	//helper datas:

	//* set up values for front/rear driving ratios
	if (target->conf.steer_ratio>100 || target->conf.steer_ratio<0 )
	{
		printlog(0, "ERROR: front/rear steering ratio should be set between 0 and 100!");
		target->conf.steer_ratio=0;
	}

	target->fsteer = (dReal) (target->conf.steer_ratio/100.0);
	target->rsteer = (dReal) (target->fsteer-1.0);
	
	if (target->conf.motor_ratio>100 || target->conf.motor_ratio<0 )
	{
		printlog(0, "ERROR: front/rear motor ratio should be set between 0 and 100!");
		target->conf.motor_ratio=0;
	}

	target->fmotor = (dReal) (target->conf.motor_ratio/100.0);
	target->rmotor = (dReal) (1.0-target->fmotor);

	if (target->conf.break_ratio>100 || target->conf.break_ratio<0 )
	{
		printlog(0, "ERROR: front/rear breaking ratio should be set between 0 and 100!");
		target->conf.break_ratio=0;
	}

	target->fbreak = (dReal) (target->conf.break_ratio/100.0);
	target->rbreak = (dReal) (1.0-target->fbreak);



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

	printlog(1, "Warning: wheels will not collide to other wheels... (wheels use cylinders)");
	printlog(1, "(note to self: only solution would be moving to capped cylinders... :-/ )");


	printlog(1, "TODO: antigravity forces");


	//begin copying of needed configuration data
	Car *car = new Car();
	car->max_torque = conf.max_torque;
	car->gear_tweak = conf.gear_tweak;
	car->max_break = conf.max_break;
	car->torque_compensator = conf.torque_compensator;
	car->fsteer = fsteer;
	car->rsteer = rsteer;
	car->fmotor = fmotor;
	car->rmotor = rmotor;
	car->fbreak = fbreak;
	car->rbreak = rbreak;

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
		//friction
		gdata->mu = conf.body_mu;
		gdata->slip = conf.body_slip;
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

		//friction
		gdata->mu = conf.body_mu;
		gdata->slip = conf.body_slip;
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
		//friction
		gdata->mu = conf.body_mu;
		gdata->slip = conf.body_slip;
	}

	//side detection sensors:
	dReal *s = conf.s;

	geom = dCreateBox(0,s[0],s[1],s[2]);
	car->sensor1 = new Geom (geom, car);
	car->sensor1->spring = 0.0; //untouchable "ghost" geom - sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,-s[3]);

	geom = dCreateBox(0,s[0],s[1],s[2]);
	car->sensor2 = new Geom (geom, car);
	car->sensor2->spring = 0.0; //sensor
	dGeomSetBody (geom, car->bodyid);
	dGeomSetOffsetPosition(geom,0,0,s[3]);

	//wheels:
	Geom *wheel_data[4];
	dGeomID wheel_geom;
	dBodyID wheel_body[4];
	for (i=0;i<4;++i)
	{
		//create cylinder
		//(geom)
		wheel_geom = dCreateCylinder (0, conf.w[0], conf.w[1]);

		//(body)
		wheel_body[i] = dBodyCreate (world);
		//never disable wheel body
		dBodySetAutoDisableFlag (wheel_body[i], 0);

		//3=z axis of cylinder
		dMassSetCylinder (&m, 1, 3, conf.w[0], conf.w[1]);
		dMassAdjust (&m, conf.wheel_mass);
		dBodySetMass (wheel_body[i], &m);

		dGeomSetBody (wheel_geom, wheel_body[i]);

		//allocate (geom) data
		wheel_data[i] = new Geom(wheel_geom, car);

		//data:
		//friction
		wheel_data[i]->mu = conf.wheel_mu;
		wheel_data[i]->mu_rim = conf.rim_mu;
		wheel_data[i]->wheel = true;
		wheel_data[i]->slip = conf.wheel_slip;
		wheel_data[i]->spring = conf.tyre_spring;
		wheel_data[i]->damping = conf.tyre_damping;

		//rim or tyre:
		wheel_data[i]->rim_angle = cos(conf.rim_angle/180.0*M_PI);

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

		//to easily get rotation speed (for slip in sideway), set all geom datas to specify connected hinge2
		wheel_data[i]->hinge2 = car->joint[i];
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

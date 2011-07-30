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


#include "wheel.hpp"
#include "../shared/geom.hpp"
#include "../shared/internal.hpp" 

//This code tries to implement a reasonably simple and realistic tyre friction model.
//(it's mostly inspired by different equations based on Pacejka's "magic formula")
//It also determines if the tyre or rim of the wheel is colliding.
//I'm probably not using all correct variable names, and I might have made some typo
//somewhere. There are also a lot of features that could/should be implemented:
//
//	* force feedback - can be calculated here, but sdl lack force feedback right now
//				(supports gamepads, but not ff specific)
//
//	* the improved wheel friction might make it necessary to improve the suspension
//		(toe, caster, camber, etc...)
//
//	* todo...............
//

//useful vector calculations (from physics/camera.cpp):
//length of vector (=|V|)
#define VLength(V) (sqrt( (V)[0]*(V)[0] + (V)[1]*(V)[1] + (V)[2]*(V)[2] ))

//normalization of vector (A=A/|A|)
#define VNormalize(V){ \
	float l=VLength(V); \
	(V)[0]/=l; (V)[1]/=l; (V)[2]/=l;}

//cross product (A=BxC)
#define VCross(A,B,C){ \
	(A)[0]=(B)[1]*(C)[2]-(B)[2]*(C)[1]; \
	(A)[1]=(B)[2]*(C)[0]-(B)[0]*(C)[2]; \
	(A)[2]=(B)[0]*(C)[1]-(B)[1]*(C)[0];}

//dot product (=AxB)
#define VDot(A,B) ( (A)[0]*(B)[0] + (A)[1]*(B)[1] + (A)[2]*(B)[2] )

//subtraction of one vector from another (A=B-C)
#define VSubtract(A,B,C){ \
	(A)[0]=(B)[0]-(C)[0]; \
	(A)[1]=(B)[1]-(C)[1]; \
	(A)[2]=(B)[2]-(C)[2];}

//just some default, crappy, values (to ensure safe simulations)
Wheel::Wheel()
{
	xpeak = 0.0;
	xpeaksch = 0.0;
	xshape = 2.0;
	xpos = 1.0;
	xposch = 0.0;
	xsharp = 1.0;
	xsharpch = 0.0;

	ypeak = 0.0;
	ypeaksch = 0.0;
	yshape = 2.0;
	ypos = 1.0;
	yposch = 0.0;
	ysharp = 1.0;
	ysharpch = 0.0;
	yshift = 0.0;
}

//simulation of wheel
void Wheel::Set_Contact(dBodyID wbody, dBodyID obody, Surface *surface, bool wheel_first,
		dContact *contact, int count, dReal stepsize)
{
	//
	//use spring+damping for tyre collision forces:
	dReal cspring = 1/( 1/(spring) + 1/(surface->spring) );
	dReal cdamping = damping + surface->damping;

	//calculate erp+cfm from stepsize, spring and damping values:
	dReal erp = (stepsize*cspring)/(stepsize*cspring +cdamping);
	dReal cfm = 1.0/(stepsize*cspring +cdamping);
	//

	//set erp+cfm (spring+damping):
	contact->surface.mode |= dContactSoftERP | dContactSoftCFM; //enable custom ERP/CFM
	contact->surface.soft_erp = erp;
	contact->surface.soft_cfm = cfm;
	//


	//"Slinger's not-so-magic formula":

	//
	//1) input values:
	//


	//directions:
	//(X:longitudinal/wheel travel, Y:lateral/sideway, Z:normal/"up"):
	dReal X[3], Y[3], Z[3];
	dReal inclination;

	//Z: vertical to ground/normal direction
	//(make sure always same direction to/from wheel)
	if (wheel_first)
	{
		Z[0] = contact->geom.normal[0];
		Z[1] = contact->geom.normal[1];
		Z[2] = contact->geom.normal[2];
	}
	else
	{
		Z[0] = -contact->geom.normal[0];
		Z[1] = -contact->geom.normal[1];
		Z[2] = -contact->geom.normal[2];
	}

	//Y: simply wheel axis Z
	//NOTE: this is actually incorrect, since this "Y" might not be tangental to ground.
	const dReal *rot = dBodyGetRotation(wbody);
	Y[0] = rot[2];
	Y[1] = rot[6];
	Y[2] = rot[10];

	//X: along wheel direction (perpendicular to Y and Z)
	//note: will be tangental, so ok.
	VCross (X, Z, Y);
	VNormalize(X);

	//Y is not correct (not tangental to ground), but X is, and Z is also ok.
	//Y can be recalculated from X and Z, but first, this Y is useful:

	//inclination:

	//first get angle between current Y and Z
	dReal tmp = VDot (Z, Y);
	inclination = (180.0/M_PI)*asin(tmp);

	//rim (outside range for tyre)
	//(rim mu calculated as the already defaults, btw)
	//if tmp is just a little bit too big (>1.0 or <-1.0), it'll also be rim
	if (isnan(inclination) || fabs(inclination) > rim_angle)
		return; //don't modify default values, skip this


	//
	//ok, we can now calculate the correct Y!
	VCross(Y, X, Z);
	//note: no need to normalize, since both X and Y are unit.
	//


	//slip ratio and slip angle:
	dReal slip_ratio, slip_angle;

	//(copy the position (damn, c++ doesn't allow vector assignment, right now...)
	dReal pos[3]; //contact point position
	pos[0] = contact->geom.pos[0];
	pos[1] = contact->geom.pos[1];
	pos[2] = contact->geom.pos[2];

	//first, get interesting velocities:
	//(velocity of wheel, point on wheel, relative point on wheel (and of point on ground))
	dVector3 Vwheel, Vpoint, Vrpoint, Vground;

	//velocity of wheel
	const dReal *Vtmp = dBodyGetLinearVel (wbody);
	//copy values to out own variables
	Vwheel[0] = Vtmp[0];
	Vwheel[1] = Vtmp[1];
	Vwheel[2] = Vtmp[2];

	//velocity of point on wheel
	dBodyGetPointVel(wbody, pos[0], pos[1], pos[2], Vpoint);

	//not assuming we're on a static surface, so check:
	if (obody) //the surface got a body (can move)
	{
		//get groun vel
		dBodyGetPointVel(obody, pos[0], pos[1], pos[2], Vground);

		//remove ground velocity from others
		//(this makes velocities relative to ground)
		VSubtract(Vpoint, Vpoint, Vground);
		VSubtract(Vwheel, Vwheel, Vground);
	}

	//the velocity of point relative to velocity of wheel:
	VSubtract(Vrpoint, Vpoint, Vwheel);


	//now, lets start calculate needed velocities from these values:

	dReal Vr = VDot(X, Vrpoint); //velocity from wheel rotation
	dReal Vx = VDot(X, Vwheel); //velocity of wheel along heading
	//(Vy - see Vsy)
	dReal Vz = VDot(Z, Vpoint);

	//normal force: how many kN of force caused by tyre compression.
	//since using a linear spring+damping solution to calculations, this is easy:
	//(note: primitive solution... and ignores features like bouncy collisions)
	dReal Fz = contact->geom.depth*cspring //collision depth * spring value
		- (Vz *cdamping); //velocity along z * damping value

	//contact joints only generates forces pulling the colliding bodies _appart_
	//so negative (traction) forces are not added (complying with reality btw)
	//(in this case it's the damping force getting higher than spring force when
	//the two bodies are moving appart from each others)
	if (Fz < 0.0) //not going to be any friction of this contact
		return; //let ode just use defaults

	//Vsx and Vsy (slip velocity along x and y):
	dReal Vsx = Vx + Vr; //Vr should be opposite sign of Vx, so this is the difference
	//Vsy = Vy = VDot (Y, Vwheel); but lets go overkill!
	dReal Vsy = VDot(Y, Vpoint); //velocity of point of wheel sideways instead


	//slip_ratio: defined as: (wheel velocity/ground velocity)-1
	//this is velocity along the X axis...
	slip_ratio = fabs(Vsx/Vx); //we don't need negative values...
	//NOTE: if v2x get low (wheel standing still) this gets quite high

	//slip_angle: angle (in degrees) between X and actual direction of movement.
	//velocity up/down (velZ) is not part of this, since only the tangental
	//movement along the surface has to do with the actual wheel vs surface
	//friction stuff:
	//use the relative velocity of ground vs wheel along x and y to calculate this:
	slip_angle = (180.0/M_PI)* atan( Vsy/fabs(Vx) );
	//NOTE: if v2x gets really low (wheel standing still) the reliability is lost...


	//
	//2) compute output values:
	//

	//max mu value
	dReal peak = (xpeak+xpeaksch*Fz)*surface->mu;

	//if Fz is too high, no friction...
	if (peak < 0.0)
		return;

	//shape
	dReal shape = xshape;
	//needed to get peak at right position, and used by peak_sharpness
	dReal K = tan( (M_PI/2)/shape );
	//where should peak be reached
	dReal peak_at = xpos*pow(Fz, xposch) *surface->tyre_pos_scale;
	//how sharp peak should be
	dReal peak_sharpness = (peak_at/K)*xsharp*pow(Fz, xsharpch) *surface->tyre_sharp_scale;

	//calculate!
	dReal amount_x = sin(shape*atan(K*pow((fabs(slip_ratio)/peak_at), peak_sharpness)));
	dReal MUx = peak*amount_x;

	//and for MUy
	peak = (ypeak+ypeaksch*Fz)*surface->mu;
	shape = yshape;
	K = tan( (M_PI/2)/shape );
	peak_at = ypos*pow(Fz, yposch) *surface->tyre_pos_scale;
	peak_sharpness = (peak_at/K)*ysharp*pow(Fz, ysharpch) *surface->tyre_sharp_scale;
	dReal shift = yshift*inclination;

	//based on the turning angle (positive or negative), the shift might change
	//(wheel leaning inwards in curve gets better grip)
	if (slip_angle < 0.0)
		shift = -shift;

	dReal amount_y = sin(shape*atan(K*pow((fabs(slip_angle)/peak_at), peak_sharpness))) + shift;
	dReal MUy = peak*amount_y;

	//MUx and MUy might get negative in the calculations, which means no friction so
	//set to 0
	if (MUx < 0.0)
		MUx = 0.0;

	//MUy might also get negative if the shift (due to bad inclination) gets negative
	if (MUy < 0.0)
		MUy = 0.0;

	//
	//3) combined slip (scale Fx and Fy to combine)
	//
	//NOTE: when ode contact joints are automatically computing "force direction" 1 and 2, they
	//_appears_ to be set so that fdir1 is along movement and fdir2 is perpendicular. thus the fdir1
	//force is always proportional to the "absolute velocity" and fdir2 force is always 0. this
	//results in a realistic "circle" approximation of the friction (same force for all direction).
	//
	//however: for tyre, fdir1 (and fdir2) must be set outside ode, so that a different mu can be given
	//for fdir1 (along wheel heading) and fdir2 (sideways). this separation results in a less realistic
	//"box" friction approximation (movement along both direction 1 and 2 results in more friction than
	//equal movement along only one direction).
	//to solve this, we apply "combined slip condition correction" to the Mu for x and y.
	//
	//the normal solution would be a simple circular approximation, but for tyre friction it seems we
	//want to make it elliptic instead - but only when (and based on) movement along both x and y.
	//this way the friction in one direction (and the total friction too) will decrease if the wheel is
	//moving along the other direction too.


	//there are different solutions to this.... TODO: decide on what to use!
	//using simple (but nice!) circular approximation
	dReal diff = VDot(Y, Vpoint)/VDot(X, Vpoint); //difference between velocity of point along X and Y
	//diff *= amount_y/amount_x; //and also difference between how much of peak mu is used in each

	//might become NaN? Probably only with 0 velocity... Anyway, default to 45° distribution
	if (isnan(diff))
		diff=1.0;

	//apply
	MUx /=sqrt(1.0+diff*diff);
	MUy /=sqrt(1.0+1.0/(diff*diff));


	//
	//4) set output values:
	//

	//for reliability don't let ode multiply by its Fz, use calculated instead:
	contact->surface.mode ^= dContactApprox1; //mask away the contactapprox1 option

	//4.0) debug values:
	if (approx1) //simulate by load (normal)
		MUx*=Fz; //multiply by Fz (MU*load=force)
		MUy*=Fz; //-''-

	if (fixedmu) //ignore all above calculation and force a constant mu (and ode slip combination)
		contact->surface.mu = fixedmu;
	else //the sane situation: use all stuff we've calculated
	{
		//4.1) actual, proper, values:
		//enable: separate mu for dir 1&2, specify dir 1
		//(note: dir2 is automatically calculated by ode)
		//also enable erp+cfm specifying (for spring+damping)
		contact->surface.mode |= dContactMu2 | dContactFDir1;

		//fdir1
		contact->fdir1[0] = X[0];
		contact->fdir1[1] = X[1];
		contact->fdir1[2] = X[2];

		//specify mu1 and mu2

		//let ode calculate friction from internal Fz
		contact->surface.mu = MUx;
		contact->surface.mu2 = MUy;
	}

	//4.2) rolling resistance (breaking torque based on normal force)
	//some simplifications:
	//*rolling speed is ignored (doesn't make much difference)
	//*compression of tyre is also ignored (assumed to be small enough)
	dReal torque = Fz*resistance*surface->tyre_rollres_scale; //breaking torque

	//rotation inertia (relative to ground if got body)
	dReal rotation;
	if (obody)
	{
		const dReal *orot = dBodyGetAngularVel(obody);
		const dReal *wrot = dBodyGetAngularVel(wbody);
		rotation = rot[2]*(wrot[0]-orot[0])+
			rot[6]*(wrot[1]-orot[1])+
			rot[10]*(wrot[2]-orot[2]);
	}
	else //just rotation of wheel
	{
		const dReal *wrot = dBodyGetAngularVel(wbody);
		rotation = rot[2]*wrot[0]+
			rot[6]*wrot[1]+
			rot[10]*wrot[2];
	}
	dReal needed = -rotation*inertia/stepsize;

	//same (negative/positive?) direction for torque
	if (needed < 0.0)
		torque = -torque;

	//can break in this step
	if (torque/needed > 1.0) //torque bigger than needed
		torque = needed; //decrease torque to needed

	dBodyAddRelTorque(wbody, 0.0, 0.0, torque);
	//TODO: if the ground has a body, perhaps the torque should affect it too?
	//perhaps add a the breaking force (at the point of the wheel) to the ground body?
}


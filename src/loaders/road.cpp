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

#include "../shared/trimesh.hpp"
#include "../shared/printlog.hpp"
#include "text_file.hpp"
#include <string.h>

//TODO: this code needs much more documentation...

//class for processing bezier curves
class Bezier
{
	public:
		//create from current line in file:
		Bezier(Text_File *file)
		{
			//name
			name = file->words[1];

			//allocate
			int words = file->word_count; //how many words
			n = (words-2)/2; //not the first two, then two words per point
			n -= 1; //and start counting on 0
			p = new point[n+1];

			//store:
			for (int i=0; i<=n; ++i)
			{
				p[i].x = atof(file->words[2+2*i]);
				p[i].y = atof(file->words[3+2*i]);
			}

			//link
			next = head;
			head = this;
		}
		~Bezier()
		{
			//free
			delete[] p;
		}
		static Bezier *Find(const char *name)
		{
			for (Bezier *point=head; point; point=point->next)
				if (point->name == name)
					return point;

			//else
			printlog(0, "WARNING: unable to find section of road named \"%s\"", name);
			return NULL;
		}
		void GetPos(float t, float *pos)
		{
			pos[0]=0;
			pos[1]=0;

			//simple way of calculating binomial coefficients for each i
			int k=1;

			//power-off for each term:
			float t1=1.0; //t^i

			float t2=1.0; //(1-t)^(n-i)
			for (int i=0; i<=n-1; ++i)
				t2*=(1.0-t);

			//if t=1, then prevent NaN...
			float t2mod = t==1.0? 1.0:1.0/(1.0-t);

			//calculate
			for (int i=0; i<=n; ++i)
			{
				//if at last, set to 1
				if (i==n)
					t2=1;

				//binomial coefficient (fixed n=row, for each i)
				pos[0] += float(k)*t1*t2*p[i].x;
				pos[1] += float(k)*t1*t2*p[i].y;

				//coefficient for next time
				k*=(n-i);
				k/=(i+1);

				//t1&t2
				t1*=t;
				t2*=t2mod;
			}
			//done
		}
		void GetDir(float t, float *dir)
		{
			dir[0]=0;
			dir[1]=0;

			//calculate (derivation of above)
			int k=1;
			//power-off for each term:
			float t1=1.0; //t^i
			float t2=1.0; //(1-t)^(n-1-i)
			for (int i=0; i<=n-2; ++i)
				t2*=(1.0-t);

			//if t=1, then prevent NaN...
			float t2mod = t==1.0? 1.0:1.0/(1.0-t);

			//calculate
			for (int i=0; i<=(n-1); ++i)
			{
				//if at last, set to 1
				if (i==n-1)
					t2=1;

				//binomial coefficient (fixed n=row, for each i)
				dir[0] += float(k)*t1*t2*(p[i+1].x-p[i].x);
				dir[1] += float(k)*t1*t2*(p[i+1].y-p[i].y);

				//coefficient for next time
				k*=(n-1-i);
				k/=(i+1);

				//t1&t2
				t1*=t;
				t2*=t2mod;
			}
			dir[0]*=n;
			dir[1]*=n;

			//normalize
			float l = sqrt(dir[0]*dir[0]+dir[1]*dir[1]);

			//specially crafted bezier curves (where p0=p1 and/or p(n-1)=pn)
			//can not be derived correctly for t=0 and t=1. make safetycheck.
			if (l == 0)
			{
				printlog(0, "WARNING: equal road section points exists, derivative not possible!");
				dir[0]=1.0;
				dir[1]=0.0;
			}
			else
			{
				dir[0]/=l;
				dir[1]/=l;
			}
		}
		static void RemoveAll()
		{
			Bezier *next;
			while (head)
			{
				next=head->next;
				delete head;
				head=next;
			}
		}

	private:
		std::string name;

		struct point
		{
			float x,y;
		};
		point *p;
		int n;

		Bezier *next;
		static Bezier *head;
};

Bezier *Bezier::head = NULL;

//keep track of both ends of the piece of the road
class End
{
	public:
		End()
		{
			active=false;
			//the rest is to prevent compiler warnings:
			shape=NULL;
			for (int i=0; i<3; ++i)
				pos[i]=0.0;
			for (int i=0; i<9; ++i)
				rot[i]=0.0;
			offset=0.0;
			stiff[0]=0.0;
			stiff[1]=0.0;
		}

		bool active;

		void Add(Text_File *file)
		{
			if ((shape=Bezier::Find(file->words[1])))
			{
				active=true;

				pos[0] = atof(file->words[2]);
				pos[1] = atof(file->words[3]);
				pos[2] = atof(file->words[4]);
			}
			else
				active=false;
		}

		void GetPos(float *pos)
		{
			memcpy(pos, this->pos, sizeof(float)*3);
		}

		void GetDirPos(float *pos, bool reverse)
		{
			GetPos(pos);

			if (reverse)
			{
				pos[0]+=rot[1]*stiff[0];
				pos[1]+=rot[4]*stiff[0];
				pos[2]+=rot[7]*stiff[0];
			}
			else
			{
				pos[0]+=rot[1]*stiff[1];
				pos[1]+=rot[4]*stiff[1];
				pos[2]+=rot[7]*stiff[1];
			}

		}
		void GetRot(float *rot)
		{
			memcpy(rot, this->rot, sizeof(float)*9);
		}

		Bezier *shape;
		float pos[3];
		float offset;
		float rot[9];
		float stiff[2];
};

//note: uses signed integers, so limited to about 2 billion (range for indices)
//also performs no overflow checking, but neither does the obj loader...

//adds vertices for one section along one block of road
//performs transformation (morphing between different ends and rotation)
void GenVertices(std::vector<Vector_Float> *vertices,
		float *pos, float *rot, bool top, End *oldend, End *newend,
		float angle, float t, int xres)
{
	float dir[2];
	float tmp1[2], tmp2[2], tmp[2], point[2]; //different points, merge together

	float dx=1.0/float(xres);
	float x=0.0;
	Vector_Float vertex;

	float offset1, offset2;
	if (top)
	{
		offset1=oldend->offset;
		offset2=newend->offset;
	}
	else //bottom
	{
		offset1=-oldend->offset;
		offset2=-newend->offset;
	}

	for (int i=0; i<=xres; ++i)
	{
		//first alternative of point
		oldend->shape->GetPos(x, tmp1);
		oldend->shape->GetDir(x, dir);
		tmp1[0]-=dir[1]*offset1;
		tmp1[1]+=dir[0]*offset1;

		//second alternative
		newend->shape->GetPos(x, tmp2);
		newend->shape->GetDir(x, dir);
		tmp2[0]-=dir[1]*offset2;
		tmp2[1]+=dir[0]*offset2;

		//transform between points (based on t)
		tmp[0] = tmp1[0]*(1.0-t)+tmp2[0]*t;
		tmp[1] = tmp1[1]*(1.0-t)+tmp2[1]*t;

		//rotate between 0 and "angle" rotation
		angle*=t;
		point[0] = +cos(angle)*tmp[0] +sin(angle)*tmp[1];
		point[1] = -sin(angle)*tmp[0] +cos(angle)*tmp[1];

		//apply to 3d space
		vertex.x=pos[0]+rot[0]*point[0]+rot[2]*point[1];
		vertex.y=pos[1]+rot[3]*point[0]+rot[5]*point[1];
		vertex.z=pos[2]+rot[6]*point[0]+rot[8]*point[1];

		//add
		vertices->push_back(vertex);

		x+=dx;
	}
}

//for info output
int triangle_count=0;

void GenIndices(std::vector<Triangle_Uint> *triangles,
		int start, int xstride, int ystride,
		int xres, int yres)
{
	Triangle_Uint triangle;
	//don't specify normals (generated later on)
	triangle.normal[0]=INDEX_ERROR;
	triangle.normal[1]=INDEX_ERROR;
	triangle.normal[2]=INDEX_ERROR;

	for (int y=0; y<yres; ++y)
		for (int x=0; x<xres; ++x)
		{
			//two triangles per "square"
			triangle.vertex[0]=start+xstride*(x+0)+ystride*(y+0);
			triangle.vertex[1]=start+xstride*(x+1)+ystride*(y+0);
			triangle.vertex[2]=start+xstride*(x+0)+ystride*(y+1);
			triangles->push_back(triangle);

			triangle.vertex[0]=start+xstride*(x+1)+ystride*(y+0);
			triangle.vertex[1]=start+xstride*(x+1)+ystride*(y+1);
			triangle.vertex[2]=start+xstride*(x+0)+ystride*(y+1);
			triangles->push_back(triangle);

			triangle_count+=2;
		}
}

//change rotation matrix, rot, based on derivative of cubic bezier line for road
//recalculates the other two directions based on old values
void Rotation(float *rot, float *p0, float *p1, float *p2, float *p3, float t)
{
	//Y=direction
	float Y[3];
	Y[0]=3.0*(p1[0]-p0[0])*(1.0-t)*(1.0-t) +6.0*(p2[0]-p1[0])*(1.0-t)*t +3.0*(p3[0]-p2[0])*t*t; //x
	Y[1]=3.0*(p1[1]-p0[1])*(1.0-t)*(1.0-t) +6.0*(p2[1]-p1[1])*(1.0-t)*t +3.0*(p3[1]-p2[1])*t*t; //y
	Y[2]=3.0*(p1[2]-p0[2])*(1.0-t)*(1.0-t) +6.0*(p2[2]-p1[2])*(1.0-t)*t +3.0*(p3[2]-p2[2])*t*t; //z
	//make unit
	float l = sqrt(Y[0]*Y[0] + Y[1]*Y[1] + Y[2]*Y[2]);

	if (l>0.0) //check that we got an actual direction
	{
		rot[1]=Y[0]/l;
		rot[4]=Y[1]/l;
		rot[7]=Y[2]/l;
	}
	//else, this would've resulted in NaN, so keep the old direction...
	//there might be some better solution to this?

	//X = Y x Zold
	rot[0]=rot[4]*rot[8] -rot[7]*rot[5];
	rot[3]=rot[7]*rot[2] -rot[1]*rot[8];
	rot[6]=rot[1]*rot[5] -rot[4]*rot[2];
	//unit
	l = sqrt(rot[0]*rot[0] +rot[3]*rot[3] +rot[6]*rot[6]);
	rot[0]/=l; rot[3]/=l; rot[6]/=l;

	//Z = X x Y
	rot[2]=rot[3]*rot[7] -rot[6]*rot[4];
	rot[5]=rot[6]*rot[1] -rot[0]*rot[7];
	rot[8]=rot[0]*rot[4] -rot[3]*rot[1];
	//unit
	l = sqrt(rot[2]*rot[2] +rot[5]*rot[5] +rot[8]*rot[8]);
	rot[2]/=l; rot[5]/=l; rot[8]/=l;
}

//change position along cubic bezier
void Position(float *pos, float *p0, float *p1, float *p2, float *p3, float t)
{
	pos[0]=p0[0]*(1.0-t)*(1.0-t)*(1.0-t) +3.0*p1[0]*(1.0-t)*(1.0-t)*t +3.0*p2[0]*(1.0-t)*t*t +p3[0]*t*t*t;
	pos[1]=p0[1]*(1.0-t)*(1.0-t)*(1.0-t) +3.0*p1[1]*(1.0-t)*(1.0-t)*t +3.0*p2[1]*(1.0-t)*t*t +p3[1]*t*t*t;
	pos[2]=p0[2]*(1.0-t)*(1.0-t)*(1.0-t) +3.0*p1[2]*(1.0-t)*(1.0-t)*t +3.0*p2[2]*(1.0-t)*t*t +p3[2]*t*t*t;
}

bool Trimesh::Load_Road(const char *f)
{
	printlog(2, "Loading trimesh from road file %s", f);

	Text_File file;

	//check if ok...
	if (!file.Open(f))
		return false;
	
	//set name to filename
	name=f;

	//empty old data (if any)
	vertices.clear();
	//texcoords.clear();
	normals.clear();
	materials.clear();

	//configuration variables:
	//
	//for each block of road:
	Material *material=NULL;
	int xres=10, yres=10; //resolutionof each piece
	bool capping=true; //enabled but only used when depth
	bool cubic=true;
	//
	//for each section/end of road ("otf"):
	//(gets copied into the End class when actually used)
	End oldend, newend;
	float depth=0.5; //depth of road
	float rotation[9] = {1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0};
	float stiffness[2] = {5,5};

	//misc:
	triangle_count=0; //for info output
	int last_xres=0; //to prevent getting fooled (guaranteed to be accurate)
	bool last_dpt=false; //the same, set correctly after each road block

	while  (file.Read_Line())
	{
		if (!strcmp(file.words[0], "section") && file.word_count >= 4 && !(file.word_count&1))
			new Bezier(&file);
		else if (!strcmp(file.words[0], "add") && file.word_count == 5)
		{
			//if last end doesn't exist, this is first part of road, cap it
			bool cap=oldend.active? false: true;

			oldend=newend;
			newend.Add(&file);

			//if this failed, we reset and move on
			if (!newend.active)
			{
				oldend.active=false;
				newend.active=false;
				continue;
			}

			//copy settings to the new end
			newend.offset = depth/2.0;
			newend.stiff[0]=stiffness[0];
			newend.stiff[1]=stiffness[1];
			memcpy(newend.rot, rotation, sizeof(float)*9);

			bool dpt=oldend.offset||newend.offset? true: false;

			//no point continuing if only got one section yet
			if (!oldend.active)
				continue;

			//else, add new block of road

			//check if got material:
			if (!material)
			{
				printlog(0, "ERROR: road file did not specify material to use, using default\n");
				materials.push_back(Material_Default); //add new material (with defaults)
				material=&materials[0];
			}

			//variables used
			float rot[9]={1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0};;
			float pos[3]={0.0, 0.0, 0.0};;
			float t;
			float dy=1.0/float(yres);

			float p0[3], p1[3], p2[3], p3[3];
			oldend.GetPos(p0);
			oldend.GetDirPos(p1, false);
			newend.GetDirPos(p2, true);
			newend.GetPos(p3);

			//determine how rotation differs after twitching from first to second end
			//(needs to be compensated when transforming between the two ends)
			//simulate rotation from generation before actual rotation:
			//copy first rotation:
			oldend.GetRot(rot);
			t=0.0;
			for (int i=0; i<=yres; ++i)
			{
				Rotation(rot, p0, p1, p2, p3, t);
				t+=dy;
			}
			//angle of difference: v=arccos(Z*Z1)
			float v;
			//safetycheck in case outside range of acos
			float dot=rot[2]*newend.rot[2] +rot[5]*newend.rot[5] +rot[8]*newend.rot[8];
			if (dot > 1.0)
				v=0.0;
			else if (dot < -1.0)
				v=M_PI;
			else
				v=acos(dot);

			//direction of difference: Z*X1 > 0.0 -> v<0.0
			if (rot[2]*newend.rot[0] +rot[5]*newend.rot[3] +rot[8]*newend.rot[6] > 0.0)
				v=-v;

			//done, start real generation.
			int start=vertices.size(); //store current position (before adding more data)
			//copy original rotation again
			oldend.GetRot(rot);
			//vertices
			t=0.0;
			int i=0;

			//check if we can reuse the old vertices (everything matches)... :-)
			if (last_xres==xres && last_dpt==dpt)
			{
				printf("reuse\n");
				t+=dy; //skip first "row" of vertices
				i+=1; //skip for for looping variable too

				//go back a bit
				if (dpt) //if depth (two rows to skip)
					start-=2*(xres+1);
				else //one row
					start-=xres+1;
			}

			for (; i<=yres; ++i)
			{
				Rotation(rot, p0, p1, p2, p3, t);
				Position(pos, p0, p1, p2, p3, t);

				if (cubic) //use "cubic" transformation (=smooth)
				{
					GenVertices(&vertices, pos, rot, true, &oldend, &newend, v, (3*t*t-2*t*t*t), xres);

					//other side
					if (dpt)
						GenVertices(&vertices, pos, rot, false, &oldend, &newend, v, (3*t*t-2*t*t*t), xres);
				}
				else //use linear approach
				{
					GenVertices(&vertices, pos, rot, true, &oldend, &newend, v, t, xres);
					if (dpt)
						GenVertices(&vertices, pos, rot, false, &oldend, &newend, v, t, xres);
				}

				t+=dy;
			}

			//indices
			if (dpt)
			{
				//top
				GenIndices(&material->triangles, start, 1, 2*(xres+1), xres, yres);

				//bottom
				GenIndices(&material->triangles, start+2*xres+1, -1, 2*(xres+1), xres, yres);

				//sides
				GenIndices(&material->triangles, start+xres, xres+1, 2*(xres+1), 1, yres);
				GenIndices(&material->triangles, start+xres+1, -(xres+1), 2*(xres+1), 1, yres);

				//capping of this end (first, got depth and conf wants)?
				if (cap&&oldend.offset&&capping)
					GenIndices(&material->triangles, start+xres+1, 1, -(xres+1), xres, 1);
			}
			else //only top
				GenIndices(&material->triangles, start, 1, xres+1, xres, yres);

			//keep track of what settings were used
			last_xres = xres;
			last_dpt = dpt;
		}
		else if (!strcmp(file.words[0], "rotation") && file.word_count == 4)
		{
			//ugly but reliable: use ode utility for rotation:
			float x=atof(file.words[1]);
			float y=atof(file.words[2]);
			float z=atof(file.words[3]);
			dMatrix3 r;
			dRFromEulerAngles (r, x*(M_PI/180), y*(M_PI/180), z*(M_PI/180));
			rotation[0] = r[0];	rotation[1] = r[1];	rotation[2] = r[2];
			rotation[3] = r[4];	rotation[4] = r[5];	rotation[5] = r[6];
			rotation[6] = r[8];	rotation[7] = r[9];	rotation[8] = r[10];
		}
		else if (!strcmp(file.words[0], "stiffness") && file.word_count == 3)
		{
			float tmp=atof(file.words[1]);
			if (tmp<0.0)
				stiffness[0]=tmp;
			else
				printlog(0, "WARNING: first stiffness value must be under 0");

			tmp=atof(file.words[2]);
			if (tmp>0.0)
				stiffness[1]=tmp;
			else
				printlog(0, "WARNING: second stiffness values must be above 0");
		}
		else if (!strcmp(file.words[0], "depth"))
			depth=atof(file.words[1]);

		else if (!strcmp(file.words[0], "resolution") && file.word_count == 3)
		{
			int tmp=atoi(file.words[1]);
			if (tmp > 0)
				xres=tmp;
			else
				printlog(0, "WARNING: x resolution value must be above 0");

			tmp=atoi(file.words[2]);
			if (tmp > 0)
				yres=tmp;
			else
				printlog(0, "WARNING: y resolution value must be above 0");
		}
		else if (!strcmp(file.words[0], "material") && file.word_count == 2)
		{
			unsigned int tmp = Find_Material(file.words[1]);

			if (tmp == INDEX_ERROR)
				printlog(0, "WARNING: failed to change material (things will probably look wrong)");
			else
				material=&materials[tmp];
		}
		else if (!strcmp(file.words[0], "material_file") && file.word_count == 2)
		{
			//directly copied from obj.cpp
			char filename[strlen(f)+strlen(file.words[1])];
			strcpy(filename, f);
			char *last = strrchr(filename, '/');

			if (last)
			{
				last[1]='\0';
				strcat(filename, file.words[1]);
			}
			else
			{
				strcpy(filename, file.words[1]);
			}
			Load_Material(filename);
		}
		else if (!strcmp(file.words[0], "cubic"))
			cubic=true;
		else if (!strcmp(file.words[0], "linear"))
			cubic=false;
		else if (!strcmp(file.words[0], "capping"))
			capping=true;
		else if (!strcmp(file.words[0], "nocapping"))
			capping=false;
		else
			printlog(0, "WARNING: malformated line in road file, ignoring");
	}

	//at end, should cap?
	if (oldend.active && newend.active && newend.offset && capping)
		GenIndices(&material->triangles, vertices.size()-2*(last_xres+1), 1, last_xres+1, last_xres, 1);

	//done, remove all data:
	Bezier::RemoveAll();

	//check that at least something got loaded:
	if (materials.empty() || vertices.empty())
	{
		printlog(0, "ERROR: road file seems to exist, but empty?!");
		return false;
	}

	//no normals created! generate them
	Generate_Missing_Normals();

	printlog(1, "ROAD generation info: %u triangles, %u materials", triangle_count, materials.size());

	return true;
}

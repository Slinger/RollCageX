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

//TODO: needs more documentation: both this code and the ".road" file type...

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
			dir[0]/=l;
			dir[1]/=l;
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
			ctrl=0.0;
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

				//ugly but reliable: use ode utility for rotation:
				float x=atof(file->words[5]);
				float y=atof(file->words[6]);
				float z=atof(file->words[7]);
				dMatrix3 r;
				dRFromEulerAngles (r, x*(M_PI/180), y*(M_PI/180), z*(M_PI/180));
				rot[0] = r[0];	rot[1] = r[1];	rot[2] = r[2];
				rot[3] = r[4];	rot[4] = r[5];	rot[5] = r[6];
				rot[6] = r[8];	rot[7] = r[9];	rot[8] = r[10];

				ctrl = atof(file->words[8]);
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
				pos[0]-=rot[1]*ctrl;
				pos[1]-=rot[4]*ctrl;
				pos[2]-=rot[7]*ctrl;
			}
			else
			{
				pos[0]+=rot[1]*ctrl;
				pos[1]+=rot[4]*ctrl;
				pos[2]+=rot[7]*ctrl;
			}

		}
		void GetRot(float *rot)
		{
			memcpy(rot, this->rot, sizeof(float)*9);
		}

		Bezier *shape;
		float pos[3];
		float rot[9];
		float ctrl;
};

//note: uses signed integers, so limited to about 2 billion (range for indices)
//also performs no overflow checking, but neither does the obj loader...

//adds vertices for one section along one block of road
//performs transformation (morphing between different ends and rotation)
void GenVertices(std::vector<Vector_Float> *vertices,
		float *pos, float *rot, Bezier *oldb, Bezier *newb,
		float angle, float offset, float t, int xres)
{
	float dir[2];
	float tmp1[2], tmp2[2], tmp[2], point[2]; //different points, merge together

	float dx=1.0/float(xres);
	float x=0.0;
	Vector_Float vertex;

	for (int i=0; i<=xres; ++i)
	{
		//first alternative of point
		oldb->GetPos(x, tmp1);
		if (offset)
		{
			oldb->GetDir(x, dir);
			tmp1[0]-=dir[1]*offset;
			tmp1[1]+=dir[0]*offset;
		}

		//second alternative
		newb->GetPos(x, tmp2);
		if (offset)
		{
			newb->GetDir(x, dir);
			tmp2[0]-=dir[1]*offset;
			tmp2[1]+=dir[0]*offset;
		}

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
	rot[1]=3.0*(p1[0]-p0[0])*(1.0-t)*(1.0-t) +6.0*(p2[0]-p1[0])*(1.0-t)*t +3.0*(p3[0]-p2[0])*t*t; //x
	rot[4]=3.0*(p1[1]-p0[1])*(1.0-t)*(1.0-t) +6.0*(p2[1]-p1[1])*(1.0-t)*t +3.0*(p3[1]-p2[1])*t*t; //y
	rot[7]=3.0*(p1[2]-p0[2])*(1.0-t)*(1.0-t) +6.0*(p2[2]-p1[2])*(1.0-t)*t +3.0*(p3[2]-p2[2])*t*t; //z
	//make unit
	float l = sqrt(rot[1]*rot[1] +rot[4]*rot[4] +rot[7]*rot[7]);
	rot[1]/=l; rot[4]/=l; rot[7]/=l;

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
	int xres=10, yres=10; //resolution of each piece
	float depth=0.5; //depth of road
	bool capping=true; //enabled but only used when depth
	Material *material=NULL;
	End oldend, newend;
	//float lastdepth, lastxres, lastyres;

	//set counter to 0
	triangle_count=0;

	while  (file.Read_Line())
	{
		if (!strcmp(file.words[0], "section") && file.word_count >= 4 && !(file.word_count&1))
				new Bezier(&file);
		else if (!strcmp(file.words[0], "add") && file.word_count == 9)
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

			//no point continuing if only got one section yet
			if (!oldend.active)
				continue;

			//else, add new block of road
			//
			//TODO: join vertices for section shared by two pieces of road
			//currently, the vertices between two pieces of road gets duplicated...
			//doesn't affect the rendering quality or simulation precision but takes
			//a little more ram (which might not be a big problem, anyway)

			//check if got material:
			if (!material)
			{
				printlog(0, "ERROR: road file did not specify material to use, using default\n");
				materials.push_back(Material_Default); //add new material (with defaults)
				material=&materials[0];
			}

			//variables used
			//TODO: some might need initialization to prevent compiler warning?
			int i;
			float rot[9];
			float pos[3];
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
			for (i=0; i<=yres; ++i)
			{
				Rotation(rot, p0, p1, p2, p3, t);
				t+=dy;
			}
			//TODO: if (Z*Z1 > 1) -> v=0
			//angle of difference: v=arccos(Z*Z1)
			float v=acos(rot[2]*newend.rot[2] +rot[5]*newend.rot[5] +rot[8]*newend.rot[8]);
			//direction of difference: Z*X1 > 0.0 -> v<0.0
			if (rot[2]*newend.rot[0] +rot[5]*newend.rot[3] +rot[8]*newend.rot[6] > 0.0)
				v=-v;

			//done, start real generation.
			int start=vertices.size(); //store current position (before adding more data)
			//copy original rotation again
			oldend.GetRot(rot);
			//vertices
			t=0.0;
			for (i=0; i<=yres; ++i)
			{
				Rotation(rot, p0, p1, p2, p3, t);
				Position(pos, p0, p1, p2, p3, t);

				//TODO: if cubic rotation -> t=3t*t-2*t*t*t

				GenVertices(&vertices, pos, rot, oldend.shape, newend.shape, v, depth/2.0, t, xres);

				//other side
				if (depth)
					GenVertices(&vertices, pos, rot, oldend.shape, newend.shape, v, -depth/2.0, t, xres);

				t+=dy;
			}

			//indices
			if (depth)
			{
				//top
				GenIndices(&material->triangles, start, 1, 2*(xres+1), xres, yres);

				//bottom
				GenIndices(&material->triangles, start+2*xres+1, -1, 2*(xres+1), xres, yres);

				//sides
				GenIndices(&material->triangles, start+xres, xres+1, 2*(xres+1), 1, yres);
				GenIndices(&material->triangles, start+xres+1, -(xres+1), 2*(xres+1), 1, yres);

				//capping of this end?
				if (cap&&capping)
					GenIndices(&material->triangles, start+xres+1, 1, -(xres+1), xres, 1);
			}
			else //only top
				GenIndices(&material->triangles, start, 1, xres+1, xres, yres);
		}
		else if (!strcmp(file.words[0], "resolution"))
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
		else if (!strcmp(file.words[0], "material"))
		{
			unsigned int tmp = Find_Material(file.words[1]);

			if (tmp == INDEX_ERROR)
				printlog(0, "WARNING: failed to change material (things will probably look wrong)");
			else
				material=&materials[tmp];
		}
		else if (!strcmp(file.words[0], "material_file"))
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
		else if (!strcmp(file.words[0], "depth"))
			depth=atof(file.words[1]);
		else if (!strcmp(file.words[0], "capping"))
			capping=true;
		else if (!strcmp(file.words[0], "nocapping"))
			capping=false;
		else
			printlog(0, "WARNING: malformated line in road file, ignoring");
	}

	//at end, should cap?
	if (depth && capping)
		GenIndices(&material->triangles, vertices.size()-2*(xres+1), 1, xres+1, xres, 1);

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

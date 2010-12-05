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
		void GetPosPoint(float t, float d, float *p)
		{
			float point[2];
			shape->GetPos(t, point);
			if (d)
			{
				float dir[2];
				shape->GetDir(t, dir);
				point[0]-=dir[1]*d;
				point[1]+=dir[0]*d;
			}
			p[0]=pos[0]+rot[0]*point[0]+rot[2]*point[1];
			p[1]=pos[1]+rot[3]*point[0]+rot[5]*point[1];
			p[2]=pos[2]+rot[6]*point[0]+rot[8]*point[1];
		}
		void GetDirPoint(float t, float d, bool reverse, float *p)
		{
			GetPosPoint(t, d, p);
			if (reverse)
			{
				p[0]-=rot[1]*ctrl;
				p[1]-=rot[4]*ctrl;
				p[2]-=rot[7]*ctrl;
			}
			else
			{
				p[0]+=rot[1]*ctrl;
				p[1]+=rot[4]*ctrl;
				p[2]+=rot[7]*ctrl;
			}
		}

	public:
		Bezier *shape;
		float pos[3];
		float rot[9];
		float ctrl;
};

//note: uses signed integers, so limited to about 2 billion (range for indices)
//also performs no overflow checking, but neither does the obj loader...
void GenVertices(std::vector<Vector_Float> *vertices, End *oldend, End *newend,
		float offset, int xres, int yres)
{
	float p0[3], p1[3], p2[3], p3[3];
	float mult0, mult1, mult2, mult3;
	Vector_Float vertex;
	float w=0.0,l=0.0; //Width&Length
	float wd=1.0/float(xres); //length of each step
	float ld=1.0/float(yres); //dito
	for (int x=0; x<=xres; ++x)
	{
		oldend->GetPosPoint(w, offset, p0);
		oldend->GetDirPoint(w, offset, false, p1);
		newend->GetDirPoint(w, offset, true, p2);
		newend->GetPosPoint(w, offset, p3);
		//create curve based on (4 point) bezier curve

		l=0.0;
		for (int y=0; y<=yres; ++y)
		{
			mult0 = (1.0-l)*(1.0-l)*(1.0-l);
			mult1 = 3*(1.0-l)*(1.0-l)*l;
			mult2 = 3*(1.0-l)*l*l;
			mult3 = l*l*l;

			vertex.x=p0[0]*mult0+p1[0]*mult1+p2[0]*mult2+p3[0]*mult3;
			vertex.y=p0[1]*mult0+p1[1]*mult1+p2[1]*mult2+p3[1]*mult3;
			vertex.z=p0[2]*mult0+p1[2]*mult1+p2[2]*mult2+p3[2]*mult3;

			//add
			vertices->push_back(vertex);

			l+=ld;
		}

		w+=wd;
	}
}

void GenIndices(std::vector<Triangle_Uint> *triangles,
		int start, int stride, int xres, int yres)
{
	Triangle_Uint triangle;
	//don't specify normals (generated later on)
	triangle.normal[0]=INDEX_ERROR;
	triangle.normal[1]=INDEX_ERROR;
	triangle.normal[2]=INDEX_ERROR;

	for (int x=0; x<xres; ++x)
		for (int y=0; y<yres; ++y)
		{
			//two triangles per "square"
			triangle.vertex[0]=start+(y)+(x)*(stride);
			triangle.vertex[1]=start+(y)+(x+1)*(stride);
			triangle.vertex[2]=start+(y+1)+(x)*(stride);
			triangles->push_back(triangle);

			triangle.vertex[0]=start+(y)+(x+1)*(stride);
			triangle.vertex[1]=start+(y+1)+(x+1)*(stride);
			triangle.vertex[2]=start+(y+1)+(x)*(stride);
			triangles->push_back(triangle);
		}
}

void GenCapIndices(std::vector<Triangle_Uint> *triangles,
		int start1, int start2, int stride, int xres)
{
	Triangle_Uint triangle;
	triangle.normal[0]=INDEX_ERROR;
	triangle.normal[1]=INDEX_ERROR;
	triangle.normal[2]=INDEX_ERROR;
	for (int x=0; x<xres; ++x)
	{
			triangle.vertex[0]=start1+(x)*(stride);
			triangle.vertex[1]=start2+(x)*(stride);
			triangle.vertex[2]=start2+(x+1)*(stride);
			triangles->push_back(triangle);

			triangle.vertex[0]=start2+(x+1)*(stride);
			triangle.vertex[1]=start1+(x+1)*(stride);
			triangle.vertex[2]=start1+(x)*(stride);
			triangles->push_back(triangle);
	}
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

			//check if got material:
			if (!material)
			{
				printlog(0, "ERROR: road file did not specify material to use, using default\n");
				materials.push_back(Material_Default); //add new material (with defaults)
				material=&materials[0];
			}

			//TODO: join vertices for section shared by two pieces of road
			//currently, the vertices between two pieces of road gets duplicated...
			//doesn't affect the rendering quality or simulation precision but takes
			//a little more ram (which might not be a big problem, anyway)

			//generate vertices (for a top surface)
			int start=vertices.size(); //store current position in vertex list
			GenVertices(&vertices, &oldend, &newend, depth/2.0, xres, yres);

			//generate indices (for the surface)
			GenIndices(&material->triangles, start, yres+1, xres, yres);

			//depth in road
			if (depth)
			{
				//other (bottom) side
				GenVertices(&vertices, &oldend, &newend, -depth/2.0, xres, yres);
				GenIndices(&material->triangles, vertices.size()-(yres+1), -(yres+1), xres, yres);

				//and sides (reuses already existing vertices)
				//left:
				GenIndices(&material->triangles, start+(xres+1)*(yres+1), -(xres+1)*(yres+1), 1, yres);
				//right:
				GenIndices(&material->triangles, start+(xres)*(yres+1), (xres+1)*(yres+1), 1, yres);

				//capping:
				if (cap&&capping)
					GenCapIndices(&material->triangles, 0, (xres+1)*(yres+1), yres+1, xres);
			}
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
		GenCapIndices(&material->triangles, (vertices.size()-1)-xres*(yres+1),
				(vertices.size()-1)-xres*(yres+1)-(xres+1)*(yres+1), yres+1, xres);

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

	//printlog(1, "road generation info: %u triangles, %u materials", triangle_count, materials.size());

	return true;
}

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
			p = new point[n];

			//store:
			for (int i=0; i<n; ++i)
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
			for (Bezier *p=head; p; p=p->next)
				if (p->name == name)
					return p;

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
			for (int i=0; i<n-1; ++i)
				t2*=(1.0-t);

			//if t=1, then prevent NaN...
			float t2mod = t==1.0? 1.0:1.0/(1.0-t);

			//calculate
			for (int i=0; i<n; ++i)
			{
				//binomial coefficient (fixed n=row, for each i)
				pos[0] += float(k)*t1*t2*p[i].x;
				pos[1] += float(k)*t1*t2*p[i].y;

				//coefficient for next time
				k*=(n-1-i);
				k/=(i+1);

				//t1&t2
				t1*=t;
				t2*=t2mod;

				//if at last, set to 1
				if (i+2==n)
					t2=1;
			}
			//done
		}
		/*void GetDir(float t, float *dir)
		{
			//derive
			dir={0,0};
			//calculate
			for (int i=0; i<(n-1); ++i)
			{
				//binomial coefficient
				dir[0] += n*binomial(n-1,i)*t^i*(1.0-t)^(n-1-i)*(p[i+1]-p[i]);
				dir[1] += 
			}
			//normalize
			float l = sqrt(dir[0]*dir[0]+dir[1]*dir[1]);
			dir[0]/l;
			dir[1]/l;
		}*/
		static void RemoveAll()
		{
			Bezier *next;
			while ((head=next))
			{
				next=head->next;
				delete head;
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
		End(): active(false) {};

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
		void GetPosPoint(float t, float *p)
		{
			float point[2];
			shape->GetPos(t, point);
			p[0]=pos[0]+rot[0]*point[0]+rot[2]*point[1];
			p[1]=pos[1]+rot[3]*point[0]+rot[5]*point[1];
			p[2]=pos[2]+rot[6]*point[0]+rot[8]*point[1];
		}
		void GetDirPoint(float t, bool reverse, float *p)
		{
			GetPosPoint(t, p);
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

	private:
		Bezier *shape;
		float pos[3];
		float rot[9];
		float ctrl;
};

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

	while  (file.Read_Line())
	{
		if (!strcmp(file.words[0], "section") && file.word_count >= 4 && !(file.word_count&1))
				new Bezier(&file);
		else if (!strcmp(file.words[0], "add") && file.word_count == 9)
		{
			oldend=newend;
			newend.Add(&file);

			//if this failed, we reset and move on
			if (!newend.active)
			{
				oldend.active=false;
				newend.active=false;
				continue;
			}

			//get all data we want:
			//newsection.pos = ...

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

			//generate vertices
			int start=vertices.size(); //store current position in vertex list
			float p0[3], p1[3], p2[3], p3[3];
			float mult0, mult1, mult2, mult3;
			Vector_Float vertex;
			//TODO: join vertices for section shared by two pieces of road
			int x,y; //looping
			float w=0.0,l=0.0; //Width&Length
			float wd=1.0/float(xres); //length of each step
			float ld=1.0/float(yres);
			for (x=0; x<=xres; ++x)
			{
				oldend.GetPosPoint(w, p0);
				oldend.GetDirPoint(w, false, p1);
				newend.GetDirPoint(w, true, p2);
				newend.GetPosPoint(w, p3);
				//create curve based on bezier curve (4 points)

				l=0.0;
				for (y=0; y<=yres; ++y)
				{
					mult0 = (1.0-l)*(1.0-l)*(1.0-l);
					mult1 = 3*(1.0-l)*(1.0-l)*l;
					mult2 = 3*(1.0-l)*l*l;
					mult3 = l*l*l;

					vertex.x=p0[0]*mult0+p1[0]*mult1+p2[0]*mult2+p3[0]*mult3;
					vertex.y=p0[1]*mult0+p1[1]*mult1+p2[1]*mult2+p3[1]*mult3;
					vertex.z=p0[2]*mult0+p1[2]*mult1+p2[2]*mult2+p3[2]*mult3;

					//add
					vertices.push_back(vertex);

					l+=ld;
				}

				w+=wd;
			}
			//generate indices
			Triangle triangle;
			//don't specify normals (generated later on)
			triangle.normal[0]=INDEX_ERROR;
			triangle.normal[1]=INDEX_ERROR;
			triangle.normal[2]=INDEX_ERROR;

			for (int x=0; x<xres; ++x)
				for (int y=0; y<yres; ++y)
				{
					//two triangles per "square"
					triangle.vertex[0]=start+(y)+(x)*(yres+1);
					triangle.vertex[1]=start+(y)+(x+1)*(yres+1);
					triangle.vertex[2]=start+(y+1)+(x)*(yres+1);
					material->triangles.push_back(triangle);

					triangle.vertex[0]=start+(y)+(x+1)*(yres+1);
					triangle.vertex[1]=start+(y+1)+(x+1)*(yres+1);
					triangle.vertex[2]=start+(y+1)+(x)*(yres+1);
					material->triangles.push_back(triangle);
				}
		}
		else if (!strcmp(file.words[0], "stop"))
		{
			//reset
			oldend.active=false;
			newend.active=false;
		}
		else
			printlog(0, "WARNING: malformated line in road file, ignoring");
		/*else if (!strcmp(file.words[0], "resolution"))
		{}
		else if (!strcmp(file.words[0], "depth"))
		{}
		else if (!strcmp(file.words[0], "material....*"))
		{}*/
	}

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

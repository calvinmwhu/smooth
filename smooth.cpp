#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <math.h>
#include <vector>
#include "SOIL/SOIL.h"

using namespace std;

#define PI 3.14159265358979323846
#define HEIGHT 0.5
#define TRIP_TIME 20.0

GLuint textureID;
GLuint enviTexID;
GLenum mode=GL_LINE;

int nFPS = 60;
int levelOfSubdv=0;
float timeIncre;
float curTime=0;
float y_max=0.0f;
float r=1.f;
float g=0.f;
float b=0.f; 
bool showTexture=false;
bool lightOn=false;

//four control points of the camera cubic Bezier path 

glm::vec3 P0=glm::vec3(-5.f,0.0f,5.0f);
glm::vec3 P1=glm::vec3(5.f,0.0f,5.0f);
glm::vec3 P2=glm::vec3(5.f,0.0f,-5.0f);
glm::vec3 P3=glm::vec3(-5.f,0.0f,-5.0f);

glm::vec3 eye=glm::vec3(-5.0f, 0.0f, 5.0f);
glm::vec3 lookAtOrigin=glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 lookAtVertex=glm::vec3(-0.6,1,HEIGHT) ;
glm::vec3 lookAtPos=lookAtOrigin;

typedef struct HE_ver_t
{
	glm::vec3 			coord;
	glm::vec3			modifiedCoord;
	glm::vec3 			vNormal;
	glm::vec2			texCoord;
	struct HE_edge_t* 	startEdge;
	bool 				connected;
	int 				indexInArray;
}HE_ver;

typedef struct HE_edge_t
{
	HE_ver* 			endVertex;
	struct HE_edge_t*   pair;
	struct HE_face_t*   leftFace;   
    struct HE_edge_t*   next; 
    glm::vec3 		    edgeVertex;
}HE_edge;

typedef struct HE_face_t
{
	glm::vec4 			adjV;
	glm::vec3 			fNormal;
	HE_edge* 			startEdge; 
	glm::vec3 			faceVertex;
}HE_face;

vector< glm::vec3 > verticesContainer;
vector< glm::vec4 > facesContainer;

vector< HE_face* > facesInitial    ;
vector< HE_face* > facesFirstLevel ;
vector< HE_face* > facesSecondLevel;
vector< HE_face* > facesThirdLevel;

vector< HE_ver* > verticesInitial;
vector< HE_ver* > verticesFirstLevel;
vector< HE_ver* > verticesSecondLevel;
vector< HE_ver* > verticesThirdLevel;

void getFaceNormal(vector<HE_face* >& faces, vector<HE_ver* >& vertices )
{
     for(int i=0;i<(int)faces.size();i++)
     {
          glm::vec3 v1=vertices[(faces[i]->adjV).x]->coord;
          glm::vec3 v2=vertices[(faces[i]->adjV).y]->coord;
          glm::vec3 v3=vertices[(faces[i]->adjV).z]->coord;

          glm::vec3 left=v2-v1;
          glm::vec3 right=v3-v1;

          glm::vec3 normal=glm::cross(left,right);
          faces[i]->fNormal=glm::normalize(normal);
     }
}

void getVertexNormal(vector<HE_face* >& faces, vector<HE_ver* >& vertices)
{
     for(int i=0;i<(int)faces.size();i++)
     {
          glm::vec4 face=faces[i]->adjV;
          glm::vec3 n=faces[i]->fNormal;
          vertices[face.x]->vNormal+=n;
          vertices[face.y]->vNormal+=n;
          vertices[face.z]->vNormal+=n;
          vertices[face.w]->vNormal+=n;
     }
     for(int i=1;i<(int)vertices.size();i++)
     {
          vertices[i]->vNormal= glm::normalize(vertices[i]->vNormal);
     }
}

void get_Ymax(vector<HE_ver* >& vertices)
{
     for(int i=1;i<(int)vertices.size();i++)
     {
          if((vertices[i]->coord).y>y_max)
               y_max=(vertices[i]->coord).y;
     }
}

void getTextureCoord(vector<HE_ver* >& vertices)
{
     for(int i=1;i<(int)vertices.size();i++)
     {
          float x=(vertices[i]->coord).x; float y=(vertices[i]->coord).y; float z=(vertices[i]->coord).z;
          float theta=atan2(z,x);
          float s=(theta+PI)/(2*PI);
          float t=y/y_max;
          vertices[i]->texCoord= glm::vec2(s,t);
     }
}

void initializeTexPara( vector<HE_face* >& faces ,vector<HE_ver* >& vertices)
{
     get_Ymax(vertices);
     getFaceNormal(faces,vertices);
     getVertexNormal(faces,vertices);
     getTextureCoord(vertices);
}

//initialize the faces and vertices half_edge vectors using two normal arrays of glm vectors
void populateHEDS(const vector<glm::vec4 >& fSrc, const vector<glm::vec3>& vSrc, 
			   vector<HE_face*  >& fDes, vector<HE_ver* >& vDes)
{
	for(int i=0; i<(int)fSrc.size();i++)
	{
		HE_face* f=(HE_face*)malloc(sizeof(HE_face));
		f->adjV=fSrc[i];
		f->startEdge=NULL;
		f->faceVertex=glm::vec3(0.0, 0.0, 0.0);

		fDes.push_back(f);
	}

	for(int i=0; i<(int)vSrc.size(); i++)
	{
		HE_ver* v=(HE_ver*)malloc(sizeof(HE_ver));
		v->coord=vSrc[i];
		v->startEdge=NULL;
		v->connected=false;
		v->indexInArray=i;
		v->modifiedCoord=glm::vec3(0.0, 0.0, 0.0);

		vDes.push_back(v);
	}
	initializeTexPara( fDes, vDes);
}

//get initial "I"
void getInitialData(vector<glm::vec4 >& faces, vector<glm::vec3>& vertices)
{
	faces.push_back(glm::vec4(1,2,3,16));
	faces.push_back(glm::vec4(16,3,12,15));
	faces.push_back(glm::vec4(15,12,13,14));
	faces.push_back(glm::vec4(3,4,11,12));
	faces.push_back(glm::vec4(5,6,7,4));
	faces.push_back(glm::vec4(4,7,8,11));
	faces.push_back(glm::vec4(11,8,9,10));

	faces.push_back(glm::vec4(17,32,19,18));
	faces.push_back(glm::vec4(32,31,28,19));
	faces.push_back(glm::vec4(31,30,29,28));
	faces.push_back(glm::vec4(19,28,27,20));
	faces.push_back(glm::vec4(21,20,23,22));
	faces.push_back(glm::vec4(20,27,24,23));
	faces.push_back(glm::vec4(27,26,25,24));

	faces.push_back(glm::vec4(1,17,18,2));
	faces.push_back(glm::vec4(2,18,19,3));
	faces.push_back(glm::vec4(3,19,20,4));
	faces.push_back(glm::vec4(4,20,21,5));
	faces.push_back(glm::vec4(5,21,22,6));
	faces.push_back(glm::vec4(6,22,23,7));
	faces.push_back(glm::vec4(7,23,24,8));
	faces.push_back(glm::vec4(8,24,25,9));
	faces.push_back(glm::vec4(9,25,26,10));
	faces.push_back(glm::vec4(10,26,27,11));
	faces.push_back(glm::vec4(11,27,28,12));
	faces.push_back(glm::vec4(12,28,29,13));
	faces.push_back(glm::vec4(13,29,30,14));
	faces.push_back(glm::vec4(14,30,31,15));
	faces.push_back(glm::vec4(15,31,32,16));
	faces.push_back(glm::vec4(16,32,17,1 ));

	vertices.push_back(glm::vec3(0,0,0));

	vertices.push_back(glm::vec3(-0.6,1,HEIGHT));
	vertices.push_back(glm::vec3(-0.6,0.6,HEIGHT));
	vertices.push_back(glm::vec3(-0.2,0.6,HEIGHT));
	vertices.push_back(glm::vec3(-0.2,-0.6,HEIGHT));
	vertices.push_back(glm::vec3(-0.6,-0.6,HEIGHT));
	vertices.push_back(glm::vec3(-0.6,-1,HEIGHT));
	vertices.push_back(glm::vec3(-0.2,-1,HEIGHT));
	vertices.push_back(glm::vec3( 0.2,-1,HEIGHT));
	vertices.push_back(glm::vec3( 0.6,-1,HEIGHT));
	vertices.push_back(glm::vec3( 0.6,-0.6,HEIGHT));
	vertices.push_back(glm::vec3( 0.2,-0.6,HEIGHT));
	vertices.push_back(glm::vec3( 0.2,0.6,HEIGHT));
	vertices.push_back(glm::vec3( 0.6,0.6,HEIGHT));
	vertices.push_back(glm::vec3( 0.6,1,HEIGHT));
	vertices.push_back(glm::vec3( 0.2,1,HEIGHT));
	vertices.push_back(glm::vec3( -0.2,1,HEIGHT));

	vertices.push_back(glm::vec3(-0.6,1, 0));
	vertices.push_back(glm::vec3(-0.6,0.6, 0 ));
	vertices.push_back(glm::vec3(-0.2,0.6, 0));
	vertices.push_back(glm::vec3(-0.2,-0.6, 0));
	vertices.push_back(glm::vec3(-0.6,-0.6, 0));
	vertices.push_back(glm::vec3(-0.6,-1, 0 ));
	vertices.push_back(glm::vec3(-0.2,-1, 0));
	vertices.push_back(glm::vec3( 0.2,-1, 0));
	vertices.push_back(glm::vec3( 0.6,-1, 0));
	vertices.push_back(glm::vec3( 0.6,-0.6, 0));
	vertices.push_back(glm::vec3( 0.2,-0.6, 0));
	vertices.push_back(glm::vec3( 0.2,0.6, 0));
	vertices.push_back(glm::vec3( 0.6,0.6, 0));
	vertices.push_back(glm::vec3( 0.6,1, 0));
	vertices.push_back(glm::vec3( 0.2,1, 0));
	vertices.push_back(glm::vec3( -0.2,1, 0));

	populateHEDS(faces,vertices, facesInitial, verticesInitial);
}

HE_ver* getStartVertex(const HE_edge* e)
{
	return e->pair->endVertex;
}

//this function connects all the faces and completes the construction of the half-edge structure
void finalizeHEDS(vector<HE_face*  >& faces, vector<HE_ver* >& vertices)
{
	for(int i=0; i<(int)faces.size();i++)
	{
		faces[i]->startEdge=(HE_edge*)malloc(sizeof(HE_edge));
		faces[i]->startEdge->pair=NULL;
		faces[i]->startEdge->edgeVertex=glm::vec3(0.0, 0.0, 0.0);
		faces[i]->startEdge->leftFace=faces[i];
		HE_edge* temp=faces[i]->startEdge;
		int j=0;
		do{
			if(j==3)
				temp->next=faces[i]->startEdge;
			else 
			{
				temp->next=(HE_edge*)malloc(sizeof(HE_edge));
				temp->next->pair=NULL;
			}
			int idx= (j<3)? j+1:0;
			int endVindex=(faces[i]->adjV)[idx];
			//if((faces[i]->adjV)[1]==(faces[i]->adjV).y) cout<< "yes"<<endl;
			if(vertices[endVindex]->startEdge==NULL)
			{
				vertices[endVindex]->startEdge=temp->next;
			}
			temp->endVertex=vertices[endVindex];
			temp->leftFace=faces[i];
			temp->edgeVertex=glm::vec3(0.0, 0.0, 0.0);
		     j++;

		     temp=temp->next;
		}while(j<=3);
				
	}
	

	for(int i=0; i<(int)faces.size();i++)
	{
		HE_edge* temp1=faces[i]->startEdge;
		do
		{
			if(temp1->pair==NULL)
			{
				HE_ver*  v1End=temp1->endVertex;
				HE_ver*  v1Start=temp1->next->next->next->endVertex;
				//cout<< "checking the edge: "<< "("<< v1Start->indexInArray<< ","<<v1End->indexInArray <<")"<<endl;
				for(int j=0; j<(int)faces.size();j++)
				{
					bool found=false;
					if(j==i) continue;
					
					HE_edge* temp2=faces[j]->startEdge;
					do
					{
						HE_ver*  v2End=temp2->endVertex;
						HE_ver*  v2Start=temp2->next->next->next->endVertex;
						if(v1Start==v2End && v1End==v2Start)
						{
							temp1->pair=temp2;
							temp2->pair=temp1;
							found=true;
							break;
						}
						temp2=temp2->next;
					}while(temp2!=faces[j]->startEdge);
					if(found) break;	
				}
			}
			temp1=temp1->next;
		}while(temp1!=faces[i]->startEdge);
	}


	for(int i=0; i<(int)faces.size();i++)
	{
		for(int j=0; j<4; j++)
		{
			glm::vec3 v= vertices[(faces[i]->adjV)[j]]->coord;
			faces[i]->faceVertex+=v;
		}
		
		faces[i]->faceVertex/=4;
		
	}

	for(int i=0; i<(int)faces.size();i++)
	{
		HE_edge* temp=faces[i]->startEdge;
		int j=0;
		do
		{	
			if(temp->edgeVertex==glm::vec3(0.0, 0.0, 0.0))
			{
				HE_face* rightFace=temp->pair->leftFace;
				HE_ver*  startVertex=getStartVertex(temp);
				temp->edgeVertex=faces[i]->faceVertex+
							   rightFace->faceVertex+
							   temp->endVertex->coord+
							   startVertex->coord;
				temp->edgeVertex/=4;
				temp->pair->edgeVertex=temp->edgeVertex;
			}
			
			temp=temp->next;
			j++;
		}while(temp!=faces[i]->startEdge);
	}

	for(int i=1; i<(int)vertices.size(); i++)
	{
		HE_ver* v=vertices[i];
		float valence=0.0f;
		HE_edge* e=v->startEdge;

		glm::vec3 Q=glm::vec3(0.0, 0.0, 0.0);
		glm::vec3 R=glm::vec3(0.0, 0.0, 0.0);
		do{
			HE_face* left=e->leftFace;
			Q+=left->faceVertex;
			R+=e->edgeVertex;
			valence+=1.0f;
			e=e->pair->next;
		}while(e!=v->startEdge);
		
		Q/=valence * valence;
		R = (R * 2.0f ) / valence;
		R /=valence;
		
		float ratioOfControlP=(valence-3.0f)/valence;
		v->modifiedCoord=Q+R+ratioOfControlP*(v->coord);
	}

	return;
}	


int findVertexInArray(const vector<glm::vec3>& array, const glm::vec3 & target)
{
	for(int i=1; i<(int)array.size(); i++)
	{
		if(array[i]==target) return i;
	}
	return 0;
}


void CatmullClark_Subdivision(vector<HE_face* >& faces, vector<HE_ver* >& vertices, int level)
{
	vector<glm::vec4 > faceArray;
	vector<glm::vec3> vertexArray;
	vector<HE_face* > fPreLevel;
	vector<HE_ver* > vPreLevel;

	vertexArray.push_back(glm::vec3(0.0, 0.0, 0.0));

	if(level<1 || level > 3) cout<< "cannot create subdivision !!"<<endl;
	else if(level==1)
	{
		fPreLevel=facesInitial;
		vPreLevel=verticesInitial;
	}
	else if(level==2)
	{
		fPreLevel=facesFirstLevel;
		vPreLevel=verticesFirstLevel;	
	}
	else
	{
		fPreLevel=facesSecondLevel;
		vPreLevel=verticesSecondLevel;	
	}

	for(int i=0; i<(int)fPreLevel.size(); i++)
	{
		HE_edge* e=fPreLevel[i]->startEdge;
		int j=0;
		do
		{
			int startIdx=(fPreLevel[i]->adjV)[j];
			glm::vec4 subFace;

			vector<glm::vec3> fourPoints(4);

			fourPoints[0]=vPreLevel[startIdx]->modifiedCoord;
			fourPoints[1]=e->edgeVertex;
			fourPoints[2]=fPreLevel[i]->faceVertex;
			fourPoints[3]=e->next->next->next->edgeVertex;

			for(int k=0; k<(int)fourPoints.size(); k++)
			{
				int pos;
				if((pos=findVertexInArray(vertexArray,fourPoints[k]))>0 )
					subFace[k]=pos;
				else
				{
					vertexArray.push_back(fourPoints[k]);
					subFace[k]=vertexArray.size()-1;
				}
			}
			faceArray.push_back(subFace);
			j++;
			e=e->next;
		}while(e!=fPreLevel[i]->startEdge);
	}
	populateHEDS(faceArray, vertexArray, faces, vertices);
	//cout<< "we have "<<faces.size() << " faces now!"<<endl;
	//cout<< "we have "<<vertices.size() << " vertices now!"<<endl;
	finalizeHEDS(faces, vertices);

	return;
}


void drawLetterI_using_HEDS(const vector<HE_face*  >& faces, const vector<HE_ver*  >& vertices)
{

	 glActiveTexture(GL_TEXTURE1);
	 if(mode==GL_FILL){
     	glEnable(GL_TEXTURE_2D);
     	glEnable(GL_TEXTURE_GEN_S);
     	glEnable(GL_TEXTURE_GEN_T);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
     	glDisable(GL_TEXTURE_GEN_S);
     	glDisable(GL_TEXTURE_GEN_T);
	}
    glActiveTexture(GL_TEXTURE0);
    if(mode==GL_FILL)
     	glEnable(GL_TEXTURE_2D);
    else
    	glDisable(GL_TEXTURE_2D);

     for(int i=0;i<(int)faces.size();i++)
     {
          int pos1=(faces[i]->adjV)[0];
          int pos2=(faces[i]->adjV)[1];
          int pos3=(faces[i]->adjV)[2];
          int pos4=(faces[i]->adjV)[3];

          glm::vec3 v1=vertices[pos1]->coord;  glm::vec2 tex1=vertices[pos1]->texCoord;
          glm::vec3 v2=vertices[pos2]->coord;  glm::vec2 tex2=vertices[pos2]->texCoord;
          glm::vec3 v3=vertices[pos3]->coord;  glm::vec2 tex3=vertices[pos3]->texCoord;
          glm::vec3 v4=vertices[pos4]->coord;  glm::vec2 tex4=vertices[pos4]->texCoord;

          glm::vec3 n1=vertices[pos1]->vNormal;
          glm::vec3 n2=vertices[pos2]->vNormal;
          glm::vec3 n3=vertices[pos3]->vNormal;
          glm::vec3 n4=vertices[pos4]->vNormal;
    
          glBegin(GL_QUADS); 
             
               glTexCoord2f(tex1.x,tex1.y);
               glNormal3f(n1.x,n1.y,n1.z );  
               glVertex3f(v1.x,v1.y,v1.z);

               glTexCoord2f(tex2.x,tex2.y);
               glNormal3f(n2.x,n2.y,n2.z );  
               glVertex3f(v2.x,v2.y,v2.z);

               glTexCoord2f(tex3.x,tex3.y);
               glNormal3f(n3.x,n3.y,n3.z );  
               glVertex3f(v3.x,v3.y,v3.z);

               glTexCoord2f(tex4.x,tex4.y);
               glNormal3f(n4.x,n4.y,n4.z );  
               glVertex3f(v4.x,v4.y,v4.z);
             
          glEnd();
     }
}

void mapTexture(char* filename)
{
     int width, height;
     unsigned char* pixel =SOIL_load_image(filename, &width, &height, 0, SOIL_LOAD_RGB);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
     SOIL_free_image_data(pixel);
} 

void setupReflection()
{
     glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
     glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
     glTexGeni(GL_R, GL_TEXTURE_GEN_MODE,GL_REFLECTION_MAP);
}   

void setUpMultiTexture( char* file1,  char* file2, GLuint* texID1,  GLuint* texID2)
{
     glGenTextures(1, texID1);
     glGenTextures(1, texID2);

     glActiveTexture(GL_TEXTURE0);
     glBindTexture(GL_TEXTURE_2D, *texID1);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
     mapTexture(file1);
     glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

     glActiveTexture(GL_TEXTURE1);
     glBindTexture(GL_TEXTURE_2D, *texID2);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
     mapTexture(file2);
     
     glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
     glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP); 
     glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void initializeMaterial()
{
     GLfloat mat_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
     GLfloat mat_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
     GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
     GLfloat mat_shininess[] = { 50.0 };
     glMaterialfv(GL_FRONT,GL_AMBIENT, mat_ambient);
     glMaterialfv(GL_FRONT,GL_DIFFUSE, mat_diffuse);
     glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
     glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
}

 void setUpLighting()
 {
     glEnable(GL_COLOR_MATERIAL);
     glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
     GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
     GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
     GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

     GLfloat light_position2[] = { 3.0, 1.0, 1.0, 0.0 };

     glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
     glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
     glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
     glLightfv(GL_LIGHT0, GL_POSITION, light_position);

     glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
     glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
     glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
     glLightfv(GL_LIGHT1, GL_POSITION, light_position2);
 }


void freeLevel(vector< HE_face* > faces , vector< HE_ver* > vertices)
{
	for(int i=0; i<(int)faces.size();i++)
	{
		HE_edge* e=faces[i]->startEdge;
		free(e->next->next->next);free(e->next->next);free(e->next);free(e);
	}
	for(int i=0; i<(int)vertices.size(); i++)
	{
		free(vertices[i]);
	}
}

//free all the allocated memory
void freeAllMemory()
{
	freeLevel(facesInitial, verticesInitial);
	freeLevel(facesFirstLevel,verticesFirstLevel);
	freeLevel(facesSecondLevel,verticesSecondLevel);
	freeLevel(facesThirdLevel,verticesThirdLevel);
	facesInitial.clear();facesFirstLevel.clear();
	facesSecondLevel.clear();facesThirdLevel.clear();
	verticesInitial.clear();verticesFirstLevel.clear();
	verticesSecondLevel.clear();verticesThirdLevel.clear();
}

void init(void)
{
	//  setup OpenGL environment 
	glClearColor(1.9,0.9,2.9,1.0); // clear color is gray		
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
	glPointSize(9.0);
	glEnable(GL_DEPTH_TEST);

	char file1[]="texture2.jpg";
	char file2[]="norway.jpg"; 
	//load texture from files
	setUpMultiTexture( file1, file2, &textureID , &enviTexID );

	setupReflection();
	initializeMaterial();
    setUpLighting();
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
	
	timeIncre=1.0f/(TRIP_TIME*nFPS);

	getInitialData(facesContainer, verticesContainer);
	finalizeHEDS(facesInitial,verticesInitial);

	//perform three levels of CatmullClark subdivision
	CatmullClark_Subdivision( facesFirstLevel , verticesFirstLevel  ,  1);
	CatmullClark_Subdivision( facesSecondLevel, verticesSecondLevel ,  2);
	CatmullClark_Subdivision( facesThirdLevel , verticesThirdLevel  ,  3);

	cout<< "initialization complete !"<<endl;
}


/************************Cubic Bezier Curve base functions B0, B1, B2, and B3******************************/
float B_zero_T(float t)
{
	//return (-1.0f/6.0f)*(t*t*t)+(1.0f/2.0f)*(t*t)-(1.0f/2.0f)*t+(1.0f/6.0f);
	return (1.0f-t)*(1.0f-t)*(1.0f-t);
}
float B_one_T(float t)
{
	//return (1.0f/2.0f)*(t*t*t)-t*t+(2.0f/3.0f);
	return 3.0f*(1.0f-t)*(1.0f-t)*t;
}
float B_two_T(float t)
{
	//return (-1.0f/2.0f)*(t*t*t)+(1.0f/2.0f)*(t*t)+(1.0f/2.0f)*t+(1.0f/6.0f);
	return 3.0f*(1.0f-t)*t*t;
}
float B_three_T(float t)
{
	return t*t*t;
}
/***********************************************************************************************************/

//use the Bezier curve to update the camera position
glm::vec3 getCurrentEyePosition(glm::vec3 & eyePos, float timePara)
{
	eyePos=P0*B_zero_T(timePara)+P1*B_one_T(timePara)+P2*B_two_T(timePara)+P3*B_three_T(timePara);
	return eyePos;
}

void display(void)
{
	// put OpenGL display commands:
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, mode); 

	// reset OpenGL transformation matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // reset transformation matrix to identity

	gluLookAt(eye.x,eye.y,eye.z, lookAtPos.x,  lookAtPos.y , lookAtPos.z ,0.f,1.f,0.f);
	//glRotatef(fRotateAngle,0.f,1.f,0.f);

	// Test drawing a solid teapot
	glColor3f(r,g,b); 

	if(levelOfSubdv==0) 	 drawLetterI_using_HEDS(facesInitial,verticesInitial);

	else if(levelOfSubdv==1) drawLetterI_using_HEDS(facesFirstLevel,verticesFirstLevel);

	else if(levelOfSubdv==2) drawLetterI_using_HEDS(facesSecondLevel,verticesSecondLevel);

	else 					 drawLetterI_using_HEDS(facesThirdLevel,verticesThirdLevel);
	
	glFlush();
	glutSwapBuffers();	// swap front/back framebuffer to avoid flickering 
}

void reshape (int w, int h)
{
	// reset viewport ( drawing screen ) size
	glViewport(0, 0, w, h);
	float fAspect = ((float)w)/h; 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70.f,fAspect,0.001f,30.f); 
}

void keyboard(unsigned char key, int x, int y)
{
	// put your keyboard control here
	if (key == 27) 
	{
		// ESC hit, so quit
		freeAllMemory();
		printf("demonstration finished.\n");
		exit(0);
	}
	else if(key=='w' || key=='W')
    {
       	if(levelOfSubdv==3) levelOfSubdv=0;
      	else levelOfSubdv++;
       	cout<< "Level of subdivision: "<< levelOfSubdv <<endl;
    }
    else if(key=='s' || key=='S')
    {
        if(levelOfSubdv==0) levelOfSubdv=3;
        else levelOfSubdv--;
      	cout<< "Level of subdivision: "<< levelOfSubdv <<endl;
   	} 
	else if(key=='l' || key=='L')
	{
		if(lookAtPos==lookAtOrigin) lookAtPos=lookAtVertex;
		else lookAtPos=lookAtOrigin;
	}
	else if(key=='a' || key=='A')
	{
		if(timeIncre==0.0f) timeIncre=1.0f/(TRIP_TIME*nFPS);
		else timeIncre=0.0f;
	}
	else if(key=='t' || key=='T')
	{
		if(mode==GL_FILL)
		{
			mode=GL_LINE;
			r=1.0f; g=0.0f; b=0.0f;
			glDisable(GL_TEXTURE_2D);
     		glDisable(GL_TEXTURE_GEN_S);
     		glDisable(GL_TEXTURE_GEN_T);
		} 
		else 
		{
			mode=GL_FILL;
			r=0.0f; g=1.0f; b=1.0f; 
			glEnable(GL_TEXTURE_2D);
     		glEnable(GL_TEXTURE_GEN_S);
     		glEnable(GL_TEXTURE_GEN_T);
		}
	}
	else if(key=='g' || key=='G')
	{
		if(lightOn)  {glDisable(GL_LIGHTING); lightOn=false; cout<<"light off!"<<endl;}
		else     	 {glEnable(GL_LIGHTING)  ; lightOn=true; cout<<"light on  "<<endl;}
	}
	
	else
		cout<< "invalid keyboard input !"<<endl;
	
}
void  arrowKeyFunct(int key, int x, int y)
{
     switch (key) 
     {  
     	case GLUT_KEY_UP:
     	{
        	if(levelOfSubdv==3) levelOfSubdv=0;
          	else levelOfSubdv++;
          	cout<< "Subdivision of level: "<< levelOfSubdv <<endl;
          	break;
     	}
     	case GLUT_KEY_DOWN:
     	{
     		if(levelOfSubdv==0) levelOfSubdv=3;
            else levelOfSubdv--;
          	cout<< "Subdivision of level: "<< levelOfSubdv <<endl;
          	break;
     	}     
        default: break;
    }            
}


void mouse(int button, int state, int x, int y)
{
	// process your mouse control here
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		printf("push left mouse button.\n");
}


void timer(int v)
{
	float timePara=abs(sin(2*PI*curTime));
	curTime+=timeIncre;

	getCurrentEyePosition(eye,timePara);
	glutPostRedisplay(); // trigger display function by sending redraw into message queue
	glutTimerFunc(1000/nFPS,timer,v); // restart timer again
}

int main(int argc, char* argv[])
{
	glutInit(&argc, (char**)argv);

	// set up for double-buffering & RGB color buffer & depth test
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); 
	glutInitWindowSize (1000, 600); 
	glutInitWindowPosition (100, 100);
	glutCreateWindow ((const char*)"mhu9_mp4_smooth_I");

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error %s\n", glewGetErrorString(err));
		exit(1);
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	if (GLEW_ARB_vertex_program)
		fprintf(stdout, "Status: ARB vertex programs available.\n");
	if (glewGetExtension("GL_ARB_fragment_program"))
		fprintf(stdout, "Status: ARB fragment programs available.\n");
	if (glewIsSupported("GL_VERSION_1_4  GL_ARB_point_sprite"))
		fprintf(stdout, "Status: ARB point sprites available.\n");
	cout<<"Please read the instructions in README.txt to perform any necessary operations!" <<endl;

	init(); // setting up user data & OpenGL environment
	
	// set up the call-back functions 
	glutDisplayFunc(display);  // called when drawing 
	glutReshapeFunc(reshape);  // called when change window size
	glutKeyboardFunc(keyboard); // called when received keyboard interaction
	glutSpecialFunc(arrowKeyFunct); // called when user adjusts the eye's position

	glutMouseFunc(mouse);	    // called when received mouse interaction
	glutTimerFunc(100,timer,nFPS); // a periodic timer. Usually used for updating animation
	
	glutMainLoop(); // start the main message-callback loop

	return 0;
}
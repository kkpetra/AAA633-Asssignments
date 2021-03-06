/*
 * Skeleton code for AAA633 Spring 2021
 *
 * Won-Ki Jeong, wkjeong@korea.ac.kr
 *
 */

#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include <cmath>
#include <iostream>
#include <assert.h>

#include "textfile.h"

// Input 3D volume (hard-coded)
#define FILE_NAME "../CThead_512_512_452.raw"//CThead_512_512_452.raw Bucky_32_32_32.raw
#define W 512
#define H 512
#define D 452

// OpenGL window ID
int volumeRenderingWindow;
int transferFunctionWindow;

// 0: MIP, 1: Alpha blending, 2: Iso-surface rendering using Phong illumination
GLint render_mode;

// iso-value (0~1, normalized)
GLfloat iso_value = 0.5;

// Shader program
GLuint p;

// texture object handles
GLuint objectTex, transferFunctionTex;

// Locations for shader uniform variables
GLint locObjectTex, locTransferFunctionTex;

float histogram[256];
float transferFunction[256*4];
bool transferFunctionChanged = false;

int	m_Mouse_Coord[2];
float m_Rotate[2];
float m_Zoom;
bool mouseLeftDown = false;
bool mouseRightDown = false;

//
// Global idle function
//
void idle()
{
	if (transferFunctionChanged) {
		glutSetWindow(volumeRenderingWindow);
		transferFunctionChanged = false;
		glutPostRedisplay();
	}
}

// Do not change the location of this include
#include "transferfunction.h"


//
// Loading 3D volume from a file and make a 3D texture
//
void load3Dfile(char *filename,int w,int h,int d) 
{
	FILE *f = fopen(filename, "rb");
	unsigned char *data = new unsigned char[w*h*d];
	fread(data, 1, w*h*d, f);
	fclose(f);


	glGenTextures(1, &objectTex);
	glBindTexture(GL_TEXTURE_3D, objectTex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, w, h, d, 0, GL_RED, GL_UNSIGNED_BYTE, data);


	for (int i = 0; i<256; i++) {
		histogram[i] = 0;
	}
	for (int i = 0; i < w*h*d; i++) {
		histogram[data[i]]++;
	}
	for (int i = 0; i<256; i++) {
		histogram[i] /= w*h*d;
	}

	delete[]data;
}


void init()
{
	load3Dfile(FILE_NAME, W, H, D);
	m_Zoom = 1.0;
	m_Mouse_Coord[0] = 0.0;
	m_Mouse_Coord[1] = 0.0;
	m_Rotate[0] = 0.0;
	m_Rotate[1] = 0.0;
	glUseProgram(p);

	// texture sampler location
	locObjectTex = glGetUniformLocation(p, "tex");
	locTransferFunctionTex = glGetUniformLocation(p, "transferFunction");

	//
	// Generate transfer function texture
	//
	for (int i = 0; i < 256; i++) {
		transferFunction[i * 4 + 0] = float(i) / 255.0;
		transferFunction[i * 4 + 1] = float(i) / 255.0;
		transferFunction[i * 4 + 2] = float(i) / 255.0;
		transferFunction[i * 4 + 3] = float(i) / 255.0;
	}

	glGenTextures(1, &transferFunctionTex);
	glBindTexture(GL_TEXTURE_1D, transferFunctionTex);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, transferFunction);


	//
	// Bind two textures
	//
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(locObjectTex, 0);
	glBindTexture(GL_TEXTURE_3D, objectTex);

	glActiveTexture(GL_TEXTURE0 + 1);
	glUniform1i(locTransferFunctionTex, 1);
	glBindTexture(GL_TEXTURE_1D, transferFunctionTex);
}


//
// when the window size is changed
//
void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0) h = 1;
	float ratio = 1.0f * (float) w / (float)h;

	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);
}

//
// Change render mode and iso-value
//
void keyboard(unsigned char key, int x, int y)
{	
	switch (key)
	{
		case 'p':
			// do something
			break;
		case '0':
			render_mode = 0;
			break;
		case '1':
			render_mode = 1;
			break;
		case '2':
			render_mode = 2;
			break;
		case '+':
			iso_value = fmin(1, iso_value + 0.02);
			std::cout << "Iso value: " << iso_value << std::endl;
			break;
		case '-':
			iso_value = fmax(0, iso_value - 0.02);
			std::cout << "Iso value: " << iso_value << std::endl;
			break;
		//default :
			// do nothing
	}

	glutPostRedisplay();
}


//
// Main volume rendering code
//
void renderScene(void) 
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(p);

	glUniform1i(glGetUniformLocation(p, "method"), render_mode);
	glUniform1f(glGetUniformLocation(p, "isoValue"), iso_value);
	glUniform1f(glGetUniformLocation(p, "xrot"), m_Rotate[0]);
	glUniform1f(glGetUniformLocation(p, "yrot"), m_Rotate[1]);
	glUniform1f(glGetUniformLocation(p, "zoom"), m_Zoom);
	
	glBindTexture(GL_TEXTURE_1D, transferFunctionTex);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_FLOAT, transferFunction);

	glLoadIdentity();
	//gluLookAt(0, 0, 100, 0, 0, 0, 0, 10, 0);
	glScalef(m_Zoom, m_Zoom, m_Zoom);
	glRotatef(m_Rotate[0], 1.0, 0.0, 0.0);
	glRotatef(m_Rotate[1], 0.0, 1.0, 0.0);

	// draw a quad for 3D volume rendering
	glBegin(GL_QUADS);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, 1, -1);
	glVertex3f(1, 1, -1);
	glVertex3f(1, -1, -1);
	glEnd();
	glUseProgram(0);

	glutSwapBuffers();
}

void  mouseClick(int button, int state, int x, int y) 
{
	m_Mouse_Coord[0] = x;
	m_Mouse_Coord[1] = y;

	mouseLeftDown = false;
	mouseRightDown = false;

	if (state == GLUT_DOWN)
	{
		if (button == GLUT_LEFT_BUTTON)
		{
			mouseLeftDown = true;
		}
		else if (button == GLUT_RIGHT_BUTTON)
		{
			mouseRightDown = true;
		}
	}
	glutPostRedisplay();
}

void mouseMove(int x, int y) 
{
	int diffx = x - m_Mouse_Coord[0];
	int diffy = y - m_Mouse_Coord[1];

	m_Mouse_Coord[0] = x;
	m_Mouse_Coord[1] = y;

	if (mouseLeftDown)
	{
		m_Rotate[0] += (float)0.1 * diffy;
		m_Rotate[1] += (float)0.1 * diffx;
	}
	else if (mouseRightDown)
	{
		m_Zoom += (float)0.1 * diffy;
	}

	glutPostRedisplay();
}


//
// Main function
//
int main(int argc, char **argv) {

	glutInit(&argc, argv);

	//
	// Window 1. Transfer function editor
	// 
	init_transferFunction();


	//
	// Window 2. Main volume rendering window
	// 
	
	// init GLUT and create Window
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(600,600);
	volumeRenderingWindow = glutCreateWindow("Volume Rendering Window");

	// register callbacks
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove);
	glutIdleFunc(idle);

	glEnable(GL_DEPTH_TEST);

	glewInit();
	if (glewIsSupported("GL_VERSION_3_3")) 	printf("Ready for OpenGL 3.3\n");
	else { printf("OpenGL 3.3 is not supported\n");	exit(1); }

	// Create shader program
	p = createGLSLProgram( "../volumeRendering.vert", NULL, "../volumeRendering.frag" );

	init();

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}
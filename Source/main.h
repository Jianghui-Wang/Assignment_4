//
// Created by gin on 25/03/2021.
//

#ifndef CS247_2021_MAIN_H
#define CS247_2021_MAIN_H


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fcntl.h>

#include <vector>

#include <GL/glew.h>
#ifdef __APPLE__
  // macOS ships GLUT as a framework; freeglut is unnecessary here.
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif

////////////////
// Structures //
////////////////

struct Vertex {
    float	x;
    float	y;
    float	z;
};


//////////////////////
// Global defines   //
//////////////////////

//rotation defines
#define INERTIA_THRESHOLD       1.0f
#define INERTIA_FACTOR          0.5f
#define SCALE_FACTOR            0.01f
#define SCALE_INCREMENT         0.5f
#define TIMER_FREQUENCY_MILLIS  20


//////////////////////
// Global variables //
//////////////////////

// Handle of the window we're rendering to
static GLint window;

//
// Movement variables
//
float fXDiff;		// rotation
float fYDiff;
float fZDiff;

int xLastIncr;		// last difference in mouse movement
int yLastIncr;

float fXInertia;	// inertia for keeping rotating even after mouse movement
float fYInertia;
float fXInertiaOld; // old inertia
float fYInertiaOld;

float fScale;		// scale/zoom factor

int xLast;			// last mouse position
int yLast;

char bmModifiers;	// keyboard modifiers (e.g. ctrl,...)

int Rotate;			// auto-rotate

float clear_color[ 4 ]; // clearcolor for shader

// parameters for raycasting

int enable_lighting;	// lighting parameters
int enable_gm_scaling;
float ambient;
bool mip;

float step_size;	// step size for raycasting

int tf_win_min;		// transfer function parameters
int tf_win_max;
float iso_value;
int clip;

int use_dvr;


// data handling

bool data_loaded;
unsigned short vol_dim[ 3 ];
unsigned short* data_array;

GLuint vol_texture;

GLuint tf_texture;

int view_width, view_height; // height and width of entire view

// buffers
GLuint framebuffer;
GLuint backface_buffer;
GLuint frontface_buffer;

// shaders
GLuint raycast_dvr_fragment_shader;
GLuint raycast_iso_fragment_shader;
GLuint raycast_dvr_shader_program;
GLuint raycast_iso_shader_program;

////////////////
// Prototypes //
////////////////

int printOglError( char *file, int line );

#define printOpenGLError() printOglError((char *)__FILE__, __LINE__)

static void reshape( int wid, int ht );

void LoadData( char* filename );
void LoadDataByte( char* filename );
void CreateTestData( void );

void DownloadVolumeAsTexture( void );

void UpdateTransferfunction(bool r, bool g, bool b, bool a);
void DownloadTransferFunctionTexture( int tf_id );

void RenderBackFaces(float x, float y, float z);
void RenderFrontFaces(float x, float y, float z);

void RenderRaycastPass( void );

void RenderBufferToScreen( GLuint buffer );

void enableRenderToBuffer( GLuint buffer );
void disableRenderToBuffer( void );

void initGL( void );

char *textFileRead( char *fn );
void LoadAndLinkShaders( void );


#endif //CS247_2021_MAIN_H

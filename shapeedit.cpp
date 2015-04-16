////////////////////////////////////////////////////////////////////////
//
//   Harvard Computer Science
//   CS 277: Geometric Modeling
//   Professor Steven Gortler
//
////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#if __GNUG__
#   include <tr1/memory>
#endif

#ifdef __MAC__
#   include <OpenGL/gl3.h>
#   include <GLUT/glut.h>
#else
#include <GL/glew.h>
#   include <GL/glut.h>
#endif

#include "ppm.h"
#include "glsupport.h"
#include "mesh.h"
#include "poisson.h"

using namespace std;      // for string, vector, iostream and other standard C++ stuff
using namespace std::tr1; // for shared_ptr

// G L O B A L S ///////////////////////////////////////////////////

// !!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!
// Before you start working on this assignment, set the following variable properly
// to indicate whether you want to use OpenGL 2.x with GLSL 1.0 or OpenGL 3.x+ with
// GLSL 1.3.
//
// Set g_Gl2Compatible = true to use GLSL 1.0 and g_Gl2Compatible = false to use GLSL 1.3.
// Make sure that your machine supports the version of GLSL you are using.
// Most of people can use GL 3.
//
// If g_Gl2Compatible=true, shaders with -gl2 suffix will be loaded.
// If g_Gl2Compatible=false, shaders with -gl3 suffix will be loaded.
// To complete the assignment you only need to edit the shader files that get loaded
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static const bool g_Gl2Compatible = false;


static int g_width             = 490;       // screen width
static int g_height            = 490;       // screen height
static bool g_leftClicked      = false;     // is the left mouse button down?
static bool g_rightClicked     = false;     // is the right mouse button down?
static float g_objScale        = 1.0;       // scale factor for object
static int g_leftClickX, g_leftClickY;      // coordinates for mouse left click event
static int g_rightClickX, g_rightClickY;    // coordinates for mouse right click event
extern int g_iterationsPerSecond;

// our global shader states
struct SquareShaderState {
  GlProgram program;
  GlArrayObject vao;

  // Handles to uniform variables
  GLint h_uTex;
  GLint h_uSolid;

  // Handles to vertex attributes
  GLint h_aPosition;
  GLint h_aTexCoord;
};

struct HandleShaderState {
	GlProgram program;
	GlArrayObject vao;

	// Handles to vertex attributes
	GLint h_aPosition;
};

static shared_ptr<SquareShaderState> g_squareShaderState;

// our global texture instance
static shared_ptr<GlTexture> g_tex, g_handleTex;

// our global geometries
struct GeometryPX {
  GlBufferObject posVbo, texVbo;
};

static shared_ptr<GeometryPX> g_geometry;


// a handle is a pair of an index to a vertex of the mesh and a position
static vector<handleType> g_handles;

static Mesh g_originalMesh;
static Mesh g_currentMesh;

static bool g_wireframeMode;
static vector<handleType>::iterator g_clickedHandle;
static bool g_updateScheduled = false;

// C A L L B A C K S ///////////////////////////////////////////////////



static void drawSquare(const SquareShaderState& shaderState, const GeometryPX& geometry, const GlTexture& tex, bool handle) {
  // using a VAO is necessary to run on OS X.
  glBindVertexArray(shaderState.vao);

  // bind textures
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);

  // set glsl uniform variables
  safe_glUniform1i(shaderState.h_uTex, 0); // 0 means GL_TEXTURE0
  safe_glUniform1i(shaderState.h_uSolid, 0);
  if (g_wireframeMode || handle)
    safe_glUniform1i(shaderState.h_uSolid, 1);

  // bind vertex buffers
  glBindBuffer(GL_ARRAY_BUFFER, geometry.posVbo);
  safe_glVertexAttribPointer(shaderState.h_aPosition,
                             2, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, geometry.texVbo);
  safe_glVertexAttribPointer(shaderState.h_aTexCoord,
                             2, GL_FLOAT, GL_FALSE, 0, 0);

  safe_glEnableVertexAttribArray(shaderState.h_aPosition);
  safe_glEnableVertexAttribArray(shaderState.h_aTexCoord);

  if (g_wireframeMode && !handle)
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE );

  glDrawArrays(GL_TRIANGLES, 0, 3*g_currentMesh.getNumFaces());

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  safe_glDisableVertexAttribArray(shaderState.h_aPosition);
  safe_glDisableVertexAttribArray(shaderState.h_aTexCoord);

  // bind VAO to default
  glBindVertexArray(0);

  // check for errors
  checkGlErrors();
}

static const double g_halfsquaresize = 0.015;

static void drawHandles() {
	for (auto it = g_handles.begin(); it != g_handles.end(); ++it) {
		vector<GLfloat> pos;
		const Cvec2 point0 = it->second + Cvec2(-g_halfsquaresize, g_halfsquaresize);
		const Cvec2 point1 = it->second + Cvec2(g_halfsquaresize, g_halfsquaresize);
		const Cvec2 point2 = it->second + Cvec2(-g_halfsquaresize, -g_halfsquaresize);
		const Cvec2 point3 = it->second + Cvec2(g_halfsquaresize, -g_halfsquaresize);
		pos.push_back((GLfloat)point0[0]); pos.push_back((GLfloat)point0[1]);
		pos.push_back((GLfloat)point2[0]); pos.push_back((GLfloat)point2[1]);
		pos.push_back((GLfloat)point3[0]); pos.push_back((GLfloat)point3[1]);
		pos.push_back((GLfloat)point0[0]); pos.push_back((GLfloat)point0[1]);
		pos.push_back((GLfloat)point3[0]); pos.push_back((GLfloat)point3[1]);
		pos.push_back((GLfloat)point1[0]); pos.push_back((GLfloat)point1[1]);

		GeometryPX temp;
		glBindBuffer(GL_ARRAY_BUFFER, temp.posVbo);
		glBufferData(
			GL_ARRAY_BUFFER,
			12 * sizeof(GLfloat),
			&pos[0],
			GL_STATIC_DRAW);
		checkGlErrors();

		drawSquare(*g_squareShaderState, temp, *g_tex, true);
	}
}

static pair<vector<handleType>::iterator, double> findClosestPoint(const int& xclick, const int& yclick, vector<handleType>& searchDomain) {
	Cvec2 point = Cvec2(((double) xclick) / g_width, ((double) yclick) / g_height);
	point = point * 2 - Cvec2(1.0, 1.0);
	double min_dist = 1.0/CS175_EPS2;
	vector<handleType>::iterator argmin;
	for (vector<handleType>::iterator it = searchDomain.begin(); it != searchDomain.end(); ++it) {
		const double dist = norm2(point - it->second);
		if (dist < min_dist) {
			min_dist = dist;
			argmin = it;
		}
	}
	return make_pair(argmin,min_dist);
}

static void addClosestVertexOrDeleteHandle(const int& xclick, const int& yclick) {
	vector<pair<int, Cvec2> > vertices;
	for (int i = 0; i < g_currentMesh.getNumVertices(); ++i)
		vertices.push_back(make_pair(i, Cvec2(g_currentMesh.getVertex(i).getPosition())));
	pair<vector<handleType>::iterator, double> closestVertex = findClosestPoint(xclick, yclick, vertices);
	pair<vector<handleType>::iterator, double> closestHandle = findClosestPoint(xclick, yclick, g_handles);
	if (g_handles.empty() || closestVertex.second < closestHandle.second)
		g_handles.push_back(*(closestVertex.first));
	else
		g_handles.erase(closestHandle.first);
}

static void evolveCallback(int whatever) {
	if (g_updateScheduled = !doIteration(g_currentMesh, g_handles))
		glutTimerFunc(1000 / g_iterationsPerSecond, evolveCallback, 0);
	glutPostRedisplay();
}

static void moveClosestHandle(const int& prevClickX, const int& prevClickY, const int& newClickX, const int& newClickY) {
	if (g_handles.empty()) return;
	Cvec2 displacement = Cvec2(2.0*((double)(newClickX - prevClickX)) / g_width, 2.0*((double)(newClickY - prevClickY)) / g_height);
	g_clickedHandle->second += displacement;
	afterMove(g_currentMesh, g_handles);
	if (!g_updateScheduled) evolveCallback(0);
}

static void loadMeshGeometry(Mesh& m, GeometryPX& g) {
	vector<GLfloat> pos, tex;
	for (int i = 0; i < m.getNumFaces(); ++i) {
		const Mesh::Face f = m.getFace(i);
		for (int j = 0; j < f.getNumVertices(); ++j) {
			const Mesh::Vertex v = f.getVertex(j);
			pos.push_back((GLfloat)v.getPosition()[0]);
			pos.push_back((GLfloat)v.getPosition()[1]);
			tex.push_back((GLfloat)v.getTexCoords()[0]);
			tex.push_back((GLfloat)v.getTexCoords()[1]);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, g.posVbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		(pos.size())*sizeof(GLfloat),
		&pos[0],
		GL_STATIC_DRAW);
	checkGlErrors();

	glBindBuffer(GL_ARRAY_BUFFER, g.texVbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		(tex.size())*sizeof(GLfloat),
		&tex[0],
		GL_STATIC_DRAW);
	checkGlErrors();
}

// _____________________________________________________
//|                                                     |
//|  display                                            |
//|_____________________________________________________|
///
///  Whenever OpenGL requires a screen refresh
///  it will call display() to draw the scene.
///  We specify that this is the correct function
///  to call with the glutDisplayFunc() function
///  during initialization

static void display(void) {
  glUseProgram(g_squareShaderState->program);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  loadMeshGeometry(g_currentMesh, *g_geometry);
  drawSquare(*g_squareShaderState, *g_geometry, *g_tex, false);
  drawHandles();

  glutSwapBuffers();

  // check for errors
  checkGlErrors();
}


// _____________________________________________________
//|                                                     |
//|  reshape                                            |
//|_____________________________________________________|
///
///  Whenever a window is resized, a "resize" event is
///  generated and glut is told to call this reshape
///  callback function to handle it appropriately.

static void reshape(int w, int h) {
  g_width = w;
  g_height = h;
  glViewport(0, 0, w, h);
  glutPostRedisplay();
}


// _____________________________________________________
//|                                                     |
//|  mouse                                              |
//|_____________________________________________________|
///
///  Whenever a mouse button is clicked, a "mouse" event
///  is generated and this mouse callback function is
///  called to handle the user input.

static void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      // right mouse button has been clicked
      g_leftClicked = true;
      g_leftClickX = x;
      g_leftClickY = g_height - y - 1;
	  g_clickedHandle = (findClosestPoint(g_leftClickX, g_leftClickY, g_handles)).first;
    }
    else {
      // right mouse button has been released
      g_leftClicked = false;
    }
  }
  if (button == GLUT_RIGHT_BUTTON) {
	  if (state == GLUT_DOWN) {
		  // right mouse button has been clicked
		  g_rightClicked = true;
		  g_rightClickX = x;
		  g_rightClickY = g_height - y - 1;
		  addClosestVertexOrDeleteHandle(g_rightClickX, g_rightClickY);
	  }
	  else {
		  // right mouse button has been released
		  g_rightClicked = false;
	  }
  }
  glutPostRedisplay();
}


// _____________________________________________________
//|                                                     |
//|  motion                                             |
//|_____________________________________________________|
///
///  Whenever the mouse is moved while a button is pressed,
///  a "mouse move" event is triggered and this callback is
///  called to handle the event.

static void motion(int x, int y) {
  const int newx = x;
  const int newy = g_height - y - 1;
  if (g_leftClicked) {
	moveClosestHandle(g_leftClickX, g_leftClickY, newx, newy);
    g_leftClickX = newx;
    g_leftClickY = newy;
  }
  glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y) {
  switch (key) {
  case 'h':
    cout << " ============== H E L P ==============\n\n"
    << "h\t\thelp menu\n"
    << "s\t\tsave screenshot\n"
	<< "w\t\ttoggle wireframe mode\n"
    << "\t\tright click to add or remove handles\n"
	<< "\t\tleft click drag to move handles\n" << endl;
    break;
  case 'q':
    exit(0);
  case 's':
    glFinish();
    writePpmScreenshot(g_width, g_height, "out.ppm");
    break;
  case 'w':
    g_wireframeMode = !g_wireframeMode;
  }
  glutPostRedisplay();
}

// H E L P E R    F U N C T I O N S ////////////////////////////////////
static void initGlutState(int argc, char **argv) {
  glutInit(&argc,argv);
#ifdef __MAC__
  glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH); // core profile flag is required for GL 3.2 on Mac
#else
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);  //  RGBA pixel channels and double buffering
#endif
  glutInitWindowSize(g_width, g_height);      // create a window
  glutCreateWindow("CS 175: Hello World");    // title the window

  glutDisplayFunc(display);                   // display rendering callback
  glutReshapeFunc(reshape);                   // window reshape callback
  glutMotionFunc(motion);                     // mouse movement callback
  glutMouseFunc(mouse);                       // mouse click callback
  glutKeyboardFunc(keyboard);
}

static void initGLState() {
  glClearColor(255./255,255./255,1,0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  if (!g_Gl2Compatible)
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void loadSquareShader(SquareShaderState& ss) {
  const GLuint h = ss.program; // short hand

  if (!g_Gl2Compatible) {
    readAndCompileShader(ss.program, "shaders/sq-tex-gl3.vshader", "shaders/sq-tex-gl3.fshader");
  }
  else {
    readAndCompileShader(ss.program, "shaders/sq-tex-gl2.vshader", "shaders/sq-tex-gl2.fshader");
  }

  // Retrieve handles to uniform variables
  ss.h_uTex = safe_glGetUniformLocation(h, "uTex");
  ss.h_uSolid = safe_glGetUniformLocation(h, "uSolid");

  // Retrieve handles to vertex attributes
  ss.h_aPosition = safe_glGetAttribLocation(h, "aPosition");
  ss.h_aTexCoord = safe_glGetAttribLocation(h, "aTexCoord");

  if (!g_Gl2Compatible)
    glBindFragDataLocation(h, 0, "fragColor");
  checkGlErrors();
}

static void initShaders() {
  g_squareShaderState.reset(new SquareShaderState);
  loadSquareShader(*g_squareShaderState);
}




static void initMeshAndGeometry() {
  g_originalMesh.load("square.mesh");
  g_currentMesh = g_originalMesh;
  g_geometry.reset(new GeometryPX());
  loadMeshGeometry(g_currentMesh, *g_geometry);
}

static void loadTexture(GLuint texHandle, const char *ppmFilename) {
  int texWidth, texHeight;
  vector<PackedPixel> pixData;

  ppmRead(ppmFilename, texWidth, texHeight, pixData);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandle);
  glTexImage2D(GL_TEXTURE_2D, 0, g_Gl2Compatible ? GL_RGB : GL_SRGB, texWidth, texHeight,
               0, GL_RGB, GL_UNSIGNED_BYTE, &pixData[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  checkGlErrors();
}

static void initTextures() {
  g_tex.reset(new GlTexture());
  loadTexture(*g_tex, "yummy.ppm");
}



// M A I N /////////////////////////////////////////////////////////////

// _____________________________________________________
//|                                                     |
//|  main                                               |
//|_____________________________________________________|
///
///  The main entry-point for the HelloWorld example
///  application.



int main(int argc, char **argv) {
  try {
    initGlutState(argc,argv);

#ifndef __MAC__
	glewInit(); // load the OpenGL extensions
#endif

    cout << (g_Gl2Compatible ? "Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.3") << endl;

#ifndef __MAC__
	if ((!g_Gl2Compatible) && !GLEW_VERSION_3_0)
		throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.3");
	else if (g_Gl2Compatible && !GLEW_VERSION_2_0)
		throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.0");
#endif

    initGLState();
    initShaders();
    initMeshAndGeometry();
    initTextures();

	initPoisson(g_currentMesh);

    glutMainLoop();
    return 0;
  }
  catch (const runtime_error& e) {
    cout << "Exception caught: " << e.what() << endl;
    return -1;
  }
}

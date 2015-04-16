// Based loosly on the first triangle OpenGL tutorial
// http://www.opengl.org/wiki/Tutorial:_OpenGL_3.1_The_First_Triangle_%28C%2B%2B/Win%29
// This program will render two triangles
// Most of the OpenGL code for dealing with buffer objects, etc has been moved to a 
// utility library, to make creation and display of mesh objects as simple as possible

// Windows specific: Uncomment the following line to open a console window for debug output
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#define _CRT_SECURE_NO_DEPRECATE
#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include "md2model.h"


using namespace std;

bool flip = false;
#define DEG_TO_RAD 0.017453293

// Globals
// Real programs don't use globals :-D
// Data would normally be read from files
GLuint cubeVertCount = 8;
GLfloat cubeVerts[] =
{
	-0.5, -0.5f, -0.5f,
	-0.5, 0.5f, -0.5f,
	0.5, 0.5f, -0.5f,
	0.5, -0.5f, -0.5f,
	-0.5, -0.5f, 0.5f,
	-0.5, 0.5f, 0.5f,
	0.5, 0.5f, 0.5f,
	0.5, -0.5f, 0.5f
};
GLfloat cubeTexCoords[] =
{
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f 
};
GLfloat cubeColours[] = 
{
	0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 1.0f 
};

GLuint cubeIndexCount = 36;

GLuint cubeIndices[] = 
{
	0, 1, 2, 0, 2, 3, // back  
	1, 0, 5, 0, 4, 5, // left					
	6, 3, 2, 3, 6, 7, // right
	1, 5, 6, 1, 6, 2, // top
	0, 3, 4, 3, 7, 4, // bottom
	6, 5, 4, 7, 6, 4  // front
}; 

//Lighting
rt3d::lightStruct light0 =
{
	{ 0.2f, 0.2f, 0.2f, 1.0f }, // ambient
	{ 0.7f, 0.7f, 0.7f, 1.0f }, // diffuse
	{ 0.8f, 0.8f, 0.8f, 1.0f }, // specular
	{ 0.0f, 0.0f, -4.0f, 1.0f }  // position
};
rt3d::materialStruct material0 = 
{
	{ 0.4f, 0.2f, 0.2f, 1.0f }, // ambient
	{ 0.8f, 0.5f, 0.5f, 1.0f }, // diffuse
	{ 1.0f, 0.8f, 0.8f, 1.0f }, // specular
	2.0f  // shininess
};
rt3d::materialStruct material1 =
{
	{ 0.4f, 0.4f, 0.4f, 1.0f }, // ambient
	{ 0.2f, 0.2f, 0.2f, 1.0f }, // diffuse
	{ 0.8f, 0.8f, 0.8f, 1.0f }, // specular
	1.0f // shininess
};
rt3d::materialStruct material2 = 
{	// material with transparency
	{ 0.2f, 1.0f, 0.2f, 1.0f }, // ambient
	{ 0.8f, 0.5f, 0.5f, 1.0f }, // diffuse
	{ 1.0f, 0.8f, 0.8f, 1.0f }, // specular
	2.0f  // shininess
};

GLfloat rot = 0.0f;

GLuint meshIndexCount = 0;
GLuint md2VertCount = 0;
GLuint meshObjects[2];

GLuint skybox[6];
GLuint textures[2];

GLuint shaderProgram;
GLuint skyboxProgram;

// Creating a Stack
stack<glm::mat4> mvStack;

//Player
glm::vec3 playerPos(1.0f, 1.2f, -5.0f);

//Camera
glm::vec3 eye(0.0f, 1.0f, 0.0f);
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);

glm::vec4 lightPos(-10.0f, 10.0f, 10.0f, 1.0f); //light position

// md2 stuff
md2model tmpModel;
int currentAnim = 0;


// Set up rendering context
SDL_Window * setupRC(SDL_GLContext &context)
{
	SDL_Window * window;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) // Initialize video
		rt3d::exitFatalError("Unable to initialize SDL");

	// Request an OpenGL 3.0 context.
	// Not able to use SDL to choose profile (yet), should default to core profile on 3.2 or later
	// If you request a context not supported by your drivers, no OpenGL context will be created

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // Turn on x4 multisampling anti-aliasing (MSAA)

	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering

	// Create 800x600 window
	window = SDL_CreateWindow("SDL/GLM/OpenGL Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1200, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (!window) // Check window was created OK
		rt3d::exitFatalError("Unable to create window");

	context = SDL_GL_CreateContext(window); // Create opengl context and attach to window
	SDL_GL_SetSwapInterval(1); // set swap buffers to sync with monitor's vertical refresh rate
	return window;
}

//Texture Loading function. *can be improved*
GLuint loadBitmap(char *fname)
{
	//To generate a ID
	GLuint textureID;
	glGenTextures(1, &textureID);
	//Load file
	SDL_Surface *tmpSurface;
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface)
	{
		cout << "Error Loading .bmp" << endl;
	}

	//Bind textures and set perameters
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;
	GLuint externalFormat, internalFormat;
	if (format->Amask)
	{
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGRA;
	}
	else
	{
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tmpSurface->w, tmpSurface->h, 0, externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(tmpSurface);

	return textureID;
}

void init()
{
	// For this simple example we'll be using the most basic of shader programs	
	skyboxProgram = rt3d::initShaders("textured.vert", "textured.frag");
	rt3d::setMaterial(skyboxProgram, material0);
	shaderProgram = rt3d::initShaders("phong-tex.vert", "phong-tex.frag");
	rt3d::setLight(shaderProgram, light0);
	rt3d::setMaterial(shaderProgram, material0);

	//textures[0] = loadBitmap("fabric.bmp");
	textures[1] = loadBitmap("studdedmetal.bmp");

	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;
	vector<GLuint> indices;
	rt3d::loadObj("cube.obj", verts, norms, tex_coords, indices);
	GLuint size = indices.size();
	meshIndexCount = size;
	textures[0] = loadBitmap("bottom.bmp");

	meshObjects[0] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), size, indices.data());
	
	textures[2] = loadBitmap("yoshi.bmp");
	meshObjects[1] = tmpModel.ReadMD2Model("yoshi.md2");
	md2VertCount = tmpModel.getVertDataSize();


	skybox[0] = loadBitmap("valley_front.bmp");
	skybox[1] = loadBitmap("valley_left.bmp");
	skybox[2] = loadBitmap("valley_back.bmp");
	skybox[3] = loadBitmap("valley_right.bmp");
	skybox[4] = loadBitmap("valley_top.bmp");
	skybox[5] = loadBitmap("skyBottom.bmp");
	//For setting the light and texture using the RT3D library
	rt3d::setLight(shaderProgram, light0);

	// Going to create our mesh objects here
	//meshObjects[0] = rt3d::createMesh(cubeVertCount, cubeVerts, nullptr, cubeVerts, cubeTexCoords, cubeIndexCount, cubeIndices);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d)
{
	return glm::vec3(pos.x + d*sin(angle*DEG_TO_RAD), pos.y, pos.z - d*cos(angle*DEG_TO_RAD));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d)
{
	return glm::vec3(pos.x + d*cos(angle*DEG_TO_RAD), pos.y, pos.z + d*sin(angle*DEG_TO_RAD));
}

void update()
{
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_W])
	{
		playerPos = moveForward(playerPos, -rot, 0.1);
		currentAnim = 1;	
	}
	else
		currentAnim = 0;
	if (keys[SDL_SCANCODE_S])	
	{
		playerPos = moveForward(playerPos, -rot - 180, 0.1);
		currentAnim = 1;
	}
	if (keys[SDL_SCANCODE_A])	
	{
		playerPos = moveForward(playerPos, -rot - 90, 0.1);
		currentAnim = 1;
	}
	if (keys[SDL_SCANCODE_D])	
	{
		playerPos = moveForward(playerPos, -rot + 90, 0.1);
		currentAnim = 1;
	}
	if (keys[SDL_SCANCODE_SPACE])
	{
		currentAnim = 6;
	}

	if (keys[SDL_SCANCODE_R]) eye.y += 0.1f;
	if (keys[SDL_SCANCODE_F]) eye.y -= 0.1f;
	
	if (keys[SDL_SCANCODE_Q]) rot += 1.0f;
	if (keys[SDL_SCANCODE_E]) rot -= 1.0f;
	
	if (keys[SDL_SCANCODE_1]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
	}
	if (keys[SDL_SCANCODE_2]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}
	if (keys[SDL_SCANCODE_Z]) {
		if (--currentAnim < 0) currentAnim = 19;
		cout << "Current animation: " << currentAnim << endl;
	}
	if (keys[SDL_SCANCODE_X]) {
		if (++currentAnim >= 20) currentAnim = 0;
		cout << "Current animation: " << currentAnim << endl;
	}

	if (keys[SDL_SCANCODE_I]) lightPos[2] -= 0.1f;
	if (keys[SDL_SCANCODE_J]) lightPos[0] -= 0.1f;
	if (keys[SDL_SCANCODE_K]) lightPos[2] += 0.1f;
	if (keys[SDL_SCANCODE_L]) lightPos[0] += 0.1f;
	if (keys[SDL_SCANCODE_ESCAPE])
		exit(0);
}

void draw(SDL_Window * window) 
{
	// clear the screen
	glEnable(GL_CULL_FACE);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection(1.0);
	projection = glm::perspective(60.0f, 1200.0f / 600.0f, 1.0f, 50.0f);
	rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));
	
	GLfloat scale(1.0f); // just to allow easy scaling of complete scene

	glm::mat4 modelview(1.0);
	mvStack.push(modelview);

	at = glm::vec3(playerPos.x + 3 * sin(rot*DEG_TO_RAD), playerPos.y + 1.5, playerPos.z + 3 * cos(rot*DEG_TO_RAD));
	eye = moveForward(at, -rot, -1);	
	mvStack.top() = glm::lookAt(eye, at, up);

	

	mvStack.top() = glm::lookAt(eye, at, up);

	// draw a skybox
	glUseProgram(skyboxProgram);
	rt3d::setUniformMatrix4fv(skyboxProgram, "projection", glm::value_ptr(projection));

	glDepthMask(GL_FALSE); // make sure depth test is off
	glm::mat3 mvRotOnlyMat3 = glm::mat3(mvStack.top());
	mvStack.push(glm::mat4(mvRotOnlyMat3));

	// front
	mvStack.push(mvStack.top());
	glBindTexture(GL_TEXTURE_2D, skybox[0]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 20.0f, 20.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, 0.0f, -2.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// left
	mvStack.push(mvStack.top());
	glBindTexture(GL_TEXTURE_2D, skybox[1]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 20.0f, 20.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-2.0f, 0.0f, 0.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// back
	mvStack.push(mvStack.top());
	glBindTexture(GL_TEXTURE_2D, skybox[2]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 20.0f, 20.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, 0.0f, 2.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// right
	mvStack.push(mvStack.top());
	glBindTexture(GL_TEXTURE_2D, skybox[3]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 20.0f, 20.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(2.0f, 0.0f, 0.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// top
	mvStack.push(mvStack.top());
	glBindTexture(GL_TEXTURE_2D, skybox[4]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 20.0f, 20.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, 2.0f, 0.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	//bottom
	mvStack.push(mvStack.top());
	glBindTexture(GL_TEXTURE_2D, skybox[5]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 20.0f, 20.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, -2.0f, 0.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	mvStack.pop();
	glDepthMask(GL_TRUE); // make sure depth test is on

	glUseProgram(shaderProgram);
	
	//at = glm::vec3(playerPos.x + 3 * sin(rot*DEG_TO_RAD), playerPos.y + 1.5, playerPos.z + 3 * cos(rot*DEG_TO_RAD));
	//mvStack.top() = glm::lookAt(eye, at, up);


	glm::vec4 tmp = mvStack.top()*lightPos;
	//light0.position[0] = tmp.x;
	//light0.position[1] = tmp.y;
	//light0.position[2] = tmp.z;
	rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));

	// Animate the md2 model, and update the mesh with new vertex data
	tmpModel.Animate(currentAnim, 0.1);
	rt3d::updateMesh(meshObjects[1], RT3D_VERTEX, tmpModel.getAnimVerts(), tmpModel.getVertDataSize());

	// draw the Player
	glCullFace(GL_FRONT);
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	rt3d::materialStruct tmpMaterial1 = material1;
	rt3d::setMaterial(shaderProgram, tmpMaterial1);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(playerPos));
	mvStack.top() = glm::rotate(mvStack.top(), 90.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
	mvStack.top() = glm::rotate(mvStack.top(), 90.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	mvStack.top() = glm::rotate(mvStack.top(), rot, glm::vec3(0.0f, 0.0f, 1.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(scale*0.05, scale*0.05, scale*0.05));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawMesh(meshObjects[1], md2VertCount, GL_TRIANGLES);
	mvStack.pop();
	glCullFace(GL_BACK);

	//ground surface
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	rt3d::setMaterial(shaderProgram, material2);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, -0.1f, -5.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 0.1f, 20.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], cubeIndexCount, GL_TRIANGLES);
	mvStack.pop();

	//staggered cubes
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	rt3d::materialStruct tmpMaterial = material0;
	for (int a = 0; a < 6; a++){
		for (int b = 0; b < 6; b++){
			tmpMaterial.ambient[0] = a*0.33f;
			tmpMaterial.ambient[1] = b*0.33f;
			tmpMaterial.diffuse[0] = a*0.33f;
			tmpMaterial.diffuse[1] = b*0.33f;
			tmpMaterial.specular[0] = a*0.33f;
			tmpMaterial.specular[1] = b*0.33f;
			rt3d::setMaterial(shaderProgram, tmpMaterial);
			mvStack.push(mvStack.top());
			mvStack.top() = glm::translate(mvStack.top(), glm::vec3((a*3.0f) - 6.0f, -0.9f, (b*3.0f) - 12.0f));
			mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.0f, 1.0f*(a + 0.1f), scale));
			rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
			rt3d::drawIndexedMesh(meshObjects[0], cubeIndexCount, GL_TRIANGLES);
			mvStack.pop();
		}
	}
	mvStack.pop();
	glDepthMask(GL_TRUE);
	SDL_GL_SwapWindow(window); // swap buffers
}


// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {
	SDL_Window * hWindow; // window handle 
	SDL_GLContext glContext; // OpenGL context handle
	hWindow = setupRC(glContext); // Create window and render context 

	// Required on Windows *only* init GLEW to access OpenGL beyond 1.1
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) { // glewInit failed, something is seriously wrong
		std::cout << "glewInit failed, aborting." << endl;
		exit(1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();

	bool running = true; // set running to true
	SDL_Event sdlEvent;  // variable to detect SDL events
	while (running)	{	// the event loop
		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_QUIT)
				running = false;
		}
		update();
		draw(hWindow); // call the draw function
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(hWindow);
	SDL_Quit();
	return 0;
}
#include <iostream>
#include <vector>
#include <random>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Camera.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"

#include "HelicopterKeyframe.h"
#include "SplineMatrix.h"

using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from

int keyPresses[256] = {0}; // only for English keyboards!

shared_ptr<Program> prog;
shared_ptr<Camera> camera;

vector<shared_ptr<Shape>> helicopterMeshes;
vector<shared_ptr<HelicopterKeyframe>> helicopterVec;

shared_ptr<HelicopterKeyframe> interpolatedHelicopter; // this will be the main helicopter

vector<glm::vec3> cps; // Control point vector
vector<glm::quat> quats;

SplineType type = CATMULL_ROM;

vector<SplineMatrix> splineMatrices;

bool showKeyframes = true; // used to toggle keyframes on/off

enum SpeedType
{
	LENGTH_DEPENDENT=0,
	CONSTANT,
	EASEIN,
	CONTROL
};

int currSpeed = LENGTH_DEPENDENT;

bool helicopterCam = false;

vector<pair<float,float> > usTable;
float smax;
float tmax = 5.0f;

void initControlPoints(){
	// First control point must be (0,0,0)
	cps.push_back(glm::vec3(0,0,0));

	cps.push_back(glm::vec3(-1, 5, -3));
	cps.push_back(glm::vec3(-4,-1, 0));
	cps.push_back(glm::vec3(-4, -4, 2));
	
	cps.push_back(glm::vec3(2,-2,0.5));
}

// Create a lookup table that will contain corresponding u and s
void buildTable()
{
	usTable.clear();
	// Creates the appropriate matrix:
	glm::mat4 B = getSplineMatrix(splineMatrices, type);
	usTable.push_back(make_pair(0.0f, 0.0f));

	// Compute using approximations:
	for (int i = 0; i < cps.size(); i++){	

		glm::mat4 G(0);
		G[0] = glm::vec4(cps.at((0 + i) % cps.size()), 0);
		G[1] = glm::vec4(cps.at((1 + i) % cps.size()), 0);
		G[2] = glm::vec4(cps.at((2 + i) % cps.size()), 0);
		G[3] = glm::vec4(cps.at((3 + i) % cps.size()), 0);

		float ua = 0.0f;
		for (float ub = 0.2f; ub <= 1.0f; ub = ub + 0.2f){
			glm::vec4 uaVec(1, ua, ua * ua, ua * ua * ua);
			glm::vec4 ubVec(1, ub, ub * ub, ub * ub * ub);
			glm::vec3 P_ua = G * B * uaVec;
			glm::vec3 P_ub = G * B * ubVec;
			float s = glm::length(P_ua - P_ub);
			usTable.push_back(make_pair(ub + i, s + usTable.at(usTable.size()-1).second));
			ua += 0.2f;
		}
	}
	// u is first, s is second
	smax = usTable.at(usTable.size()-1).second;

}

// Interpolate between the values of u given an s value
float s2u(float s)
{
	for (int i = 0; i < usTable.size() - 1; i++){
		if (s > usTable.at(i).second && s < usTable.at(i + 1).second){
			float u0 = usTable.at(i).first;
			float s0 = usTable.at(i).second;
			
			float u1 = usTable.at(i+1).first;
			float s1 = usTable.at(i+1).second;

			// Use linear interpolation to approximate the value of the corresponding u
			float alpha = (s - s0)/(s1 - s0);
			float u = (1 - alpha) * u0 + alpha * u1;

			return u;
		}
	}
	return -1.0f;
}


static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// What to do at certain key presses:
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if(action == GLFW_PRESS) {
		switch(key) {
			case GLFW_KEY_K:
				showKeyframes = !showKeyframes;
				break;
			case GLFW_KEY_S:
				currSpeed = (currSpeed + 1) % 4; 
				cout << "Mode set to: ";
				switch(currSpeed){
					case (LENGTH_DEPENDENT):
						cout << "no arc-length.\n";
						break;
					case (CONSTANT):
						cout << "with arc-length.\n";
						break;
					case (EASEIN):
						cout << "ease in/out.\n";
						break;
					case (CONTROL):
						cout << "custom function.\n";
						break;
				}
				break;
			case GLFW_KEY_SPACE:
				helicopterCam = !helicopterCam;
				break;
		}
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyPresses[key]++;
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved(xmouse, ymouse);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = mods & GLFW_MOD_SHIFT;
		bool ctrl  = mods & GLFW_MOD_CONTROL;
		bool alt   = mods & GLFW_MOD_ALT;
		camera->mouseClicked(xmouse, ymouse, shift, ctrl, alt);
	}
}

static void init()
{
	GLSL::checkVersion();
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);

	keyPresses[(unsigned)'c'] = 1;
	
	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "phong_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addUniform("lightPos");
	prog->addUniform("ka");
	prog->addUniform("kd");
	prog->addUniform("ks");
	prog->addUniform("s");
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->setVerbose(false);
	
	camera = make_shared<Camera>();

	// Load and initialize the necessary helicopter meshes
	std::shared_ptr<Shape> body0;
	std::shared_ptr<Shape> body1;
	std::shared_ptr<Shape> prop0;
	std::shared_ptr<Shape> prop1;

	// new code
	body0 = std::make_shared<Shape>();
	body0->loadMesh(RESOURCE_DIR + "helicopter_body1.obj");
	body0->init();

	body1 = std::make_shared<Shape>();
	body1->loadMesh(RESOURCE_DIR + "helicopter_body2.obj");
	body1->init();

	prop0 = std::make_shared<Shape>();
	prop0->loadMesh(RESOURCE_DIR + "helicopter_prop1.obj");
	prop0->init();

	prop1 = std::make_shared<Shape>();
	prop1->loadMesh(RESOURCE_DIR + "helicopter_prop2.obj");
	prop1->init();
	
	// Store the meshes in the helicopter vector
	// MUST BE DONE IN SPECIFIC ORDER IN ACCORDANCE TO HELICOPTER.CPP
	helicopterMeshes.push_back(body0);
	helicopterMeshes.push_back(body1);
	helicopterMeshes.push_back(prop0);
	helicopterMeshes.push_back(prop1);

	addMatrices(splineMatrices);
	initControlPoints();
	buildTable();

	//Initialize the keyframed helicopters
	for (int i = 0; i < cps.size(); i++){
		shared_ptr<HelicopterKeyframe> h = make_shared<HelicopterKeyframe>(cps.at(i), glm::angleAxis((float)(90.0f/180.0f*M_PI * (float)i), glm::vec3(1, 0, 1)));
		helicopterVec.push_back(h);
		quats.push_back(helicopterVec.at(i)->quat);
	}

	// Initialize the interpolated helicopter
	interpolatedHelicopter = make_shared<HelicopterKeyframe>(cps.at(0), glm::angleAxis(0.0f, glm::vec3(1, 1, 1)));
	interpolatedHelicopter->propSpin = true;
	
	// Initialize time.
	glfwSetTime(0.0);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

float maxValForT = 0.0f;

void render()
{
	// Update time.
	double t = glfwGetTime();
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	// Use the window size for camera.
	glfwGetWindowSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyPresses[(unsigned)'c'] % 2) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyPresses[(unsigned)'z'] % 2) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();

	camera->applyViewMatrix(MV);

	prog->bind();

	float u = 0.0f;
	float k = 0.0f;

	// Draw the interpolated helicopter:
	if (currSpeed == LENGTH_DEPENDENT){
		float umax = helicopterVec.size();
		u = std::fmod(t, umax);

	}else if (currSpeed == CONSTANT){
		float tNorm = std::fmod(t, tmax) / tmax;
		float sNorm = tNorm;
		float s = smax * sNorm;
		u = s2u(s);

	}else if (currSpeed == EASEIN){
		float tNorm = std::fmod(t, tmax) / tmax;

		float sNorm = -2 * pow(tNorm, 3) + 3 * pow(tNorm, 2);
		
		float s = smax * sNorm;

		u = s2u(s);

	}else if (currSpeed == CONTROL){
		float tNorm = std::fmod(t, tmax * 1.5f) / (tmax * 1.5f);

		// Note: tNorm and sNorm go from 0 to 1

		// Fill A COLUMN BY COLUMN:
		glm::mat4 A;

		A[0] = glm::vec4(pow((0.0f), 3.0f),pow((0.3f), 3.0f), pow((0.4f), 3.0f),pow((1.0f), 3.0f));
		A[1] = glm::vec4(pow((0.0f), 2.0f),pow((0.3f), 2.0f), pow((0.4f), 2.0f),pow((1.0f), 2.0f));
		A[2] = glm::vec4(0, 0.3, 0.4f, 1.0f);
		A[3] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		
		glm::vec4 b = glm::vec4(0.0f, 0.6, 0.5, 1.0f);

		// Solve for x by multiplying A^(-1) * b
		glm::vec4 x = glm::inverse(A) * b;

		float sNorm = x[0]*tNorm*tNorm*tNorm + x[1]*tNorm*tNorm + x[2]*tNorm + x[3];
		float s = smax * sNorm;

		u = s2u(s);
	}

	k = floor(u);
	
	float u_hat = u - k; //u_hat is between 0 and 1

	glm::mat4 G;
	glm::mat4 B = getSplineMatrix(splineMatrices, type);
	glm::vec4 uVec(1.0f, u_hat, u_hat * u_hat, u_hat * u_hat * u_hat);

	for (int i = 1; i < 4; i++){
		if (dot(quats.at((i + int(k)) % quats.size()), quats.at((i + 1 + int(k)) % quats.size())) <= 0.0f){
			quats.at((i + 1 + int(k)) % quats.size()) = -quats.at((i + 1 + int(k)) % quats.size());
		}
	}

	// Define G for rotations
	G[0] = glm::vec4(quats.at((0 + int(k)) % quats.size()).x, quats.at((0 + int(k)) % quats.size()).y, quats.at((0 + int(k)) % quats.size()).z, quats.at((0 + int(k)) % quats.size()).w);
	G[1] = glm::vec4(quats.at((1 + int(k)) % quats.size()).x, quats.at((1 + int(k)) % quats.size()).y, quats.at((1 + int(k)) % quats.size()).z, quats.at((1 + int(k)) % quats.size()).w);
	G[2] = glm::vec4(quats.at((2 + int(k)) % quats.size()).x, quats.at((2 + int(k)) % quats.size()).y, quats.at((2 + int(k)) % quats.size()).z, quats.at((2 + int(k)) % quats.size()).w);
	G[3] = glm::vec4(quats.at((3 + int(k)) % quats.size()).x, quats.at((3 + int(k)) % quats.size()).y, quats.at((3 + int(k)) % quats.size()).z, quats.at((3 + int(k)) % quats.size()).w);

	glm::vec4 qVec = G * (B * uVec);
	glm::quat q(qVec[3], qVec[0], qVec[1], qVec[2]); // Constructor argument order: (w, x, y, z)
	glm::mat4 E = glm::mat4_cast(glm::normalize(q)); // Creates a rotation matrix

	// Define G for translations
	G[0] = glm::vec4(cps.at((0 + (int)k) % cps.size()), 0);
	G[1] = glm::vec4(cps.at((1 + (int)k) % cps.size()), 0);
	G[2] = glm::vec4(cps.at((2 + (int)k) % cps.size()), 0);
	G[3] = glm::vec4(cps.at((3 + (int)k) % cps.size()), 0);

	glm::vec3 pos = G * B * uVec;

	E[3] = glm::vec4(pos, 1.0f); // Puts the position into the last column

	if (helicopterCam){
		MV->rotate(-M_PI/2.0f, 0, 1, 0);
		MV->multMatrix(glm ::inverse(E));
	}

	// Draw the keyframed helicopters
	if (showKeyframes){
		for (int i = 0; i < helicopterVec.size(); i++){
			helicopterVec.at(i)->drawHelicopter(prog, P, MV, helicopterMeshes, t);
		}
	}

	// Draw the interpolated helicopter
	MV->pushMatrix();

		MV->multMatrix(E);

		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		interpolatedHelicopter->drawHelicopter(prog, P, MV, helicopterMeshes, t);
	MV->popMatrix();

	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));

	prog->unbind();
	
	// Draw the frame and the grid with OpenGL 1.x (no GLSL)
	
	// Setup the projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(glm::value_ptr(P->topMatrix()));
	
	// Setup the modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(glm::value_ptr(MV->topMatrix()));
	
	// Draw frame
	glLineWidth(2);
	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(1, 0, 0);
	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 1, 0);
	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 1);
	glEnd();
	glLineWidth(1);
	
	// Draw grid
	float gridSizeHalf = 20.0f;
	int gridNx = 40;
	int gridNz = 40;
	glLineWidth(1);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_LINES);
	for(int i = 0; i < gridNx+1; ++i) {
		float alpha = i / (float)gridNx;
		float x = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
		glVertex3f(x, 0, -gridSizeHalf);
		glVertex3f(x, 0,  gridSizeHalf);
	}
	for(int i = 0; i < gridNz+1; ++i) {
		float alpha = i / (float)gridNz;
		float z = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
		glVertex3f(-gridSizeHalf, 0, z);
		glVertex3f( gridSizeHalf, 0, z);
	}
	glEnd();

	// Draw the spline curves
	if (showKeyframes){

		for (int i = 0; i < cps.size(); i++){

			glm::mat4 G(0);
			G[0] = glm::vec4(cps[(0 + i) % cps.size()], 1.0f);
			G[1] = glm::vec4(cps[(1 + i) % cps.size()], 1.0f);
			G[2] = glm::vec4(cps[(2 + i) % cps.size()], 1.0f);
			G[3] = glm::vec4(cps[(3 + i) % cps.size()], 1.0f);

			glLineWidth(1.0f);
			glBegin(GL_LINE_STRIP);
			for (float u = 0; u < 1; u = u + 0.005){
				glm::vec4 uVec(1, u, u * u, u * u * u);
				// Compute position at u
				glm::vec4 pos = G * B * uVec;
				glColor3f(0.3f, 0.8f, 0.3f);
				glVertex3f(pos.x, pos.y, pos.z);
			}

			glEnd();
		}

		// Draw the equidistant points along the spline curve
		if(!usTable.empty()) {
			float ds = 1.5f;
			glPointSize(5.0f);
			glBegin(GL_POINTS);
			float smax = usTable.back().second; // spline length
			for(float s = 0.0f; s < smax; s += ds) {
				// Convert from s to (concatenated) u
				float uu = s2u(s);
				// Convert from concatenated u to the usual u between 0 and 1.
				float kfloat;
				float u = std::modf(uu, &kfloat);
				// k is the index of the starting control point
				int k = (int)std::floor(kfloat);
				// Compute spline point at u
				glm::mat4 Gk;
				for(int i = 0; i < 4; ++i) {
					Gk[i] = glm::vec4(cps[(k+i) % cps.size()], 0.0f);
				}
				glm::vec4 uVec(1.0f, u, u*u, u*u*u);
				glm::vec3 P(Gk * (B * uVec));

				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv(&P[0]);
			}
			glEnd();
		}
	}
	
	// Pop modelview matrix
	glPopMatrix();
	
	// Pop projection matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	
	// Pop stacks
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		if(!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
			// Render scene.
			render();
			// Swap front and back buffers.
			glfwSwapBuffers(window);
		}
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <string>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Eigen/Dense>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "Texture.h"
#include "Link.h"

#include "ObjectiveRosenbrock.h"
#include "ObjectiveLink.h"
#include "OptimizerGD.h"
#include "OptimizerBFGS.h"
#include "OptimizerNM.h"

using namespace std;
using namespace glm;
using namespace Eigen;

char taskLetter = 'A';
int taskNumber = 1;

bool keyToggles[256] = {false}; // only for English keyboards!

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from

shared_ptr<Program> progSimple;
shared_ptr<Program> progTex;
shared_ptr<Shape> shape;
shared_ptr<Texture> texture;

vector<shared_ptr<Link> > links;

class IK
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	int nlinks;
	Vector3d weights;
	Vector2d target;
};
IK ik;

static string testRosenbrock()
{
	stringstream output;
	auto objective = make_shared<ObjectiveRosenbrock>(1.0, 1.0);
	
	// TODO: Implement Task A
	VectorXd x(2);
	VectorXd g(2);
	MatrixXd H(2,2);

	shared_ptr<Optimizer> optimizer;

	switch(taskNumber){
		case (1): {
			x << -1.0, 0.0;
			double f_x = objective->evalObjective(x, g, H);
			output << f_x << endl;
			output << g << endl;
			output << H << endl;
			break;
		}

		case (2): {
			// Gradient descent
			optimizer = make_shared<OptimizerGD>();

			optimizer->setIterMax(50);
			optimizer->setTol(1e-6);
			optimizer->setAlphaInit(1e-1);
			x << -1.0, 0.0;

			// Perform the optimization:
			x = optimizer->optimize(objective, x);
			
			output << optimizer->getIter() << endl;
			output << x << endl;			
			
			break;
		}

		case (3): {
			// Gradient descent with line search
			optimizer = make_shared<OptimizerGD>();

			optimizer->setIterMax(50);
			optimizer->setTol(1e-6);
			optimizer->setAlphaInit(1.0);
			optimizer->setGamma(0.8);
			optimizer->setIterMaxLS(20);
			x << -1.0, 0.0;
			
			// Perform the optimization:
			x = optimizer->optimize(objective, x);
			
			output << optimizer->getIter() << endl;
			output << x << endl;			
			
			break;
		}

		case (4):{
			// Newton's method with line search
			optimizer = make_shared<OptimizerNM>();
			
			optimizer->setIterMax(50);
			optimizer->setTol(1e-6);
			optimizer->setAlphaInit(1.0);
			optimizer->setGamma(0.8);
			optimizer->setIterMaxLS(20);
			x << -1.0, 0.0;

			// Perform the optimization:
			x = optimizer->optimize(objective, x);

			output << optimizer->getIter() << endl;
			output << x << endl;			
			
			break;
		}

		case (5):{
			// BFGS with line search
			optimizer = make_shared<OptimizerBFGS>();

			optimizer->setIterMax(50);
			optimizer->setTol(1e-6);
			optimizer->setAlphaInit(1.0);
			optimizer->setGamma(0.8);
			optimizer->setIterMaxLS(20);
			x << -1.0, 0.0;

			// Perform the optimization:
			x = optimizer->optimize(objective, x);

			output << optimizer->getIter() << endl;
			output << x << endl;	

			break;	
		}

		default:
			break;
	}
	
	return output.str();
}

static void createLinks()
{
	switch(taskNumber) {
		case 1:
			ik.nlinks = 1;
			ik.target << 0.0, 1.0;
			ik.weights << 1e3, 1e0, 0.0;
			break;
		case 2:
			ik.nlinks = 2;
			ik.target << 1.0, 1.0;
			ik.weights << 1e3, 1e0, 1e0;
			break;
		case 3:
			ik.nlinks = 4;
			ik.target << 3.0, 1.0;
			ik.weights << 1e3, 0.0, 1e0;
			break;
		case 4:
		case 5:
			ik.nlinks = 10;
			ik.target << 9.0, 1.0;
			ik.weights << 1e3, 0.0, 1e0;
			break;
	}
	
	// Create the links
	Matrix4d E(Matrix4d::Identity());
	for(int i = 0; i < ik.nlinks; ++i) {
		auto link = make_shared<Link>();
		links.push_back(link);
		link->setAngle(0.0);
		link->setPosition((i == 0 ? 0.0 : 1.0), 0.0);
		E(0,3) = 0.5;
		link->setMeshMatrix(E);
		if(i > 0) {
			links[i-1]->addChild(links[i]);
		}
	}
}

static string runIK()
{
	stringstream output;
	
	int n = ik.nlinks;
	// const Vector3d &weights = ik.weights;
	const Vector2d &target = ik.target;
	
	// Extract angles
	VectorXd x(n);
	for(int i = 0; i < n; ++i) {
		x(i) = links[i]->getAngle();
	}
	
	// TODO: Implement Task B
	switch(taskNumber){
		case (1):{
			double targetWeight = 1e3;
			VectorXd reg(1);
			reg << 1e0;

			auto objective = make_shared<ObjectiveLink>(targetWeight, reg, links[n-1], target);

			auto optimizer = make_shared<OptimizerBFGS>();
			optimizer->setIterMax(5);
			optimizer->setTol(1e-6);
			optimizer->setIterMaxLS(20);
			optimizer->setAlphaInit(1.0);
			optimizer->setGamma(0.5);

			x = optimizer->optimize(objective, x);

			break;
		}
		case (2):{
			double targetWeight = 1e3;
			VectorXd reg(2);
			reg << 1e0, 1e0;

			auto objective = make_shared<ObjectiveLink>(targetWeight, reg, links[n-1], target);

			auto optimizer = make_shared<OptimizerBFGS>();
			optimizer->setIterMax(5);
			optimizer->setTol(1e-6);
			optimizer->setIterMaxLS(20);
			optimizer->setAlphaInit(1.0);
			optimizer->setGamma(0.5);

			x = optimizer->optimize(objective, x);

			break;
		}
		case (3):{
			double targetWeight = 1e3;
			VectorXd reg(4);
			reg << 0.0, 1e0, 1e0, 1e0;

			auto objective = make_shared<ObjectiveLink>(targetWeight, reg, links[n-1], target);

			auto optimizer = make_shared<OptimizerBFGS>();
			optimizer->setIterMax(50);
			optimizer->setTol(1e-6);
			optimizer->setIterMaxLS(20);
			optimizer->setAlphaInit(1.0);
			optimizer->setGamma(0.5);

			x = optimizer->optimize(objective, x);
			break;
		}
		case (4):{
			double targetWeight = 1e3;
			VectorXd reg(10);
			reg << 0.0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0;

			auto objective = make_shared<ObjectiveLink>(targetWeight, reg, links[n-1], target);

			auto optimizer = make_shared<OptimizerBFGS>();
			optimizer->setIterMax(150);
			optimizer->setTol(1e-6);
			optimizer->setIterMaxLS(20);
			optimizer->setAlphaInit(1.0);
			optimizer->setGamma(0.5);

			x = optimizer->optimize(objective, x);
			break;
		}
		case (5):{
			double targetWeight = 1e3;
			VectorXd reg(10);
			reg << 0.0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0, 1e0;

			auto objective = make_shared<ObjectiveLink>(targetWeight, reg, links[n-1], target);
			
			auto BFGS = make_shared<OptimizerBFGS>();
			auto NM = make_shared<OptimizerNM>();
			BFGS->setIterMax(2);
			BFGS->setTol(1e-6);
			BFGS->setIterMaxLS(20);
			BFGS->setAlphaInit(1.0);
			BFGS->setGamma(0.5);

			NM->setIterMax(30);
			NM->setTol(1e-6);
			NM->setIterMaxLS(20);
			NM->setAlphaInit(1.0);
			NM->setGamma(0.5);

			x = BFGS->optimize(objective, x);
			x = NM->optimize(objective, x);

			break;
		}
		default:
			break;
	}
	
	// Set angles
	for(int i = 0; i < n; ++i) {
		double xi = x(i);
		links[i]->setAngle(xi);
	}
	
	// Write angles to output
	for(const auto &link : links) {
		output << link->getAngle() << endl;
	}
	
	return output.str();
}

static void writeOutput(const string &output)
{
	string filename = RESOURCE_DIR + "output" + taskLetter + to_string(taskNumber) + ".txt";
	ofstream out;
	out.open(filename, ofstream::out);
	if(!out.good()) {
		cout << "Cannot write to " << filename << endl;
		return;
	}
	cout << "Writing to " << filename << endl;
	out << output;
	out.close();
}

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
	switch(key) {
		case 'r':
			// Reset all angles to 0.0
			for(auto link : links) {
				link->setAngle(0.0);
			}
			break;
		case '.':
			// Increment all angles
			if(!keyToggles[(unsigned)' ']) {
				for(auto link : links) {
					link->setAngle(link->getAngle() + 0.1);
				}
			}
			break;
		case ',':
			// Decrement all angles
			if(!keyToggles[(unsigned)' ']) {
				for(auto link : links) {
					link->setAngle(link->getAngle() - 0.1);
				}
			}
			break;
	}
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	// Convert from window coord to world coord assuming that we're
	// using an orthgraphic projection
	double aspect = (double)width/height;
	double ymax = (double)links.size();
	double xmax = aspect*ymax;
	Vector2d x;
	x(0) = 2.0 * xmax * ((xmouse / width) - 0.5);
	x(1) = 2.0 * ymax * (((height - ymouse) / height) - 0.5);
	if(keyToggles[(unsigned)' ']) {
		ik.target = x;
		runIK();
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
}

static void init()
{
	GLSL::checkVersion();
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	
	keyToggles[(unsigned)'c'] = true;
	
	progSimple = make_shared<Program>();
	progSimple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	progSimple->setVerbose(true); // Set this to true when debugging.
	progSimple->init();
	progSimple->addUniform("P");
	progSimple->addUniform("MV");
	progSimple->setVerbose(false);
	
	progTex = make_shared<Program>();
	progTex->setVerbose(true); // Set this to true when debugging.
	progTex->setShaderNames(RESOURCE_DIR + "tex_vert.glsl", RESOURCE_DIR + "tex_frag.glsl");
	progTex->init();
	progTex->addUniform("P");
	progTex->addUniform("MV");
	progTex->addAttribute("aPos");
	progTex->addAttribute("aTex");
	progTex->addUniform("texture0");
	progTex->setVerbose(false);
	
	texture = make_shared<Texture>();
	texture->setFilename(RESOURCE_DIR + "metal_texture_15_by_wojtar_stock.jpg");
	texture->init();
	texture->setUnit(0);
	
	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "link.obj");
	shape->setProgram(progTex);
	shape->init();
	
	// Initialize time.
	glfwSetTime(0.0);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}

void render()
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'l']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	P->pushMatrix();
	MV->pushMatrix();
	
	// Apply camera transforms
	double aspect = (double)width/height;
	double ymax = (double)links.size();
	double xmax = aspect*ymax;
	P->multMatrix(glm::ortho(-xmax, xmax, -ymax, ymax, -1.0, 1.0));
	
	// Draw grid
	progSimple->bind();
	glUniformMatrix4fv(progSimple->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progSimple->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	// Draw axes
	glLineWidth(2.0f);
	glColor3d(0.2, 0.2, 0.2);
	glBegin(GL_LINES);
	glVertex2d(-xmax, 0.0);
	glVertex2d( xmax, 0.0);
	glVertex2d(0.0, -ymax);
	glVertex2d(0.0,  ymax);
	glEnd();
	// Draw grid lines
	glLineWidth(1.0f);
	glColor3d(0.8, 0.8, 0.8);
	glBegin(GL_LINES);
	for(int x = 1; x < xmax; ++x) {
		glVertex2d( x, -ymax);
		glVertex2d( x,  ymax);
		glVertex2d(-x, -ymax);
		glVertex2d(-x,  ymax);
	}
	for(int y = 1; y < ymax; ++y) {
		glVertex2d(-xmax,  y);
		glVertex2d( xmax,  y);
		glVertex2d(-xmax, -y);
		glVertex2d( xmax, -y);
	}
	glEnd();
	progSimple->unbind();
	
	// Draw shape
	progTex->bind();
	texture->bind(progTex->getUniform("texture0"));
	glUniformMatrix4fv(progTex->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	MV->pushMatrix();
	if(!links.empty()) {
		links.front()->draw(progTex, MV, shape);
	}
	MV->popMatrix();
	texture->unbind();
	progTex->unbind();
	
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
	
	if(argc < 3) {
		cout << "Please specify the task type." << endl;
	}
	string type = argv[2];
	if(type.length() < 3) {
		cout << "Please specify A.1-A.5 or B.1-B.5." << endl;
	}
	taskLetter = type.at(0);
	taskNumber = type.at(2) - '0';
	
	if(taskLetter == 'A') {
		// Task A
		string output = testRosenbrock();
		writeOutput(output);
		return 0;
	}
	
	// Task B
	bool useGL = (argc == 4);
	if(useGL) {
		// Set error callback.
		glfwSetErrorCallback(error_callback);
		// Initialize the library.
		if(!glfwInit()) {
			return -1;
		}
		// Create a windowed mode window and its OpenGL context.
		window = glfwCreateWindow(640, 480, "OCTAVIO ALMANZA", NULL, NULL);
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
		createLinks();
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
	} else {
		createLinks();
		string output = runIK();
		writeOutput(output);
	}
	return 0;
}

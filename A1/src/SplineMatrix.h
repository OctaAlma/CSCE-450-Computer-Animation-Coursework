#ifndef SPLINEMATRIX
#define SPLINEMATRIX

#include <iostream>

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

enum SplineType
{
	BEZIER = 0,
	CATMULL_ROM,
	BASIS,
	SPLINE_TYPE_COUNT
};

struct SplineMatrix{
	glm::mat4 mat;
	int type;

	SplineMatrix(glm::mat4 Mat, int TYPE){
		mat = Mat;
		type = TYPE;
	}
};

SplineMatrix createBezier(){
	glm::mat4 matBezier;
	// note: mat[col][row]
	// bezier curve matrix
	glm::vec4 col1(1,0,0,0);
	glm::vec4 col2(-3,3,0,0);
	glm::vec4 col3(3,-6,3,0);
	glm::vec4 col4(-1,3,-3,1);

	matBezier[0] = col1;
	matBezier[1] = col2;
	matBezier[2] = col3;
	matBezier[3] = col4;

	SplineMatrix b(matBezier, BEZIER);
	return b;
}

SplineMatrix createCatmull(){
	glm::mat4 matCatmull;
	// note: mat[col][row]
	// bezier curve matrix
	glm::vec4 col1(0,2,0,0);
	glm::vec4 col2(-1,0,1,0);
	glm::vec4 col3(2,-5,4,-1);
	glm::vec4 col4(-1,3,-3,1);

	matCatmull[0] = col1;
	matCatmull[1] = col2;
	matCatmull[2] = col3;
	matCatmull[3] = col4;

	matCatmull = 1.0f/2.0f * matCatmull;

	SplineMatrix b(matCatmull, CATMULL_ROM);
	return b;
}

SplineMatrix createBspline(){
	glm::mat4 matBspline;

	// note: mat[col][row]
	// bezier curve matrix
	glm::vec4 col1(1,4,1,0);
	glm::vec4 col2(-3,0,3,0);
	glm::vec4 col3(3,-6,3,0);
	glm::vec4 col4(-1,3,-3,1);

	matBspline[0] = col1;
	matBspline[1] = col2;
	matBspline[2] = col3;
	matBspline[3] = col4;

	matBspline = 1.0f/6.0f * matBspline;

	SplineMatrix b(matBspline, BASIS);
	return b;
}

void addMatrices(std::vector<SplineMatrix>& sm){
    sm.push_back(createBezier());
    sm.push_back(createCatmull());
    sm.push_back(createBspline());
}

glm::mat4 getSplineMatrix(std::vector<SplineMatrix>& sm, int type){
    for (int i = 0; i < sm.size(); i++){
        if (sm.at(i).type == type){
            return sm.at(i).mat;
        }
    }
    
    glm::mat4 B(0);
    return B;
}

#endif
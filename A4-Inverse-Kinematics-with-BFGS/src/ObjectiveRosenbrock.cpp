#include "ObjectiveRosenbrock.h"
#include <cmath>

using namespace Eigen;

ObjectiveRosenbrock::ObjectiveRosenbrock(double a, double b)
{
	this->a = a;
	this->b = b;
}

ObjectiveRosenbrock::~ObjectiveRosenbrock()
{
	
}

// Returns just the objective value
double ObjectiveRosenbrock::evalObjective(const VectorXd &x) const
{
	return (pow((a - x[0]), 2.0) + b * pow((x[1] - pow((x[0]), 2.0)), 2.0));
}

// Return the objective and the gradient
double ObjectiveRosenbrock::evalObjective(const VectorXd &x, VectorXd &g) const
{
	double g_1 = -1.0 * (a - x[0]) - 2.0 * b * x[0] * (x[1] - pow(x[0], 2.0));
	double g_2 = b * (x[1] - pow(x[0], 2.0));
	// Update the gradient:
	g = 2.0 * Vector2d(g_1, g_2);

	// Return the objective f(x):
	return this->evalObjective(x);
}

// Returns the objective, the gradient and Hessian
double ObjectiveRosenbrock::evalObjective(const VectorXd &x, VectorXd &g, MatrixXd &H) const
{
	double h_11, h_12, h_21, h_22;
	h_11 = 2.0 * b * (3.0 * pow(x[0], 2.0) - x[1]) + 1.0;
	h_12 = -2.0 * b * x[0];
	h_21 = h_12;
	h_22 = b;

	H << h_11, h_12, h_21, h_22;
	H *= 2;

	// Return the objective and the gradient
	return this->evalObjective(x, g);
}

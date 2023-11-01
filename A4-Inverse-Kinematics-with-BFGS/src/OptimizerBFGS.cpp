#include "OptimizerBFGS.h"
#include "Objective.h"
#include <iostream>

using namespace std;
using namespace Eigen;

OptimizerBFGS::OptimizerBFGS()
{
	
}

OptimizerBFGS::~OptimizerBFGS()
{
	
}

VectorXd OptimizerBFGS::optimize(const shared_ptr<Objective> objective, const VectorXd &xInit)
{
	const int n = xInit.size();
	VectorXd x = xInit;
	VectorXd x0 = xInit;
	VectorXd g(n);
	VectorXd g0(n);
	VectorXd p(n);
	VectorXd d_x(n);

	MatrixXd I(n, n);
	I.setIdentity();
	MatrixXd A = I;

	// NOTE: Vectors in Eigen are automatically initialized to 0

	for (int i = 1; i <= this->iterMax; i++){
		this->iter = i;
		
		// Search direction
		// Evaluate f and g at x
		double f_x = objective->evalObjective(x, g);
		
		if (iter > 1){
			VectorXd s = x - x0;
			VectorXd y = g - g0;

			double rho = 1.0 / (y.transpose() * s);
			A = (I - rho * (s * y.transpose())) * A * (I - rho * (y * s.transpose())) + rho * s * s.transpose();
		}

		p = -1.0 * A * g;

		// Line search
		double alpha = alphaInit;
		for (iterLS = 1; iterLS <= iterMaxLS; iterLS++){
			d_x = alpha * p;
			double f_1 = objective->evalObjective(x + d_x);

			if (f_1 < f_x){
				break;
			}

			alpha = alpha * this->gamma;
		}

		// Step
		x0 = x;
		g0 = g;
		x = x + d_x;
	
		if (g.norm() < this->tol){
			break;
		}
	}

	// Implement the wrap around for all the angles:
	for (int j = 0; j < n; j++){
		while (x(j) > M_PI){
			x(j) -= 2.0*M_PI;
		}
		
		while (x(j) < -M_PI){
			x(j) += 2.0*M_PI;
		}
	}

	return x;
}

#include "OptimizerGD.h"
#include "Objective.h"
#include <iostream>

using namespace std;
using namespace Eigen;

OptimizerGD::OptimizerGD()
{
	
}

OptimizerGD::~OptimizerGD()
{
	
}

VectorXd OptimizerGD::optimize(const shared_ptr<Objective> objective, const VectorXd &xInit)
{
	VectorXd x = xInit;
	VectorXd g(2);
	VectorXd p(2);
	VectorXd d_x(2);

	for (int i = 1; i <= this->iterMax; i++){
		iter = i;
		// search direction
		// Evaluate f and g at x:
		double f_x = objective->evalObjective(x, g);
		p = -g;

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
		
		// Step:
		x = x + d_x;

		if (g.norm() < this->tol){
			break;
		}
	}
	return x;
}

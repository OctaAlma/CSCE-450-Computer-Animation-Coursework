#include "OptimizerNM.h"
#include "Objective.h"
#include <iostream>

using namespace std;
using namespace Eigen;

OptimizerNM::OptimizerNM()
{
	
}

OptimizerNM::~OptimizerNM()
{
	
}

VectorXd OptimizerNM::optimize(const shared_ptr<Objective> objective, const VectorXd &xInit)
{
	const int n = xInit.size();
	VectorXd x = xInit;
	VectorXd g(n);
	VectorXd p(n);
	VectorXd d_x(n);
	MatrixXd H(n,n);

	for (int i = 1; i <= this->iterMax; i++){
		this->iter = i;
		// Search direction
		double f_x = objective->evalObjective(x, g, H);
		MatrixXd negH = -1.0 * H;
		p = negH.inverse() * g;

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
		x = x + d_x;
		if (g.norm() < this->tol){
			break;
		}
	}
	
	return x;
}

#pragma once
#ifndef OBJECTIVE_ROSENBROCK_H
#define OBJECTIVE_ROSENBROCK_H

#include "Objective.h"

class ObjectiveRosenbrock : public Objective
{
public:
	ObjectiveRosenbrock(double a = 1.0, double b = 1.0);
	virtual ~ObjectiveRosenbrock();
	virtual double evalObjective(const Eigen::VectorXd &x) const;
	virtual double evalObjective(const Eigen::VectorXd &x, Eigen::VectorXd &g) const;
	virtual double evalObjective(const Eigen::VectorXd &x, Eigen::VectorXd &g, Eigen::MatrixXd &H) const;
	
private:
	double a;
	double b;
};

#endif

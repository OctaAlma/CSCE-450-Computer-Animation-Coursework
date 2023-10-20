#pragma once
#ifndef OBJECTIVE_LINK_H
#define OBJECTIVE_LINK_H

#include "Objective.h"
#include <memory>
#include "Link.h"

class ObjectiveLink : public Objective
{
public:
	ObjectiveLink(double tar, Eigen::VectorXd reg, std::shared_ptr<Link> el, Eigen::Vector2d target);
	virtual ~ObjectiveLink();
	virtual double evalObjective(const Eigen::VectorXd &x) const;
	virtual double evalObjective(const Eigen::VectorXd &x, Eigen::VectorXd &g) const;
	virtual double evalObjective(const Eigen::VectorXd &x, Eigen::VectorXd &g, Eigen::MatrixXd &H) const;
	
private:
	double wTar;
	Eigen::MatrixXd wReg;
    std::shared_ptr<Link> endLink;
    Eigen::Vector2d pTarget;
};

Eigen::Vector3d computep(std::shared_ptr<Link> l);

#endif

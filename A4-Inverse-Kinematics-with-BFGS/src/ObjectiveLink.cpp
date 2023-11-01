#include "ObjectiveLink.h"
#include <cmath>
#include <iostream>

using namespace Eigen;
using namespace std;

ObjectiveLink::ObjectiveLink(double tar, Eigen::VectorXd reg, std::shared_ptr<Link> el, Eigen::Vector2d target)
{
    this->wTar = tar;
    this->endLink = el;
    this->pTarget = target;

    // Set up the regularizer matrix:
    const int n = reg.size();
    this->wReg = MatrixXd(n, n);
    wReg.setZero();

    for (int i = 0; i < n; i++){
        wReg(i,i) = reg(i);
    }
}

ObjectiveLink::~ObjectiveLink()
{
	
}

// Computes the Translation matrix for the provided link position
Matrix3d computeT(Vector2d lPos){
    Matrix3d T(3,3);
    T.setIdentity();

    T(0, 2) = lPos(0);
    T(1, 2) = lPos(1);

    return T;
}

// Computes the Rotation matrix for the provided link angle
Matrix3d computeR(double theta){
    Matrix3d R(3,3);

    double sine = sin(theta);
    double cosine = cos(theta);
    
    R << cosine, -sine, 0.0,
        sine, cosine, 0.0,
        0.0, 0.0, 1.0;

    return R;
}

// Computes the position for the provided link using forward kinematics
// Note: r should be the "end effector" for the LAST link
Eigen::Vector3d computep(std::shared_ptr<Link> l){
    Vector3d r;
    r << 1.0, 0.0, 1.0;

    Vector3d p = computeT(l->getPosition()) * computeR(l->getAngle()) * r;

    std::shared_ptr<Link> parent = l->getParent();
    while (parent != NULL){
        p = computeT(parent->getPosition()) * computeR(parent->getAngle()) * p;
        parent = parent->getParent();
    }

    return p;
}

// The following functions will be used to compute p'
Eigen::Matrix3d computeR_1Prime(double theta){
    Matrix3d R_1prime;
    
    double sine = sin(theta);
    double cosine = cos(theta);
    
    R_1prime << -sine, -cosine, 0.0,
        cosine, -sine, 0.0,
        0.0, 0.0, 0.0;

    return R_1prime;
}

Eigen::Matrix3d computeR_2Prime(double theta){
    Matrix3d R_2prime(3,3);
    
    double sine = sin(theta);
    double cosine = cos(theta);
    
    R_2prime << -cosine, sine, 0.0,
        -sine, -cosine, 0.0,
        0.0, 0.0, 0.0;

    return R_2prime;
}

std::shared_ptr<Link> getRoot(std::shared_ptr<Link> l){
    if (l->getDepth() == 0){
        return l;
    }

    shared_ptr<Link> root = l;
    while (root->getDepth() != 0){
        root = root->getParent();
    }

    return root;
}

Eigen::Vector2d computep_iPrime(std::shared_ptr<Link> root, int i){

    std::shared_ptr<Link> child = root;
    Matrix3d allTransforms(3,3);
    allTransforms.setIdentity();

    // We will do the multiplication from left to right, starting from theta_0:
    while (child != NULL){
        if (child->getDepth() == i){
            allTransforms = allTransforms * computeT(child->getPosition()) * computeR_1Prime(child->getAngle());
        }else{
            allTransforms = allTransforms * computeT(child->getPosition()) * computeR(child->getAngle());
        }

        child = child->getChild();
    }

    Vector3d r(3);
    r << 1.0, 0.0, 1.0;

    Vector3d p = allTransforms * r;

    return p.head<2>();
}

Eigen::MatrixXd computepPrime(std::shared_ptr<Link> l){
    // p' is a matrix of column vectors
    // You will have one column for every link in the chain
    
    const int numChains = l->getDepth() + 1;
    MatrixXd pPrime(2, numChains);

    auto root = getRoot(l);

    for (int i = 0; i < numChains; i++){
        Vector2d p_i = computep_iPrime(root, i);
        pPrime(0, i) = p_i(0);
        pPrime(1, i) = p_i(1);
    }

    return pPrime;
}


// Compute the second derivative of a specific link
Eigen::Vector2d computep_ij2Prime(std::shared_ptr<Link> root, int i, int j){

    Matrix3d allTransforms;
    allTransforms.setIdentity();

    auto currLink = root;

    while (currLink != NULL){
        double currDepth = currLink->getDepth();
        if (currDepth == i && i == j){
            allTransforms = allTransforms * computeT(currLink->getPosition()) * computeR_2Prime(currLink->getAngle());            
        }
        else if (currDepth == i || currDepth == j){
            allTransforms = allTransforms * computeT(currLink->getPosition()) * computeR_1Prime(currLink->getAngle());
        }
        else{
            allTransforms = allTransforms * computeT(currLink->getPosition()) * computeR(currLink->getAngle()); 
        }

        currLink = currLink->getChild();
    }

    Vector3d r(3);
    r << 1.0, 0.0, 1.0;
    
    Vector3d p_ij2Prime = allTransforms * r;

    return p_ij2Prime.head<2>();
}

Eigen::MatrixXd computeP2prime(std::shared_ptr<Link> l){
    const int numChains = l->getDepth() + 1;
    const int rows = numChains * 2;
    const int cols = numChains;
    MatrixXd P2prime(rows, cols);

    auto root = getRoot(l);

    for (int i = 0; i < numChains; i++){
        for (int j = 0; j < numChains; j++){
            Vector2d p_ij2prime = computep_ij2Prime(root, i, j);

            P2prime(i * 2, j) = p_ij2prime(0);
            P2prime((i * 2) + 1, j) = p_ij2prime(1);
        }
    }

    return P2prime;
}

Eigen::MatrixXd computeMatrixDot(Vector2d dp, MatrixXd P2prime){
    const int n = P2prime.cols();
    MatrixXd dotMatrix(n, n);

    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            
            Vector2d pij;
            pij(0) = P2prime(i * 2, j);
            pij(1) = P2prime((i * 2) + 1, j);

            dotMatrix(i,j) = dp.dot(pij);
        }
    }

    return dotMatrix;
}


double ObjectiveLink::evalObjective(const VectorXd &x) const{
    
    // Put the corresponding value of x/theta on its respective link
    auto curr = this->endLink;
    for (int i = (x.size() - 1); i >= 0; i--){
        curr->setAngle(x(i));
        curr = curr->getParent();
    }


    Vector2d p = computep(this->endLink).head<2>();
    Vector2d dp = p - pTarget;
    
    double f_theta = -1;
    
    if (x.size() == 1){
        double f_tar = 0.5 * wTar * (dp.dot(dp)); 
        double f_reg = 0.5 * wReg(0,0) * pow(x(0), 2);
        
        f_theta = f_tar + f_reg;
    }
    else{
        f_theta = 0.5 * wTar * (dp.dot(dp)) + 0.5 * (x.transpose() * wReg * x)(0);
    }

    return f_theta;
}

double ObjectiveLink::evalObjective(const VectorXd &x, VectorXd &g) const{
    Vector2d p = computep(this->endLink).head<2>();
    Vector2d dp = p - pTarget;
    MatrixXd Pprime = computepPrime(this->endLink);

    g = wTar * (dp.transpose() * Pprime).transpose() + wReg * x;

    return evalObjective(x);
}

double ObjectiveLink::evalObjective(const VectorXd &x, VectorXd &g, MatrixXd &H) const{
    const int n = x.size();

    Vector2d p = computep(this->endLink).head<2>();
    Vector2d dp = p - pTarget;

    MatrixXd Pprime = computepPrime(this->endLink);
    MatrixXd P2prime = computeP2prime(this->endLink);

    if (n > 1){

        H = wTar * ((Pprime.transpose() * Pprime) + computeMatrixDot(dp, P2prime)).transpose() + wReg;
        
    }else{

        // In the 1D case, p' and p'' should be 2x1 matrices / vectors:
        Vector2d pPrimeVec(2);
        pPrimeVec << Pprime(0,0), Pprime(1, 0);

        Vector2d p2PrimeVec(2);
        p2PrimeVec << P2prime(0,0), P2prime(1, 0);

        // H will just be a 1x1
        H = MatrixXd(1,1);

        H(0,0) = wTar * (pPrimeVec.dot(pPrimeVec) + (dp.transpose() * p2PrimeVec)) + wReg(0,0);
    }

    return evalObjective(x, g);
}

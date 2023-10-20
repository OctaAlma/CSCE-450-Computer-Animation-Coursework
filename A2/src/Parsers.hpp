#pragma once
#ifndef Parsers_H
#define Parsers_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ShapeSkin.h"

extern std::string DATA_DIR;

void loadBoneAnimation(std::string skeletonData){
	//std::string filename = DATA_DIR + "bigvegas_Capoeira_skel.txt";
	std::string filename = DATA_DIR + skeletonData;
	std::ifstream in;
	in.open(filename);
	if(!in.good()) {
		std::cout << "Cannot read " << filename << std::endl;
		return;
	}
	std::cout << "Loading quaternions from " << filename << std::endl;
	
	std::string line;
	// # of frames then # of bones
	size_t numFrames = 0;
	size_t numBones = 0;
	size_t currFrameNo = 0;
	
	while(1) {
		getline(in, line);
		if(in.eof()) {
			break;
		}
		if(line.empty()) {
			continue;
		}
		// Skip comments
		if(line.at(0) == '#') {
			continue;
		}
		
		std::stringstream ss(line);

		// If num frames and numBones are 0, then this means we have not parsed the first line
		if (numFrames == 0 && numBones == 0){
			ss >> numFrames;
			ss >> numBones;

			bindPose = Frame(numBones);
			continue;
		}
		else{
			Frame currFrame(numBones);
			// Parse data lines
			for (int i = 0; i < numBones; i++){
				glm::quat q;
				glm::vec3 p;
				ss >> q.x;
				ss >> q.y;
				ss >> q.z;
				ss >> q.w;

				ss >> p.x;
				ss >> p.y;
				ss >> p.z;

				if (currFrameNo == 0){
					bindPose.bonePlacements.at(i) = Bone(q, p);
				}
				
				currFrame.bonePlacements.at(i) = Bone(q, p);
			}

			if (currFrameNo != 0){
				allFrames.push_back(currFrame);
			}
			currFrameNo++;
		}
	}

	in.close();
}

#endif
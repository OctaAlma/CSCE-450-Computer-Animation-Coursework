#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>


#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"
#include "TextureMatrix.h"

using namespace std;
using namespace glm;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
	T = make_shared<TextureMatrix>();
}

ShapeSkin::~ShapeSkin()
{
}

void ShapeSkin::setTextureMatrixType(const std::string &meshName)
{
	T->setType(meshName);
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	// This works only if the OBJ file has the same indices for v/n/t.
	// In other words, the 'f' lines must look like:
	// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;

		// ADDED THIS:
		initialPosBuf = attrib.vertices;
		initialNorBuf = attrib.normals;

		norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				//shapes[s].mesh.material_ids[f];
			}
		}
	}
}

void ShapeSkin::loadAttachment(const std::string &filename)
{	
	// TODO
	std::ifstream in;
	in.open(filename);
	if(!in.good()) {
		std::cout << "Cannot read " << filename << std::endl;
		return;
	}
	std::cout << "Loading vertex information from " << filename << std::endl;

	std::string line;

	int currMaxInf;
	while(1){
		getline(in, line);
		if (in.eof()){
			break;
		}
		if (line.empty()){
			continue;
		}
		// skip comments
		if (line.at(0) == '#'){
			continue;
		}

		std::stringstream ss(line);

		// Check if this is the info line
		if ((vertCount == 0) && (boneCount == 0) && (maxInfluences == 0)){
			ss >> vertCount;
			ss >> boneCount;
			ss >> maxInfluences;
			continue;
		}

		ss >> currMaxInf;

		for (int i = 0; i < maxInfluences; i++){
			int boneInd = 0.0f;
			float weight = 0.0f;
			if (i < currMaxInf){
				ss >> boneInd;
				ss >> weight;
			}

			this->boneIndBuf.push_back(boneInd);
			this->weightBuf.push_back(weight);
		}

	}
}

glm::vec3 ShapeSkin::getInitialPos(int vertInd){
	glm::vec3 originalPos(0);
	originalPos.x = this->initialPosBuf[vertInd * 3];
	originalPos.y = this->initialPosBuf[vertInd * 3 + 1];
	originalPos.z = this->initialPosBuf[vertInd * 3 + 2];

	return originalPos;
}

glm::vec3 ShapeSkin::getInitialNor(int vertInd){
	glm::vec3 originalNor(0);
	originalNor.x = this->initialNorBuf[vertInd * 3];
	originalNor.y = this->initialNorBuf[vertInd * 3 + 1];
	originalNor.z = this->initialNorBuf[vertInd * 3 + 2];

	return originalNor;
}


std::vector<std::pair<unsigned int, float> > ShapeSkin::getBoneInfluences(int vertInd){
	std::vector<std::pair<unsigned int, float> > influences(this->maxInfluences);

	for (int i = 0; i < this->maxInfluences; i++){
		influences.at(i).first = this->boneIndBuf.at((vertInd * maxInfluences) + i);
		influences.at(i).second = this->weightBuf.at((vertInd * maxInfluences) + i);
	}

	return influences;
}

void ShapeSkin::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::updatePos(int vertInd, glm::vec3 newPos){
	this->posBuf.at(vertInd * 3) = newPos.x;
	this->posBuf.at((vertInd * 3) + 1) = newPos.y;
	this->posBuf.at((vertInd * 3) + 2) = newPos.z;
}

void ShapeSkin::updateNor(int vertInd, glm::vec3 newNor){
	this->norBuf.at(vertInd * 3) = newNor.x;
	this->norBuf.at((vertInd * 3) + 1) = newNor.y;
	this->norBuf.at((vertInd * 3) + 2) = newNor.z;
}

// Pass in the current frame, k
void ShapeSkin::update(int k)
{
	// TODO: CPU skinning calculations.
	// After computing the new positions and normals, send the new data
	// to the GPU by copying and pasting the relevant code from the
	// init() function.

	if (boneMatrices == nullptr){
		boneMatrices = make_shared<std::vector<std::vector<glm::mat4> >>(allFrames.size());
		for (int frameNo = 0; frameNo < allFrames.size(); frameNo++){
			boneMatrices->at(frameNo) = std::vector<glm::mat4>(this->maxInfluences * this->vertCount);
		}
	}

	// Compute the equation and update posBuf!
	for (int i = 0; i < this->vertCount; i++){
		glm::vec3 initialPos = getInitialPos(i);
		glm::vec3 initialNor = getInitialNor(i);

		std::vector<std::pair<unsigned int, float> > boneInfluences = getBoneInfluences(i);
		glm::vec3 newPos(0.0f,0.0f,0.0f);
		glm::vec3 newNor(0.0f,0.0f,0.0f);

		for (int j = 0; j < boneInfluences.size(); j++){
			int boneID = boneInfluences.at(j).first;
			float boneWeight = boneInfluences.at(j).second;

			if (boneWeight == 0){
				continue;
			}

			glm::mat4 matKthFrameJthBone;

			if (boneMatrices->at(k).at((this->maxInfluences * i) + j) != glm::mat4(0)){
				matKthFrameJthBone = boneMatrices->at(k).at((this->maxInfluences * i) + j);
			}
			else{
				glm::mat4 j_kthMat = allFrames.at(k).bonePlacements.at(boneID).quatMat;
				matKthFrameJthBone = j_kthMat * glm::inverse(bindPose.bonePlacements.at(boneID).quatMat);
				boneMatrices->at(k).at((this->maxInfluences * i) + j) = matKthFrameJthBone;
			}

			newPos = newPos + boneWeight * glm::vec3(matKthFrameJthBone * glm::vec4(initialPos,1.0f));
			newNor = newNor + boneWeight * glm::vec3(matKthFrameJthBone * glm::vec4(initialNor,0.0f));

			// Pre-optimizations:
			// newPos = newPos + boneWeight * glm::vec3(j_kthMat * glm::inverse(bindPose.bonePlacements.at(boneID).quatMat) * glm::vec4(initialPos,1.0f));
			// newNor = newNor + boneWeight * glm::vec3(j_kthMat * glm::inverse(bindPose.bonePlacements.at(boneID).quatMat) * glm::vec4(initialNor,0.0f));

		}

		updatePos(i, newPos);
		updateNor(i, newNor);
	}

	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);

	glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::draw(int k) const
{
	assert(prog);

	// Send texture matrix
	glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

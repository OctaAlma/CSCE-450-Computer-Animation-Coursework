#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Shape.h"
#include "GLSL.h"
#include "Program.h"

using namespace std;
using namespace glm;

Shape::Shape() :
	prog(NULL),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
}

Shape::~Shape()
{
}

void Shape::loadObj(const string &filename, vector<float> &pos, vector<float> &nor, vector<float> &tex, bool loadNor, bool loadTex)
{
	
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
	if(!warn.empty()) {
		//std::cout << warn << std::endl;
	}
	if(!err.empty()) {
		std::cerr << err << std::endl;
	}
	if(!ret) {
		return;
	}
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			// Loop over vertices in the face.
			for(size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				pos.push_back(attrib.vertices[3*idx.vertex_index+0]);
				pos.push_back(attrib.vertices[3*idx.vertex_index+1]);
				pos.push_back(attrib.vertices[3*idx.vertex_index+2]);
				if(!attrib.normals.empty() && loadNor) {
					nor.push_back(attrib.normals[3*idx.normal_index+0]);
					nor.push_back(attrib.normals[3*idx.normal_index+1]);
					nor.push_back(attrib.normals[3*idx.normal_index+2]);
				}
				if(!attrib.texcoords.empty() && loadTex) {
					tex.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
					tex.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
				}
			}
			index_offset += fv;
		}
	}
}

void Shape::loadMesh(const string &meshName)
{
	// Load geometry
	meshFilename = meshName;
	loadObj(meshFilename, posBuf, norBuf, texBuf);
}

void Shape::init()
{
	if (this->meshFilename == "../data/Victor_leftEyeInner.obj"){
		this->isLeftEye = true;
	}

	if (this->meshFilename == "../data/Victor_rightEyeInner.obj"){
		this->isRightEye = true;
	}
	
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
	
	// Send each of the blendshape delta arrays to the GPU
	for (int i = 0; i < this->blendshapes.size(); i++){
		shared_ptr<BlendShape> currBs = blendshapes.at(i);
		
		glGenBuffers(1, &(currBs->bsPosDeltasID));
		glBindBuffer(GL_ARRAY_BUFFER, currBs->bsPosDeltasID);
		glBufferData(GL_ARRAY_BUFFER, currBs->bsPosDeltas.size()*sizeof(float), &(currBs->bsPosDeltas[0]), GL_STATIC_DRAW);

		glGenBuffers(1, &(currBs->bsNorDeltasID));
		glBindBuffer(GL_ARRAY_BUFFER, currBs->bsNorDeltasID);
		glBufferData(GL_ARRAY_BUFFER, currBs->bsNorDeltas.size()*sizeof(float), &(currBs->bsNorDeltas[0]), GL_STATIC_DRAW);
	}

	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void Shape::draw() const
{
	assert(prog);

	if (!(this->blendshapes.empty())){
		for (int i = 0; i < blendshapes.size(); i++){
			shared_ptr<BlendShape> curr = blendshapes.at(i);
			int bs_pos = prog->getAttribute("blendshape" + std::to_string(i + 1) + "Pos");
			
			glEnableVertexAttribArray(bs_pos);
			glBindBuffer(GL_ARRAY_BUFFER, curr->bsPosDeltasID);
			glVertexAttribPointer(bs_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

			int bs_nor = prog->getAttribute("blendshape" + std::to_string(i + 1) + "Nor");
			glEnableVertexAttribArray(bs_nor);
			glBindBuffer(GL_ARRAY_BUFFER, curr->bsNorDeltasID);
			glVertexAttribPointer(bs_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

			GLSL::checkError(GET_FILE_LINE);
		}
	}

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
	int count = (int)posBuf.size()/3; // number of indices to be rendered
	glDrawArrays(GL_TRIANGLES, 0, count);
	
	glDisableVertexAttribArray(h_tex);
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

// new functions: 
void Shape::addBlendShape(std::string filename, std::vector<float>& objPos, std::vector<float>& objNor, int actionNo){
	if (objPos.size() != this->posBuf.size()){
		std::cout << "The # of the blendshape's positions and the mesh's vertices does not match. " << objPos.size() << " != " << this->posBuf.size() << std::endl;
	}

	if (objNor.size() != this->norBuf.size()){
		std::cout << "The # of the blendshape's normals and the mesh's normals does not match. " << objPos.size() << " != " << this->posBuf.size() << std::endl;
	}

	// Assuming the topology is the same, we can go ahead and create the delta vectors:
	std::shared_ptr<BlendShape> bs = std::make_shared<BlendShape>();
	bs->bsName = filename;
	bs->actionNo = actionNo;
	bs->bsNorDeltas.clear();
	bs->bsPosDeltas.clear();

	// Create the delta positions:
	for (int i = 0; i < objPos.size(); i = (i + 3)){
		float deltaX = objPos.at(i) - this->posBuf.at(i);
		float deltaY = objPos.at(i + 1) - this->posBuf.at(i + 1);
		float deltaZ = objPos.at(i + 2) - this->posBuf.at(i + 2);

		bs->bsPosDeltas.push_back(deltaX);
		bs->bsPosDeltas.push_back(deltaY);
		bs->bsPosDeltas.push_back(deltaZ);
	}

	// Create the delta normals:
	for (int i = 0; i < objNor.size(); i = (i + 3)){
		float deltaX = objPos.at(i) - this->posBuf.at(i);
		float deltaY = objPos.at(i + 1) - this->posBuf.at(i + 1);
		float deltaZ = objPos.at(i + 2) - this->posBuf.at(i + 2);

		bs->bsNorDeltas.push_back(deltaX);
		bs->bsNorDeltas.push_back(deltaY);
		bs->bsNorDeltas.push_back(deltaZ);
	}

	this->blendshapes.push_back(bs);
}

// A function that will load blendshape obj files. WILL NOT CREATE DELTAS, that is done when added to a Shape object
void loadBlendShapeObj(const string &filename, vector<float> &pos, vector<float> &nor)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	std::cout << "Loading blendshape: " << filename << std::endl;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
	if(!warn.empty()) {
		//std::cout << warn << std::endl;
	}
	if(!err.empty()) {
		std::cerr << err << std::endl;
	}
	if(!ret) {
		return;
	}
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			// Loop over vertices in the face.
			for(size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				pos.push_back(attrib.vertices[3*idx.vertex_index+0]);
				pos.push_back(attrib.vertices[3*idx.vertex_index+1]);
				pos.push_back(attrib.vertices[3*idx.vertex_index+2]);
				if(!attrib.normals.empty()) {
					nor.push_back(attrib.normals[3*idx.normal_index+0]);
					nor.push_back(attrib.normals[3*idx.normal_index+1]);
					nor.push_back(attrib.normals[3*idx.normal_index+2]);
				}
			}
			index_offset += fv;
		}
	}
}
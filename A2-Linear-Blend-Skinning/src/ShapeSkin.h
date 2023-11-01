#pragma once
#ifndef SHAPESKIN_H
#define SHAPESKIN_H

#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>

class MatrixStack;
class Program;
class TextureMatrix;

struct Bone{
	glm::mat4 quatMat;

	Bone(){
		quatMat = glm::mat4(0);
	}

	Bone(glm::quat q, glm::vec3 p){
		quatMat = glm::mat4_cast(q);
		quatMat[3] = glm::vec4(p, 1.0f);
	}
};

// A frame is composed of a set of bones
struct Frame{
	std::vector<Bone> bonePlacements;

	Frame(int numBones){
		bonePlacements = std::vector<Bone>(numBones);
	}
};

// The fist frame created is the bind pose!
extern Frame bindPose;
extern std::vector<Frame> allFrames;

class ShapeSkin
{
public:
	ShapeSkin();
	virtual ~ShapeSkin();
	void setTextureMatrixType(const std::string &meshName);
	void loadMesh(const std::string &meshName);
	void loadAttachment(const std::string &filename); // THIS IS THE FUNCTION TO WORK ON
	void setProgram(std::shared_ptr<Program> p) { prog = p; }
	void init();
	void update(int k);
	void draw(int k) const;
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }
	std::shared_ptr<TextureMatrix> getTextureMatrix() { return T; }

	glm::vec3 getInitialPos(int vertInd); // Returns a vector containing the initial position of the specified vertex
	glm::vec3 getInitialNor(int vertInd); // Returns a vector containing the initial normal of the specified vertex
	
	std::vector<std::pair<unsigned int, float> > getBoneInfluences(int vertInd); // Returns a vector of pairs containing the index for the bone and its repective weight on vertex position
	void updatePos(int vertInd, glm::vec3 newPos);
	void updateNor(int vertInd, glm::vec3 newNor);

private:
	std::shared_ptr<Program> prog;
	std::vector<unsigned int> elemBuf;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;

	// new attributes
	size_t maxInfluences = 0;
	size_t vertCount = 0;
    size_t boneCount = 0;

	std::vector<float> initialPosBuf;
	std::vector<float> initialNorBuf;
	std::vector<unsigned int> boneIndBuf;
	std::vector<float> weightBuf;
	
	std::shared_ptr< std::vector<std::vector<glm::mat4> > > boneMatrices = nullptr;
	std::vector<glm::mat4> initialBoneMatrices;


	GLuint elemBufID;
	GLuint posBufID;
	GLuint norBufID;
	GLuint texBufID;
	std::string textureFilename;
	std::shared_ptr<TextureMatrix> T;
};

#endif

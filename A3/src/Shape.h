#pragma once
#ifndef SHAPE_H
#define SHAPE_H

#include <memory>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>

class MatrixStack;
class Program;

struct Emotion{
	std::string emotionName;
	std::vector<int> actionNumbers;
};

struct BlendShape{
	std::string bsName;
	int actionNo;

	GLuint bsPosDeltasID = 0;
	GLuint bsNorDeltasID = 0;

	std::vector<float> bsPosDeltas; // Blend shape position deltas
	std::vector<float> bsNorDeltas; // Blend shape normal deltas
};

class Shape
{
public:
	Shape();
	virtual ~Shape();
	void loadObj(const std::string &filename, std::vector<float> &pos, std::vector<float> &nor, std::vector<float> &tex, bool loadNor = true, bool loadTex = true);
	void loadMesh(const std::string &meshName);
	void setProgram(std::shared_ptr<Program> p) { prog = p; }
	virtual void init();
	virtual void draw() const;
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }

	std::vector<std::shared_ptr<BlendShape>> blendshapes; // A vector containing the deltas for all blendshapes affecting this shape
	void addBlendShape(std::string filename, std::vector<float>& objPos, std::vector<float>& objNor, int actionNo);
	
	void saveBufs(){ // Saves the position buffer to a temp variable
		this->originalPos = this->posBuf;
		this->originalNor = this->norBuf;
	} 
	void restoreBufs(){ // restores the position buffer from the saved variable
		this->posBuf = this->originalPos;
		this->norBuf = this->originalNor;
	}

	std::vector<float> posBuf;
	std::vector<float> norBuf;

	bool isLeftEye = false;
	bool isRightEye = false;

	// For FACS
	Emotion e;

	std::string getMeshFilename(){ return this-> meshFilename; }

protected:
	std::string meshFilename;
	std::string textureFilename;
	std::shared_ptr<Program> prog;
	std::vector<float> texBuf;

	std::vector<float> originalPos; // used to save the posBuf for CPU rendering
	std::vector<float> originalNor; // used to save the norBuf for CPU rendering

	GLuint posBufID;
	GLuint norBufID;
	GLuint texBufID;
};

void loadBlendShapeObj(const std::string &filename, std::vector<float> &pos, std::vector<float> &nor);


#endif

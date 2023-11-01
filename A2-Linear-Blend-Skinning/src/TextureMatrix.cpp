#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "TextureMatrix.h"

using namespace std;
using namespace glm;

TextureMatrix::TextureMatrix()
{
	type = Type::NONE;
	T = mat3(1.0f);
}

TextureMatrix::~TextureMatrix()
{
	
}

void TextureMatrix::setType(const string &str)
{
	if(str.find("Body") != string::npos) {
		type = Type::BODY;
	} else if(str.find("Mouth") != string::npos) {
		type = Type::MOUTH;
	} else if(str.find("Eyes") != string::npos) {
		type = Type::EYES;
	} else if(str.find("Brows") != string::npos) {
		type = Type::BROWS;
	} else {
		type = Type::NONE;
	}
}

void TextureMatrix::update(unsigned int key)
{
	// Update T here

	// Recall that the bottom left of the texture is 0,0 and the top right is 1,1
	// Translate	1.0 0.0 tx
	//	 			0.0 1.0 ty
	// 				0.0 0.0 1.0

	if(type == Type::BODY) {
		// Do nothing
	} else if(type == Type::MOUTH) {
		// TODO
		float horizMove = 1.0/10.0f;
		float vertMove = 1.0/10.0f;
		if (key == 'm'){
			// m Move the mouth texture coordinates horizontally within the texture

			T[2][0] += horizMove;
			if (T[2][0] - 2/10.0f > 0.01f){
				T[2][0] = 0.0f;
			}
		}else if (key == 'M'){
			// M Move the mouth texture coordinates vertically within the texture
			T[2][1] += vertMove;
		}
	} else if(type == Type::EYES) {
		// TODO
		// e Move the eye texture coordinates horizontally within the texture
		// E Move the eye texture coordinates vertically within the texture

		float horizMove = 1.0/5.0f;
		float vertMove = 1.0/10.0f;

		if (key == 'e'){
			T[2][0] = T[2][0] + horizMove;
			// The horizontal limit is 3/5;
			if (abs(T[2][0] - 3.0f/5.0f) < 0.01f){
				T[2][0] = 0.0f;
			}
		}
		else if (key == 'E'){
			T[2][1] = T[2][1] + vertMove;
		}

	} else if(type == Type::BROWS) {
		// TODO
		// b Move the brow texture coordinates vertically within the texture
		float vertMove = 1.0/10.0f;
		if (key == 'b'){
			T[2][1] += vertMove;
		}
	}
}

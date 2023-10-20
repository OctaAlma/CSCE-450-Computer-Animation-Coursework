#ifndef HELICOPTER_KEYFRAME
#define HELICOPTER_KEYFRAME

#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Camera.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"

enum HelicopterMeshes
{
    BODY0=0,
    BODY1,
    PROP0,
    PROP1
};

class HelicopterKeyframe{
    public:
        HelicopterKeyframe(){
            pos = glm::vec3(0,0,0);
            propSpin = false;
        }
        HelicopterKeyframe(glm::vec3 Pos, glm::quat Quat){
            this->pos = Pos;
            this->quat = Quat;
            propSpin = false;
        }
        //void init(std::string RESOURCE_DIR);
        void drawHelicopter(std::shared_ptr<Program> &prog, std::shared_ptr<MatrixStack> &P, std::shared_ptr<MatrixStack> &MV, std::vector<std::shared_ptr<Shape>>& meshes, float t);

        glm::vec3 pos;
        glm::quat quat;
        bool propSpin;

    private:

};

#endif
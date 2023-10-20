#include "HelicopterKeyframe.h"

void HelicopterKeyframe::drawHelicopter(std::shared_ptr<Program> &prog, std::shared_ptr<MatrixStack> &P, std::shared_ptr<MatrixStack> &MV, std::vector<std::shared_ptr<Shape>>& meshes, float t){
    std::shared_ptr<Shape> body0 = meshes.at(BODY0);
    std::shared_ptr<Shape> body1 = meshes.at(BODY1);
    std::shared_ptr<Shape> prop0 = meshes.at(PROP0);
    std::shared_ptr<Shape> prop1 = meshes.at(PROP1);
    
    MV->pushMatrix();

    glm::mat4 rotationDest = glm::mat4_cast(glm::normalize(quat));

    rotationDest[3] = glm::vec4(pos, 1.0f);    
    
    MV->multMatrix(rotationDest);
    
    glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniform3f(prog->getUniform("kd"), 0.2f, 0.6f, 0.8f);
	body0->draw(prog);
	
	glUniform3f(prog->getUniform("kd"), 1.0f, 0.5f, 0.0f);
	body1->draw(prog);
	
	glUniform3f(prog->getUniform("kd"), 0.8f, 0.8f, 0.8f);
	MV->pushMatrix();
        if (propSpin){
		    MV->rotate(M_PI * t * 2.0f, 0, 1, 0);
        }
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		prop0->draw(prog);
	MV->popMatrix();
	
	MV->pushMatrix();
        if (propSpin){
            MV->translate(0.6228, 0.1179, 0.1365);
            MV->rotate(M_PI * t * 1.5, 0, 0, 1);
            MV->translate(-0.6228, -0.1179, -0.1365);
        }

		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		prop1->draw(prog);
	MV->popMatrix();

    MV->popMatrix();
}
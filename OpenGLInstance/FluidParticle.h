#pragma once

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <filesystem>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "camera.h"
#include "shader.h"
#include "Coordination.h"

class FluidParticle
{
public:
	FluidParticle(std::string vPath = "D:/RenderData/");
	~FluidParticle();
	std::vector<double>& getVerticesVec(int index) { return vertices[index]; }
	int getVerticesNum(int index) { return vertices_num[index]; }
	int getFrameNum() { return vertices.size(); }
	void setTranslate(glm::dvec3 T) { translate = T; }

	void bindFrameData(int index);

	unsigned VBO;
	unsigned VAO;
protected:
	void loadParticleData(std::filesystem::path path);
	void loadVertices(std::filesystem::path file);
private:
	std::vector<std::vector<double>> vertices;
	glm::dvec3 translate;
	std::vector<int> vertices_num;

};


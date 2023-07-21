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
	FluidParticle(std::filesystem::path path, glm::dmat4& model, glm::dmat4& view, glm::dvec3 &T);
	~FluidParticle();

	FluidParticle* loadVertices(std::filesystem::path file);
	void bind() { glCall(glBindVertexArray(VAO)); }

	int getVerticesNum(int index) { return vertices.size(); }
	void setTranslate(glm::dvec3 &T) { translate = T; }
	GLuint getVBOId() { return VBO; }
	
	void preMultiView(glm::dmat4& model, glm::dmat4& view);

private:
	std::vector<double> vertices;
	glm::dvec3 translate;

	GLuint VBO;
	GLuint VAO;
};


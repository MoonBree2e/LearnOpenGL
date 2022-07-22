#pragma once
#ifndef _PARTICLES_H
#define _PARTICLES_H
#include "buffer.h"
#include "shader.h"
#include "vertex_array_object.h"
#include <glm/glm.hpp>

using namespace glcs;

struct alignas(16) Particle {
    glm::vec4 position = glm::vec4(0);
    glm::vec4 velocity = glm::vec4(0);
    float mass = 0;
};

class Particles {
private:
    VertexArrayObject VAO;
    const Buffer* ParticlesBuffer;

public:
    Particles() {}

    void setParticles(const Buffer* buffer) {
        this->ParticlesBuffer = buffer;
        VAO.bindVertexBuffer(*buffer, 0, 0, sizeof(Particle));

        // position
        VAO.activateVertexAttribution(0, 0, 3, GL_FLOAT, 0);
        
        // velocity
        VAO.activateVertexAttribution(0, 1, 3, GL_FLOAT, 4 * sizeof(GLfloat));

        // mass
        VAO.activateVertexAttribution(0, 2, 1, GL_FLOAT, 8 * sizeof(GLfloat));
    }

    void draw(const Pipeline& pipleline) const {
        pipleline.activate();
        VAO.activate();
        glDrawArrays(GL_POINTS, 0, ParticlesBuffer->getLength());
        VAO.deactivate();
        pipleline.deactivate();
    }
};

#endif
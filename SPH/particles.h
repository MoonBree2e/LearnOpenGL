#pragma once
#pragma once
#include "buffer.h"
#include "shader.h"
#include "vertex_array_object.h"
#include <glm/glm.hpp>

using namespace glcs;

struct Particle {
    alignas(16) glm::vec3 position = glm::vec3(0);
    alignas(16) glm::vec3 velocity = glm::vec3(0);
    float density = 0;
    float pressure = 0;
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
    }

    void draw(const Pipeline& pipleline) const {
        pipleline.activate();
        VAO.activate();
        glDrawArrays(GL_POINTS, 0, ParticlesBuffer->getLength());
        VAO.deactivate();
        pipleline.deactivate();
    }
};
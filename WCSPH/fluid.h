#pragma once

#include <glm/glm.hpp>

#include "baseObject.h"
#include "particles.h"
#include "shader.h"
#include "sort.h"

namespace glcs {

typedef std::shared_ptr<class Fluid> FluidRef;

class Fluid : public BaseObject {
public:
    Fluid(const std::string& name): BaseObject(name)
    {
        m_ParticlesNum = 80000;
        m_GridRes = glm::ivec3(21);
        m_GravityStrength = 900.f;
        m_GravityDir = glm::vec3(0, -1, 0);
        m_ParticleRadius = 0.01f;
        m_RestDensity = 500.f;
        m_ViscosityCoefficient = 200;
        m_Stiffness = 100.0f;
        m_RestPressure = 0.0f;
        m_RenderMode = 0;
        m_TimeScale = 0.012f;
    }

public:

    FluidRef setUp();
    void update(double time) override;
    void draw() override;
    void reset() override {}

    static FluidRef create(const std::string& name) { return std::make_shared<Fluid>(name); }

protected:
    void _dispatchDensityCS(Buffer& ParticlesBuffer);
    void _dispatchUpdateCS(Buffer& inParticles, Buffer& outParticles, float timeStep);

    void _generateInitialParticles();
    void _initBuffers();
private:
    GLuint m_ParticlesNum;
    glm::ivec3 m_GridRes;
    GLuint m_GridCellsNum;
    GLuint m_WorkGroupsNum;
    GLuint m_RenderMode;

    float m_BoxSize;
    float m_GridSpacing;
    float m_KernelRadiuis;
    float m_ParticleMass;
    float m_ParticleRadius;
    float m_ViscosityCoefficient;
    float m_Stiffness;
    float m_RestDensity;
    float m_RestPressure;
    float m_Poly6KernelConst;

    float m_TimeScale;
    glm::vec3 m_GravityDir;
    float m_GravityStrength;
    float m_SpikyKernelConst;
    float m_ViscosityKernelConst;

    SortRef m_Sort;

    std::vector<Particle> m_InitParticles;

    ComputeShader m_DensityCS;
    ComputeShader m_UpdateCS;
    Pipeline m_DensityPipe;
    Pipeline m_UpdatePipe;

    Particles m_Particles;
    Particles m_SortedParticles;


    Buffer m_ParticlesBuffer;
    Buffer m_SortedParticlesBuffer;
};


}
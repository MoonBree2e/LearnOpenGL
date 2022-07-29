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
    }

public:

    FluidRef setUp();
    void update(double time) override;
    void draw() override;
    void reset() override {}

    static FluidRef create(const std::string& name) { return std::make_shared<Fluid>(name); }

protected:
    void _dispatchDensityCS();
    void _dispatchUpdateCS();

private:
    GLuint m_ParticlesNum;
    GLuint m_GridRes;
    GLuint m_GridCellsNum;
    GLuint m_WorkGroupsNum;
    GLuint m_RenderMode;

    float m_BoxSize;
    float m_GridSpacing;
    float m_KernelRadiuis;
    float m_ParticleMass;
    float m_ParticleRadius;
    float m_ViscosityCoefficient;

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
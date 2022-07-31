#pragma once

#include <glm/glm.hpp>

#include "baseObject.h"
#include "particles.h"
#include "shader.h"
#include "sort.h"
#include "camera.h"
#include "buffer.h"

#define VIEWRES_X 800
#define VIEWRES_Y 600

namespace glcs {

typedef std::shared_ptr<class Fluid> FluidRef;

class Fluid : public BaseObject {
public:
    Fluid(const std::string& name) : BaseObject(name), 
        m_UpdateCS("fluid_update.comp"),
        m_DensityCS("fluid_density.comp"),
        m_VertexShader("fluid_render.vert"),
        m_FragmentShader("fluid_render.frag")
    {
        m_ParticlesNum = 8000;
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

        m_DensityPipe.attachComputeShader(m_DensityCS);
        m_UpdatePipe.attachComputeShader(m_UpdateCS);

        m_RenderPipe.attachVertexShader(m_VertexShader);
        m_RenderPipe.attachFragmentShader(m_FragmentShader);

        m_Sort = Sort::create();

        setUp();
    }
    ~Fluid() override{
        m_ParticlesBuffer.release();
        m_SortedParticlesBuffer.release();
    }

    void setUp();
    void update(double time) override;
    void draw() override;
    void reset() override {}


    int getNParticles() { return m_ParticlesNum; }
    void setNParticles(int num) { m_ParticlesNum = num; }
    void processKeyboard(const CameraMoveDirection& vDir, float vDeltaTime) { m_Camera.processKeyboard(vDir, vDeltaTime); }
    void processMouseMovement(float vXoffset, float vYoffset, GLboolean constrainPitch = true) { m_Camera.processMouseMovement(vXoffset, vYoffset, constrainPitch); }
    void processMouseScroll(float vYoffset) { m_Camera.processMouseScroll(vYoffset); }

    static FluidRef create(std::string name) { return std::make_shared<Fluid>(name); }

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

    float m_BoxSize = 1.0f;
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

    float m_PointRenderScale = 300.0f;

    SortRef m_Sort;

    std::vector<Particle> m_InitParticles;

    ComputeShader m_DensityCS;
    ComputeShader m_UpdateCS;
    Pipeline m_DensityPipe;
    Pipeline m_UpdatePipe;

    VertexShader m_VertexShader;
    FragmentShader m_FragmentShader;
    Pipeline m_RenderPipe;

    Particles m_Particles;
    Particles m_SortedParticles;


    Buffer m_ParticlesBuffer;
    Buffer m_SortedParticlesBuffer;

    Camera m_Camera{ glm::vec3(0.f, 0.f, 2.f) };
};
}
#pragma once

#include <glm/glm.hpp>
#include <cmath>

#include "baseObject.h"
#include "particles.h"
#include "shader.h"
#include "sort.h"
#include "camera.h"
#include "buffer.h"

#define uint unsigned int
#define VIEWRES_X 800
#define VIEWRES_Y 600
#define PI 3.14159265358979323846

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
        spdlog::info("[Fluid] create Fluid");
        
        m_Camera.setSensitivity(0.05);
        m_Camera.setMoveSpeed(2);
            
        // particles
        m_ParticlesNum = 40000;
        m_ParticleRadius = 0.01f;
        m_ParticleMass = m_ParticleRadius * 8.0f;
        m_GravityStrength = 900.f;
        m_GravityDir = glm::vec3(0, -1, 0);
        m_RestDensity = 500.f;
        m_ViscosityCoefficient = 200;
        m_Stiffness = 100.0f;
        m_RestPressure = 0.0f;
        m_RenderMode = 0;
        m_TimeScale = 0.012f;

        // params
        m_BoxSize = 1.f;
        m_GridRes = glm::ivec3(21);
        m_GridCellsNum = m_GridRes.x * m_GridRes.y * m_GridRes.z;
        m_GridSpacing = m_BoxSize / glm::vec3(m_GridRes);

        // kernel function
        m_KernelRadiuis = m_ParticleRadius * 4.0f;
        m_Poly6KernelConst = static_cast<float>(315.0 / (64.0 * PI * glm::pow(m_KernelRadiuis, 9)));
        m_SpikyKernelConst = static_cast<float>(-45.0 / (PI * glm::pow(m_KernelRadiuis, 6)));
        m_ViscosityKernelConst = static_cast<float>(45.0 / (PI * glm::pow(m_KernelRadiuis, 6)));

        
       
        m_DensityPipe.attachComputeShader(m_DensityCS);
        m_UpdatePipe.attachComputeShader(m_UpdateCS);

        m_RenderPipe.attachVertexShader(m_VertexShader);
        m_RenderPipe.attachFragmentShader(m_FragmentShader);

        m_Sort = Sort::create();
        m_Sort->setParticlesNum(m_ParticlesNum);
        m_Sort->setGridRes(m_GridRes);
        m_Sort->setGridSpacing(m_GridSpacing);
        m_Sort->setUp();

        m_LinePiepe.attachVertexShader(m_LineVertexShader);
        m_LinePiepe.attachFragmentShader(m_LineFragmentShader);

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
    int _computeWorkGoupsNum() { return int(ceil(float(m_ParticlesNum) / float(m_WorkGroupSize)));  }

    void _drawLine(glm::vec3 a, glm::vec3 b);
    void _drawCube(std::vector<glm::vec3>&);

private:
    GLuint m_ParticlesNum;
    glm::ivec3 m_GridRes;
    GLuint m_GridCellsNum;
    GLuint m_RenderMode;
    const uint m_WorkGroupSize = 128;

    float m_BoxSize;
    glm::vec3 m_GridSpacing;
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

    Buffer m_ParticlesBuffer = Buffer("ParticlesBuffer");
    Buffer m_SortedParticlesBuffer = Buffer("SortedParticlesBuffer");

    Camera m_Camera{ glm::vec3(0.f, 0.f, 2.f) };

    VertexShader m_LineVertexShader = VertexShader("line.vert");
    FragmentShader m_LineFragmentShader = FragmentShader("line.frag");
    Pipeline m_LinePiepe;
    VertexArrayObject m_LineVAO;
    Buffer m_LineBuffer = Buffer("LineBuffer");
};
}
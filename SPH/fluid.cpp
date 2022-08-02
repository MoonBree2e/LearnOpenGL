#include "fluid.h"
#include <time.h>

using namespace glcs;
void Fluid::setUp()
{
    _generateInitialParticles();
    _initBuffers();

    m_Particles.setParticles(&m_ParticlesBuffer);
    m_SortedParticles.setParticles(&m_SortedParticlesBuffer);

    glObjectLabel(GL_PROGRAM, m_DensityCS.getProgram(), -1, "DensityComputeShader");
    glObjectLabel(GL_PROGRAM, m_UpdateCS.getProgram(), -1, "UpdateComputeShader");
    glObjectLabel(GL_PROGRAM_PIPELINE, m_DensityPipe.PipelineID, -1, "DensityPipeline");
    glObjectLabel(GL_PROGRAM_PIPELINE, m_UpdatePipe.PipelineID, -1, "UpdatePipeline");

    glObjectLabel(GL_PROGRAM, m_VertexShader.getProgram(), -1, "FluidRenderVertexShader");
    glObjectLabel(GL_PROGRAM, m_FragmentShader.getProgram(), -1, "FluidRenderFragmentShader");
    glObjectLabel(GL_PROGRAM_PIPELINE, m_RenderPipe.PipelineID, -1, "FluidRenderPipeline");
}

void Fluid::update(double time) {
    spdlog::info("[Fluid] update");

    m_Sort->run(m_ParticlesBuffer, m_SortedParticlesBuffer);
    _dispatchDensityCS(m_SortedParticlesBuffer);
    _dispatchUpdateCS(m_SortedParticlesBuffer, m_ParticlesBuffer, time);
}

void Fluid::draw()
{
    spdlog::info("[Fluid] draw");
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Render Fluid pass");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, VIEWRES_X, VIEWRES_Y);
    glEnable(GL_PROGRAM_POINT_SIZE);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE);

    m_VertexShader.setUniform("viewProj", m_Camera.getProjectViewMatrix(VIEWRES_X, VIEWRES_Y));
    m_FragmentShader.setUniform("cameraPos", m_Camera.Position);
    m_FragmentShader.setUniform("lightPos", glm::vec3(0, m_BoxSize / 2, m_BoxSize));
    glPointSize(m_PointRenderScale * m_ParticleRadius * 8);
    m_Particles.draw(m_RenderPipe);
    std::vector<glm::vec3> cubeCoords = { glm::vec3(0,0,0), glm::vec3(1,0,0), glm::vec3(1,1,0), glm::vec3(0,1,0),
                                      glm::vec3(0,0,1), glm::vec3(1,0,1), glm::vec3(1,1,1), glm::vec3(0,1,1) };
    _drawCube(cubeCoords);
    glPopDebugGroup();
}

void Fluid::_dispatchDensityCS(Buffer& ParticlesBuffer) {
    spdlog::info("[Fluid] _dispatchDensityCS");
    ParticlesBuffer.bindToShaderStorageBuffer(0);
    m_Sort->fetchCountBuffer().bindToShaderStorageBuffer(1);
    m_Sort->fetchOffsetBuffer().bindToShaderStorageBuffer(2);

    m_DensityCS.setUniform("BoxSize", m_BoxSize);
    m_DensityCS.setUniform("GridSpacing", m_GridSpacing);
    m_DensityCS.setUniform("GridRes", m_GridRes);
    m_DensityCS.setUniform("ParticlesNum", m_ParticlesNum);
    m_DensityCS.setUniform("KernelRadius", m_KernelRadiuis);
    m_DensityCS.setUniform("Stiffness", m_Stiffness);
    m_DensityCS.setUniform("RestDensity", m_RestDensity);
    m_DensityCS.setUniform("RestPressure", m_RestPressure);
    m_DensityCS.setUniform("Poly6KernelConst", m_RestDensity);

    m_DensityPipe.activate();
    glDispatchCompute(_computeWorkGoupsNum(), 1, 1);
    m_DensityPipe.deactivate();
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::_dispatchUpdateCS(Buffer& inParticles, Buffer& outParticles, float timeStep) {
    spdlog::info("[Fluid] _dispatchUpdateCS");
    inParticles.bindToShaderStorageBuffer(0);
    m_Sort->fetchCountBuffer().bindToShaderStorageBuffer(1);
    m_Sort->fetchOffsetBuffer().bindToShaderStorageBuffer(2);
    outParticles.bindToShaderStorageBuffer(3);

    m_UpdateCS.setUniform("BoxSize", m_BoxSize);
    m_UpdateCS.setUniform("GridSpacing", m_GridSpacing);
    m_UpdateCS.setUniform("GridRes", m_GridRes);
    m_UpdateCS.setUniform("dt", timeStep * m_TimeScale);
    m_UpdateCS.setUniform("ParticlesNum", m_ParticlesNum);
    m_UpdateCS.setUniform("Gravity", m_GravityDir * m_GravityStrength);
    m_UpdateCS.setUniform("ParticleMass", m_ParticleMass);
    m_UpdateCS.setUniform("KernelRadius", m_KernelRadiuis);
    m_UpdateCS.setUniform("ViscosityCoefficient", m_ViscosityCoefficient);
    m_UpdateCS.setUniform("SpikyKernelConst", m_SpikyKernelConst);
    m_UpdateCS.setUniform("ViscosityKernelConst", m_ViscosityKernelConst);

    m_UpdatePipe.activate();
    glDispatchCompute(_computeWorkGoupsNum(), 1, 1);
    m_UpdatePipe.deactivate();

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::_generateInitialParticles() {
    srand(0);
    spdlog::info("[Fluid] createing particles");
    int num = m_ParticlesNum;
    m_InitParticles.assign(num, Particle());
    float distance = m_ParticleRadius * 1.75f;

    int d = int(ceil(std::cbrt(num))); //cube root num

    float jitter = distance * 0.5f;
    auto getJitter = [jitter]() { return ((float)rand() / RAND_MAX) * jitter - (jitter / 2.0f); };

    for (int z = 0; z < d; ++z) {
        for (int y = 0; y < d; ++y) {
            for (int x = 0; x < d; ++x) {
                int index = z * d * d + y * d + x;
                if (index >= num) break;

                // dam break
                m_InitParticles[index].position = glm::vec3(x, y, z) * distance;
                //m_InitParticles[index].position += glm::vec3(getJitter(), getJitter(), getJitter());
            }
        }
    }
}

void Fluid::_initBuffers() {
    spdlog::info("[Fluid] init buffers");

    const auto size = m_ParticlesNum * sizeof(Particle);
    m_ParticlesBuffer.setStorage(m_InitParticles, 0);
    m_SortedParticlesBuffer.setStorage(m_InitParticles, 0);


    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
}

void Fluid::_drawLine(glm::vec3 a, glm::vec3 b)
{
    std::vector<glm::vec3> vec = { a, b };
    m_LineBuffer.setData(vec, GL_STATIC_DRAW);
    m_LineVAO.bindVertexBuffer(m_LineBuffer, 0, 0, 3 * sizeof(float));
    m_LineVAO.activateVertexAttribution(0, 0, 3, GL_FLOAT, 0);

    m_LineVertexShader.setUniform("viewProj", m_Camera.getProjectViewMatrix(VIEWRES_X, VIEWRES_Y));
    m_LinePiepe.activate();
    m_LineVAO.activate();
    glDrawArrays(GL_LINES, 0, 2);
    m_LineVAO.deactivate();
    m_LinePiepe.deactivate();
}

void Fluid::_drawCube(std::vector<glm::vec3>& vec)
{
    assert(vec.size() == 8);
    std::vector<int> index = { 0,1, 1,2, 2,3, 3,0, 0,4, 1,5, 2,6, 3,7, 4,5, 5,6, 6,7, 7,4 };
    for (int i = 0; i < index.size(); i += 2)
    {
        _drawLine(vec[index[i]], vec[index[i + 1]]);
    }
}

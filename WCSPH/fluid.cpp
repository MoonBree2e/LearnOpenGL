#include "fluid.h"
#include <time.h>

using namespace glcs;

void Fluid::update(double time) {
    m_Sort->run(m_ParticlesBuffer, m_SortedParticlesBuffer);
    _dispatchDensityCS();
    _dispatchUpdateCS();
}

void Fluid::_dispatchDensityCS(Buffer& ParticlesBuffer) {
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
    glDispatchCompute(std::ceil(m_ParticlesNum / 128.f), 1, 1);
    m_DensityPipe.deactivate();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::_dispatchUpdateCS(Buffer& inParticles, Buffer& outParticles, float timeStep) {
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
    glDispatchCompute(std::ceil(m_ParticlesNum / 128.f), 1, 1);
    m_UpdatePipe.deactivate();

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::_generateInitialParticles() {
    srand(0);
    spdlog::info("createing particles");
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
                m_InitParticles[index].position += glm::vec3(getJitter(), getJitter(), getJitter());
            }
        }
    }
}

void Fluid::_initBuffers() {
    spdlog::info("init buffers");

    const auto size = m_ParticlesNum * sizeof(Particle);
    m_ParticlesBuffer.setStorage(m_InitParticles, 0);
    m_SortedParticlesBuffer.setStorage(m_InitParticles, 0);
    

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

}
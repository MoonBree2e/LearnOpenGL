#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "shader.h"
#include "buffer.h"
namespace glcs {
    typedef std::shared_ptr<class Sort> SortRef;

    class Sort {
    public:
        Sort();
        ~Sort() {}

        SortRef setParticlesNum(int n)
        {

        }

    protected:
        void _clearCountBuffer()
        {
            std::vector<GLuint> inital(m_PartcilesNum, 0);
            m_CountBuffer.clearData(inital, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        }
        void _clearOffsetBuffer()
        {
            std::vector<GLuint> inital(m_PartcilesNum, 0);
            m_OffsetBuffer.clearData(inital, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        }
        void _dispatchCountCS(Buffer& particleBuffer)
        {
            particleBuffer.bindToShaderStorageBuffer(0);
            m_CountBuffer.bindToShaderStorageBuffer(1);

            m_CountCS.setUniform("spacing", m_Spacing);
            m_CountCS.setUniform("particleTotalNum", m_PartcilesNum);
            m_CountCS.setUniform("gridRes", m_GriRes);

            m_CountPipe.activate();
            glDispatchCompute(std::ceil(m_PartcilesNum / 128.f), 1, 1);
            m_CountPipe.deactivate();
        }
        void _dispatchLinearScanCS()
        {
            m_LinearScanCS.setUniform("gridRes", m_GriRes);
            m_CountBuffer.bindToShaderStorageBuffer(0);
            m_OffsetBuffer.bindToShaderStorageBuffer(1);

            m_LinearScanPipe.activate();
            glDispatchCompute(1, 1, 1);
            m_LinearScanPipe.deactivate();
        }
        void _dispatchReorderCS(Buffer& inParticles, Buffer& outParticles)
        {
            inParticles.bindToShaderStorageBuffer(0);
            outParticles.bindToShaderStorageBuffer(1);
            m_CountBuffer.bindToShaderStorageBuffer(2);
            m_OffsetBuffer.bindToShaderStorageBuffer(3);

            m_ReorderPipe.activate();
            glDispatchCompute(std::ceil(m_PartcilesNum / 128.f), 1, 1);
            m_ReorderPipe.deactivate();
        }

        float m_Spacing;
        GLuint m_PartcilesNum;
        glm::ivec3 m_GriRes;

        ComputeShader m_CountCS;
        ComputeShader m_LinearScanCS;
        ComputeShader m_ReorderCS;

        Pipeline m_CountPipe;
        Pipeline m_LinearScanPipe;
        Pipeline m_ReorderPipe;

        Buffer m_CountBuffer;
        Buffer m_OffsetBuffer;
    };

}
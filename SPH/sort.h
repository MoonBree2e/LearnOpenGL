#pragma once
#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "shader.h"
#include "buffer.h"

namespace glcs {
    typedef std::shared_ptr<class Sort> SortRef;

    class Sort {
    public:
        Sort() :m_CountCS("sort_count.comp"), m_LinearScanCS("sort_linearSort.comp"), m_ReorderCS("sort_reorder.comp") {
            spdlog::info("[Sort] sort create");
        }
        ~Sort() {
            m_CountBuffer.release();
            m_OffsetBuffer.release();
        }

        void setParticlesNum(GLuint n)
        {
            m_PartcilesNum = n;
        }

        void setGridRes(glm::ivec3 r)
        {
            m_GriRes = r;
            m_GridCellNum = r.x * r.y * r.z;
        }

        void setGridSpacing(glm::vec3 s)
        {
            m_Spacing = s;
        }

        void setUp()
        {
            spdlog::info("[Sort] setUp");
            m_CountPipe.attachComputeShader(m_CountCS);
            m_LinearScanPipe.attachComputeShader(m_LinearScanCS);
            m_ReorderPipe.attachComputeShader(m_ReorderCS);

            m_CountBuffer.setStorage<uint32_t>(m_GridCellNum);
            m_OffsetBuffer.setStorage<uint32_t>(m_GridCellNum);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

            //glObjectLabel(GL_BUFFER, m_CountBuffer.getName(), -1, "CountBuffer");
            //glObjectLabel(GL_BUFFER, m_OffsetBuffer.getName(), -1, "OffsetBuffer");

            glObjectLabel(GL_PROGRAM, m_CountCS.getProgram(), -1, "CountComputeShader");
            glObjectLabel(GL_PROGRAM, m_LinearScanCS.getProgram(), -1, "LinearScanComputeShader");
            glObjectLabel(GL_PROGRAM, m_ReorderCS.getProgram(), -1, "ReordertComputeShader");

            glObjectLabel(GL_PROGRAM_PIPELINE, m_CountPipe.PipelineID, -1, "CountPipeline");
            glObjectLabel(GL_PROGRAM_PIPELINE, m_LinearScanPipe.PipelineID, -1, "LinearScanPipeline");
            glObjectLabel(GL_PROGRAM_PIPELINE, m_ReorderPipe.PipelineID, -1, "ReorderPipeline");
        }

        void run(Buffer& inParticles, Buffer& outParticles)
        {
            spdlog::info("[Sort] run");
            _clearCountBuffer();
            _dispatchCountCS(inParticles);

            _clearOffsetBuffer();
            _dispatchLinearScanCS();

            _clearCountBuffer();
            _dispatchReorderCS(inParticles, outParticles);
        }

        Buffer& fetchCountBuffer() { return m_CountBuffer; }
        Buffer& fetchOffsetBuffer() { return m_OffsetBuffer; }

        static SortRef create() { return std::make_shared<Sort>(); }

    protected:
        void _clearCountBuffer()
        {
            spdlog::info("[Sort] _clearCountBuffer");

            std::vector<GLuint> inital(m_PartcilesNum, 0);
            m_CountBuffer.clearData(inital, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        }
        void _clearOffsetBuffer()
        {
            spdlog::info("[Sort] _clearOffsetBuffer");

            std::vector<GLuint> inital(m_PartcilesNum, 0);
            m_OffsetBuffer.clearData(inital, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        }
        void _dispatchCountCS(Buffer& particleBuffer)
        {
            spdlog::info("[Sort] _dispatchCountCS");

            particleBuffer.bindToShaderStorageBuffer(0);
            m_CountBuffer.bindToShaderStorageBuffer(1);

            m_CountCS.setUniform("spacing", m_Spacing);
            m_CountCS.setUniform("particleTotalNum", m_PartcilesNum);
            m_CountCS.setUniform("gridRes", m_GriRes);

            m_CountPipe.activate();
            glDispatchCompute((GLuint)std::ceil(m_PartcilesNum / 128.f), 1, 1);
            m_CountPipe.deactivate();
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        void _dispatchLinearScanCS()
        {
            spdlog::info("[Sort] _dispatchLinearScanCS");

            m_LinearScanCS.setUniform("gridRes", m_GriRes);
            m_CountBuffer.bindToShaderStorageBuffer(0);
            m_OffsetBuffer.bindToShaderStorageBuffer(1);

            m_LinearScanPipe.activate();
            glDispatchCompute(1, 1, 1);
            m_LinearScanPipe.deactivate();
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        void _dispatchReorderCS(Buffer& inParticles, Buffer& outParticles)
        {
            spdlog::info("[Sort] _dispatchReorderCS");

            inParticles.bindToShaderStorageBuffer(0);
            outParticles.bindToShaderStorageBuffer(1);
            m_CountBuffer.bindToShaderStorageBuffer(2);
            m_OffsetBuffer.bindToShaderStorageBuffer(3);
            
            m_ReorderCS.setUniform("spacing", m_Spacing);
            m_ReorderCS.setUniform("particleTotalNum", m_PartcilesNum);
            m_ReorderCS.setUniform("gridRes", m_GriRes);

            m_ReorderPipe.activate();
            glDispatchCompute((GLuint)std::ceil(m_PartcilesNum / 128.f), 1, 1);
            m_ReorderPipe.deactivate();
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        SortRef _thisRef() { return std::shared_ptr<Sort>(this); }


        glm::vec3 m_Spacing;
        GLuint m_GridCellNum;
        GLuint m_PartcilesNum;
        glm::ivec3 m_GriRes;

        ComputeShader m_CountCS;
        ComputeShader m_LinearScanCS;
        ComputeShader m_ReorderCS;

        Pipeline m_CountPipe;
        Pipeline m_LinearScanPipe;
        Pipeline m_ReorderPipe;

        Buffer m_CountBuffer = Buffer("CountBuffer");
        Buffer m_OffsetBuffer = Buffer("OffsetBuffer");
    };

}
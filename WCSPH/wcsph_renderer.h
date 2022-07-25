#pragma once
#include <random>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "buffer.h"
#include "shader.h"
#include "camera.h"

#include "particles.h"

using namespace glcs;

class Renderer
{
private:
private:
    glm::uvec2 m_Resolution;
    uint32_t m_NParticles;
    glm::vec3 m_GravityCenter;
    float m_GravityIntensity;
    float m_K;
    float m_Dt;
    bool m_IncreaseK;
    bool m_Pause;
    glm::vec3 m_BaseColor;

    Camera m_Camera{ glm::vec3(0.f, 0.f, 2.f) };

    Particles m_Particles;
    Buffer m_ParticlesBuffer;

    ComputeShader m_UpdateParticlesCS;
    Pipeline m_UpdateParticlesPipelie;

    VertexShader m_VertexShader;
    FragmentShader m_FragmentShader;
    Pipeline m_RenderPipeline;

    float m_ElapsedTime;

public:
    Renderer() :m_Resolution{ 512, 512 }, m_NParticles(1000000), m_GravityCenter{ 0 }, m_GravityIntensity{ 0.1f },
        m_K{ 0.1f }, m_Dt{ 0.01f }, m_IncreaseK{ false }, m_Pause{ false }, m_BaseColor{ 0.2, 0.4, 0.8 },
        m_UpdateParticlesCS("update_particles.comp"),
        m_VertexShader("render_particles.vert"), m_FragmentShader("render_particles.frag"),
        m_ElapsedTime{ 0 }
    {
        m_Particles.setParticles(&m_ParticlesBuffer);

        m_UpdateParticlesPipelie.attachComputeShader(m_UpdateParticlesCS);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        m_RenderPipeline.attachVertexShader(m_VertexShader);
        m_RenderPipeline.attachFragmentShader(m_FragmentShader);

        placeParticles();
    }

    void placeParticles() {
        std::random_device rnd_dev;
        std::mt19937 mt(rnd_dev());
        std::uniform_real_distribution<float> dist(-1, 1);

        std::vector<Particle> data(m_NParticles);
        for (std::size_t i = 0; i < m_NParticles; ++i) {
            data[i].position = 0.5f * glm::vec4(dist(mt), dist(mt), dist(mt), 0);
            data[i].velocity = glm::vec4(0);
            data[i].mass = 1.0f;
        }

        m_ParticlesBuffer.setData(data, GL_DYNAMIC_DRAW);
    }

    glm::vec3 screenToWorld(const glm::vec2& screen) const {
        auto [origin, direction] = m_Camera.getRay(screen, m_Resolution);

        // ray-plane intersection at z = 0
        const float t = -origin.z / direction.z;
        glm::vec3 p = origin + t * direction;

        return p;
    }

    void render(float vDeletaTime)
    {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Render pass");

        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, m_Resolution.x, m_Resolution.y);
        m_VertexShader.setUniform("viewProjection", m_Camera.getProjectViewMatrix(m_Resolution.x, m_Resolution.y));
        m_FragmentShader.setUniform("baseColor", m_BaseColor);
        m_Particles.draw(m_RenderPipeline);

        glPopDebugGroup();

        m_ElapsedTime += vDeletaTime;
        if (vDeletaTime > m_Dt && !m_Pause)
        {
            m_ElapsedTime = 0;

            m_ParticlesBuffer.bindToShaderStorageBuffer(0);
            m_UpdateParticlesCS.setUniform("gravityCenter", m_GravityCenter);
            m_UpdateParticlesCS.setUniform("gravityIntensity", m_GravityIntensity);
            m_UpdateParticlesCS.setUniform("increaseK", m_IncreaseK);
            m_UpdateParticlesCS.setUniform("k", m_K);
            m_UpdateParticlesCS.setUniform("dt", m_Dt);
            m_UpdateParticlesPipelie.activate();
            glDispatchCompute(std::ceil(m_NParticles / 128.f), 1, 1);
            m_UpdateParticlesPipelie.deactivate();
        }
    }

    void processKeyboard(const CameraMoveDirection& vDir, float vDeltaTime)
    {
        m_Camera.processKeyboard(vDir, vDeltaTime);
    }

    void processMouseMovement(float vXoffset, float vYoffset, GLboolean constrainPitch = true)
    {
        m_Camera.processMouseMovement(vXoffset, vYoffset, constrainPitch);
    }

    void processMouseScroll(float vYoffset)
    {
        m_Camera.processMouseScroll(vYoffset);
    }

    void setResolution(glm::uvec2 vResolution)
    {
        m_Resolution = vResolution;
    }

    int getNParticles()
    {
        return m_NParticles;
    }

    void setNParticles(int num)
    {
        m_NParticles = num;
    }

    glm::vec3 getBaseColor()
    {
        return m_BaseColor;
    }

    void setBaseColor(glm::vec3 vColor)
    {
        m_BaseColor = vColor;
    }
};
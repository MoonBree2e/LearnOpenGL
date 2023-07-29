#pragma once

#include "GrFramebuffer.h"
#include "GrCamera.h"

#include <glm/glm.hpp>

#include <vector>

// TODO: use the new Framebuffer2 class, and have the fbo be a member rather
// than inheriting from it
// TODO: handle directional lights (only handling point light at the moment)
class ShadowMap : public Framebuffer {
public:
	ShadowMap(
		const glm::vec3& lightPosition,
		const glm::vec3& lightLookAt = glm::vec3(0.0),
		size_t size = 1024,
		const std::vector<ColorLayerInfo>& colorLayerInfos = {}
	);

	void setLookAt(const glm::vec3& position, const glm::vec3& lookAt);
	void setProjection(float fov, float nearDistance, float farDistance);

	const GrCamera& camera() const { return m_camera; }

private:
	void updateProjectionMatrix();

private:
	GrCamera m_camera;
	float m_fov = 35.0f;
	float m_near = 0.1f;
	float m_far = 20.f;
};

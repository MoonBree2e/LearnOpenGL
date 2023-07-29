#include "ShadowMap.h"
#include "Logger.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

ShadowMap::ShadowMap(
	const glm::vec3& lightPosition,
	const glm::vec3& lightLookAt,
	size_t size,
	const std::vector<ColorLayerInfo>& colorLayerInfos
)
	: Framebuffer(size, size, colorLayerInfos)
{
	m_camera.setResolution(static_cast<int>(size), static_cast<int>(size));  // must be first
	setLookAt(lightPosition, lightLookAt);
}

void ShadowMap::setLookAt(const glm::vec3& position, const glm::vec3& lookAt) {
	m_camera.setViewMatrix(glm::lookAt(position, lookAt, glm::vec3(0, 0, 1)));
	updateProjectionMatrix();
}

void ShadowMap::setProjection(float fov, float nearDistance, float farDistance)
{
	m_fov = fov;
	m_near = nearDistance;
	m_far = farDistance;
	updateProjectionMatrix();
}

void ShadowMap::updateProjectionMatrix()
{
	m_camera.setProjectionMatrix(glm::perspective<float>(glm::radians(m_fov), 1.f, m_near, m_far));
	m_camera.updateUbo();
}
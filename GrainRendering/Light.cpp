#include "Light.h"
#include "ShadowMap.h"

Light::Light(const glm::vec3& position, const glm::vec3& color, size_t shadowMapSize, bool isRich, bool hasShadowMap)
	: m_lightPosition(position)
	, m_lookAt(0.0, 0.0, 0.0)
	, m_lightColor(color)
	, m_isRich(isRich)
	, m_hasShadowMap(hasShadowMap)
{
	const std::vector<ColorLayerInfo>& colorLayerInfos = { { GL_RGBA32UI, GL_COLOR_ATTACHMENT0 } }; // debug
	m_shadowMap = std::make_unique<ShadowMap>(position, glm::vec3(0.f), shadowMapSize, colorLayerInfos);
	glObjectLabel(GL_FRAMEBUFFER, m_shadowMap->raw(), -1, "shadow map frame buffer");
}

void Light::setPosition(const glm::vec3& position) {
	m_lightPosition = position;
	m_shadowMap->setLookAt(position, m_lookAt);
}
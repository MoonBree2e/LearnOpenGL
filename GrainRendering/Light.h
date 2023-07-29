#pragma once
#include "ShadowMap.h"

class Light {
public:
	Light(const glm::vec3& position, const glm::vec3& color, size_t shadowMapSize = 1024, bool isRich = false, bool hasShadowMap = true);

	inline glm::vec3 position() const { return m_lightPosition; }
	void setPosition(const glm::vec3& position);

	inline glm::vec3 lookAt() const { return m_lookAt; }

	inline const glm::vec3& color() const { return m_lightColor; }
	inline glm::vec3& color() { return m_lightColor; }

	inline bool isRich() const { return m_isRich; }

	inline bool hasShadowMap() const { return m_hasShadowMap; }
	inline void setHasShadowMap(bool value) { m_hasShadowMap = value; }

	inline const ShadowMap& shadowMap() const { return *m_shadowMap; }
	inline ShadowMap& shadowMap() { return *m_shadowMap; }

	virtual void update(float time) {}

protected:
	glm::vec3 m_lightPosition;
	glm::vec3 m_lookAt;
	glm::vec3 m_lightColor;
	std::unique_ptr<ShadowMap> m_shadowMap;
	bool m_isRich;
	bool m_hasShadowMap;
};
#include "GrCamera.h"

#include "Logger.h"
#include "GrFramebuffer.h"

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <iostream>
#include <algorithm>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/quaternion.hpp>

GrCamera::GrCamera()
	: m_isMouseRotationStarted(false)
	, m_isMouseZoomStarted(false)
	, m_isMousePanningStarted(false)
	, m_isLastMouseUpToDate(false)
	, m_fov(45.0f)
	, m_orthographicScale(1.0f)
	, m_freezeResolution(false)
	, m_projectionType(PerspectiveProjection)
	, m_extraFramebuffers(static_cast<int>(ExtraFramebufferOption::_Count))
{
	initUbo();
	setResolution(1280, 720);
}

GrCamera::~GrCamera() {
	glDeleteBuffers(1, &m_ubo);
}

void GrCamera::update(float time) {
	m_position = glm::vec3(3.f * cos(time), 3.f * sin(time), 0.f);
	m_uniforms.viewMatrix = glm::lookAt(
		m_position,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
}

void GrCamera::initUbo() {
	glCreateBuffers(1, &m_ubo);
	glNamedBufferStorage(m_ubo, static_cast<GLsizeiptr>(sizeof(CameraUbo)), NULL, GL_DYNAMIC_STORAGE_BIT);
	glObjectLabel(GL_BUFFER, m_ubo, -1, "CameraUBO");

}
void GrCamera::updateUbo() {

	m_uniforms.inverseViewMatrix = inverse(m_uniforms.viewMatrix);
	m_uniforms.resolution = resolution();
	glNamedBufferSubData(m_ubo, 0, static_cast<GLsizeiptr>(sizeof(CameraUbo)), &m_uniforms);
}

void GrCamera::setResolution(glm::vec2 resolution)
{
	if (!m_freezeResolution) {
		glm::vec4 rect = properties().viewRect;
		m_uniforms.resolution = resolution * glm::vec2(rect.z, rect.w);
	}

	updateProjectionMatrix();

	// Resize extra framebuffers
	size_t width = static_cast<size_t>(m_uniforms.resolution.x);
	size_t height = static_cast<size_t>(m_uniforms.resolution.y);
	for (auto& fbo : m_extraFramebuffers) {
		if (fbo) fbo->setResolution(width, height);
	}

	updateUbo();
}

void GrCamera::setFreezeResolution(bool freeze)
{
	m_freezeResolution = freeze;
	if (m_freezeResolution) {
		size_t width = static_cast<size_t>(m_uniforms.resolution.x);
		size_t height = static_cast<size_t>(m_uniforms.resolution.y);
		const std::vector<ColorLayerInfo> colorLayerInfos = { { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
		m_targetFramebuffer = std::make_shared<Framebuffer>(width, height, colorLayerInfos);
	}
	else {
		m_targetFramebuffer = nullptr;
	}
}

void GrCamera::setFov(float fov)
{
	m_fov = fov;
	updateProjectionMatrix();
}

float GrCamera::focalLength() const
{
	return 1.0f / (2.0f * glm::tan(glm::radians(fov()) / 2.0f));
}

void GrCamera::setNearDistance(float distance)
{
	m_uniforms.uNear = distance;
	updateProjectionMatrix();
}

void GrCamera::setFarDistance(float distance)
{
	m_uniforms.uFar = distance;
	updateProjectionMatrix();
}


void GrCamera::setOrthographicScale(float orthographicScale)
{
	m_orthographicScale = orthographicScale;
	updateProjectionMatrix();
}

void GrCamera::setProjectionType(ProjectionType projectionType)
{
	m_projectionType = projectionType;
	updateProjectionMatrix();
}

void GrCamera::updateMousePosition(float x, float y)
{
	if (m_isLastMouseUpToDate) {
		if (m_isMouseRotationStarted) {
			updateDeltaMouseRotation(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMouseZoomStarted) {
			updateDeltaMouseZoom(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMousePanningStarted) {
			updateDeltaMousePanning(m_lastMouseX, m_lastMouseY, x, y);
		}
	}
	m_lastMouseX = x;
	m_lastMouseY = y;
	m_isLastMouseUpToDate = true;
}

void GrCamera::updateProjectionMatrix()
{
	switch (m_projectionType) {
	case PerspectiveProjection:
		if (m_uniforms.resolution.x > 0.0f && m_uniforms.resolution.y > 0.0f) {
			m_uniforms.projectionMatrix = glm::perspectiveFov(glm::radians(m_fov), m_uniforms.resolution.x, m_uniforms.resolution.y, m_uniforms.uNear, m_uniforms.uFar);
			m_uniforms.uRight = m_uniforms.uNear / m_uniforms.projectionMatrix[0][0];
			m_uniforms.uLeft = -m_uniforms.uRight;
			m_uniforms.uTop = m_uniforms.uNear / m_uniforms.projectionMatrix[1][1];
			m_uniforms.uBottom = -m_uniforms.uTop;
		}
		break;
	case OrthographicProjection:
	{
		float ratio = m_uniforms.resolution.x > 0.0f ? m_uniforms.resolution.y / m_uniforms.resolution.x : 1.0f;
		m_uniforms.projectionMatrix = glm::ortho(-m_orthographicScale, m_orthographicScale, -m_orthographicScale * ratio, m_orthographicScale * ratio, m_uniforms.uNear, m_uniforms.uFar);
		m_uniforms.uRight = 1.0f / m_uniforms.projectionMatrix[0][0];
		m_uniforms.uLeft = -m_uniforms.uRight;
		m_uniforms.uTop = 1.0f / m_uniforms.projectionMatrix[1][1];
		m_uniforms.uBottom = -m_uniforms.uTop;
		break;
	}
	}
}

glm::vec3 GrCamera::projectSphere(glm::vec3 center, float radius) const
{
	// http://www.iquilezles.org/www/articles/sphereproj/sphereproj.htm
	glm::vec3 o = (viewMatrix() * glm::vec4(center, 1.0f));

	glm::mat2 R = glm::mat2(o.x, o.y, -o.y, o.x);

	float r2 = radius * radius;
	float fl = focalLength();
	float ox2 = o.x * o.x;
	float oy2 = o.y * o.y;
	float oz2 = o.z * o.z;
	float fp = fl * fl * r2 * (ox2 + oy2 + oz2 - r2) / (oz2 - r2);
	float outerRadius = sqrt(abs(fp / (r2 - oz2)));

	glm::vec2 circleCenter = glm::vec2(o.x, o.y) * o.z * fl / (oz2 - r2);

	float pixelRadius = outerRadius * m_uniforms.resolution.y;
	glm::vec2 pixelCenter = circleCenter * m_uniforms.resolution.y * glm::vec2(-1.0f, 1.0f) + m_uniforms.resolution * 0.5f;

	return glm::vec3(pixelCenter, pixelRadius);
}

float GrCamera::linearDepth(float zbufferDepth) const
{
	return (2.0f * m_uniforms.uNear * m_uniforms.uFar) / (m_uniforms.uFar + m_uniforms.uNear - (zbufferDepth * 2.0f - 1.0f) * (m_uniforms.uFar - m_uniforms.uNear));
}

///////////////////////////////////////////////////////////////////////////////
// Framebuffer pool
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Framebuffer> GrCamera::getExtraFramebuffer(const std::string& label, ExtraFramebufferOption option) const
{
	// TODO: add mutex
	auto& fbo = m_extraFramebuffers[static_cast<int>(option)];
	// Lazy construction
	if (!fbo) {
		int width = static_cast<int>(m_uniforms.resolution.x);
		int height = static_cast<int>(m_uniforms.resolution.y);
		std::vector<ColorLayerInfo> colorLayerInfos;
		using Opt = ExtraFramebufferOption;
		switch (option)
		{
		case Opt::Rgba32fDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{ { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
			break;
		case Opt::TwoRgba32fDepth:
		case Opt::LinearGBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT1 }
			};
			break;
		case Opt::LeanLinearGBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT1 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT2 }
			};
			break;
		case Opt::Depth:
			colorLayerInfos = std::vector<ColorLayerInfo>{};
			break;
		case Opt::GBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32UI,  GL_COLOR_ATTACHMENT1 },
				{ GL_RGBA32UI,  GL_COLOR_ATTACHMENT2 }
			};
			break;
		}
		fbo = std::make_shared<Framebuffer>(width, height, colorLayerInfos);
	}
	return fbo;
}

void GrCamera::releaseExtraFramebuffer(std::shared_ptr<Framebuffer>) const
{
	// TODO: release mutexes
}

using glm::vec3;
using glm::quat;

TurntableCamera::TurntableCamera()
	: GrCamera()
	, m_center(0, 0, 0)
	, m_zoom(3.f)
	, m_sensitivity(0.003f)
	, m_zoomSensitivity(0.01f)
{
	m_quat = glm::quat(sqrt(2.f) / 2.f, -sqrt(2.f) / 2.f, 0.f, 0.f) * glm::quat(0.f, 0.f, 0.f, 1.f);
	updateViewMatrix();
}

void TurntableCamera::updateViewMatrix() {
	m_uniforms.viewMatrix = glm::translate(vec3(0.f, 0.f, -m_zoom)) * glm::toMat4(m_quat) * glm::translate(m_center);
	m_position = vec3(glm::inverse(m_uniforms.viewMatrix)[3]);

	updateUbo();
}

void TurntableCamera::updateDeltaMouseRotation(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float theta;

	// rotate around camera X axis by dy
	theta = dy * m_sensitivity;
	vec3 xAxis = vec3(glm::row(m_uniforms.viewMatrix, 0));
	m_quat *= quat(static_cast<float>(cos(theta / 2.f)), static_cast<float>(sin(theta / 2.f)) * xAxis);

	// rotate around world Z axis by dx
	theta = dx * m_sensitivity;
	m_quat *= quat(static_cast<float>(cos(theta / 2.f)), static_cast<float>(sin(theta / 2.f)) * vec3(0.f, 0.f, 1.f));

	updateViewMatrix();
}

void TurntableCamera::updateDeltaMouseZoom(float x1, float y1, float x2, float y2) {
	float dy = y2 - y1;
	float fac = 1.f + dy * m_zoomSensitivity;
	m_zoom *= fac;
	m_zoom = std::max(m_zoom, 0.0001f);
	updateViewMatrix();

	float s = orthographicScale();
	s *= fac;
	s = std::max(s, 0.0001f);
	setOrthographicScale(s);
}

void TurntableCamera::updateDeltaMousePanning(float x1, float y1, float x2, float y2) {
	float dx = (x2 - x1) / 600.f * m_zoom;
	float dy = -(y2 - y1) / 300.f * m_zoom;

	// move center along the camera screen plane
	vec3 xAxis = vec3(glm::row(m_uniforms.viewMatrix, 0));
	vec3 yAxis = vec3(glm::row(m_uniforms.viewMatrix, 1));
	m_center += dx * xAxis + dy * yAxis;

	updateViewMatrix();
}

void TurntableCamera::tilt(float theta) {
	vec3 zAxis = vec3(glm::row(m_uniforms.viewMatrix, 2));
	m_quat *= quat(static_cast<float>(cos(theta / 2.f)), static_cast<float>(sin(theta / 2.f)) * zAxis);
	updateViewMatrix();
}
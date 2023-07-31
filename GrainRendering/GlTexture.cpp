
#include "GlTexture.h"

#include <climits>

const GLuint GlTexture::invalid = UINT_MAX;

GlTexture::GlTexture(GLenum target)
	: m_id(invalid)
	, m_target(target)
{
	glCreateTextures(m_target, 1, &m_id);

	glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	setWrapMode(GL_CLAMP_TO_EDGE);
}

GlTexture::GlTexture(GLuint id, GLenum target)
	: m_id(id)
	, m_target(target)
{}

void GlTexture::storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth)
{
	m_width = width;
	m_height = height;
	m_depth = depth;
	m_levels = levels;
	glTextureStorage3D(m_id, levels, internalFormat, width, height, depth);
}

void GlTexture::storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height)
{
	m_width = width;
	m_height = height;
	m_depth = 1;
	m_levels = levels;
	glTextureStorage2D(m_id, levels, internalFormat, width, height);
}

void GlTexture::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
	glTextureSubImage2D(m_id, level, xoffset, yoffset, width, height, format, type, pixels);
}

void GlTexture::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels)
{
	glTextureSubImage3D(m_id, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void GlTexture::generateMipmap() const
{
	glGenerateTextureMipmap(m_id);
}

void GlTexture::setWrapMode(GLenum wrap) const
{
	glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, wrap);
	glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, wrap);
	glTextureParameteri(m_id, GL_TEXTURE_WRAP_R, wrap);
}

GlTexture::~GlTexture()
{
	if (isValid()) {
		glDeleteTextures(1, &m_id);
	}
}

void GlTexture::bind(GLint unit) const
{
	bind(static_cast<GLuint>(unit));
}

void GlTexture::bind(GLuint unit) const
{
	glBindTextureUnit(unit, m_id);
}
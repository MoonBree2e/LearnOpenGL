
#pragma once

#include <glad/glad.h>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include "ResourceManager.h"

class GlTexture {
public:
	/**
	 * Create a texture for a given target (same target as in glCreateTextures)
	 */
	explicit GlTexture(GLenum target);

	/**
	 * Take ownership of a raw gl texture.
	 * This object will take care of deleting texture, unless you release() it.
	 */
	explicit GlTexture(GLuint id, GLenum target);

	~GlTexture();
	GlTexture(const GlTexture&) = delete;
	GlTexture& operator=(const GlTexture&) = delete;
	GlTexture(GlTexture&&) = default;
	GlTexture& operator=(GlTexture&&) = default;

	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
	void storage(GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
	void generateMipmap() const;
	void setWrapMode(GLenum wrap) const;

	GLuint raw() const { return m_id; }
	GLsizei width() const { return m_width; }
	GLsizei height() const { return m_height; }
	GLsizei depth() const { return m_depth; }
	GLsizei levels() const { return m_levels; }
	GLenum target() const { return m_target; }

	bool isValid() const { return m_id != invalid; }
	void bind(GLint unit) const;
	void bind(GLuint unit) const;

	// forget texture without deleting it, use with caution
	void release() { m_id = invalid; }

private:
	static const GLuint invalid;

private:
	GLuint m_id;
	GLenum m_target;
	GLsizei m_width = 0;
	GLsizei m_height = 0;
	GLsizei m_depth = 0;
	GLsizei m_levels = 1;
};

#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

class GlTexture;

class ResourceManager {
public:
	enum Rotation {
		ROTATION0,
		ROTATION90,
		ROTATION180,
		ROTATION270
	};
public:
	/**
	 * Load a regular texture
	 */
	static std::unique_ptr<GlTexture> loadTexture(const std::string& filename, GLsizei levels = 0);

	/**
	 * Load of stack of textures as a GL_TEXTURE_2D_ARRAY
	 */
	static std::unique_ptr<GlTexture> loadTextureStack(const std::string& textureDirectory, int levels = 0);

	/**
	 * Get the width and height of an image
	 */
	static bool imageDimensions(const fs::path& filepath, int& width, int& height, Rotation rotation = ROTATION0);

	/**
	 * Add data in an already existing texture
	 * (TODO: this may belong on GlTexture)
	 */
	static bool loadTextureSubData(GlTexture& texture, const fs::path& filepath, GLint zoffset, GLsizei refWidth, GLsizei refHeight, Rotation rotation = ROTATION0);

public:
	// File output
	static bool saveImage(const std::string& filename, int width, int height, int channels, const void* data, bool vflip = false);

	static bool saveTextureStack(const std::string& dirname, const GlTexture& texture, bool vflip = false);
	static bool saveTexture(const std::string& filename, GLuint tex, GLint level = 0, bool vflip = false, GLint slice = 0);
	static bool saveTexture(const std::string& filename, const GlTexture& texture, GLint level = 0, bool vflip = false, GLint slice = 0);

	// Save all mipmap levels in prefixXX.png
	static bool saveTextureMipMaps(const std::string& prefix, GLuint tex);
	static bool saveTextureMipMaps(const std::string& prefix, const GlTexture& texture);

private:
	static std::unique_ptr<GlTexture> loadTextureSOIL(const fs::path& filepath, GLsizei levels);

	static bool imageDimensionsSOIL(const fs::path& filepath, int& width, int& height, Rotation rotation = ROTATION0);

	static bool loadTextureSubDataSOIL(GlTexture& texture, const fs::path& filepath, GLint zoffset, GLsizei refWidth, GLsizei refHeight, Rotation rotation = ROTATION0);
};


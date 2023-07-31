#define _CRT_SECURE_NO_WARNINGS

#include "ResourceManager.h"
#include "strutils.h"
#include "fileutils.h"
#include "GlTexture.h"
#include "Logger.h"

#include <glm/glm.hpp>
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cmath>
#include <fstream>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;



using namespace std;

std::unique_ptr<GlTexture> ResourceManager::loadTexture(const std::string& filename, GLsizei levels)
{
	fs::path fullFilename = filename;

	LOG << "Loading texture " << fullFilename << "...";

	std::unique_ptr<GlTexture> tex;


	tex = loadTextureSOIL(fullFilename, levels);


	if (tex) {
		tex->setWrapMode(GL_REPEAT);

		// TODO
		/*
		if (GL_EXT_texture_filter_anisotropic) {
			GLfloat maxi;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxi);
			glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxi);
		}
		*/

		tex->generateMipmap();
	}

	return tex;
}

std::unique_ptr<GlTexture> ResourceManager::loadTextureStack(const string& textureDirectory, int levels) {
	vector<fs::path> textureFilenames;

	std::string fullTextureDirectory = textureDirectory;

	if (!fs::is_directory(fullTextureDirectory)) {
		ERR_LOG << "Texture directory does not exist or is not a directory: " << fullTextureDirectory;
		return nullptr;
	}

	fs::directory_iterator dir(fullTextureDirectory);
	if (dir == fs::directory_iterator()) {
		ERR_LOG << "Error openning texture directory " << fullTextureDirectory;
		return nullptr;
	}

	for (auto& p : dir) {
		if (p.is_regular_file()) {
			textureFilenames.push_back(p.path());
		}
	}

	if (textureFilenames.empty()) {
		ERR_LOG << "Empty texture directory: " << textureDirectory;
		return nullptr;
	}
	DEBUG_LOG << "Found " << textureFilenames.size() << " textures in " << textureDirectory;

	sort(textureFilenames.begin(), textureFilenames.end());

	GLsizei stackSize = static_cast<GLsizei>(textureFilenames.size());
	if (stackSize > GL_MAX_ARRAY_TEXTURE_LAYERS) {
		ERR_LOG << "Number of textures higher than the hardware limit GL_MAX_ARRAY_TEXTURE_LAYERS";
		return nullptr;
	}

	// Init 3D texture
	int imageWidth = 0, imageHeight = 0;
	if (!ResourceManager::imageDimensions(textureFilenames[0], imageWidth, imageHeight)) {
		ERR_LOG << "Could not load texture files.";
		return nullptr;
	}
	GLsizei width = static_cast<GLsizei>(imageWidth);
	GLsizei height = static_cast<GLsizei>(imageHeight);
	DEBUG_LOG << "Allocating texture array of size " << imageWidth << "x" << imageHeight << "x" << stackSize;
	if (levels == 0) {
		levels = static_cast<GLsizei>(1 + floor(log2(max(width, height))));
	}

	auto tex = std::make_unique<GlTexture>(GL_TEXTURE_2D_ARRAY);
	tex->setWrapMode(GL_CLAMP_TO_EDGE);
	tex->storage(levels, GL_RGBA8, width, height, stackSize);

	// Read all layers
	for (size_t i = 0; i < textureFilenames.size(); ++i) {
		if (!ResourceManager::loadTextureSubData(*tex, textureFilenames[i], static_cast<GLint>(i), width, height)) {
			return nullptr;
		}
	}

	tex->generateMipmap();

	return tex;
}


bool ResourceManager::imageDimensions(const fs::path& filepath, int& width, int& height, Rotation rotation) {
	return imageDimensionsSOIL(filepath, width, height, rotation);
}


bool ResourceManager::loadTextureSubData(GlTexture& texture, const fs::path& filepath, GLint zoffset, GLsizei width, GLsizei height, Rotation rotation) {
	return loadTextureSubDataSOIL(texture, filepath, zoffset, width, height, rotation);
}

///////////////////////////////////////////////////////////////////////////////
// Private static utility
///////////////////////////////////////////////////////////////////////////////

template<typename T>
static T* rotateImage(T* image, size_t width, size_t height, size_t nbChannels, size_t* rotatedWidth, size_t* rotatedHeight, ResourceManager::Rotation angle) {
	T* rotated_image;
	if (angle == ResourceManager::ROTATION90 || angle == ResourceManager::ROTATION270) {
		*rotatedWidth = height;
		*rotatedHeight = width;
	}
	else {
		*rotatedWidth = width;
		*rotatedHeight = height;
	}
	rotated_image = new T[*rotatedWidth * *rotatedHeight * nbChannels];
	for (size_t u = 0; u < *rotatedWidth; ++u) {
		for (size_t v = 0; v < *rotatedHeight; ++v) {
			size_t readOffset, writeOffset = v * *rotatedWidth + u;
			switch (angle) {
			case ResourceManager::ROTATION0:
				readOffset = v * width + u;
				break;

			case ResourceManager::ROTATION90:
				readOffset = u * width + (height - v - 1);
				break;

			case ResourceManager::ROTATION180:
				readOffset = (width - v - 1) * width + (height - u - 1);
				break;

			case ResourceManager::ROTATION270:
				readOffset = (width - u - 1) * width + v;
				break;
			}
			for (size_t k = 0; k < nbChannels; ++k) {
				rotated_image[writeOffset * nbChannels + k] = image[readOffset * nbChannels + k];
			}
		}
	}
	return rotated_image;
}

///////////////////////////////////////////////////////////////////////////////
// File output
///////////////////////////////////////////////////////////////////////////////

bool ResourceManager::saveImage(const std::string& filename, int width, int height, int channels, const void* data, bool vflip)
{
	stbi_flip_vertically_on_write(vflip ? 1 : 0);
	fs::create_directories(fs::path(filename).parent_path());
	return stbi_write_png(filename.c_str(), width, height, channels, data, width * channels) == 0;
}

bool ResourceManager::saveTextureStack(const std::string& dirname, const GlTexture& texture, bool vflip)
{
	GLint level = 0;
	GLint d;
	glGetTextureLevelParameteriv(texture.raw(), level, GL_TEXTURE_DEPTH, &d);

	fs::path dir = dirname;
	if (!fs::is_directory(dir) && !fs::create_directories(dir)) {
		DEBUG_LOG << "Could not create directory " << dirname;
		return false;
	}

	for (GLint slice = 0; slice < d; ++slice) {
		fs::path slicename = dir / string_format("view%04d.png", slice);
		if (!saveTexture(slicename.string(), texture, level, vflip, slice)) {
			WARN_LOG << "Could not write file '" << slicename << "'";
		}
	}

	return true;
}

bool ResourceManager::saveTexture(const std::string& filename, GLuint tex, GLint level, bool vflip, GLint slice)
{
	// Avoid padding
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	stbi_flip_vertically_on_write(vflip ? 1 : 0);

	// Check level
	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(tex, GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(tex, GL_TEXTURE_MAX_LEVEL, &maxLevel);
	if (level < baseLevel || level >= maxLevel) {
		ERR_LOG
			<< "Invalid mipmap level " << level << " for texture " << tex
			<< " (expected in range " << baseLevel << ".." << maxLevel << ")";
		return false;
	}

	GLint w, h, d;
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_WIDTH, &w);
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_HEIGHT, &h);
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_DEPTH, &d);

	if (w == 0 || h == 0) {
		ERR_LOG << "Texture with null size: " << tex;
		return false;
	}

	if (slice >= d) {
		ERR_LOG << "Texture depth " << d << " is lower than the selected slice " << slice;
		return false;
	}

	GLint internalformat;
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);

	std::vector<unsigned char> pixels;

	switch (internalformat) {
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	{
		GLsizei byteCount = w * h;
		pixels = std::vector<unsigned char>(byteCount, 0);
		glGetTextureSubImage(tex, 0, 0, 0, slice, w, h, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, byteCount * sizeof(unsigned char), pixels.data());
		return stbi_write_png(filename.c_str(), w, h, 1, pixels.data(), w) == 0;
	}
	default:
	{
		GLsizei byteCount = 4 * w * h;
		pixels.resize(byteCount);
		glGetTextureSubImage(tex, 0, 0, 0, slice, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, byteCount * sizeof(unsigned char), pixels.data());
		return stbi_write_png(filename.c_str(), w, h, 4, pixels.data(), 4 * w) == 0;
	}
	}
}

bool ResourceManager::saveTexture(const std::string& filename, const GlTexture& texture, GLint level, bool vflip, GLint slice)
{
	return saveTexture(filename, texture.raw(), level, vflip, slice);
}


bool ResourceManager::saveTextureMipMaps(const std::string& prefix, GLuint tex)
{
	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(tex, GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(tex, GL_TEXTURE_MAX_LEVEL, &maxLevel);
	for (GLint level = baseLevel; level < maxLevel; ++level) {
		if (!ResourceManager::saveTexture(prefix + std::to_string(level) + ".png", tex, level)) {
			return false;
		}
	}
	return true;
}

bool ResourceManager::saveTextureMipMaps(const std::string& prefix, const GlTexture& texture)
{
	return saveTextureMipMaps(prefix, texture.raw());
}

///////////////////////////////////////////////////////////////////////////////
// Private loading procedures
///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<GlTexture> ResourceManager::loadTextureSOIL(const fs::path& filepath, GLsizei levels)
{
	// Load from file
	int width, height, channels;
	unsigned char* image;
	image = stbi_load(filepath.string().c_str(), &width, &height, &channels, 4);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file: '" << filepath << "' with stb_image: " << stbi_failure_reason();
		return nullptr;
	}

	if (levels == 0) {
		levels = static_cast<GLsizei>(1 + floor(log2(max(width, height))));
	}

	auto tex = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	tex->storage(levels, GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	tex->subImage(0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, image);

	stbi_image_free(image);

	return tex;
}

bool ResourceManager::imageDimensionsSOIL(const fs::path& filepath, int& width, int& height, Rotation rotation) {
	unsigned char* image = NULL;
	int channels;
	image = stbi_load(filepath.string().c_str(), &width, &height, &channels, 0);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file: '" << filepath << "' with stb_image: " << stbi_failure_reason();
		return false;
	}
	else {
		stbi_image_free(image);
		if (rotation == ROTATION0 || rotation == ROTATION270) {
			int tmp = width;
			width = height;
			height = tmp;
		}
		return true;
	}
}


bool ResourceManager::loadTextureSubDataSOIL(GlTexture& texture, const fs::path& filepath, GLint zoffset, GLsizei width, GLsizei height, Rotation rotation) {
	int imageWidth = 0, imageHeight = 0, channels;
	unsigned char* image = stbi_load(filepath.string().c_str(), &imageWidth, &imageHeight, &channels, 4);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file: '" << filepath << "' with stb_image: " << stbi_failure_reason();
		return false;
	}

	if (rotation != ROTATION0) {
		size_t rotatedWidth, rotatedHeight;
		unsigned char* rotatedImage = rotateImage(image, static_cast<size_t>(imageWidth), static_cast<size_t>(imageHeight), 4, &rotatedWidth, &rotatedHeight, rotation);
		stbi_image_free(image);
		image = rotatedImage;
		imageWidth = static_cast<int>(rotatedWidth);
		imageHeight = static_cast<int>(rotatedHeight);
	}

	if (imageWidth != width || imageHeight != height) {
		ERR_LOG << "Error: texture array slices must all have the same dimensions.";
		LOG << "Slice #" << (zoffset + 1) << " has dimensions " << imageWidth << "x" << height
			<< " but " << width << "x" << height << " was expected"
			<< " (in file " << filepath << ")." << endl;
		return false;
	}
	texture.subImage(0, 0, 0, zoffset, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, image);

	if (rotation != ROTATION0) {
		free(image);
	}
	else {
		stbi_image_free(image);
	}
	return true;
}
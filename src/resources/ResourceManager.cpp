#include "ResourceManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

std::map<std::string, ShaderProgram> ResourceManager::m_shaders;
std::map<std::string, Texture2D> ResourceManager::m_textures;

ShaderProgram* ResourceManager::loadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
	auto it = m_shaders.find(name);
	if (it != m_shaders.end()) {
		return &it->second;
	}

	auto [insertedIt, success] = m_shaders.emplace(name, ShaderProgram());

	if (!insertedIt->second.loadShaders(vertexPath, fragmentPath)) {
		std::cerr << "ResourceManager: Failed to load shader " << name << std::endl;
		m_shaders.erase(insertedIt);
		return nullptr;
	}
	
	return &insertedIt->second;
}

Texture2D* ResourceManager::loadTexture(const std::string& name, const std::string& path, GLint filter) {
	auto it = m_textures.find(name);
	if (it != m_textures.end()) {
		return &it->second;
	}

	auto [insertedIt, success] = m_textures.emplace(name, Texture2D());

	if (!insertedIt->second.load(path, filter)) {
		std::cerr << "ResourceManager: Failed to load texture " << name << std::endl;
		m_textures.erase(insertedIt);
		return nullptr;
	}

	return &insertedIt->second;
}

ShaderProgram* ResourceManager::getShader(const std::string& name) {
	auto it = m_shaders.find(name);
	if (it != m_shaders.end()) {
		return &it->second;
	}
	std::cerr << "ResourceManager: Shader \"" << name << "\" not found!" << std::endl;
	return nullptr;
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
	auto it = m_textures.find(name);
	if (it != m_textures.end()) {
		return &it->second;
	}
	std::cerr << "ResourceManager: Texture \"" << name << "\" not found!" << std::endl;
	return nullptr;
}

bool ResourceManager::hasTexture(const std::string& name) {
	return m_textures.find(name) != m_textures.end();
}

Texture2D* ResourceManager::getWhiteTexture() {
	auto it = m_textures.find("whitePixel");
	if (it != m_textures.end()) {
		return &it->second;
	}
	auto [insertedIt, success] = m_textures.emplace("whitePixel", Texture2D());
	unsigned char white[4] = { 255, 255, 255, 255 };
	insertedIt->second.loadFromMemory(1, 1, white, 4);
	return &insertedIt->second;
}

Texture2D* ResourceManager::getSoftRadialTexture() {
	auto it = m_textures.find("procSoftRadial");
	if (it != m_textures.end()) {
		return &it->second;
	}
	constexpr int size = 64;
	std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4, 255);
	for (int y = 0; y < size; ++y) {
		float ny = ((static_cast<float>(y) + 0.5f) / static_cast<float>(size)) * 2.0f - 1.0f;
		for (int x = 0; x < size; ++x) {
			float nx = ((static_cast<float>(x) + 0.5f) / static_cast<float>(size)) * 2.0f - 1.0f;
			float r = std::min(1.0f, std::sqrt(nx * nx + ny * ny));
			float falloff = 1.0f - r * r;
			falloff = falloff * falloff;
			size_t idx = (static_cast<size_t>(y) * size + static_cast<size_t>(x)) * 4;
			pixels[idx + 3] = static_cast<unsigned char>(std::clamp(falloff * 255.0f, 0.0f, 255.0f));
		}
	}
	auto [insertedIt, success] = m_textures.emplace("procSoftRadial", Texture2D());
	insertedIt->second.loadFromMemory(size, size, pixels.data(), 4, GL_LINEAR);
	return &insertedIt->second;
}

Texture2D* ResourceManager::getLightConeTexture() {
	auto it = m_textures.find("procLightCone");
	if (it != m_textures.end()) {
		return &it->second;
	}
	constexpr int size = 64;
	std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4, 255);
	for (int y = 0; y < size; ++y) {
		float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
		float halfW = 0.13f + 0.85f * v;
		float topSoft = std::clamp(v / 0.09f, 0.0f, 1.0f);
		topSoft = topSoft * topSoft * (3.0f - 2.0f * topSoft);
		float vertFade = std::pow(1.0f - v, 1.65f);
		for (int x = 0; x < size; ++x) {
			float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);
			float dx = (u - 0.5f) * 2.0f;
			float rn = std::min(1.0f, std::abs(dx) / halfW);
			float horiz = 1.0f - rn * rn;
			horiz = horiz * horiz;
			float alpha = horiz * topSoft * vertFade;
			size_t idx = (static_cast<size_t>(y) * size + static_cast<size_t>(x)) * 4;
			pixels[idx + 3] = static_cast<unsigned char>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
		}
	}
	auto [insertedIt, success] = m_textures.emplace("procLightCone", Texture2D());
	insertedIt->second.loadFromMemory(size, size, pixels.data(), 4, GL_LINEAR);
	return &insertedIt->second;
}

Texture2D* ResourceManager::getLinearGradYTexture() {
	auto it = m_textures.find("procLinearGradY");
	if (it != m_textures.end()) {
		return &it->second;
	}
	constexpr int width = 4;
	constexpr int height = 64;
	std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4, 255);
	for (int y = 0; y < height; ++y) {
		float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
		float a = std::pow(1.0f - v, 1.85f);
		unsigned char alphaByte = static_cast<unsigned char>(std::clamp(a * 255.0f, 0.0f, 255.0f));
		for (int x = 0; x < width; ++x) {
			size_t idx = (static_cast<size_t>(y) * width + static_cast<size_t>(x)) * 4;
			pixels[idx + 3] = alphaByte;
		}
	}
	auto [insertedIt, success] = m_textures.emplace("procLinearGradY", Texture2D());
	insertedIt->second.loadFromMemory(width, height, pixels.data(), 4, GL_LINEAR);
	return &insertedIt->second;
}

Texture2D* ResourceManager::getVignetteTexture() {
	auto it = m_textures.find("procVignette");
	if (it != m_textures.end()) {
		return &it->second;
	}
	constexpr int size = 128;
	std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4, 255);
	for (int y = 0; y < size; ++y) {
		float dy = ((static_cast<float>(y) + 0.5f) / static_cast<float>(size)) * 2.0f - 1.0f;
		for (int x = 0; x < size; ++x) {
			float dx = ((static_cast<float>(x) + 0.5f) / static_cast<float>(size)) * 2.0f - 1.0f;
			float dist = std::sqrt(std::pow(std::abs(dx), 2.4f) * 0.76f + std::pow(std::abs(dy), 2.4f) * 1.06f);
			float t = std::clamp((dist - 0.68f) / (1.32f - 0.68f), 0.0f, 1.0f);
			float smooth = t * t * (3.0f - 2.0f * t);
			float alpha = smooth * smooth;
			size_t idx = (static_cast<size_t>(y) * size + static_cast<size_t>(x)) * 4;
			pixels[idx + 3] = static_cast<unsigned char>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
		}
	}
	auto [insertedIt, success] = m_textures.emplace("procVignette", Texture2D());
	insertedIt->second.loadFromMemory(size, size, pixels.data(), 4, GL_LINEAR);
	return &insertedIt->second;
}

void ResourceManager::clear() {
	m_shaders.clear();
	m_textures.clear();
}
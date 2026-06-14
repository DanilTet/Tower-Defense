#pragma once

#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class Texture2D;

// четенькая структура которая описывает кусок текстуры в атласе 
struct SpriteUV {
	glm::vec2 uvMin = glm::vec2(0.0f, 0.0f); // левый верхний угол
	glm::vec2 uvMax = glm::vec2(1.0f, 1.0f); // правый нижний угол

	// метод который сам считает дроби из пикселей
	static SpriteUV fromPixels(int x, int y, int width, int height, int atlasWidth, int atlasHeight) {
		SpriteUV uv;
		uv.uvMin.x = static_cast<float>(x) / atlasWidth;
		uv.uvMin.y = static_cast<float>(y) / atlasHeight;
		uv.uvMax.x = static_cast<float>(x + width) / atlasWidth;
		uv.uvMax.y = static_cast<float>(y + height) / atlasHeight;
		return uv;
	}
};



class SpriteRenderer
{
public:
	explicit SpriteRenderer(std::shared_ptr<ShaderProgram> shader);
	~SpriteRenderer();

	SpriteRenderer(const SpriteRenderer&) = delete;
	SpriteRenderer& operator=(const SpriteRenderer&) = delete;

	SpriteRenderer(SpriteRenderer&& other) noexcept;
	SpriteRenderer& operator=(SpriteRenderer&& other) noexcept;


	void setProjection(const glm::mat4& projection);

	void drawSprite(const std::shared_ptr<Texture2D>& texture,
		glm::vec2 position,
		glm::vec2 size = glm::vec2(10.0f, 10.0f),
		float rotation = 0.0f,
		glm::vec3 color = glm::vec3(1.0f),
		SpriteUV uv = SpriteUV());

private:
	std::shared_ptr<ShaderProgram> m_shader;

	unsigned int m_quadVAO = 0;
	unsigned int m_vbo = 0;
	unsigned int m_ebo = 0;

	void initRenderData();
};
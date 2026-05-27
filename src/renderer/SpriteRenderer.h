#pragma once

#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class Texture2D;

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
		glm::vec3 color = glm::vec3(1.0f));

private:
	std::shared_ptr<ShaderProgram> m_shader;

	unsigned int m_quadVAO = 0;
	unsigned int m_vbo = 0;
	unsigned int m_ebo = 0;

	void initRenderData();
};
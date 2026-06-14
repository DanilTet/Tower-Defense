#include "SpriteRenderer.h"
#include "shaders/ShaderProgram.h"
#include "textures/Texture2D.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

SpriteRenderer::SpriteRenderer(std::shared_ptr<ShaderProgram> shader)
    : m_shader(std::move(shader))
{
    this->initRenderData();
}

SpriteRenderer::~SpriteRenderer() {
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

SpriteRenderer::SpriteRenderer(SpriteRenderer&& other) noexcept
    : m_shader(std::move(other.m_shader)),
    m_quadVAO(other.m_quadVAO),
    m_vbo(other.m_vbo),
    m_ebo(other.m_ebo)
{
    other.m_quadVAO = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
}

SpriteRenderer& SpriteRenderer::operator=(SpriteRenderer&& other) noexcept {
    if (this != &other) {
        glDeleteVertexArrays(1, &m_quadVAO);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);

        m_shader = std::move(other.m_shader);
        m_quadVAO = other.m_quadVAO;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;

        other.m_quadVAO = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
    }
    return *this;
}

void SpriteRenderer::initRenderData() {
    float vertices[] = {
        0.0f, 0.0f,    0.0f, 0.0f, // Левый верхний угол
        1.0f, 0.0f,    1.0f, 0.0f, // Правый верхний угол
        1.0f, 1.0f,    1.0f, 1.0f, // Правый нижний угол
        0.0f, 1.0f,    0.0f, 1.0f  // Левый нижний угол
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SpriteRenderer::setProjection(const glm::mat4& projection) {
    m_shader->use();
    glUniformMatrix4fv(glGetUniformLocation(m_shader->getId(), "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void SpriteRenderer::drawSprite(const std::shared_ptr<Texture2D>& texture,
    glm::vec2 position,
    glm::vec2 size,
    float rotation,
    glm::vec3 color,
    SpriteUV uv)
{
    m_shader->use();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position, 0.0f));

    if (rotation != 0.0f) {
        model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));
    }
    model = glm::scale(model, glm::vec3(size, 1.0f));

    GLuint programId = m_shader->getId();
    glUniformMatrix4fv(glGetUniformLocation(programId, "u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(programId, "u_color"), 1, glm::value_ptr(color));

    // короче тут новый массив вершин где позиция квадрата таже НО UV координаты наши! УКРАИНСКИЕ!

    float vertices[] = {
        // позиция      // текстурные координаты (U, V)
        0.0f, 0.0f,     uv.uvMin.x, uv.uvMin.y, // левый верхний
        1.0f, 0.0f,     uv.uvMax.x, uv.uvMin.y, // правый верхний
        1.0f, 1.0f,     uv.uvMax.x, uv.uvMax.y, // правый нижний
        0.0f, 1.0f,     uv.uvMin.x, uv.uvMax.y  // левый нижний
    };

    // обновляем буфер в видеокарте
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // перезаписуем только кусок данных
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    texture->bind(0);

    glBindVertexArray(m_quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
#include "ParticleSystem.h"
#include <random>
#include "../renderer/SpriteRenderer.h" 
#include "../textures/Texture2D.h"

// генератор рандомных чисел для красивого разлета
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

ParticleSystem::ParticleSystem(size_t maxParticles) {
    m_pool.resize(maxParticles);
    m_poolIndex = maxParticles - 1;
}

void ParticleSystem::update(float dt) {
    for (auto& p : m_pool) {
        if (!p.active) continue;

        // отнимаем время жизни
        p.lifeRemaining -= dt;
        if (p.lifeRemaining <= 0.0f) {
            p.active = false; // если частица все то камбекаем ее в пулл
            continue;
        }

        // двигаем гада
        p.position += p.velocity * dt;
    }
}

void ParticleSystem::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture) {
    for (auto& p : m_pool) {
        if (!p.active) continue;

        // вычисляем процент жизни от 0.0 это только родилась до 1.0 умерла
        float lifeFactor = 1.0f - (p.lifeRemaining / p.lifeTime);

        // интерполяция цвета и размера
        glm::vec3 currentColor = glm::mix(p.startColor, p.endColor, lifeFactor);
        float currentSize = glm::mix(p.startSize, p.endSize, lifeFactor);

        // рисуем отцентрованную частицу
        glm::vec2 drawPos = p.position - glm::vec2(currentSize / 2.0f);
        renderer->drawSprite(texture, drawPos, glm::vec2(currentSize), 0.0f, currentColor);
    }
}

void ParticleSystem::emit(const ParticleEmitterProps& props, int count) {
    for (int i = 0; i < count; ++i) {
        Particle& p = m_pool[m_poolIndex];
        p.active = true;
        p.position = props.position;

        // генерируем случайный вектор разлета
        glm::vec2 randomVec = glm::vec2(dis(gen), dis(gen));

        // если вектор не нулевой то нормализуем его для ровного круга взрыва
        if (glm::length(randomVec) > 0.0f) {
            randomVec = glm::normalize(randomVec);
        }

        // задаем физику
        p.velocity = props.velocityDir + randomVec * props.velocityVariation;

        // задаем визуал
        p.startColor = props.startColor;
        p.endColor = props.endColor;
        p.startSize = props.startSize;
        p.endSize = props.endSize;

        // добавляем микро-рандом к времени жизни чтобы они не исчезали синхронно
        float lifeRand = (dis(gen) * 0.2f); // +/- 20%
        p.lifeTime = props.lifeTime + lifeRand;
        p.lifeRemaining = p.lifeTime;

        // идем по кругу пула
        if (m_poolIndex == 0) {
            m_poolIndex = m_pool.size() - 1;
        }
        else {
            m_poolIndex = m_poolIndex - 1;
        }
    }
}

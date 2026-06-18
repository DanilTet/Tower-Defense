#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include "CircleCollider.h"
#include <string>
#include "../core/Animator.h"

// Структура для хранения характеристик врага
struct EnemyStats {
	float speed;
	int Maxhealth;
	float sizeScale;
	int reward;
	std::string textureId;
	glm::vec3 color;
	std::string deathSound;
	std::string deathParticle;

	int atlasWidth;
	int atlasHeight;
	std::unordered_map<std::string, AnimationClip> animations;
};

class SpriteRenderer;
class Texture2D;
class Grid;
class Pathfinder;

class Enemy
{
private:
	std::string m_type; // тип характеристик врага
	int m_health; // здоровье врага
	float m_speed; // скорость врага
	int m_reward; // награда за убийство врага
	glm::vec3 m_color;
	std::string m_deathSound; // звук смерти
	std::string m_deathParticle; // кеш для звука смерти

	Animator m_animator; // компонент аниматора

	std::vector<glm::ivec2> m_path; // маршрут врага
	size_t m_currentWayPoint; // текущая точка к которой враг идет
	glm::vec2 m_pixelPos; // координаты врага в пикселях экрнана
	bool m_reachedEnd; // флаг дошел ли враг до конца

	float m_radiusMultiplier; // соотношение врага отностительно клетки

	static int s_nextId; // счетчик врагов общий
	int m_id; // личный номер врага

	float m_distanceTraveled;// пройденная дистанция врага

	int m_targetBaseIndex; //индекс базы

	float m_angle = 0.0f; // угол поворота

public:

	// функци для перерасчета пути
	void recalculatePath(Pathfinder* pathfinder, const Grid& grid, const std::vector<glm::ivec2>& bases);

	glm::ivec2 getTargetBase() const { return m_path.empty() ? glm::ivec2(0) : m_path.back(); }

	// Функция для получения характеристик врага в зависимости от его типа
	static EnemyStats getStatsfromEnemyType(const std::string& type);

	void recalculatePosition(const Grid& oldGrid, const Grid& newGrid); // вызывается при изменении размера окна, чтобы враг всегда был точно на клетке, даже если размер клеток изменится при ресайзе окна

	Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, const std::string& type, int targetBaseIndex);
	void update(float dt, const Grid& grid);

	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, std::shared_ptr<Texture2D> radiusTex, glm::vec2 offset, const Grid& grid);
	glm::vec2 getPixelPos() const { return m_pixelPos; }
	bool isReachedEnd() const { return m_reachedEnd; }
	void takeDamage(int damage) {
		m_health -= damage;
	}

	bool isDead() const { // возвращает true если враг имеет мельше 0 хп и false если больше 
		if (m_health <= 0) {
			return true;
		}
		return false;
	}

	int getReward() const { return m_reward; } // геттер который возгращает награду за убийство врага
	std::string getDeathSound() const { return m_deathSound; }
	CircleCollider getCollider(const Grid& grid) const; // генераттор хитбокса
	std::string getDeathParticle() const { return m_deathParticle; } // геттер партиклов
	int getId() const { return m_id; } // геттер для получения айди врага

	float getDistanceTraveled() const { return m_distanceTraveled; } // геттер для того щоб отримати пройдений шлях ворога

	int getHealth() const { return m_health; } // получить хп
};
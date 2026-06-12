#include <vector>
#include <glm/glm.hpp>
#include "Grid.h"

struct Node {
	glm::ivec2 pos; // тут храним координаті клетки в сетке x y

	int gCost; // стоимость пути от старта до клетки
	int hCost; // примерное растояние до финиша
	Node* parent; // указатель на батю клетки батя єто тот от которого мі пришли

	int fCost() const { return gCost + hCost; } // стоимость клетки

	// конструктор
	Node(glm::ivec2 p = glm::ivec2(0, 0)) : pos(p) {}
};

class Pathfinder {
private:
	// открітій список єто клетки на которіх мі еще не стояли
	std::vector<Node*> m_openList;
	// закрітій список єто клетки которіе уже проверили
	std::vector<Node*> m_closedList;

	// масив масивов где хранятся все узлі
	std::vector<std::vector<Node>> m_allNodes;

	int getDistance(Node* nodeA, Node* nodeB); // считаем дистанцию октальную




public:
	// инициализирует m_allNodes
	Pathfinder(int gridWidth, int gridHeight);
	// принимает все что надо и возвращает список координат для движения
	std::vector<Node*> getNeighbors(Node* node, const Grid& grid); // ищет соседей вокруг
	std::vector<glm::ivec2> findPath(const Grid& grid, glm::ivec2 startPos, glm::ivec2 targetPos, int& outTotalCost);
};
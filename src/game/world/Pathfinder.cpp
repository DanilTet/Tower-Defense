#include "Pathfinder.h"
#include <vector>
#include <glm/glm.hpp>
#include "Grid.h"
#include <algorithm>
#include <cmath>

Pathfinder::Pathfinder(int width, int height) {
	m_allNodes.resize(width); // задаем количество столбцов

	for (int x = 0; x < width; x++) {
		m_allNodes[x].resize(height); // в каждом столбце задаем колво строк
		for (int y = 0; y < height; y++) {
			m_allNodes[x][y] = Node(glm::ivec2(x, y)); // кладем ноду в ячейку
		}
	}
}

int Pathfinder::getDistance(Node* A, Node* B) {
	int distX = std::abs(A->pos.x - B->pos.x); // ищем разницу по х
	int distY = std::abs(A->pos.y - B->pos.y); // ищем разницу по у

	// и тут
	if (distX > distY) {
		return 14 * distY + 10 * (distX - distY);
	}
	return 14 * distX + 10 * (distY - distX);
}

std::vector<Node*> Pathfinder::getNeighbors(Node* node, const Grid& grid) {
    std::vector<Node*> neighbors;

    // двигаемся в 8 направлений
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {

            if (x == 0 && y == 0) continue; // скип себя

            int checkX = node->pos.x + x;
            int checkY = node->pos.y + y;

            // чекаем не вышли ли за границу
            if (checkX < 0 || checkX >= grid.getWidth() || checkY < 0 || checkY >= grid.getHeight()) {
                continue;
            }

            // чекаем стен и башен
            CellType cellType = grid.getCellType(checkX, checkY);
            bool isWalkable = (cellType == CellType::Empty || cellType == CellType::Path ||
                cellType == CellType::Spawner || cellType == CellType::Base);

            if (!isWalkable) {
                continue; // туда нельзя
            }

            // чекаем не срезаем ли мы угол
            if (std::abs(x) == 1 && std::abs(y) == 1) {
                // если идем по диагонали то чекам нету ли там челов по краям
                CellType corner1 = grid.getCellType(node->pos.x + x, node->pos.y);
                CellType corner2 = grid.getCellType(node->pos.x, node->pos.y + y);

                bool isCorner1Walkable = (corner1 == CellType::Empty || corner1 == CellType::Path);
                bool isCorner2Walkable = (corner2 == CellType::Empty || corner2 == CellType::Path);

                // если хоть один забанен то не идем туда
                if (!isCorner1Walkable || !isCorner2Walkable) {
                    continue;
                }
            }

            // если все четенько то добавляем в список 
            neighbors.push_back(&m_allNodes[checkX][checkY]);
        }
    }

    return neighbors;
}

std::vector<glm::ivec2> Pathfinder::findPath(const Grid& grid, glm::ivec2 startPos, glm::ivec2 targetPos, int& outTotalCost) {
    // очищаем открытый и закрытый список
    m_openList.clear();
    m_closedList.clear();

    // обнуляем все клетки в двумерном масиве
    for (size_t x = 0; x < m_allNodes.size(); x++) {
        for (size_t y = 0; y < m_allNodes[x].size(); y++) {
            m_allNodes[x][y].gCost = 0;
            m_allNodes[x][y].hCost = 0;
            m_allNodes[x][y].parent = nullptr;
        }
    }
    // защита от не существующих координат
    if (startPos.x < 0 || startPos.x >= m_allNodes.size() ||
        startPos.y < 0 || startPos.y >= m_allNodes[0].size() ||
        targetPos.x < 0 || targetPos.x >= m_allNodes.size() ||
        targetPos.y < 0 || targetPos.y >= m_allNodes[0].size()) {

        return std::vector<glm::ivec2>(); // если координаты бредовые
    }


    // получаем укащатели на стартовый и финишный узел
    Node* startNode = &m_allNodes[startPos.x][startPos.y];
    Node* targetNode = &m_allNodes[targetPos.x][targetPos.y];
    // добавляем startNode в открітій список
    m_openList.push_back(startNode);

    // пока открытый список не пустой
    while (!m_openList.empty()) {
        // берем самый лучший елемент это первый елемент списка
        Node* currentNode = m_openList[0];
        int currentIndex = 0;

        // пробегаемся по всем остальным елементам в открытом списке
        for (int i = 1; i < m_openList.size(); i++) {
            // сравниваем если у текущего елемента в цикле F меньше или если F одинаковій но растояние H меньше
            if (m_openList[i]->fCost() < currentNode->fCost() ||
                (m_openList[i]->fCost() == currentNode->fCost() && m_openList[i]->hCost < currentNode->hCost)) {
                // то теперь он становитья лучшей следующей клеткой???
                currentNode = m_openList[i];
                currentIndex = i;
            }
        }

        //удаляем клетку из открітого списка
        m_openList.erase(m_openList.begin() + currentIndex);
        // добавляем ее в закрітій список
        m_closedList.push_back(currentNode);

        //если дошли до конца
        if (currentNode == targetNode) {
            std::vector<glm::ivec2> path;

            // востанавливаем маршрут до старта идем по отцам

            outTotalCost = targetNode->gCost;

            while (currentNode != startNode) {
                path.push_back(currentNode->pos);
                currentNode = currentNode->parent;
            }
            // разворачиваем вектор
            std::reverse(path.begin(), path.end());
            // возращаем маршрут
            return path;
            
        }

        // считіваем соседей
        std::vector<Node*> neighbors = getNeighbors(currentNode, grid);

        // проходимся по соседям
        for (size_t i = 0; i < neighbors.size(); i++) {
            Node* neighbor = neighbors[i];

            // если сосед уже в закрытом списке - ШКИП
            if (std::find(m_closedList.begin(), m_closedList.end(), neighbor) != m_closedList.end()) {
                continue;
            }
            
            // считаем новую стоимость пути
            int newMovementCostToNeighbor = currentNode->gCost + getDistance(currentNode, neighbor);

            // проверяем есть ли сосед в открытом списке
            bool inOpenList = std::find(m_openList.begin(), m_openList.end(), neighbor) != m_openList.end();

            // если путь выгоднее или соседа нету в открытом списке 
            if (newMovementCostToNeighbor < neighbor->gCost || !inOpenList) {
                neighbor->gCost = newMovementCostToNeighbor;
                neighbor->hCost = getDistance(neighbor, targetNode);
                neighbor->parent = currentNode; // запоминаем откуда пришлимы

                // если его небыло в открытом списке то добавляем
                if (!inOpenList) {
                    m_openList.push_back(neighbor);
                }
            }

        }   
    }
    outTotalCost = 999999;
    return std::vector<glm::ivec2>(); // возвращаем пустой маршрут
}
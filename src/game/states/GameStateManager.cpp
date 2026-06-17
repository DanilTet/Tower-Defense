#include "GameStateManager.h"

GameStateManager::~GameStateManager() {
	while (!m_states.empty()) {
		m_states.back()->cleanup();
		m_states.pop_back();
	}
}

void GameStateManager::setState(std::unique_ptr<IGameState> newState) {
	m_nextState = std::move(newState);
	m_clearAllAndSet = true;
}

void GameStateManager::pushState(std::unique_ptr<IGameState> newState) {
	m_nextState = std::move(newState);
	m_clearAllAndSet = false;
}

void GameStateManager::popState() {
	m_popRequested = true;
}

void GameStateManager::processInput(GLFWwindow* window, float dt) {
	if (!m_states.empty()) {
		m_states.back()->processInput(window, dt);
	}
}

void GameStateManager::update(float dt) {
	if (!m_states.empty()) {
		m_states.back()->update(dt);
	}
}

void GameStateManager::render() {
	// если на паузе
	if (m_states.size() > 1) {
		// рисуем последний стейт
		m_states[m_states.size() - 2]->render();
	}

	// и тут самый верхний стейт типа паузу
	if (!m_states.empty()) {
		m_states.back()->render();
	}
}

void GameStateManager::applyPendingChanges() {
	// если возврат
	if (m_popRequested) {
		if (!m_states.empty()) {
			m_states.back()->cleanup();
			m_states.pop_back();
		}
		m_popRequested = false;
	}
	// если отложеный стейт
	if (m_nextState) {
		if (m_clearAllAndSet) {
			while (!m_states.empty()) {
				m_states.back()->cleanup();
				m_states.pop_back();
			}
		}
		m_states.push_back(std::move(m_nextState));
		m_states.back()->init();

		m_clearAllAndSet = false;
	}
}

void GameStateManager::resize(int width, int height) {
	if (!m_states.empty()) {
		m_states.back()->resize(width, height);
	}
}
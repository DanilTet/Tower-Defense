#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include "resources/ResourceManager.h"
#include "renderer/SpriteRenderer.h"
#include "game/Grid.h"

int windowWidth = 640;
int windowHeight = 480;

std::unique_ptr<SpriteRenderer> renderer;

void glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
{
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, windowWidth, windowHeight);

    if (renderer) {
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
            static_cast<float>(windowHeight), 0.0f,
            -1.0f, 1.0f);
        renderer->setProjection(projection);
    }
}

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}


int main(void)
{
    /* Initialize the library */
    if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "IASA tower defence", nullptr, nullptr);
    if (!window)
    {
		std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetWindowSizeCallback(window, glfwWindowSizeCallback);
	glfwSetKeyCallback(window, glfwKeyCallback);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);


    if (!gladLoadGL()) {
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
    }

	std::cout << "Rendderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(!ResourceManager::loadShader("spriteShader", "res/shaders/vertex_shader.vert", "res/shaders/fragment_shader.frag")){
		std::cout << "Failed to load shaders" << std::endl;
        glfwTerminate();
		return -1;
    }
    if(!ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png")){
        std::cerr << "Failed to load texture via ResourceManager!" << std::endl;
        glfwTerminate();
        return -1;
    }

    ShaderProgram* shader = ResourceManager::getShader("spriteShader");
    std::shared_ptr<ShaderProgram> shaderPtr(shader, [](ShaderProgram*) {});

    renderer = std::make_unique<SpriteRenderer>(shaderPtr);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth),
        static_cast<float>(windowHeight), 0.0f,
        -1.0f, 1.0f);
    renderer->setProjection(projection);

    shader->use();
    int textureLocation = glGetUniformLocation(shader->getId(), "u_texture");
    glUniform1i(textureLocation, 0);

    // делаем переменные которые будем использовать для управления временем
	double lastFrame = glfwGetTime(); // время начала предыдущего кадра
    float deltaTime = 0.0f; // время между кадрами

    float towerRotation = 45.0f; // тест поворота башни

	// Создаем игровую сетку и загружаем текстуру для клеток
    Grid gameGrid(10, 7, 64.0f);
    Texture2D* cellTex = ResourceManager::getTexture("towerTexture");
    std::shared_ptr<Texture2D> cellTexturePtr(cellTex, [](Texture2D*) {});


    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        // Считаем время вот эту дельту
        double currentFrame = glfwGetTime();
        deltaTime = static_cast<float>(currentFrame - lastFrame);
        lastFrame = currentFrame;

        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }
        
        towerRotation += 90.0f * deltaTime;
        if (towerRotation >= 360.0f) {
            towerRotation -= 360;
        }

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        
		Texture2D* texture = ResourceManager::getTexture("towerTexture");
		std::shared_ptr<Texture2D> texturePtr(texture, [](Texture2D*) {});
        gameGrid.draw(renderer.get(), cellTexturePtr);
        renderer->drawSprite(texturePtr, glm::vec2(100.0f, 100.0f), glm::vec2(64.0f, 64.0f), 0.0f);

        renderer->drawSprite(texturePtr, glm::vec2(250.0f, 100.0f), glm::vec2(48.0f, 48.0f), towerRotation, glm::vec3(1.0f, 0.3f, 0.3f));

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    renderer.reset();

    ResourceManager::clear();
    glfwTerminate();
    return 0;
}
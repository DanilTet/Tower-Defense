#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "ShaderProgram.h"

int windowWidth = 640;
int windowHeight = 480;

GLfloat point[] = {
    0.0f, 0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f
};

GLfloat color[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

void glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
{
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, windowWidth, windowHeight);
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

    glClearColor(1, 1, 0, 1);

    ShaderProgram shaderProgram;
    if (!shaderProgram.loadShaders("res/shaders/vertex_shader.vert", "res/shaders/fragment_shader.frag")) {
        std::cerr << "Failed to load/compile shaders!" << std::endl;
        glfwTerminate();
        return -1;
    }

	GLuint points_vbo = 0;
    glGenBuffers(1, &points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point), point, GL_STATIC_DRAW);

	GLuint colors_vbo = 0;
    glGenBuffers(1, &colors_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

	GLuint vao = 0;
    glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        
        shaderProgram.use();
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);


        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    shaderProgram.deleteProgram();
    glfwTerminate();
    return 0;
}
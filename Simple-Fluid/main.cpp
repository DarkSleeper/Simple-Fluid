#define _CRT_SECURE_NO_WARNINGS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include "tool/camera.h"
#include "renderer/renderer.hpp"

#define TIME_FRAME_CNT 5
#define OUTPUT_FRAME_CNT 1000

// camera
Camera camera(glm::vec3(0.0f, 2.0f, 15.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;
float speed = 0.016f;
bool reset_obstacle = true;
GLFWwindow* window;
Renderer renderer;

void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

int main(int argc, char* argv[]) {
	/* Initialize the library */
	if (!glfwInit()) return -1;

	// version 4.6 core
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Heat", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, onKeyPress);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback); 
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	renderer.init();

	// timing
	float delta_time = 0.0f;
	float last_time = 0.0f;

	float sum_delta_time = 0.0f;
	int frame_cnt = 0;
	float delta_time_output = 0.0f;

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		auto current_time = (float)glfwGetTime();
		if (last_time == 0.0) {
			delta_time = 0.0;
			last_time = current_time;
		}
		else {
			delta_time = current_time - last_time;
			last_time = current_time;
		}
		sum_delta_time += delta_time;
		delta_time_output += delta_time;
		processInput(window);

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		auto aspect = (float)width / (float)height;
		glm::mat4 projection_mat = glm::perspective(toRadians(45.f), aspect, 0.01f, 1000.f);
		glm::mat4 view_mat = camera.GetViewMatrix();

		//show result
		if (frame_cnt > 0) {
			if (frame_cnt % TIME_FRAME_CNT == 0) {
				std::string title = "Smoke   delta_time: " + std::to_string(sum_delta_time / TIME_FRAME_CNT) + "s,   fps: " + std::to_string(int(1 / (sum_delta_time / TIME_FRAME_CNT)));
				glfwSetWindowTitle(window, title.data());
				sum_delta_time = 0.0f;
			}
			if (frame_cnt % OUTPUT_FRAME_CNT == 0) {
				std::cout << "time for #frame" << frame_cnt << " is : " << delta_time_output / OUTPUT_FRAME_CNT << "s/frame" << std::endl;
				delta_time_output = 0.0f;
			}
		}
		std::string title = "Smoke   delta_time: " + std::to_string(delta_time).substr(0, 7) + "   fps: " + std::to_string(int(1 / delta_time));
		glfwSetWindowTitle(window, title.data());
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 0.f);

		// called by each frame
		renderer.render(0.016, view_mat, projection_mat);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		frame_cnt++;
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods)
{

}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, speed);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, speed);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, speed);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, speed);

	if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
		setupScene(renderer.scene, 0);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		setupScene(renderer.scene, 1);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		setupScene(renderer.scene, 2);
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		setupScene(renderer.scene, 3);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (button)
		{
			case GLFW_MOUSE_BUTTON_LEFT: {
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				int width; int height;
				glfwGetWindowSize(window, &width, &height);
				auto domainHeight = 1.0f;
				auto domainWidth = domainHeight / SIM_HEIGHT * SIM_WIDTH;
				float x = xpos / width * domainWidth;
				float y = (height - ypos) / height * domainHeight;
				setObstacle(renderer.scene, x, y, true);
				break;
			}
			case GLFW_MOUSE_BUTTON_MIDDLE:
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				break;
			default:
				return;
		}
	}
	return;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		camera.ProcessMouseMovement(xoffset, yoffset);
		int width; int height;
		glfwGetWindowSize(window, &width, &height);
		auto domainHeight = 1.0f;
		auto domainWidth = domainHeight / SIM_HEIGHT * SIM_WIDTH;
		float x = xpos / width * domainWidth;
		float y = (height - ypos) / height * domainHeight;
		setObstacle(renderer.scene, x, y, false);
	}
}




#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include "../scene/scene.hpp"

#define STEP 1
#define SCR_WIDTH 1280
#define SCR_HEIGHT 720
#define pai 3.1415926f
#define toRadians(degrees) ((degrees*2.f*pai)/360.f)

struct Renderer
{
	Renderer() 
	{
		srand(time(0));
	}
	
	~Renderer() {
	}

	bool init() {
		auto rand_0_1 = []() -> float {
			return (float)(rand()) / RAND_MAX;
		};

		// scene
		setupScene(scene, 1);

		// image
		img_size_x = scene.fluid->numX;
		img_size_y = scene.fluid->numY;
		img_data.resize(img_size_x * img_size_y * 4, (unsigned char)0);
		glGenTextures(1, &texture);

		// shader
		auto vertex_path = "runtime/shader/opacity.vs";
		auto fragment_path = "runtime/shader/opacity.fs";
		init_shader(vertex_path, fragment_path, renderingProgram);

		// vao & vbo
		glGenVertexArrays(1, vao);
		glBindVertexArray(vao[0]);

		std::vector<float> pValues =
		{ -1, -1, 0,   1, -1, 0,
		  1,  1, 0,  -1,  1, 0 };
		std::vector<float> tValues =
		{ 0,0,  1,0,  1,1,  0,1 };

		unsigned int indices[] = {
		   0, 1, 3,
		   1, 2, 3
		};

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), &(indices[0]), GL_STATIC_DRAW);

		// vert
		glGenBuffers(2, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, pValues.size() * sizeof(float), &(pValues[0]), GL_STATIC_DRAW);

		// uv
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, tValues.size() * sizeof(float), &(tValues[0]), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		return true;
	}

	void render(float dt, glm::mat4& world_to_view_matrix, glm::mat4& view_to_clip_matrix) {

		simulate(scene);

		auto& f = *scene.fluid.get();
		auto minP = f.p[0];
		auto maxP = f.p[0];

		for (int i = 0; i < f.numCells; i++) {
			minP = std::min(minP, f.p[i]);
			maxP = std::max(maxP, f.p[i]);
		}

		for (int i = 0; i < img_size_x; i++) {
			for (int j = 0; j < img_size_y; j++) {
				auto color = glm::vec4();

				if (scene.showPressure) {
					auto p = f.p[i * img_size_y + j];
					auto s = f.m[i * img_size_y + j];
					color = getSciColor(p, minP, maxP);
					if (scene.showSmoke) {
						color[0] = std::max(0.0f, color[0] - 255 * s);
						color[1] = std::max(0.0f, color[1] - 255 * s);
						color[2] = std::max(0.0f, color[2] - 255 * s);
					}
				}
				else if (scene.showSmoke) {
					auto s = f.m[i * img_size_y + j];
					color[0] = 255 * s;
					color[1] = 255 * s;
					color[2] = 255 * s;
					if (scene.sceneNr == 2)
						color = getSciColor(s, 0.0, 1.0);
				}
				else if (f.s[i * img_size_y + j] == 0.0) {
					color[0] = 0;
					color[1] = 0;
					color[2] = 0;
				}

				img_data[4 * (img_size_x * j + i) + 0] = (unsigned char)(color.r);
				img_data[4 * (img_size_x * j + i) + 1] = (unsigned char)(color.g);
				img_data[4 * (img_size_x * j + i) + 2] = (unsigned char)(color.b);
				img_data[4 * (img_size_x * j + i) + 3] = (unsigned char)(color.a);
			}
		}

		// image upload
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_size_x, img_size_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &img_data[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		/* Render here */
		glUseProgram(renderingProgram);
		glBindVertexArray(vao[0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(renderingProgram, "colorMap"), 0);
		glDisable(GL_DEPTH_TEST);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

private:
	int img_size_x;
	int img_size_y;
	std::vector<unsigned char> img_data;
	GLuint texture;
	GLuint vao[1];
	GLuint vbo[2];
	GLuint ebo;
	GLuint renderingProgram;

	Scene scene;

	glm::vec4 getSciColor(float val, float minVal, float maxVal) {
		val = std::min(std::max(val, minVal), maxVal - 0.1f);
		auto d = maxVal - minVal;
		val = (d == 0.0) ? 0.5 : ((val - minVal) / d);
		auto m = 0.25;
		auto num = (int)floor(val / m);
		auto s = (val - num * m) / m;
		float r, g, b;

		switch (num) {
			case 0: r = 0.0; g = s; b = 1.0; break;
			case 1: r = 0.0; g = 1.0; b = 1.0 - s; break;
			case 2: r = s; g = 1.0; b = 0.0; break;
			case 3: r = 1.0; g = 1.0 - s; b = 0.0; break;
		}

		return glm::vec4(255 * r, 255 * g, 255 * b, 255);
	}

	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM") {
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}

	void init_shader(const char* vertexPath, const char* fragmentPath, GLuint& ID) {
		std::string glsl_version = "#version 450\n";
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << glsl_version;
			fShaderStream << glsl_version;
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();
		// 2. compile shaders
		unsigned int vertex, fragment;
		int success;
		char infoLog[512];
		// vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}
};
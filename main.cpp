// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

// GLFW and GLAD
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static char *read_file(const char *path)
{
	char *source;
	FILE *fp = fopen(path, "r");
	if (!fp) {
		printf("Failed to open shader source\n");
		return NULL;
	}

	// Get file size
	fseek(fp, 0, SEEK_END);

	// Allocate memory for shader source
	long size = ftell(fp);
	source = (char *) malloc(size + 1);

	// Rewind file pointer
	fseek(fp, 0, SEEK_SET);

	// Read shader source
	fread(source, 1, size, fp);
	source[size] = '\0';

	// Close file
	fclose(fp);

	return source;
}

static unsigned int compile_shader(const char *path, unsigned int type)
{
	char *source = read_file(path);
	if (!source) {
		printf("Failed to read shader source\n");
		return 0;
	}

	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	// Check for errors
	int success;
	char info_log[512];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, info_log);
		printf("Failed to compile shader (%s):\n%s\n", path, info_log);
		free(source);
		return 0;
	}

	free(source);

	printf("Successfully compiled shader %s\n", path);
	return shader;
}

static int link_program(unsigned int program)
{
	int success;
	char info_log[512];

	glLinkProgram(program);

	// Check if program linked successfully
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, info_log);
		printf("Failed to link program: %s\n", info_log);
		return 0;
	}

	return 1;
}

static void set_int(unsigned int program, const char *name, int value)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform1i(i, value);
}

static void set_vec3(unsigned int program, const char *name, const glm::vec3 &vec)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform3fv(i, 1, glm::value_ptr(vec));
}

int main()
{
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const int PIXEL_SIZE = 4;

	// Basic window
	GLFWwindow *window = NULL;
	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(WIDTH, HEIGHT, "Weaxor", NULL, NULL);

	// Check if window was created
	if (!window) {
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize OpenGL context\n");
		return -1;
	}

	// Create texture for output
	unsigned int texture;

	// Create and bind texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	// Bind texture as image
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// Read shader source
	unsigned int shader = compile_shader("shader.glsl", GL_COMPUTE_SHADER);

	// Create program
	unsigned int program = glCreateProgram();
	glAttachShader(program, shader);

	// Link program
	if (!link_program(program))
		return -1;

	// Delete shaders
	glDeleteShader(shader);

	glm::vec3 origin {0, 0, -5};
	glm::vec3 lookat {0, 0, 0};
	glm::vec3 up {0, 1, 0};

	auto setup_camera = [program](const glm::vec3 &origin, const glm::vec3 &lookat, const glm::vec3 &up_) {
		glm::vec3 a = lookat - origin;
		glm::vec3 b = up_;

		glm::vec3 front = glm::normalize(a);
		glm::vec3 right = glm::normalize(glm::cross(b, front));
		glm::vec3 up = glm::cross(front, right);

		set_int(program, "width", WIDTH);
		set_int(program, "height", HEIGHT);
		set_int(program, "pixel", PIXEL_SIZE);

		set_vec3(program, "camera.origin", origin);
		set_vec3(program, "camera.front", front);
		set_vec3(program, "camera.up", up);
		set_vec3(program, "camera.right", right);
	};

	setup_camera(origin, lookat, up);

	// Set up vertex data
	float vertices[] = {
		// positions		// texture coords
		-1.0f,	-1.0f,	0.0f,	0.0f,	0.0f,
		1.0f,	-1.0f,	0.0f,	1.0f,	0.0f,
		-1.0f,	1.0f,	0.0f,	0.0f,	1.0f,
		1.0f,	1.0f,	0.0f,	1.0f,	1.0f
	};

	// Set up vertex array object
	unsigned int vao;

	// Create vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create vertex buffer object
	unsigned int vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Upload vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) (sizeof(float) * 3));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// Compile respective shaders
	unsigned int vertex_shader = compile_shader("texture.vert", GL_VERTEX_SHADER);
	unsigned int fragment_shader = compile_shader("texture.frag", GL_FRAGMENT_SHADER);

	// Create program
	unsigned int program_texture = glCreateProgram();
	glAttachShader(program_texture, vertex_shader);
	glAttachShader(program_texture, fragment_shader);

	// Link program
	if (!link_program(program_texture))
		return -1;

	// Loop until the user closes the window
	float last_time = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		// Close if escape or q
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
			continue;
		}

		// Update
		float t = glfwGetTime();
		origin = glm::vec3 {cos(t) * 5, sin(t) * 5, sin(t) * 5};
		setup_camera(origin, lookat, up);

		// Ray tracing
		{
			// Bind program and dispatch
			glUseProgram(program);
			glDispatchCompute(WIDTH/PIXEL_SIZE, HEIGHT/PIXEL_SIZE, 1);
		}

		// Wait
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Render texture
		{
			// Bind program and draw
			glUseProgram(program_texture);
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();

		// Print frame-time
		// TODO: set frame rate to 24 fps
		float current_time = glfwGetTime();
		float fps = 1.0f / (current_time - last_time);
		// printf("%f, fps: %f\n", current_time - last_time, fps);
		last_time = current_time;
	}
}

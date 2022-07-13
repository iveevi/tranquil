#ifndef COMMON_H_
#define COMMON_H_

// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <vector>
#include <memory>

// GLFW and GLAD
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

const int WIDTH = 800;
const int HEIGHT = 600;
const int PIXEL_SIZE = 4;

inline char *read_file(const char *path)
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

inline unsigned int compile_shader(const char *path, unsigned int type)
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

inline int link_program(unsigned int program)
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

inline void set_int(unsigned int program, const char *name, int value)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform1i(i, value);
}

inline void set_vec3(unsigned int program, const char *name, const glm::vec3 &vec)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform3fv(i, 1, glm::value_ptr(vec));
}

#endif

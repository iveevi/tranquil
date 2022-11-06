#include "common.hpp"

int compile_shader(const char *path, unsigned int type)
{
	std::string glsl_source = read_glsl(path);
	const char *source = glsl_source.c_str();

	// Write the source to a temporary file
	system("mkdir -p tmp");

	// Extract the filename from the path
	std::string filename = path;
	filename = filename.substr(filename.find_last_of("/") + 1);

	std::ofstream tmp("tmp/out_" + filename);
	tmp << source;
	tmp.close();

	if (!source) {
		printf("Failed to read shader source\n");
		throw std::runtime_error("Failed to read shader source");
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
		throw std::runtime_error("Failed to compile shader");
	}

	printf("Successfully compiled shader %s\n", path);
	return shader;
}

int link_program(unsigned int program)
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

void set_int(unsigned int program, const char *name, int value)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform1i(i, value);
}

void set_float(unsigned int program, const char *name, float value)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform1f(i, value);
}

void set_vec2(unsigned int program, const char *name, const glm::vec2 &vec)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform2fv(i, 1, glm::value_ptr(vec));
}

void set_vec3(unsigned int program, const char *name, const glm::vec3 &vec)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform3fv(i, 1, glm::value_ptr(vec));
}

static bool dragging = false;

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	static double last_x = WIDTH/2.0;
	static double last_y = HEIGHT/2.0;

	static float sensitivity = 0.001f;

	static bool first_mouse = true;

	static float yaw = 0.0f;
	static float pitch = 0.0f;

	if (!state.viewing_mode)
		return;
	
	if (first_mouse) {
		last_x = xpos;
		last_y = ypos;
		first_mouse = false;
	}
	
	double xoffset = last_x - xpos;
	double yoffset = last_y - ypos;

	last_x = xpos;
	last_y = ypos;
	
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	// Only drag when left mouse button is pressed
	if (dragging) {
		yaw += xoffset;
		pitch += yoffset;

		// Clamp pitch
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	camera.set_yaw_pitch(yaw, pitch);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		dragging = true;
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		dragging = false;
}

void keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		state.paused = !state.paused;
}

GLFWwindow *initialize_graphics()
{
	// Basic window
	GLFWwindow *window = nullptr;
	if (!glfwInit())
		return nullptr;

	window = glfwCreateWindow(WIDTH, HEIGHT, "Tranquil", NULL, NULL);

	// Check if window was created
	if (!window) {
		glfwTerminate();
		return nullptr;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize OpenGL context\n");
		return nullptr;
	}

	// Set up callbacks
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, keyboard_callback);

	const GLubyte* renderer = glGetString(GL_RENDERER);

	printf("Renderer: %s\n", renderer);
	return window;
}

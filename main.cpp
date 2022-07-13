#include "common.hpp"

// Generate pillar mesh
Mesh generate_pillar(const glm::mat4 &transform)
{
	Mesh box;

	// Properties
	glm::vec4 center_ = transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 dx = transform * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) - center_;
	glm::vec3 dy = transform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) - center_;
	glm::vec3 dz = transform * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) - center_;

	// Vertices
	glm::vec3 center = glm::vec3(center_);
	glm::vec3 v1 = center + dx/2.0f + dy/2.0f + dz/2.0f;
	glm::vec3 v2 = center + dx/2.0f + dy/2.0f - dz/2.0f;
	glm::vec3 v3 = center + dx/2.0f - dy/2.0f - dz/2.0f;
	glm::vec3 v4 = center + dx/2.0f - dy/2.0f + dz/2.0f;
	glm::vec3 v5 = center - dx/2.0f + dy/2.0f + dz/2.0f;
	glm::vec3 v6 = center - dx/2.0f + dy/2.0f - dz/2.0f;
	glm::vec3 v7 = center - dx/2.0f - dy/2.0f - dz/2.0f;
	glm::vec3 v8 = center - dx/2.0f - dy/2.0f + dz/2.0f;

	// Push vertices
	box.vertices = {
		Vertex {v1}, Vertex {v2}, Vertex {v3}, Vertex {v4},
		Vertex {v5}, Vertex {v6}, Vertex {v7}, Vertex {v8}
	};

	// Indices
	box.triangles = std::vector <Triangle> {
		Triangle {0, 1, 2, Shades::ePillar}, Triangle {0, 2, 3, Shades::ePillar},
		Triangle {4, 5, 6, Shades::ePillar}, Triangle {4, 6, 7, Shades::ePillar},
		Triangle {0, 1, 5, Shades::ePillar}, Triangle {0, 5, 4, Shades::ePillar},
		Triangle {1, 2, 6, Shades::ePillar}, Triangle {1, 6, 5, Shades::ePillar},
		Triangle {2, 3, 7, Shades::ePillar}, Triangle {2, 7, 6, Shades::ePillar},
		Triangle {3, 0, 4, Shades::ePillar}, Triangle {3, 4, 7, Shades::ePillar}
	};

	return box;
}

// Generate terrain tile mesh
// TODO: try to optimizde the resulting mesh (low deltas could be merged)
Mesh generate_terrain(int resolution)
{
	float width = 10.0f;
	float height = 10.0f;

	// Height map
	size_t mapres = resolution + 1;
	std::vector <float> height_map(mapres * mapres);

	srand(clock());
	for (size_t i = 0; i < mapres * mapres; i++)
		height_map[i] = randf() * 1.0f;

	float slice = width/resolution;

	float x = -width/2.0f;
	float z = -height/2.0f;

	Mesh tile;
	for (int i = 0; i <= resolution; i++) {
		for (int j = 0; j <= resolution; j++) {
			Vertex vertex;
			vertex.position = glm::vec3(x, height_map[i * resolution + j], z);
			tile.vertices.push_back(vertex);
			x += slice;
		}

		x = -width/2.0f;
		z += slice;
	}

	// Generate indices
	for (int i = 0; i < resolution; i++) {
		for (int j = 0; j < resolution; j++) {
			// Indices of square
			uint32_t a = i * (resolution + 1) + j;
			uint32_t b = (i + 1) * (resolution + 1) + j;
			uint32_t c = (i + 1) * (resolution + 1) + j + 1;
			uint32_t d = i * (resolution + 1) + j + 1;

			// Push
			tile.triangles.push_back(Triangle {a, b, c, Shades::eGrass});
			tile.triangles.push_back(Triangle {a, c, d, Shades::eGrass});
		}
	}

	return tile;
}

// Generate a scene tile
Mesh generate_tile(int resolution)
{
	// Generate terrain tile
	// TODO: pass height map
	Mesh tile = generate_terrain(resolution);

	// Add random columns
	int nboxes = rand() % 5 + 5;
	for (int i = 0; i < nboxes; i++) {
		// Random size
		float width = randf() * 0.6f + 0.5f;
		float depth = randf() * 0.6f + 0.5f;
		float height = randf() * 2.0f + 0.5f;

		// Random position within the tile
		float x = randf(-4.5, 4.5);
		float z = randf(-4.5, 4.5);
		float y = height / 2.0f + randf();

		// Random rotation
		float rx = randf() * 15.0f;
		float ry = randf() * 360.0f;
		float rz = randf() * 15.0f;

		// Generate box
		glm::mat4 mat = Transform {
			glm::vec3(x, y, z),
			glm::vec3(rx, ry, rz),
			glm::vec3(width, height, depth)
		}.matrix();

		// Add box
		tile.add(generate_pillar(mat));
	}

	return tile;
}

GLFWwindow *initialize_graphics()
{
	// Basic window
	GLFWwindow *window = nullptr;
	if (!glfwInit())
		return nullptr;

	window = glfwCreateWindow(WIDTH, HEIGHT, "Weaxor", NULL, NULL);

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

	const GLubyte* renderer = glGetString(GL_RENDERER);

	printf("Renderer: %s\n", renderer);
	return window;
}

int main()
{
	GLFWwindow *window = initialize_graphics();
	if (!window)
		return -1;

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
	unsigned int shader = compile_shader("shaders/shader.glsl", GL_COMPUTE_SHADER);

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
	float vs[] = {
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(vs), vs, GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) (sizeof(float) * 3));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// Compile respective shaders
	unsigned int vertex_shader = compile_shader("shaders/texture.vert", GL_VERTEX_SHADER);
	unsigned int fragment_shader = compile_shader("shaders/texture.frag", GL_FRAGMENT_SHADER);

	// Create program
	unsigned int program_texture = glCreateProgram();
	glAttachShader(program_texture, vertex_shader);
	glAttachShader(program_texture, fragment_shader);

	// Link program
	if (!link_program(program_texture))
		return -1;

	// Vertices of tile
	Mesh tile = generate_tile(5);

	BVH *bvh = tile.make_bvh();
	BVHBuffer bvh_buffer;
	bvh->serialize(bvh_buffer);
	delete bvh;

	VBuffer vertices;
	IBuffer indices;

	tile.serialize_vertices(vertices);
	tile.serialize_indices(indices);

	/* std::cout << "Triangles: " << indices.size() << std::endl;
	for (const auto &i : indices)
		std::cout << "\t" << i << std::endl; */

	// Create storage buffer for tile vertices
	unsigned int ssbo;

	// Create storage buffer object
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(aligned_vec4) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 1);

	// Create storage buffer for tile indices
	unsigned int issbo;

	// Create storage buffer object
	glGenBuffers(1, &issbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, issbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(aligned_uvec4) * indices.size(), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 2);

	// Create storage buffer for BVH
	unsigned int bsbo;

	// Create storage buffer object
	glGenBuffers(1, &bsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(aligned_vec4) * bvh_buffer.size(), bvh_buffer.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 3);

	set_int(program, "primitives", tile.triangles.size());

	std::cout << "Buffer size = " << bvh_buffer.size() << std::endl;
	std::cout << "Triangles = " << tile.triangles.size() << std::endl;

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(window)) {
		// Close if escape or q
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
			continue;
		}

		// Update
		float t = glfwGetTime();
		t /= 2;

		float radius = 10;
		origin = glm::vec3 {cos(t) * radius, 5, sin(t) * radius};
		setup_camera(origin, lookat, up);

		// Ray tracing
		{
			// Bind program and dispatch
			glUseProgram(program);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, issbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bsbo);
			glDispatchCompute(WIDTH/PIXEL_SIZE, HEIGHT/PIXEL_SIZE, 1);
		}

		// Wait
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Render texture
		{
			// Bind program and draw
			glUseProgram(program_texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
	}
}

#include "common.hpp"

#include <PerlinNoise.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

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
	// Mesh tile = generate_terrain(resolution);
	Mesh tile;

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

// Camera and mouse handling
Camera camera;

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	static double last_x = WIDTH/2.0;
	static double last_y = HEIGHT/2.0;

	static float sensitivity = 0.001f;

	static bool first_mouse = true;

	static float yaw = 0.0f;
	static float pitch = 0.0f;
	
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

	yaw += xoffset;
	pitch += yoffset;

	// Clamp pitch
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	camera.set_yaw_pitch(yaw, pitch);
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

	// Set up callbacks
	glfwSetCursorPosCallback(window, mouse_callback);

	// Cursor is disabled by default
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);

	// Bind texture as image
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// Create texture for height map
	// TODO: struct for height map and resources
	unsigned int heightmap;
	unsigned int heightmap_normals;

	int resolution = 512;

	float *heightmap_data = new float[resolution * resolution];
	glm::vec3 *heightmap_normals_data = new glm::vec3[resolution * resolution];

	srand(clock());
	uint32_t seed = rand();
	const siv::PerlinNoise perlin {seed};
	float frequency = 2.0f;
	const double f = (frequency / resolution);

	for (int i = 0; i < resolution * resolution; i++) {
		int x = i % resolution;
		int y = i / resolution;
		heightmap_data[i] = perlin.octave2D_01(x * f, y * f, 4);
	}

	// Compute normals
	for (int x = 0; x < resolution; x++) {
		for (int y = 0; y < resolution; y++) {
			float d = 10.0f/resolution;

			glm::vec3 grad_x {2 * d, 0, 0};
			if (x > 0 && x < resolution - 1) {
				float y1 = heightmap_data[(x - 1) * resolution + y];
				float y2 = heightmap_data[(x + 1) * resolution + y];

				grad_x.y = (y2 - y1) / (2 * d);
			}

			glm::vec3 grad_z {0, 0, 2 * d};
			if (y > 0 && y < resolution - 1) {
				float y1 = heightmap_data[x * resolution + y - 1];
				float y2 = heightmap_data[x * resolution + y + 1];

				grad_z.y = (y2 - y1) / (2 * d);
			}

			glm::vec3 n = glm::normalize(glm::cross(grad_x, grad_z));
			heightmap_normals_data[x * resolution + y] = (-n * 0.5f + 0.5f);
		}
	}

	unsigned char *heightmap_image = new unsigned char[resolution * resolution];
	for (int i = 0; i < resolution * resolution; i++)
		heightmap_image[i] = (unsigned char) (heightmap_data[i] * 255.0f);

	// Create and bind texture (binding 1)
	glGenTextures(1, &heightmap);
	glBindTexture(GL_TEXTURE_2D, heightmap);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, resolution, resolution, 0, GL_RED, GL_UNSIGNED_BYTE, heightmap_image);

	// Bind texture as sampler
	glBindImageTexture(1, heightmap, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);

	// Create and bind texture (binding 2)
	glGenTextures(1, &heightmap_normals);
	glBindTexture(GL_TEXTURE_2D, heightmap_normals);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, resolution, resolution, 0, GL_RGB, GL_FLOAT, heightmap_normals_data);

	// Bind texture as sampler
	glBindImageTexture(2, heightmap_normals, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGB32F);

	// Cloud density
	srand(clock());

	seed = rand();
	const siv::PerlinNoise perlin_cloud {seed};

	int cloud_resolution = 128;
	unsigned char *cloud_density_image = new unsigned char[cloud_resolution * cloud_resolution];

	glm::vec2 cloud_offset {0.0f, 0.0f};
	for (int i = 0; i < cloud_resolution * cloud_resolution; i++) {
		int x = i % cloud_resolution;
		int y = i / cloud_resolution;

		float density = perlin_cloud.octave2D_01(x * f + cloud_offset.x, y * f + cloud_offset.y, 4);
		cloud_density_image[i] = (unsigned char) (density * 250.0f + 1);
	}

	// Create and bind texture (binding 3)
	unsigned int cloud_density;

	glGenTextures(1, &cloud_density);
	glBindTexture(GL_TEXTURE_2D, cloud_density);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, cloud_resolution, cloud_resolution, 0, GL_RED, GL_UNSIGNED_BYTE, cloud_density_image);

	// Bind texture as sampler
	glBindImageTexture(3, cloud_density, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);

	// Read shader source
	unsigned int shader = compile_shader("shaders/pixelizer.glsl", GL_COMPUTE_SHADER);

	// Create program
	unsigned int program = glCreateProgram();
	glAttachShader(program, shader);

	// Link program
	if (!link_program(program))
		return -1;

	// Delete shaders
	glDeleteShader(shader);

	glm::vec3 origin {0, 5, -5};
	glm::vec3 lookat {0, 2, 0};
	glm::vec3 up {0, 1, 0};

	set_int(program, "width", WIDTH);
	set_int(program, "height", HEIGHT);
	set_int(program, "pixel", PIXEL_SIZE);

	camera = Camera {origin, lookat, up};
	camera.send_to_shader(program);

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

	set_vec3(program, "light_dir", glm::normalize( glm::vec3 {1, 1, 1} ));

	// Loop until the user closes the window
	float cloud_time = 0;
	float sun_time = 0;

	while (!glfwWindowShouldClose(window)) {
		// Close if escape or q
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
			continue;
		}
		
		float t = glfwGetTime();

		// Move camera
		float speed = 0.1f;

		float dx = 0, dy = 0, dz = 0;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			dz += speed;
		else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			dz -= speed;	

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			dx += speed;
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			dx -= speed;

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			dy -= speed;
		else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			dy += speed;

		camera.move(dx, dy, dz);
		camera.send_to_shader(program);

		// Update cloud density every so often
		cloud_time += t;

		if (cloud_time > 0.2f) {
			cloud_time = 0;
		
			cloud_offset += 0.005f;

			for (int i = 0; i < cloud_resolution * cloud_resolution; i++) {
				int x = i % cloud_resolution;
				int y = i / cloud_resolution;

				float density = perlin_cloud.octave2D_01(x * f + cloud_offset.x, y * f + cloud_offset.y, 4);
				cloud_density_image[i] = (unsigned char) (density * 250.0f + 1);
			}

			// Update texture
			glBindTexture(GL_TEXTURE_2D, cloud_density);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, cloud_resolution, cloud_resolution, 0, GL_RED, GL_UNSIGNED_BYTE, cloud_density_image);
		}

		// Update sun direction, should lie on the x = z plane
		float st = sun_time;
		float y = glm::sin(st);
		float x = glm::cos(st);

		set_vec3(program, "light_dir", glm::normalize(glm::vec3 {x, y, x}));
		sun_time = std::fmod(sun_time + t/1000.0f, 2 * glm::pi <float>());

		// Ray tracing
		{
			// Bind program and dispatch
			glUseProgram(program);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, heightmap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, heightmap_normals);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, cloud_density);

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

			glActiveTexture(GL_TEXTURE0);
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

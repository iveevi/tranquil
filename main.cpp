#include "common.hpp"

#include <PerlinNoise.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Camera and mouse handling
Camera camera;

template <class T>
unsigned int make_ssbo(const std::vector <T> &data, int binding)
{
	unsigned int ssbo;

	// Create storage buffer object
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * data.size(), data.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, binding);

	return ssbo;
}

// TODO: use in initialize graphics
void initialize_imgui(GLFWwindow *window)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");
}

// Struct for managing data for the heightmap
struct HeightMap {
};

// Application state
State state;

int main()
{
	GLFWwindow *window = initialize_graphics();
	if (!window)
		return -1;

	initialize_imgui(window);

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
	float frequency = 1.0f;
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
	set_int(program, "clouds", state.show_clouds);
	set_int(program, "normals", state.show_normals);

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

	// TODO: group all the shaders into a struct

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

	// Store buffers for vertices, indices and BVH
	unsigned int ssbo_vertices = make_ssbo(vertices, 1);
	unsigned int ssbo_indices = make_ssbo(indices, 2);
	unsigned int ssbo_bvh = make_ssbo(bvh_buffer, 3);

	set_int(program, "primitives", tile.triangles.size());
	// set_int(program, "primitives", 0);

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

		// Tab to toggle viewing mode
		if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !state.tab) {
			state.viewing_mode = !state.viewing_mode;
			unsigned int cursor_mode = state.viewing_mode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
			glfwSetInputMode(window, GLFW_CURSOR, cursor_mode);
			state.tab = true;
		} else if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
			state.tab = false;
		}

		// Update cloud density every so often
		cloud_time += t;

		if (cloud_time > 0.2f) {
			cloud_time = 0;

			cloud_offset += 0.005f;

			float frequency = 8.0f;
			const double f = (frequency/cloud_resolution);
			for (int i = 0; i < cloud_resolution * cloud_resolution; i++) {
				int x = i % cloud_resolution;
				int y = i / cloud_resolution;

				float density = perlin_cloud.octave2D_01(x * f + cloud_offset.x, y * f + cloud_offset.y, 16);
				cloud_density_image[i] = (unsigned char) (density * 250.0f + 1);
			}

			// Update texture
			glBindTexture(GL_TEXTURE_2D, cloud_density);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, cloud_resolution, cloud_resolution, 0, GL_RED, GL_UNSIGNED_BYTE, cloud_density_image);
		}

		/* Update sun direction, should lie on the x = z plane
		float st = sun_time;
		float y = glm::sin(st);
		float x = glm::cos(st);

		set_vec3(program, "light_dir", glm::normalize(glm::vec3 {x, y, x}));
		sun_time = std::fmod(sun_time + t/1000.0f, 2 * glm::pi <float>()); */

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

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vertices);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_indices);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_bvh);

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

		// ImGui
		if (!state.viewing_mode) {
			ImGui_ImplGlfw_NewFrame();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui::NewFrame();
			
			// Settings window
			{
				ImGui::Begin("Settings");
				ImGui::Checkbox("Show triangles", &state.show_triangles);
				ImGui::Checkbox("Show clouds", &state.show_clouds);
				ImGui::Checkbox("Show normals", &state.show_normals);
				ImGui::End();
			}

			// Info window
			{
				ImGui::Begin("Info");
				ImGui::Text("framerate: %.1f", ImGui::GetIO().Framerate);
				ImGui::Text("primitives: %lu", tile.triangles.size());
				ImGui::End();
			}

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
		
		// Apply settings
		int primitives = state.show_triangles * tile.triangles.size();
		set_int(program, "primitives", primitives);

		set_int(program, "clouds", state.show_clouds);
		set_int(program, "normals", state.show_normals);

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
	}
}

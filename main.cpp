#include "common.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

// Global variables
Camera camera;
State state;
Shaders *shaders = nullptr;

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

	// Create heightmap
	size_t seed;
	HeightMap heightmap(128, 1.5f, 8);

	// Create grass map
	GrassMap grassmap(1024, 256.0f, 8);

	// Cloud density
	srand(clock());

	seed = rand();
	const siv::PerlinNoise perlin_cloud {seed};

	int cloud_resolution = 128;
	unsigned char *cloud_density_image = new unsigned char[cloud_resolution * cloud_resolution];

	glm::vec2 cloud_offset {0.0f, 0.0f};

	float f = (1.0f/cloud_resolution);
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

	// Create shaders
	shaders = new Shaders();

	glm::vec3 origin {0, 5, -5};
	glm::vec3 lookat {0, 2, 0};
	glm::vec3 up {0, 1, 0};

	set_int(shaders->pixelizer, "width", WIDTH);
	set_int(shaders->pixelizer, "height", HEIGHT);
	set_int(shaders->pixelizer, "pixel", PIXEL_SIZE);
	set_float(shaders->pixelizer, "terrain_size", state.terrain_size);
	set_vec2(shaders->pixelizer, "wind_offset", {0, 0});
	state.apply();

	camera = Camera {origin, lookat, up};
	camera.send_to_shader(shaders->pixelizer);

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

	set_int(shaders->pixelizer, "primitives", tile.triangles.size());
	// set_int(shaders->pixelizer, "primitives", 0);

	std::cout << "Buffer size = " << bvh_buffer.size() << std::endl;
	std::cout << "Triangles = " << tile.triangles.size() << std::endl;

	set_vec3(shaders->pixelizer, "light_dir", glm::normalize( glm::vec3 {1, 1, 1} ));

	// Loop until the user closes the window
	float cloud_time = 0;
	float sun_time = 0;
	float last_time = 0;

	const int BATCH_SIZE = 1000;

	int offx = 0;
	int offy = 0;

	glm::vec2 wind_velocity {0, 0};

	while (!glfwWindowShouldClose(window)) {
		// Close if escape or q
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
			continue;
		}

		float t = glfwGetTime();
		float dt = t - last_time;
		last_time = t;

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
		camera.send_to_shader(shaders->pixelizer);

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
		cloud_time += dt;

		if (cloud_time > 0.01f) {
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

			// Random wind offset
			float angle = randf(-glm::pi <float> (), glm::pi <float> ());
			float strength = 0.1f;

			glm::vec2 next = strength * glm::vec2 {glm::cos(angle), glm::sin(angle)};
			wind_velocity = lerp(wind_velocity, next, 0.1f);
			wind_velocity = glm::clamp(wind_velocity, -0.5f, 0.5f);

			// Update wind offset
			set_vec2(shaders->pixelizer, "wind_offset", wind_velocity);
		}

		/* Update sun direction, should lie on the x = z plane
		float st = sun_time;
		float y = glm::sin(st);
		float x = glm::cos(st);

		set_vec3(shaders->pixelizer, "light_dir", glm::normalize(glm::vec3 {x, y, x}));
		sun_time = std::fmod(sun_time + t/1000.0f, 2 * glm::pi <float>()); */

		// Ray tracing
		{
			// Set offsets
			set_int(shaders->pixelizer, "offx", offx);
			set_int(shaders->pixelizer, "offy", offy);

			// Bind shaders->pixelizer and dispatch
			glUseProgram(shaders->pixelizer);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, heightmap.t_height);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, heightmap.t_normal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, cloud_density);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, grassmap.t_grass);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, grassmap.t_normal);
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, grassmap.t_length);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, grassmap.t_power);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vertices);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_indices);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_bvh);

			glDispatchCompute(BATCH_SIZE/PIXEL_SIZE, BATCH_SIZE/PIXEL_SIZE, 1);

			// Update offsets
			offx += BATCH_SIZE;
			if (offx > WIDTH) {
				offx = 0;
				offy += BATCH_SIZE;
			}

			if (offy > HEIGHT)
				offy = 0;
		}

		// Wait
		// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Render texture
		{
			// Bind shaders->pixelizer and draw
			glUseProgram(shaders->texturizer);

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
				ImGui::Checkbox("Show grass", &state.show_grass);
				ImGui::Checkbox("Show grass blades", &state.show_grass_blades);

				if (ImGui::Checkbox("Show grass map", &state.show_grass_map)) {
					state.show_grass_length = false;
					state.show_grass_power = false;
				}

				if (ImGui::Checkbox("Show grass length", &state.show_grass_length)) {
					state.show_grass_map = false;
					state.show_grass_power = false;
				}

				if (ImGui::Checkbox("Show grass power", &state.show_grass_power)) {
					state.show_grass_map = false;
					state.show_grass_length = false;
				}

				ImGui::Checkbox("Show normals", &state.show_normals);
				ImGui::SliderFloat("Ray marching step", &state.ray_marching_step, 1e-3f, 1.0f, "%.3g", 1 << 5);
				ImGui::SliderFloat("Ray shadow step", &state.ray_shadow_step, 1e-3f, 1.0f, "%.3g", 1 << 5);
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
		set_int(shaders->pixelizer, "primitives", primitives);
		state.apply();

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
	}
}

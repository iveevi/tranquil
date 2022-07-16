#include "common.hpp"

#define STB_IMAGE_IMPLEMENTATION
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
	ImGuiIO &io = ImGui::GetIO(); (void) io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");
}

// Struct for managing data for the heightmap
class GrassMap {
	float		*grass;
	float		*grass_length;
	float		*grass_power;
	int		data_res;

	// Generate heightmap
	void generate_heightmap(float frequency, int octaves) {
		srand(clock());

		// Perlin noise generator
		uint32_t seed1 = rand();
		uint32_t seed2 = rand();
		uint32_t seed3 = rand();

		const siv::PerlinNoise perlin1 {seed1};
		const siv::PerlinNoise perlin2 {seed2};
		const siv::PerlinNoise perlin3 {seed3};

		const double f1 = (frequency/data_res);
		const double f2 = f1/10.0f;
		const double f3 = f1/100.0f;

		for (int i = 0; i < data_res * data_res; i++) {
			int x = i % data_res;
			int y = i / data_res;

			grass[i] = perlin1.octave2D_01(x * f1, y * f1, octaves);
			grass_length[i] = perlin2.octave2D_01(x * f2, y * f2, 16);
			grass_power[i] = perlin3.octave2D_01(x * f3, y * f3, 4);
		}
	}

	// Evaluate the heightmap at a given point
	float eval(float x, float z) {
		// x and z are in the range +/- terrain_size

		// Scale to [0, resolution]
		x = data_res/2.0f + x * data_res/state.terrain_size;
		z = data_res/2.0f + z * data_res/state.terrain_size;

		// Floor to nearest int
		int x0 = glm::clamp(floor(x), 0.0f, data_res - 1.0f);
		int z0 = glm::clamp(floor(z), 0.0f, data_res - 1.0f);

		int z1 = glm::clamp((float) ceil(z), 0.0f, data_res - 1.0f);
		int x1 = glm::clamp((float) ceil(x), 0.0f, data_res - 1.0f);

		// Get fractional parts
		float xf = x - x0;
		float zf = z - z0;

		// Get heights
		float h00 = grass[z0 * data_res + x0];
		float h01 = grass[z0 * data_res + x1];
		float h10 = grass[z1 * data_res + x0];
		float h11 = grass[z1 * data_res + x1];

		// Interpolate
		float h0 = lerp(h00, h01, xf);
		float h1 = lerp(h10, h11, xf);
		float h = lerp(h0, h1, zf);

		return h;
	}

	glm::vec3	*normals;
	int		normals_res;

	// Generate normals
	void generate_normals() {
		float d = state.terrain_size/normals_res;
		float eps = 0.01f;

		for (int x = 0; x < normals_res; x++) {
			for (int y = 0; y < normals_res; y++) {
				float x_ = x * d - state.terrain_size/2.0f;
				float z_ = y * d - state.terrain_size/2.0f;

				glm::vec3 grad_x {2 * d, 0, 0};
				if (x > 0 && x < normals_res - 1) {
					float y1 = eval(x_ + eps, z_);
					float y2 = eval(x_ - eps, z_);

					grad_x.y = (y1 - y2) / (2 * eps);
				}

				glm::vec3 grad_z {0, 0, 2 * d};
				if (y > 0 && y < normals_res - 1) {
					float y1 = eval(x_, z_ + eps);
					float y2 = eval(x_, z_ - eps);

					grad_z.y = (y1 - y2) / (2 * eps);
				}

				glm::vec3 n = -glm::normalize(glm::cross(grad_x, grad_z));
				normals[x * normals_res + y] = (n * 0.5f + 0.5f);
			}
		}
	}

	// Creating texture
	static unsigned int make_texture(uint8_t *data, int res) {
		unsigned int tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, res, res, 0, GL_RED, GL_UNSIGNED_BYTE, data);
		glBindImageTexture(1, tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
		return tex;
	}
public:
	unsigned int	t_grass;
	unsigned int	t_length;
	unsigned int	t_power;
	unsigned int	t_normal;

	// Constructor
	GrassMap(int resolution, float frequency, int octaves)
			: data_res(resolution), normals_res(2 * resolution) {
		// Allocate memory for heightmap
		grass = new float[data_res * data_res];
		grass_length = new float[data_res * data_res];
		grass_power = new float[data_res * data_res];
		normals = new glm::vec3[normals_res * normals_res];

		// Generate heightmap and normals
		generate_heightmap(frequency, octaves);
		generate_normals();

		// Convert to byte array
		uint8_t *image_grass = new uint8_t[resolution * resolution];
		uint8_t *image_grass_length = new uint8_t[resolution * resolution];
		uint8_t *image_grass_power = new uint8_t[resolution * resolution];

		constexpr uint8_t max = 255;
		for (int i = 0; i < resolution * resolution; i++) {
			image_grass[i] = (uint8_t) (grass[i] * max);
			image_grass_length[i] = (uint8_t) (grass_length[i] * max);
			image_grass_power[i] = (uint8_t) (grass_power[i] * max);
		}

		// Create heightmap texture
		t_grass = make_texture(image_grass, resolution);
		t_length = make_texture(image_grass_length, resolution);
		t_power = make_texture(image_grass_power, resolution);

		// Create normal texture
		glGenTextures(1, &t_normal);
		glBindTexture(GL_TEXTURE_2D, t_normal);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, normals_res, normals_res, 0, GL_RGB, GL_FLOAT, normals);
		glBindImageTexture(2, t_normal, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGB32F);

		// Unbind textures
		glBindTexture(GL_TEXTURE_2D, 0);

		// Free memory
		delete[] grass;
		delete[] grass_length;
		delete[] grass_power;
		delete[] normals;
		delete[] image_grass;
		delete[] image_grass_length;
		delete[] image_grass_power;
	}
};

// Application state
State state;

// Shaders
struct Shaders {
	unsigned int pixelizer;
	unsigned int texturizer;

	// Construction
	Shaders() {
		// Pixelizer
		unsigned int shader = compile_shader("shaders/pixelizer.glsl", GL_COMPUTE_SHADER);

		pixelizer = glCreateProgram();
		glAttachShader(pixelizer, shader);

		// Link shaders.pixelizer
		if (!link_program(pixelizer))
			throw std::runtime_error("Failed to link pixelizer shaders.pixelizer");

		// Texturizer
		unsigned int vertex_shader = compile_shader("shaders/texture.vert", GL_VERTEX_SHADER);
		unsigned int fragment_shader = compile_shader("shaders/texture.frag", GL_FRAGMENT_SHADER);

		// Create shaders.pixelizer
		texturizer = glCreateProgram();
		glAttachShader(texturizer, vertex_shader);
		glAttachShader(texturizer, fragment_shader);

		// Link shaders.pixelizer
		if (!link_program(texturizer))
			throw std::runtime_error("Failed to link texturizer shaders.pixelizer");

		// Delete shaders
		glDeleteShader(shader);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}

	// Destruction
	~Shaders() {
		glDeleteProgram(pixelizer);
		glDeleteProgram(texturizer);
	}
};

// TODO: bvh per 3-cluster of quads

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
	HeightMap heightmap(128, 1.0f, 4);

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

	// Grass texture
	const char *grass_texture_path = "grass.png";

	int grass_texture_width, grass_texture_height;

	// Enable flipping the image vertically
	stbi_set_flip_vertically_on_load(true);
	unsigned char *grass_texture_data = stbi_load(
		grass_texture_path,
		&grass_texture_width,
		&grass_texture_height,
		nullptr, STBI_rgb_alpha
	);

	if (!grass_texture_data) {
		std::cout << "Failed to load grass texture" << std::endl;
		return -1;
	} else {
		std::cout << "Loaded grass texture" << std::endl;
	}

	// Create and bind texture (binding 4)
	unsigned int grass_texture;
	glGenTextures(1, &grass_texture);
	glBindTexture(GL_TEXTURE_2D, grass_texture);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, grass_texture_width, grass_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, grass_texture_data);

	// Bind texture as sampler
	glBindImageTexture(4, grass_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA);

	// Create shaders
	Shaders shaders;

	glm::vec3 origin {0, 5, -5};
	glm::vec3 lookat {0, 2, 0};
	glm::vec3 up {0, 1, 0};

	set_int(shaders.pixelizer, "width", WIDTH);
	set_int(shaders.pixelizer, "height", HEIGHT);
	set_int(shaders.pixelizer, "pixel", PIXEL_SIZE);
	set_int(shaders.pixelizer, "clouds", state.show_clouds);
	set_int(shaders.pixelizer, "normals", state.show_normals);
	set_int(shaders.pixelizer, "grass", state.show_grass);
	set_int(shaders.pixelizer, "grass_lenght", state.show_grass_length);
	set_int(shaders.pixelizer, "grass_power", state.show_grass_power);
	set_float(shaders.pixelizer, "ray_marching_step", state.ray_marching_step);
	set_float(shaders.pixelizer, "ray_shadow_step", state.ray_shadow_step);
	set_float(shaders.pixelizer, "terrain_size", state.terrain_size);

	camera = Camera {origin, lookat, up};
	camera.send_to_shader(shaders.pixelizer);

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

	set_int(shaders.pixelizer, "primitives", tile.triangles.size());
	// set_int(shaders.pixelizer, "primitives", 0);

	std::cout << "Buffer size = " << bvh_buffer.size() << std::endl;
	std::cout << "Triangles = " << tile.triangles.size() << std::endl;

	set_vec3(shaders.pixelizer, "light_dir", glm::normalize( glm::vec3 {1, 1, 1} ));

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
		camera.send_to_shader(shaders.pixelizer);

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

		set_vec3(shaders.pixelizer, "light_dir", glm::normalize(glm::vec3 {x, y, x}));
		sun_time = std::fmod(sun_time + t/1000.0f, 2 * glm::pi <float>()); */

		// Ray tracing
		{
			// Bind shaders.pixelizer and dispatch
			glUseProgram(shaders.pixelizer);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, heightmap.t_height);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, heightmap.t_normal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, cloud_density);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, grass_texture);
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

			glDispatchCompute(WIDTH/PIXEL_SIZE, HEIGHT/PIXEL_SIZE, 1);
		}

		// Wait
		// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Render texture
		{
			// Bind shaders.pixelizer and draw
			glUseProgram(shaders.texturizer);

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

				if (ImGui::Checkbox("Show grass length", &state.show_grass_length))
					state.show_grass_power = false;

				if (ImGui::Checkbox("Show grass power", &state.show_grass_power))
					state.show_grass_length = false;

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
		set_int(shaders.pixelizer, "primitives", primitives);

		set_int(shaders.pixelizer, "clouds", state.show_clouds);
		set_int(shaders.pixelizer, "normals", state.show_normals);
		set_int(shaders.pixelizer, "grass", state.show_grass);
		set_int(shaders.pixelizer, "grass_length", state.show_grass_length);
		set_int(shaders.pixelizer, "grass_power", state.show_grass_power);
		set_float(shaders.pixelizer, "ray_marching_step", state.ray_marching_step);
		set_float(shaders.pixelizer, "ray_shadow_step", state.ray_shadow_step);

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
	}
}

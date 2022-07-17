#ifndef COMMON_H_
#define COMMON_H_

// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

// GLFW and GLAD
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

// Perlin noise
#include <PerlinNoise.hpp>

// App headers
#include "bvh.hpp"
#include "core.hpp"
#include "shades.hpp"

const int WIDTH = 500;
const int HEIGHT = 500;
const int PIXEL_SIZE = 4;

inline float randf()
{
	return (float) rand() / (float) RAND_MAX;
}

inline float randf(float min, float max)
{
	return randf() * (max - min) + min;
}

inline std::string read_glsl(const std::string &path)
{
	// Open file
	std::ifstream file(path);
	if (!file.is_open()) {
		printf("Failed to open file: %s\n", path.c_str());
		return "";
	}

	// Read lines
	std::string source;

	std::string line;
	while (std::getline(file, line)) {
		// Tokenize line
		std::stringstream ss(line);
		std::string token;

		ss >> token;
		if (token == "#include") {
			ss >> token;

			// check that token is of the format "<file>"
			if (token.size() < 2 || token[0] != '<' || token[token.size() - 1] != '>') {
				printf("Invalid include directive: %s\n", line.c_str());
				return "";
			}

			// Get file name
			std::string file_name = token.substr(1, token.size() - 2);

			// Get file path, which is relative
			std::string file_path = path.substr(0, path.find_last_of('/') + 1);

			// Read file
			std::string include_source = read_glsl(file_path + file_name);

			// Append include source
			source += include_source;
		} else {
			source += line + "\n";
		}
	}

	return source;
}

// GLFW and OpenGL helpers
int compile_shader(const char *, unsigned int);
int link_program(unsigned int);
void set_int(unsigned int, const char *, int);
void set_float(unsigned int, const char *, float);
void set_vec2(unsigned int, const char *, const glm::vec2 &);
void set_vec3(unsigned int, const char *, const glm::vec3 &);

// Camera stuff
struct Camera {
	glm::vec3 front {0.0f, 0.0f, -1.0f};
	glm::vec3 up {0.0f, 1.0f, 0.0f};
	glm::vec3 right {1.0f, 0.0f, 0.0f};
	glm::vec3 eye {0.0f};

	// Default constructor
	Camera() = default;

	// Initialize camera
	Camera(const glm::vec3 &eye_, const glm::vec3 &lookat_, const glm::vec3 &up_) {
		glm::vec3 a = lookat_ - eye_;
		glm::vec3 b = up_;

		front = glm::normalize(a);
		right = glm::normalize(glm::cross(b, front));
		up = glm::cross(front, right);

		eye = eye_;
	}

	// Send camera info to shader program
	void send_to_shader(unsigned int program) const {
		set_vec3(program, "camera.origin", eye);
		set_vec3(program, "camera.front", front);
		set_vec3(program, "camera.up", up);
		set_vec3(program, "camera.right", right);
	}

	// Move camera
	void move(float dx, float dy, float dz) {
		eye += dx * right + dy * up + dz * front;
	}

	// Set yaw and pitch
	void set_yaw_pitch(float yaw, float pitch) {
		front = glm::normalize(glm::vec3(
			cos(pitch) * cos(yaw),
			sin(pitch),
			cos(pitch) * sin(yaw)
		));

		right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		up = glm::cross(right, front);
	}
};

extern Camera camera;

void mouse_callback(GLFWwindow *, double, double);
GLFWwindow *initialize_graphics();

// TODO: add info about normals
// TODO: use indices and another vertex structure
struct Vertex {
	glm::vec3 position;
};

struct Triangle {
	uint32_t v1, v2, v3;
	Shades shade = Shades::eNone;

	Triangle(uint32_t v1, uint32_t v2, uint32_t v3)
			: v1(v1), v2(v2), v3(v3) {}

	Triangle(uint32_t v1, uint32_t v2, uint32_t v3, Shades shade)
			: v1(v1), v2(v2), v3(v3), shade(shade) {}
};

using VBuffer = std::vector <aligned_vec4>;
using IBuffer = std::vector <aligned_uvec4>;

struct Mesh {
	std::vector <Vertex> vertices;
	std::vector <Triangle> triangles;

	// Add another mesh to this mesh
	void add(const Mesh &mesh) {
		int size = vertices.size();
		vertices.insert(vertices.end(),
			mesh.vertices.begin(),
			mesh.vertices.end()
		);

		// Need to reindex indices
		for (auto &triangle : mesh.triangles) {
			triangles.push_back({
				triangle.v1 + size,
				triangle.v2 + size,
				triangle.v3 + size,
				triangle.shade
			});
		}
	}

	// Serialize mesh vertices and indices to buffers
	void serialize_vertices(VBuffer &vbuffer) const {
		vbuffer.reserve(vertices.size());
		for (const auto &v : vertices)
			vbuffer.push_back(aligned_vec4(v.position));
	}

	// TODO: format should be uvec4: v1, v2, v3, type (water, leaves, grass,
	// etc.)
	void serialize_indices(IBuffer &ibuffer) const {
		for (const auto &triangle : triangles) {
			ibuffer.push_back(glm::uvec4 {
				triangle.v1,
				triangle.v2,
				triangle.v3,
				(uint32_t) triangle.shade
			});
		}
	}

	// Make BVH
	BVH *make_bvh() {
		std::vector <BVH *> nodes;
		nodes.reserve(triangles.size());

		int id = 0;
		for (const auto &tri: triangles) {
			glm::vec3 min = vertices[tri.v1].position;
			glm::vec3 max = vertices[tri.v1].position;

			min = glm::min(min, vertices[tri.v2].position);
			max = glm::max(max, vertices[tri.v2].position);

			min = glm::min(min, vertices[tri.v3].position);
			max = glm::max(max, vertices[tri.v3].position);


			nodes.push_back(new BVH(BBox {min, max}, id++));
		}

		return partition(nodes);
	}
};

// Generate transform matrix
struct Transform {
	glm::vec3 pos;
	glm::vec3 rot;
	glm::vec3 scale;

	Transform(const glm::vec3 &pos_ = glm::vec3(0.0f),
			const glm::vec3 &rot_ = glm::vec3(0.0f),
			const glm::vec3 &scale_ = glm::vec3(1.0f))
			: pos(pos_), rot(rot_), scale(scale_) {}

	glm::mat4 matrix() {
		// Convert rot to radians
		glm::vec3 rad_rot = rot * (float) M_PI / 180.0f;

		glm::mat4 mat {1.0f};
		mat = glm::translate(mat, pos);
		mat = glm::rotate(mat, rad_rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
		mat = glm::rotate(mat, rad_rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
		mat = glm::rotate(mat, rad_rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
		mat = glm::scale(mat, scale);
		return mat;
	}
};

// Generate pillar mesh
inline Mesh generate_pillar(const glm::mat4 &transform)
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
inline Mesh generate_terrain(int resolution)
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
inline Mesh generate_tile(int resolution)
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

// State for the application
struct State {
	bool show_clouds = true;
	bool show_grass = true;
	bool show_grass_map = false;
	bool show_grass_length = false;
	bool show_grass_power = false;
	bool show_normals = false;
	bool show_triangles = false;
	bool tab = false;
	bool viewing_mode = true;
	float ray_marching_step = 0.1f;
	float ray_shadow_step = 0.001f;
	const float terrain_size = 20.0f;

	// TODO: method to apply settings if changed
};

extern State state;

inline float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

class HeightMap {
	float		*data;
	int		data_res;

	// Generate heightmap
	void generate_grassmap(float frequency, int octaves) {
		srand(clock());

		// Perlin noise generator
		uint32_t seed = rand();
		const siv::PerlinNoise perlin_grass {seed};

		const double f = (frequency/data_res);
		for (int i = 0; i < data_res * data_res; i++) {
			int x = i % data_res;
			int y = i / data_res;
			data[i] = perlin_grass.octave2D_01(x * f, y * f, octaves);
		}
	}

	// Evaluate the heightmap at a given point
	float eval(float x, float z) {
		// x and z are in the range +/- terrain_size

		// Scale to [0, resolution]
		x = data_res/2.0f + x * data_res/state.terrain_size;
		z = data_res/2.0f + z * data_res/state.terrain_size;

		// Floor to nearest int
		int x0 = glm::clamp((float) floor(x), 0.0f, data_res - 1.0f);
		int z0 = glm::clamp((float) floor(z), 0.0f, data_res - 1.0f);

		int z1 = glm::clamp((float) ceil(z), 0.0f, data_res - 1.0f);
		int x1 = glm::clamp((float) ceil(x), 0.0f, data_res - 1.0f);

		// Get fractional parts
		float xf = x - x0;
		float zf = z - z0;

		// Get heights
		float h00 = data[z0 * data_res + x0];
		float h01 = data[z0 * data_res + x1];
		float h10 = data[z1 * data_res + x0];
		float h11 = data[z1 * data_res + x1];

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
public:
	unsigned int	t_height;
	unsigned int	t_normal;

	// Constructor
	HeightMap(int resolution, float frequency, int octaves)
			: data_res(resolution), normals_res(2 * resolution) {
		// Allocate memory for heightmap
		data = new float[data_res * data_res];
		normals = new glm::vec3[normals_res * normals_res];

		// Generate heightmap and normals
		generate_grassmap(frequency, octaves);
		generate_normals();

		// Convert to byte array
		uint8_t *image = new uint8_t[resolution * resolution];
		for (int i = 0; i < resolution * resolution; i++)
			image[i] = (uint8_t) (data[i] * std::numeric_limits <uint8_t> ::max());

		// Create texture
		glGenTextures(1, &t_height);
		glBindTexture(GL_TEXTURE_2D, t_height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, resolution, resolution, 0, GL_RED, GL_UNSIGNED_BYTE, image);
		glBindImageTexture(5, t_height, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);

		// Create normal texture
		glGenTextures(1, &t_normal);
		glBindTexture(GL_TEXTURE_2D, t_normal);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, normals_res, normals_res, 0, GL_RGB, GL_FLOAT, normals);
		glBindImageTexture(6, t_normal, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGB32F);

		// TODO: constexpr binding points

		// Unbind textures
		glBindTexture(GL_TEXTURE_2D, 0);

		// Free memory
		delete[] data;
		delete[] normals;
	}
};

#endif

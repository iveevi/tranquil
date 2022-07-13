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

// App headers
#include "bvh.hpp"
#include "core.hpp"
#include "shades.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;
const int PIXEL_SIZE = 4;

inline float randf()
{
	return (float) rand() / (float) RAND_MAX;
}

inline float randf(float min, float max)
{
	return randf() * (max - min) + min;
}

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

int compile_shader(const char *, unsigned int);
int link_program(unsigned int);
void set_int(unsigned int, const char *, int);
void set_vec3(unsigned int, const char *, const glm::vec3 &);

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

#endif

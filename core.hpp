#ifndef CORE_H_
#define CORE_H_

// Standard headers
#include <iostream>

// GLM headers
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

// Aligned types
struct alignas(16) aligned_vec4 {
	glm::vec4 v;

	aligned_vec4() : v(glm::vec4(0.0f)) {}
	aligned_vec4(const glm::vec3 &v) : v(glm::vec4(v, 0.0f)) {}
	aligned_vec4(const glm::vec4 &v) : v(v) {}
};

struct alignas(16) aligned_uvec4 {
	glm::uvec4 v;

	aligned_uvec4() : v(glm::uvec4(0)) {}
	aligned_uvec4(const glm::uvec3 &v) : v(glm::uvec4(v, 0)) {}
	aligned_uvec4(const glm::uvec4 &v) : v(v) {}
};

inline std::ostream &operator<<(std::ostream &os, const aligned_vec4 &v)
{
	return os << glm::to_string(v.v);
}

inline std::ostream &operator<<(std::ostream &os, const aligned_uvec4 &v)
{
	return os << glm::to_string(v.v);
}

#endif

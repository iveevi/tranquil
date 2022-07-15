// Shader program inputs
layout (local_size_x = 1, local_size_y = 1) in;

layout (rgba32f, binding = 0) uniform image2D image;

layout (std430, binding = 1) buffer Vertices {
	vec4 data[];
} vertices;

layout (std430, binding = 2) buffer Triangles {
	uvec4 data[];
} triangles;

layout (std430, binding = 3) buffer BVH {
	vec4 data[];
} bvh;

uniform sampler2D s_heightmap;

uniform int width;
uniform int height;
uniform int pixel;
uniform int primitives;

struct Camera {
	vec3 origin;
	vec3 front;
	vec3 up;
	vec3 right;
};

uniform Camera camera;

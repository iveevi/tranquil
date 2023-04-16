// Shader program inputs
layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba32f, binding = 0) uniform image2D image;
// layout (r8, binding = 8) uniform image2D segments;

layout (std430, binding = 1) buffer Vertices {
	vec4 data[];
} vertices;

layout (std430, binding = 2) buffer Triangles {
	uvec4 data[];
} triangles;

layout (std430, binding = 3) buffer BVH {
	vec4 data[];
} bvh;

layout (binding = 0) uniform sampler2D s_heightmap;
layout (binding = 1) uniform sampler2D s_heightmap_normal;

layout (binding = 2) uniform sampler2D s_clouds;

layout (binding = 3) uniform sampler2D s_grass;

layout (binding = 4) uniform sampler2D s_grassmap;
layout (binding = 5) uniform sampler2D s_grassmap_normal;
layout (binding = 6) uniform sampler2D s_grass_length;
layout (binding = 7) uniform sampler2D s_grass_power;

layout (binding = 8) uniform sampler2D s_water_level;
layout (binding = 9) uniform sampler2D s_water_normal1;
layout (binding = 10) uniform sampler2D s_water_normal2;

layout (binding = 11) uniform sampler2D s_wind;

uniform int width;
uniform int height;
uniform int pixel;
uniform int offx;
uniform int offy;

uniform float ray_marching_step;
uniform float ray_shadow_step;
uniform float terrain_size;

uniform int clouds;
uniform int grass;
uniform int grass_blades;
uniform int grass_density;
uniform int grass_length;
uniform int grass_power;
uniform int normals;
uniform int primitives;

uniform int wind_map;

// uniform vec2 wind_offset;
uniform vec2 water_offset;

uniform vec3 light_dir;

struct Camera {
	vec3 origin;
	vec3 front;
	vec3 up;
	vec3 right;
};

uniform Camera camera;

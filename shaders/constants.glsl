// Shading constants
uint eNone = 0;
uint eWater = 1;
uint eGrass = 2;
uint ePillar = 3;
uint eGrassBlade = 4;

// Constants
const float PI = 3.14159265358979323846f;
const float fov = 60.0f;

// Terrain properties

// x and z range
// TODO: concat into vec2s
float xmin = -terrain_size * 0.5f;
float xmax = terrain_size * 0.5f;

float zmin = -terrain_size * 0.5f;
float zmax = terrain_size * 0.5f;

// Height scaling
const float scale = 3.0f;
const float water_level = 1.5f;

// Shading
const vec3 light_intensity = 2 * vec3(1, 1, 0.6);

// Terrain uv
vec2 terrain_uv(vec2 xz)
{
	return (xz - vec2(xmin, zmin)) / vec2(terrain_size);
}

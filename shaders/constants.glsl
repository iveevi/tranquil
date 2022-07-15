// Shading constants
uint eNone = 0;
uint eWater = 1;
uint eGrass = 2;
uint ePillar = 3;

// Constants
const float PI = 3.14159265358979323846f;
const float fov = 45.0f;

// Intersection between ray and height map
const float hmap_width = 10.0f;
const float hmap_height = 10.0f;

// x and z range
const float xmin = -hmap_width * 0.5f;
const float xmax = hmap_width * 0.5f;

const float zmin = -hmap_height * 0.5f;
const float zmax = hmap_height * 0.5f;

float scale = 2.0f;

// Shading
const vec3 light_dir = normalize(vec3(0.5, 0.5, 0.5));
const vec3 light_intensity = vec3(1, 1, 0.8);


#version 430

// Structures
struct Ray {
	vec3 p;
	vec3 d;
};

struct Camera {
	vec3 origin;
	vec3 front;
	vec3 up;
	vec3 right;
};

struct Triangle {
	vec3 v1;
	vec3 v2;
	vec3 v3;
};

struct Intersection {
	float t;
	vec3 p;
	vec3 n;
	int id;
	uint shading;
};

struct BoundingBox {
	vec3 pmin;
	vec3 pmax;
};

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

uniform Camera camera;

// Constants
const float PI = 3.14159265358979323846f;
const float fov = 45.0f;

// Shading constants
uint eNone = 0;
uint eWater = 1;
uint eGrass = 2;
uint ePillar = 3;

// Ray generation
Ray generate_ray(vec2 uv)
{
	float rad_fov = fov * PI/180.0f;
	float scale = tan(rad_fov * 0.5f);
	float aspect = float(width) / float(height);

	vec2 cuv = (1.0f - 2.0f * uv) * vec2(scale * aspect, scale);
	vec3 dir = normalize(camera.right * cuv.x - camera.up * cuv.y + camera.front);

	return Ray(camera.origin, dir);
}

// Shapes and intersection
float _intersect_time(Ray r, Triangle t)
{
	vec3 e1 = t.v2 - t.v1;
	vec3 e2 = t.v3 - t.v1;
	vec3 s1 = cross(r.d, e2);
	float divisor = dot(s1, e1);
	if (divisor == 0.0)
		return -1.0;
	vec3 s = r.p - t.v1;
	float inv_divisor = 1.0 / divisor;
	float b1 = dot(s, s1) * inv_divisor;
	if (b1 < 0.0 || b1 > 1.0)
		return -1.0;
	vec3 s2 = cross(s, e1);
	float b2 = dot(r.d, s2) * inv_divisor;
	if (b2 < 0.0 || b1 + b2 > 1.0)
		return -1.0;
	float time = dot(e2, s2) * inv_divisor;
	return time;
}

// Generate default intersection
Intersection def_it()
{
	return Intersection(1.0/0.0, vec3(0.0), vec3(0.0), -1, eNone);
}

Intersection _intersect(Ray r, Triangle tri, uint shading)
{
	float t = _intersect_time(r, tri);
	if (t < 0.0)
		return def_it();
	vec3 p = r.p + r.d * t;
	vec3 e1 = tri.v2 - tri.v1;
	vec3 e2 = tri.v3 - tri.v1;
	vec3 n = -cross(e1, e2);
	if (dot(n, r.d) > 0.0)
		n = -n;
	return Intersection(t, p, normalize(n), 0, shading);
}

Intersection intersect(Ray r, int i)
{
	uvec4 tri = triangles.data[i];
	uint a = tri.x;
	uint b = tri.y;
	uint c = tri.z;

	Triangle t = Triangle(
		vertices.data[a].xyz,
		vertices.data[b].xyz,
		vertices.data[c].xyz
	);

	return  _intersect(r, t, tri.w);
}

// Get left and right child of the node
int object(int node)
{
	vec4 prop = bvh.data[node];
	return floatBitsToInt(prop.x);
}

int hit(int node)
{
	vec4 prop = bvh.data[node];
	return floatBitsToInt(prop.y);
}

int miss(int node)
{
	vec4 prop = bvh.data[node];
	return floatBitsToInt(prop.z);
}

// Intersect bounding box
// TODO: optimize or shorten this function
float intersect_box(Ray ray, BoundingBox box)
{
	float tmin = (box.pmin.x - ray.p.x) / ray.d.x;
	float tmax = (box.pmax.x - ray.p.x) / ray.d.x;

	// TODO: swap function?
	if (tmin > tmax) {
		float tmp = tmin;
		tmin = tmax;
		tmax = tmp;
	}

	float tymin = (box.pmin.y - ray.p.y) / ray.d.y;
	float tymax = (box.pmax.y - ray.p.y) / ray.d.y;

	if (tymin > tymax) {
		float tmp = tymin;
		tymin = tymax;
		tymax = tmp;
	}

	if ((tmin > tymax) || (tymin > tmax))
		return -1.0;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (box.pmin.z - ray.p.z) / ray.d.z;
	float tzmax = (box.pmax.z - ray.p.z) / ray.d.z;

	if (tzmin > tzmax) {
		float tmp = tzmin;
		tzmin = tzmax;
		tzmax = tmp;
	}

	if ((tmin > tzmax) || (tzmin > tmax))
		return -1.0;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	return tmin;
}

// Check if point is in bounding box
bool in_box(vec3 point, BoundingBox box)
{
	bvec3 lt = lessThan(point, box.pmin);
	bvec3 gt = greaterThan(point, box.pmax);
	return !(any(lt) || any(gt));
}

BoundingBox bbox(int node)
{
	vec3 min = bvh.data[node + 1].xyz;
	vec3 max = bvh.data[node + 2].xyz;
	return BoundingBox(min, max);
}

Intersection trace(Ray ray)
{
	// "Min" intersection
	Intersection mini = def_it();

	// Traverse BVH as a threaded binary tree
	int node = 0;
	while (node != -1) {
		if (object(node) != -1) {
			// Get object index
			int index = object(node);

			// Get object
			Intersection it = intersect(ray, index);

			// If intersection is valid, update minimum
			if (it.id != -1 && it.t < mini.t)
				mini = it;

			// Go to next node (same as miss)
			node = miss(node);
		} else {
			// Get bounding box
			BoundingBox box = bbox(node);

			// Check if ray intersects (or is inside)
			// the bounding box
			float t = intersect_box(ray, box);
			bool inside = in_box(ray.p, box);

			if ((t > 0.0 && t < mini.t) || inside)
				node = hit(node);
			else
				node = miss(node);
		}
	}

	// Return intersection
	return mini;
}

void get_base(uint shading, out vec3 Kd, out vec3 Ks, inout bool valid)
{
	if (shading == eGrass) {
		Kd = vec3(0.5, 1, 0.5);
		Ks = vec3(0, 0, 0);
	} else if (shading == ePillar) {
		Kd = vec3(0.5, 0.5, 0.5);
		Ks = vec3(0.5, 0.5, 0.5);
	} else {
		valid = false;
		Kd = vec3(1, 0, 1);
	}
}

float vec_min(vec3 v)
{
	return min(min(v.x, v.y), v.z);
}

float vec_max(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}

float quantize(float v, float b)
{
	return floor(v * b) / b;
}

vec4 shade(Intersection it)
{
	bool valid = true;

	vec3 Kd = vec3(0, 0, 0);
	vec3 Ks = vec3(0, 0, 0);

	get_base(it.shading, Kd, Ks, valid);
	if (!valid)
		return vec4(Kd, 1);

	// Directional light
	vec3 light = normalize(vec3(1, 1, 1));
	vec3 normal = normalize(it.n);
	vec3 view = normalize(camera.origin - it.p);
	vec3 whalf = normalize(light + view);

	float ndotl = max(dot(normal, light), 0.0);
	float ndoth = max(dot(normal, whalf), 0.0);
	float pf = pow(ndoth, 32.0);

	vec3 diffuse = Kd * ndotl;
	vec3 specular = 0.1 * Ks  * pf;
	vec3 ambient = Kd * 0.25f;
	vec3 color = diffuse + specular + ambient;

	return vec4(color, 1);
}

// Intersection between ray and height map
const float hmap_width = 10.0f;
const float hmap_height = 10.0f;

// x and z range
const float xmin = -hmap_width * 0.5f;
const float xmax = hmap_width * 0.5f;

const float zmin = -hmap_height * 0.5f;
const float zmax = hmap_height * 0.5f;

float scale = 2.0f;

// Height value at xz
float hmap(float x, float z)
{
	vec2 uv = (vec2(x, z) - vec2(xmin, zmin)) / vec2(hmap_width, hmap_height);
	return scale * texture(s_heightmap, uv).r;
}

float hmap(Ray r, float t)
{
	vec3 p = r.p + r.d * t;
	return hmap(p.x, p.z);
}

vec3 hmap_normal(float x, float z)
{
	float y_x1 = hmap(x + 0.001, z);
	float y_x2 = hmap(x - 0.001, z);

	vec3 grad_x = vec3(0.002, y_x1 - y_x2, 0);

	float y_z1 = hmap(x, z + 0.001);
	float y_z2 = hmap(x, z - 0.001);

	vec3 grad_z = vec3(0, y_z1 - y_z2, 0.002);

	return -normalize(cross(grad_x, grad_z));

	/* float dy_dx = (hmap(x + 0.01, z) - hmap(x - 0.01, z))/0.02;
	float dy_dz = (hmap(x, z + 0.01) - hmap(x, z - 0.01))/0.02;

	return normalize(vec3(dy_dx, 1, dy_dz)); */
}

float hmap_derivative(Ray r, float t)
{
	vec3 p0 = r.p + r.d * (t - 0.01);
	vec3 p1 = r.p + r.d * (t + 0.01);

	float dy_dt = (hmap(p0.x, p0.z) - hmap(p1.x, p1.z))/0.02;
	return dy_dt;
}

float _intersect_heightmap(Ray r)
{

	// Solve for the time when ray intrsects
	// these planes
	float t1 = (xmin - r.p.x) / r.d.x;
	float t2 = (xmax - r.p.x) / r.d.x;

	float t3 = (zmin - r.p.z) / r.d.z;
	float t4 = (zmax - r.p.z) / r.d.z;

	float tmin = max(min(t1, t2), min(t3, t4));
	float tmax = min(max(t1, t2), max(t3, t4));

	// Find when y = 0
	float t5 = (0.0f - r.p.y) / r.d.y;
	if (tmax < 0.0f)
		return -1.0f;

	// Brute search for the intersection
	vec3 p = r.p + r.d * tmin;
	float y = hmap(p.x, p.z);

	if (y < p.y) {
		float dt = 0.01f;

		float lh = 0.0f;
		float ly = 0.0f;

		for (float t = tmin; t < tmax; t += dt) {
			p = r.p + r.d * t;
			y = hmap(p.x, p.z);
			if (y >= p.y) {
				// Interpolate distance
				return t - dt + dt * (lh - ly)/(p.y - ly - y + lh);
			}

			ly = p.y;
			lh = y;
		}
	}

	// Everything under is not visible
	return -1.0f;
}

Intersection intersect_heightmap(Ray r)
{
	float t = _intersect_heightmap(r);
	if (t < 0.0f)
		return def_it();

	Intersection it;
	it.id = 0;
	it.t = t;
	it.p = r.p + r.d * t;
	it.n = hmap_normal(it.p.x, it.p.z);
	it.shading = eGrass;
	return it;
}

void main()
{
	ivec2 coord = pixel * ivec2(gl_GlobalInvocationID.xy);

	vec2 uv = vec2(coord) + vec2(pixel/2.0f);
	uv /= vec2(width, height);

	Ray r = generate_ray(uv);
	// Intersection it = trace(r);

	// If no hit, then some blue gradient
	vec4 color = vec4(0.1, 0.1, 1 - dot(r.d, camera.front), 1.0);

	Intersection it = intersect_heightmap(r);
	if (it.id != -1)
		color = shade(it);

	// Set same color to all pixels
	for (int i = 0; i < pixel; i++) {
		for (int j = 0; j < pixel; j++) {
			imageStore(image,
				ivec2(coord.x + i, coord.y + j),
				clamp(color, 0.0, 1.0)
			);
		}
	}
}

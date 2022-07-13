#version 430

layout (local_size_x = 1, local_size_y = 1) in;

layout (rgba32f, binding = 0) uniform image2D image;

layout (std430, binding = 1) buffer Vertices {
	vec4 data[];
} vertices;

layout (std430, binding = 2) buffer Triangles {
	uint data[];
} triangles;

layout (std430, binding = 3) buffer BVH {
	vec4 data[];
} bvh;

uniform int width;
uniform int height;
uniform int pixel;
uniform int primitives;

float fov = 45.0f;

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

uniform Camera camera;

// Ray generation
Ray gen_ray(vec2 uv)
{
	float rad_fov = fov * 3.1415926535897932384626433832795 / 180.0f;
	float scale = tan(rad_fov * 0.5f);
	float aspect = float(width) / float(height);

	vec2 cuv = (1.0f - 2.0f * uv) * vec2(scale * aspect, scale);
	vec3 dir = normalize(camera.right * cuv.x - camera.up * cuv.y + camera.front);

	return Ray(camera.origin, dir);
}

// Shapes and intersection
struct Sphere {
	vec3 center;
	float radius;
};

float _intersect_time(Ray r, Sphere s)
{
	vec3 oc = r.p - s.center;
	float a = dot(r.d, r.d);
	float b = 2.0 * dot(oc, r.d);
	float c = dot(oc, oc) - s.radius * s.radius;
	float d = b * b - 4.0 * a * c;

	if (d < 0.0)
		return -1.0;

	float t1 = (-b - sqrt(d)) / (2.0 * a);
	float t2 = (-b + sqrt(d)) / (2.0 * a);

	return min(t1, t2);
}

struct Triangle {
	vec3 v1;
	vec3 v2;
	vec3 v3;
};

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

struct Intersection {
	float t;
	vec3 p;
	vec3 n;
	int id;
};

Intersection _intersect(Ray r, Triangle tri)
{
	float t = _intersect_time(r, tri);
	if (t < 0.0)
		return Intersection(-1, vec3(0.0), vec3(0.0), -1);
	vec3 p = r.p + r.d * t;
	vec3 e1 = tri.v2 - tri.v1;
	vec3 e2 = tri.v3 - tri.v1;
	vec3 n = -cross(e1, e2);
	if (dot(n, r.d) > 0.0)
		n = -n;
	return Intersection(t, p, normalize(n), 0);
}

Intersection intersect(Ray r, int i)
{
	uint a = triangles.data[i * 3];
	uint b = triangles.data[i * 3 + 1];
	uint c = triangles.data[i * 3 + 2];

	Triangle t = Triangle(
		vertices.data[a].xyz,
		vertices.data[b].xyz,
		vertices.data[c].xyz
	);

	return  _intersect(r, t);
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

// Bounding box
struct BoundingBox {
	vec3 pmin;
	vec3 pmax;
};

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
	Intersection mini = Intersection(1.0/0.0, vec3(0.0), vec3(0.0), -1);

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

			if ((t > 0.0 && t < mini.t) || inside) {
				// Traverse left child
				node = hit(node);
			} else {
				// Traverse right child
				node = miss(node);
			}
		}
	}

	// Return intersection
	return mini;
}

void main()
{
	ivec2 coord = pixel * ivec2(gl_GlobalInvocationID.xy);

	vec2 uv = vec2(coord) + vec2(pixel/2.0f);
	uv /= vec2(width, height);

	Ray r = gen_ray(uv);
	Intersection it = trace(r);

	// If no hit, then some blue gradient
	vec4 color = vec4(0.7, 0.7, dot(r.d, camera.front), 1.0);
	if (it.id != -1) {
		color = vec4(0, 0, 1, 1);


		// Directional light
		vec3 light = normalize(vec3(1, 1, 1));
		vec3 normal = normalize(it.n);
		vec3 view = normalize(camera.origin - it.p);
		vec3 whalf = normalize(light + view);

		float ndotl = max(dot(normal, light), 0.0);
		float ndoth = max(dot(normal, whalf), 0.0);
		float pf = pow(ndoth, 32.0);

		vec3 diffuse = vec3(0.5, 1, 0.5) * ndotl;
		vec3 specular = 0.25 * vec3(1, 1, 1) * pf;
		color = vec4(diffuse + specular, 1.0);
	}

	// Set same color to all pixels
	for (int i = 0; i < pixel; i++) {
		for (int j = 0; j < pixel; j++)
			imageStore(image, ivec2(coord.x + i, coord.y + j), color);
	}
}

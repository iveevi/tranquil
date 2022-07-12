#version 430

layout (local_size_x = 400, local_size_y = 1) in;

layout (rgba32f, binding = 0) uniform image2D image;

uniform int width;
uniform int height;
uniform int pixel;

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

struct Intersection {
	float t;
	vec3 p;
	vec3 n;
};

Intersection _intersect(Ray r, Sphere s)
{
	float t = _intersect_time(r, s);
	if (t < 0.0)
		return Intersection(-1, vec3(0.0), vec3(0.0));
	return Intersection(t, r.p + r.d * t, normalize(r.p + r.d * t - s.center));
}

void main()
{
	Sphere s = Sphere(vec3(0.0, 0.0, 0.0), 1);

	ivec2 coord = pixel * ivec2(gl_GlobalInvocationID.xy);

	vec2 uv = vec2(coord) + vec2(pixel/2.0f);
	uv /= vec2(width, height);

	Ray r = gen_ray(uv);

	Intersection it = _intersect(r, s);

	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	if (it.t > 0.0) {
		color = vec4(0, 0, 1, 1);

		vec3 lpos = vec3(0.0, 5.0, 0.0);

		vec3 ldir = normalize(lpos - it.p);
		vec3 lcolor = vec3(1.0, 1.0, 1.0);
		vec3 lintensity = lcolor * max(0.0, dot(it.n, ldir));

		color = vec4(it.n * 0.5 + 0.5, 1.0);
	}

	// Set same color to all pixels
	for (int i = 0; i < pixel; i++) {
		for (int j = 0; j < pixel; j++)
			imageStore(image, ivec2(coord.x + i, coord.y + j), color);
	}
}

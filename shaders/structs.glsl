// Structures
struct Ray {
	vec3 p;
	vec3 d;
};

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

// Triangle
struct Triangle {
	vec3 v1;
	vec3 v2;
	vec3 v3;
};

// Ray-triangle intersection
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

// Quad
struct Quad {
	// Points are assumed to be coplanar
	vec3 v1;	// uv: (0, 0)
	vec3 v2;	// uv: (1, 0)
	vec3 v3;	// uv: (1, 1)
	vec3 v4;	// uv: (0, 1)
};

// UV coordinate at point on quad
vec2 quad_uv(Quad q, vec3 x)
{
	// Project onto plane
	vec3 p = x - q.v1;

	// Calculate uv coordinates
	vec2 uv;
	uv.x = dot(p, q.v2 - q.v1) / dot(q.v2 - q.v1, q.v2 - q.v1);
	uv.y = dot(p, q.v4 - q.v1) / dot(q.v4 - q.v1, q.v4 - q.v1);
	return uv;
}

// Ray-quad intersection
float _intersect_time(Ray r, Quad q)
{
	float t1 = _intersect_time(r, Triangle(q.v1, q.v2, q.v3));
	float t2 = _intersect_time(r, Triangle(q.v1, q.v3, q.v4));
	if (t1 < 0.0 && t2 < 0.0)
		return -1.0;
	if (t1 < 0.0)
		return t2;
	if (t2 < 0.0)
		return t1;
	return min(t1, t2);
}

struct Intersection {
	float t;
	vec3 p;
	vec3 n;
	int id;
	vec3 Kd;
	uint shading;	// For color palette
};

// Default constructor
Intersection def_it()
{
	return Intersection(1.0/0.0, vec3(0.0), vec3(0.0), -1, vec3(0), eNone);
}

struct BoundingBox {
	vec3 pmin;
	vec3 pmax;
};

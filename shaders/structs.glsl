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

// Quadratic bezier
struct QuadraticBezier {
	vec3 v1;
	vec3 v2;
	vec3 v3;
};

vec3 eval(QuadraticBezier q, float t)
{
	return (1.0 - t) * (1.0 - t) * q.v1 + 2.0 * t * (1.0 - t) * q.v2 + t * t * q.v3;
}

const float thickness = 0.02;

#define PI 3.14159265358979
#define HALFPI 1.57079632679

//Find roots using Cardano's method. http://en.wikipedia.org/wiki/Cubic_function#Cardano.27s_method
vec2 solveCubic2(float a, float b, float c)
{
	float p = b-a*a/3., p3 = p*p*p;
	float q = a*(2.*a*a-9.*b)/27.+ c;
	float d = q*q+4.*p3/27.;
	float offset = -a / 3.;
	if(d>0.)
	{ 
		float z = sqrt(d);
		vec2 x = (vec2(z,-z)-q)*0.5;
		vec2 uv = sign(x)*pow(abs(x), vec2(1./3.));
		return vec2(offset + uv.x + uv.y);
	}
	float v = acos(-sqrt(-27./p3)*q/2.)/3.;
	float m = cos(v), n = sin(v)*1.732050808;
	return vec2(m + m, -n - m) * sqrt(-p / 3.0) + offset;
}

// How to resolve the equation below can be seen on this image.
// http://www.perbloksgaard.dk/research/DistanceToQuadraticBezier.jpg
vec3 intersectQuadraticBezier(vec3 p0, vec3 p1, vec3 p2) 
{
	vec2 A2 = p1.xy - p0.xy;
	vec2 B2 = p2.xy - p1.xy - A2;
	vec3 r = vec3(-3.*dot(A2,B2), dot(-p0.xy,B2)-2.*dot(A2,A2), dot(-p0.xy,A2)) / -dot(B2,B2);
	vec2 t = clamp(solveCubic2(r.x, r.y, r.z), 0., 1.);
	vec3 A3 = p1 - p0;
	vec3 B3 = p2 - p1 - A3;
	vec3 D3 = A3 * 2.;
	vec3 pos1 = (D3+B3*t.x)*t.x+p0;
	vec3 pos2 = (D3+B3*t.y)*t.y+p0;
	
	pos1.xy /= (thickness * max(1 - t.x, 0.2));
	pos2.xy /= (thickness * max(1 - t.y, 0.2));

	float pos1Len = length(pos1.xy);
	if (pos1Len > 1.0f)
		pos1 = vec3(1e8);

	float pos2Len = length(pos2.xy);
	if (pos2Len > 1.0f)
		pos2 = vec3(1e8);

	pos1.z -= cos(pos1Len*HALFPI)*thickness;
	pos2.z -= cos(pos2Len*HALFPI)*thickness;
	return (length(pos1) < length(pos2)) ? vec3(pos1Len,pos1.z,t.x) : vec3(pos2Len,pos2.z,t.y);
}

mat3 inverseView(vec2 a)
{
	vec2 c = cos(a);
	vec2 s = sin(a);
	return mat3(c.y,0.,-s.y,s.x*s.y,c.x,s.x*c.y,c.x*s.y,-s.x,c.x*c.y);
}

// Ray-quadratic bezier intersection
float _intersect_time(Ray r, QuadraticBezier qb)
{
	// Transform points so that ray is the +x-axis
	mat3 m = inverseView(vec2(asin(-r.d.y), atan(r.d.x, r.d.z)));

	vec3 v1 = (qb.v1 - r.p) * m;
	vec3 v2 = (qb.v2 - r.p) * m;
	vec3 v3 = (qb.v3 - r.p) * m;

	vec3 t = intersectQuadraticBezier(v1, v2, v3);
	if (t.y < 0.0 || t.y > 1e4)
		return -1.0;

	return t.y;
}

// Coarse ray-quadratic bezier intersection
float _coarse_intersect_time(Ray r, QuadraticBezier qb)
{
	// Pretend its a triangle (for thickness)
	vec3 n1 = thickness * normalize(cross(qb.v3 - qb.v1, r.d));

	vec3 v1 = qb.v3;
	vec3 v2 = qb.v1 - n1;
	vec3 v3 = qb.v1 + n1;

	// Triangle intersection routine
	// TODO: optimize and use a line intersection routine instead
	vec3 e1 = v2 - v1;
	vec3 e2 = v3 - v1;
	vec3 s1 = cross(r.d, e2);
	float divisor = dot(s1, e1);
	if (divisor == 0.0)
		return -1.0;
	vec3 s = r.p - v1;
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

	/* TODO: fix this to use normal
	Triangle t1 = Triangle(v1, qb.v1, qb.v1 + 2 * thickness * n1);
	return _intersect_time(r, t1); */
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

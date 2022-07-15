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
	const float eps = hmap_width/100.0f;

	float y_x1 = hmap(x + eps, z);
	float y_x2 = hmap(x - eps, z);

	vec3 grad_x = vec3(2 * eps, y_x1 - y_x2, 0);

	float y_z1 = hmap(x, z + eps);
	float y_z2 = hmap(x, z - eps);

	vec3 grad_z = vec3(0, y_z1 - y_z2, 2 * eps);

	return -normalize(cross(grad_x, grad_z));
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
	tmin = max(tmin, 0.0);

	// Find when y = 0
	float t5 = (0 - r.p.y) / r.d.y;
	if (tmax < 0.0f || tmin < 0.0f)
		return -1.0f;

	// Brute search for the intersection
	vec3 p = r.p + r.d * tmin;
	float y = hmap(p.x, p.z);

	if (y < p.y) {
		float dt = 0.1f;

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


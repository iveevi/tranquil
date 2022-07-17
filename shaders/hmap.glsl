// Ray-qadratic bezier intersection
Intersection intersect(Ray r, QuadraticBezier q)
{
	float t = _intersect_time(r, q);
	if (t < 0.0)
		return def_it();

	Intersection it;
	it.id = primitives + 2;
	it.p = r.p + r.d * t;
	it.shading = eGrassBlade;
	it.t = t;
	it.Kd = vec3(0.4, 0.9, 0.4);

	vec3 e1 = q.v2 - q.v1;
	vec3 e2 = q.v3 - q.v1;

	vec3 n = normalize(cross(e1, e2));
	/* if (dot(n, r.d) > 0.0)
		n = -n; */
	it.n = n;

	return it;
}

// Height value at xz
float hmap(float x, float z)
{
	vec2 uv1 = terrain_uv(vec2(x, z));

	float h = scale * texture(s_heightmap, uv1).r;
	if (grass == 1) {
		vec2 uv2 = terrain_uv(vec2(x, z) + wind_offset);
		float g = texture(s_grassmap, uv2).r;
		float l = texture(s_grass_length, uv2).r;
		float p = texture(s_grass_power, uv2).r;
		h += 0.2 * l * pow(g, 8 * p);
	}

	return h;
}

float hmap(Ray r, float t)
{
	vec3 p = r.p + r.d * t;
	return hmap(p.x, p.z);
}

vec3 hmap_normal(float x, float z)
{
	vec2 uv = terrain_uv(vec2(x, z));
	vec3 nh = normalize(texture(s_heightmap_normal, uv).xyz * 2.0 - 1.0);

	if (grass == 1) {
		float k = 0.1;
		vec3 ng = normalize(texture(s_grassmap_normal, uv).xyz * 2.0 - 1.0);
		return normalize((1 - k) * nh + k * ng);
	}

	return nh;
}

// Grass density
float gmap(float x, float z)
{
	vec2 uv = terrain_uv(vec2(x, z));
	return texture(s_grassmap, uv).r;
}

// Grass length
float glen(float x, float z)
{
	vec2 uv = terrain_uv(vec2(x, z));
	return texture(s_grass_length, uv).r;
}

// Gradient at xz
vec2 glen_grad(float x, float z)
{
	const float eps = 0.01;

	float y_x1 = glen(x + eps, z);
	float y_x2 = glen(x - eps, z);

	float y_z1 = glen(x, z + eps);
	float y_z2 = glen(x, z - eps);

	float dy_dx = (y_x1 - y_x2) / (2 * eps);
	float dy_dz = (y_z1 - y_z2) / (2 * eps);

	return vec2(dy_dx, dy_dz);
}

Intersection intersect_heightmap(Ray r)
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
		return def_it();

	// Brute search for the intersection
	vec3 p = r.p + r.d * tmin;
	float y = hmap(p.x, p.z);

	if (y < p.y) {
		float dt = ray_marching_step;

		float lh = 0.0f;
		float ly = 0.0f;

		for (float t = tmin; t < tmax; t += dt) {
			p = r.p + r.d * t;
			y = hmap(p.x, p.z);

			/* See if it intersects grass blade
			vec2 uv = terrain_uv(vec2(p.x, p.z));

			float g = texture(s_grassmap, uv).r;
			float l = texture(s_grass_length, uv).r;
			float pw = texture(s_grass_power, uv).r;
			l = min(pow(l, 8 * pw), 1);

			if (g > 0.6 && l > 0.1) {
				vec2 wo = wind_offset/0.2f;
				QuadraticBezier grass = QuadraticBezier(
					vec3(p.x, y, p.z),
					vec3(p.x, y + l/2, p.z),
					vec3(p.x + wo.x, y + l, p.z + wo.y)
				);

				Intersection gi = intersect(r, grass);
				if (gi.id != -1)
					return gi;
			} */

			if (y >= p.y) {
				// Interpolate distance
				t += dt * (lh - ly)/(p.y - ly - y + lh) - dt;

				Intersection it;
				it.id = primitives;
				it.t = t;
				it.p = r.p + r.d * t;
				it.n = hmap_normal(p.x, p.z);
				it.shading = eGrass;

				it.Kd = vec3(0.5, 1, 0.5);
				if (grass_length == 1) {
					vec2 uv = terrain_uv(vec2(p.x, p.z));
					it.Kd.rgb = vec3(texture(s_grass_length, uv).r);
				} else if (grass_power == 1) {
					vec2 uv = terrain_uv(vec2(p.x, p.z));
					it.Kd.rgb = vec3(texture(s_grass_power, uv).r);
				} else if (grass_density == 1) {
					vec2 uv = terrain_uv(vec2(p.x, p.z));
					it.Kd.rgb = vec3(texture(s_grassmap, uv).r);
				}

				return it;
			}

			ly = p.y;
			lh = y;
		}
	} else {
		float dt = ray_marching_step;

		float lh = 0.0f;
		float ly = 0.0f;

		for (float t = tmin; t < tmax; t += dt) {
			p = r.p + r.d * t;
			y = hmap(p.x, p.z);
			if (y < p.y) {
				// Interpolate distance
				t += dt * (lh - ly)/(p.y - ly - y + lh) - dt;

				Intersection it;
				it.id = primitives;
				it.t = t;
				it.p = r.p + r.d * t;
				it.n = -hmap_normal(p.x, p.z);
				it.shading = eGrass;

				it.Kd = vec3(0.5, 1, 0.5);
				if (grass_length == 1) {
					vec2 uv = terrain_uv(vec2(p.x, p.z));
					it.Kd.rgb = vec3(texture(s_grass_length, uv).r);
				} else if (grass_power == 1) {
					vec2 uv = terrain_uv(vec2(p.x, p.z));
					it.Kd.rgb = vec3(texture(s_grass_power, uv).r);
				} else if (grass_density == 1) {
					vec2 uv = terrain_uv(vec2(p.x, p.z));
					it.Kd.rgb = vec3(texture(s_grassmap, uv).r);
				}

				return it;
			}

			ly = p.y;
			lh = y;
		}
	}

	// Everything under is not visible
	return def_it();
}

/* Intersection intersect_heightmap(Ray r)
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
} */

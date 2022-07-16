// Height value at xz
float hmap(float x, float z)
{
	vec2 uv = terrain_uv(vec2(x, z));
	float h = scale * texture(s_heightmap, uv).r;
	float g = texture(s_grassmap, uv).r;
	float l = texture(s_grass_length, uv).r;
	float p = texture(s_grass_power, uv).r;
	if (grass == 1)
		h += l * pow(g, 4 * p);
	return h;
}

float gmap(float x, float z)
{
	vec2 uv = terrain_uv(vec2(x, z));
	return scale * texture(s_grassmap, uv).r;
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
	vec3 ng = normalize(texture(s_grassmap_normal, uv).xyz * 2.0 - 1.0);

	if (grass == 1) {
		float k = 0.1;
		return normalize((1 - k) * nh + k * ng);
	}

	return nh;
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

// Ray-triangle intersection
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
	return Intersection(t, p, normalize(n), 0, vec3(0), shading);
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

	return _intersect(r, t, tri.w);
}

// Ray-quad intersection
Intersection intersect(Ray r, Quad q)
{
	float t = _intersect_time(r, q);
	if (t < 0.0)
		return def_it();

	Intersection it;
	it.id = primitives + 1;
	it.p = r.p + r.d * t;
	it.shading = eGrass;
	it.t = t;

	vec2 uv = quad_uv(q, it.p);
	vec4 g = texture(s_grass, uv);
	if (g.a == 0.0)
		return def_it();

	it.Kd = g.rgb;

	vec3 e1 = q.v2 - q.v1;
	vec3 e2 = q.v3 - q.v1;

	vec3 n = cross(e1, e2);
	if (dot(n, r.d) > 0.0)
		n = -n;

	it.n = normalize(n);

	return it;
}

// RNG
uvec3 pcg3d(uvec3 v)
{
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	return v;
}

vec3 rand3f(vec3 f)
{
	return uintBitsToFloat((pcg3d(floatBitsToUint(f)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
}

float randf(vec3 f)
{
	return rand3f(f).x;
}

float nrandf(vec3 f)
{
	return 2 * randf(f) - 1;
}

// Whole scene intersection
// TODO: bool porameter for shadowing or not
Intersection trace(Ray ray, bool shadow)
{
	// "Min" intersection
	Intersection mini = def_it();

	// TODO: option for self-shadowing
	if (!shadow)
		mini = intersect_heightmap(ray);

	// Grass (sprinkled evenly for now)
	float dx = 2.0f;
	float dz = 2.0f;

	// TODO: different number per region
	int blades_per_region = 25;
	int max_iterations = 2 * blades_per_region;

	// Wind affects blades more than grass mush
	// TODO: function for this whole thing
	// TODO: utilize a max mipmap level for grass
	vec2 wo = wind_offset * 5.0f;
	for (float x = xmin; x < xmax; x += dx) {
		for (float z = zmin; z < zmax; z += dz) {
			// TOOD: skip region is max grass density is too low
			float y = hmap(x, z);

			// Bounding box for region
			// x -> [x, x + dx] and z -> [z, z + dz]
			// TODO: more optimized and nice function
			vec3 pmin = vec3(x, y, z);
			vec3 pmax = vec3(x + dx, y + 1.0, z + dz);

			BoundingBox bbox = BoundingBox(pmin, pmax);

			// Skip entire region if it's not visible
			if (intersect_box(ray, bbox) < 0.0f)
				continue;

			// Now generate grass in the region
			//	gradient ascent towards higher density
			float xp = x + dx/2.0f;
			float zp = z + dz/2.0f;

			int i = 0;
			int true_count = 0;

			while (i < blades_per_region && (true_count++) < max_iterations) {
				float y = hmap(xp, zp);
				vec2 uv = terrain_uv(vec2(xp, zp));

				float l = texture(s_grass_length, uv).r;
				if (l < 0.3)
					continue;

				l *= 0.8f;
				QuadraticBezier qb = QuadraticBezier(
					vec3(xp, y, zp),
					vec3(xp, y + l/2, zp),
					vec3(xp + wo.x, y + l, zp + wo.y)
				);

				Intersection it = intersect(ray, qb);
				if (it.id != -1 && it.t < mini.t)
					mini = it;

				/* xp = xp + randf(vec3(xp, zp, 0.0f));
				zp = zp + randf(vec3(xp, zp, 0.0f)); */

				vec2 grad = glen_grad(xp, zp);
				float ddx = grad.x + nrandf(vec3(xp, zp, 0.0f)) * 0.1f;
				float ddz = grad.y + nrandf(vec3(xp, zp, 0.0f)) * 0.1f;

				xp += ddx;
				zp += ddz;

				xp = x + mod(x - xp, dx);
				zp = z + mod(z - zp, dz);

				i++;
			}
		}
	}

	/* Quads beziers
	QuadraticBezier qb = QuadraticBezier(
		vec3(0, 0, 0),
		vec3(3, 4, 0),
		vec3(5, 5, 0)
	);

	Intersection qi = intersect(ray, qb);
	// return qi;

	// TODO: min function
	if (qi.id != -1 && qi.t < mini.t)
		mini = qi; */

	if (primitives == 0)
		return mini;

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

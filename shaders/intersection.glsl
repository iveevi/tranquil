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

	return  _intersect(r, t, tri.w);
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

// Whole scene intersection
Intersection trace(Ray ray)
{
	// "Min" intersection
	Intersection mini = intersect_heightmap(ray);

	// Quads
	Quad q = Quad(
		vec3(0, 0, 0),
		vec3(5, 0, 0),
		vec3(5, 5, 0),
		vec3(0, 5, 0)
	);

	Intersection qi = intersect(ray, q);

	// TODO: min function
	if (qi.id != -1 && qi.t < mini.t)
		mini = qi;

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

// Scene intersection for shadows
// 	omits the terrain, by assumption
// 	that is it "flat" enough to not
// 	self shadow
Intersection shadow_trace(Ray ray)
{
	// "Min" intersection
	Intersection mini = def_it();

	// Quads
	Quad q = Quad(
		vec3(0, 0, 0),
		vec3(5, 0, 0),
		vec3(5, 5, 0),
		vec3(0, 5, 0)
	);

	Intersection qi = intersect(ray, q);

	// TODO: min function
	if (qi.id != -1 && qi.t < mini.t)
		mini = qi;

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


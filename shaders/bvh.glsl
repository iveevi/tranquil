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


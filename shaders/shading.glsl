void get_base(uint shading, out vec3 Kd, out vec3 Ks, inout bool valid)
{
	if (shading == eGrass) {
		Kd = vec3(0.5, 1, 0.5);
		Ks = vec3(0, 0, 0);
	} else if (shading == ePillar) {
		Kd = vec3(0.5, 0.5, 0.5);
		Ks = vec3(0.5, 0.5, 0.5);
	} else {
		valid = false;
		Kd = vec3(1, 0, 1);
	}
}

float vec_min(vec3 v)
{
	return min(min(v.x, v.y), v.z);
}

float vec_max(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}

float quantize(float v, float b)
{
	return floor(v * b) / b;
}

vec4 shade(Intersection it)
{
	bool valid = true;

	vec3 Kd = vec3(0, 0, 0);
	vec3 Ks = vec3(0, 0, 0);

	get_base(it.shading, Kd, Ks, valid);
	if (!valid)
		return vec4(Kd, 1);

	// Directional light
	vec3 light = light_dir;
	vec3 normal = normalize(it.n);
	vec3 view = normalize(camera.origin - it.p);
	vec3 whalf = normalize(light + view);

	float ndotl = max(dot(normal, light), 0.0);
	float ndoth = max(dot(normal, whalf), 0.0);
	float pf = pow(ndoth, 32.0);

	vec3 diffuse = Kd * ndotl;
	vec3 specular = 0.1 * Ks  * pf;
	vec3 ambient = Kd * 0.25f;
	vec3 color = ambient;

	// Check if light is visible
	Ray shadow_ray = Ray(it.p + it.n * 0.1f, light_dir);

	Intersection shadow_it = trace(shadow_ray);

	vec3 ds = diffuse;

	float cloud_density = 0.0f;
	vec3 p = it.p;
	if (p.x > xmin && p.x < xmax && p.z > zmin && p.z < zmax) {
		vec2 uv = (vec2(p.x, p.z) - vec2(xmin, zmin)) / vec2(hmap_width, hmap_height);
		cloud_density = texture(s_clouds, uv).r;
	}

	float kcloud = 1.0f;
	if (clouds == 1)
		kcloud = max((1 - cloud_density), 0.1);

	if (shadow_it.id == -1)
		color += light_intensity * ds * kcloud;
	else
		color += 0.1f * ds * kcloud;

	return vec4(color, 1);
}

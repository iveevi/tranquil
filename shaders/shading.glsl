// TODO: function for color palette-ing

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
	vec3 Kd = it.Kd;
	if (grass_length == 1 || grass_power == 1)
		return vec4(Kd, 1.0);

	// Directional light
	vec3 light = light_dir;
	vec3 normal = normalize(it.n);
	vec3 view = normalize(camera.origin - it.p);
	vec3 whalf = normalize(light + view);

	float ndotl = max(dot(normal, light), 0.0);
	vec3 diffuse = Kd * ndotl;
	vec3 ambient = Kd * 0.25f;
	vec3 color = ambient;

	// Check if light is visible
	Ray shadow_ray = Ray(it.p + it.n * ray_shadow_step, light_dir);

	Intersection shadow_it = shadow_trace(shadow_ray);

	vec3 ds = diffuse;

	float cloud_density = 0.0f;
	vec3 p = it.p;
	if (p.x > xmin && p.x < xmax && p.z > zmin && p.z < zmax) {
		vec2 uv = terrain_uv(vec2(p.x, p.z));
		cloud_density = texture(s_clouds, uv).r;
	}

	float kcloud = 1.0f;
	if (clouds == 1)
		kcloud = max((1 - cloud_density), 0.1);

	if (shadow_it.id == -1)
		color += light_intensity * ds * kcloud;
	else {
		color += 0.1f * ds * kcloud;
		// color = vec3(1, 0, 1);
	}

	return vec4(color, 1);
}

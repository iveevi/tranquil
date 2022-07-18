// TODO: function for color palette-ing

float vec_min(vec3 v)
{
	return min(min(v.x, v.y), v.z);
}

float vec_max(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}

vec3 shade(Intersection it)
{
	vec3 Kd = it.Kd;

	// TODO: helper function
	if (grass_length == 1 || grass_power == 1 || grass_density == 1)
		return Kd;

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

	Intersection shadow_it = trace(shadow_ray, true);

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

	if (it.shading == eWater) {
		vec3 c = light_intensity * Kd * kcloud;
		if (shadow_it.id != -1)
			c *= 0.1f;
	}

	if (shadow_it.id == -1)
		color += light_intensity * ds * kcloud;
	else {
		vec3 k = 0.1f * light_intensity;

		// If the shadow hit was grass, then add little bit back
		if (shadow_it.shading == eGrass) {
			// TODO: also do a second shadow test, since self
			// shadowing is smol
			k *= 7.0f;
		}

		color += k * ds * kcloud;
		// color = vec3(1, 0, 1);
	}

	return color;
}

vec4 shade(Intersection it, Ray r)
{
	// In case of water
	if (it.shading == eWater) {
		// Reflection ray
		vec3 r = reflect(r.d, it.n);
		Ray refl_ray = Ray(it.p + it.n * ray_shadow_step, r);

		vec3 refl_color = vec3(0.5, 0.5, 0.5);

		Intersection refl_it = trace(refl_ray, false);
		if (refl_it.id != -1)
			refl_color = shade(refl_it);

		// Refraction ray
		vec3 t = refract(r, it.n, 1.0f / 1.33f);
		Ray refr_ray = Ray(it.p - it.n * ray_shadow_step, t);

		vec3 refr_color = vec3(0.5, 0.5, 0.5);

		Intersection refr_it = trace(refr_ray, false);
		if (refr_it.id != -1)
			refr_color = shade(refr_it);

		// TODO: mix based on skim angle/fresnel
		vec3 color = vec3(0.7, 0.7, 1) * mix(refl_color, refr_color, 0.5);
		return vec4(color, 1.0f);
	}

	return vec4(shade(it), 1.0f);
}

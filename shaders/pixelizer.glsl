#version 430

// Modules
#include <inputs.glsl>
#include <constants.glsl>
#include <structs.glsl>
#include <bvh.glsl>
#include <hmap.glsl>
#include <intersection.glsl>
#include <shading.glsl>

void main()
{
	ivec2 coord = pixel * ivec2(gl_GlobalInvocationID.xy);

	vec2 uv = vec2(coord) + vec2(pixel/2.0f);
	uv /= vec2(width, height);

	Ray r = generate_ray(uv);

	// Base color gets brighter as ray and light dir
	float k = max(dot(r.d, light_dir), 0) * 0.5 + 0.5;
	vec4 color = vec4(k, k, 0.8, 1.0);

	// Intersection it = intersect_heightmap(r);
	Intersection it = trace(r);
	if (it.id != -1) {
		color = shade(it);
		// color.xyz = it.n * 0.5 + 0.5;
	}

	// Set same color to all pixels
	for (int i = 0; i < pixel; i++) {
		for (int j = 0; j < pixel; j++) {
			imageStore(image,
				ivec2(coord.x + i, coord.y + j),
				clamp(color, 0.0, 1.0)
			);
		}
	}
}

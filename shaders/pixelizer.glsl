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
	ivec2 coord = pixel * ivec2(gl_GlobalInvocationID.xy) + ivec2(offx, offy);

	vec2 uv = vec2(coord) + vec2(pixel/2.0f);
	uv /= vec2(width, height);

	Ray r = generate_ray(uv);

	// Base color gets brighter as ray and light dir
	float k = pow(max(dot(r.d, light_dir), 0), 8) * 0.5 + 0.5;
	// vec4 color = vec4(k, k, 0.7, 1.0);
	vec4 color = vec4(0.5, 0.5, 0.7, 1.0);

	// Intersection it = intersect_heightmap(r);
	Intersection it = trace(r, false);

	if (normals == 1) {
		color = vec4(0, 0, 0, 1.0);
		if (it.id != -1)
			color.xyz = it.n * 0.5 + 0.5;
	} else {
		if (it.id != -1) {
			color = shade(it);
			// color.xyz = it.n * 0.5 + 0.5;
		} else {
			// Possibility of clouds (TODO: shade clouds)
			if (clouds == 1) {	
				// Solve for ray pos at 20
				float h = 7.0f;
				float t = (h - r.p.y) / r.d.y;
				vec3 pos = r.p + t * r.d;

				float x = pos.x;
				float z = pos.z;

				if (t > 0 && x > xmin && x < xmax && z > zmin && z < zmax) {
					// TODO: function to get terrain uv coordinate
					vec2 uv = terrain_uv(vec2(x, z));
					float cloud = texture(s_clouds, uv).x;

					if (cloud > 0.2f) {
						vec4 c = vec4(0.3, 0.3, 0.3, 1.0);
						color = mix(color, c, cloud);
					}
				}
			}
		}
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

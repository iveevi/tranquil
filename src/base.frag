#version 450

in vec3 fpos;

uniform sampler2D heightmap;

out vec4 fragment;

void main()
{
	float l = length(fpos);
	fragment = vec4(1.0/(l * l), 1, 0.0, 1.0);
	fragment.xyz = vec3(texture(heightmap, fpos.xz/10).r);
}

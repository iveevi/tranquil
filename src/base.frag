#version 450

in vec3 fpos;

out vec4 fragment;

void main()
{
	float l = length(fpos);
	fragment = vec4(1.0/(l * l), 1, 0.0, 1.0);
}

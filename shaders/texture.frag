#version 430

layout (location = 0) in vec2 in_coord;
layout (location = 0) out vec4 out_color;

uniform sampler2D in_sampler;

void main()
{
	out_color = texture(in_sampler, in_coord);
}

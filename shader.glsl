#version 430

layout (local_size_x = 400, local_size_y = 1) in;

layout (rgba32f, binding = 0) uniform image2D image;


uniform int width;
uniform int height;
uniform int pixel;

void main()
{
	ivec2 coord = pixel * ivec2(gl_GlobalInvocationID.xy);

	vec2 uv = vec2(coord) / vec2(width, height);
	vec4 color = vec4(uv, 0, 1);
	
	// Set same color to all pixels
	for (int i = 0; i < pixel; i++)
	{
		for (int j = 0; j < pixel; j++)
		{
			imageStore(image, ivec2(coord.x + i, coord.y + j), color);
		}
	}
}

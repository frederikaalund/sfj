#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform ivec2 window_dimensions;
uniform int ldm_view_count;


struct ldm_data {
	uint32_t next;
	float depth;
};
writeonly restrict layout (std430) buffer data_buffer
{ ldm_data data[]; };

writeonly restrict layout (std430) buffer debug_view_buffer
{ uint32_t debug_view[]; };

struct color_data {
	uint32_t r, g, b;
};


struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;
layout(pixel_center_integer) in uvec2 gl_FragCoord;

void main()
{
	uint32_t index = gl_FragCoord.x + gl_FragCoord.y * window_dimensions.x;
	
	for (int i = 0; i < ldm_view_count; ++i)
		data[i*window_dimensions.x*window_dimensions.y + index].next = 0;

	debug_view[index] = 0;
}

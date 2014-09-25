#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform ivec2 window_dimensions;

layout(binding = 0, offset = 0) uniform atomic_uint count;

layout (std430) buffer fragment_count_buffer
{
	uint32_t fragment_count[];
};

layout (std430) buffer data_buffer
{
	uint32_t data_storage[];
};

struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;
layout(pixel_center_integer) in uvec2 gl_FragCoord;

void main()
{
	atomicCounterIncrement(count);
	uint32_t index = gl_FragCoord.x + gl_FragCoord.y * window_dimensions.x;
	fragment_count[index] = 0;
	data_storage[index] = 0;
}

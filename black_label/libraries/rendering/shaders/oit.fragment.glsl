#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform ivec2 window_dimensions;



struct oit_data {
	uint32_t next, compressed_diffuse;
	float depth;
};

layout(binding = 0, offset = 0) uniform atomic_uint count;
layout (std430) buffer data_buffer
{ oit_data data[]; };
layout (std430) buffer head_buffer
{ uint32_t heads[]; };



struct vertex_data
{ float negative_ec_position_z; };
in vertex_data vertex;
layout(pixel_center_integer) in uvec2 gl_FragCoord;



uint32_t allocate() { return atomicCounterIncrement(count); }



void main()
{
	uint32_t compressed_diffuse = 0xFFFFFF3F;

	// Calculate indices
	uint32_t head_index = gl_FragCoord.x + gl_FragCoord.y * window_dimensions.x;
	uint32_t data_index = allocate();

	// Store fragment data in node
	data[data_index].compressed_diffuse = compressed_diffuse;
	data[data_index].depth = vertex.negative_ec_position_z;
	
	// Update head index
	uint32_t old_head = atomicExchange(heads[head_index], data_index);
	// Store next index
	data[data_index].next = old_head;
}
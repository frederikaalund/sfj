#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform float specular_exponent;
uniform ivec2 window_dimensions;
uniform float z_far, z_near;



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
{
	vec3 wc_normal;
	vec2 oc_texture_coordinate;
	float normalized_ec_position_z;
};
in vertex_data vertex;
layout(pixel_center_integer) in uvec2 gl_FragCoord;

layout(location = 0) out vec3 wc_normal;
layout(location = 1) out vec4 albedo;



uint32_t allocate() {
	return atomicCounterIncrement(count);
}

uint32_t compress( vec4 value ) {
	float opacity = 0.5;
	return (uint32_t(value.x*255.0) << 24u) + (uint32_t(value.y*255.0) << 16u) + (uint32_t(value.z*255.0) << 8u) + (uint32_t(opacity*255.0));
}



void main()
{
	vec2 tc_texture_coordinates = vec2(vertex.oc_texture_coordinate.x, 1.0 - vertex.oc_texture_coordinate.y);

	vec4 diffuse = texture(diffuse_texture, tc_texture_coordinates);

	if (diffuse.a <= 1.0e-5) discard;

	// TODO: Do this correctly.
	float specular_exponent_ = specular_exponent * texture(specular_texture, tc_texture_coordinates).r;

	wc_normal = normalize(vertex.wc_normal);
	albedo.rgb = diffuse.rgb;
	albedo.a = specular_exponent_;




	// A-buffer
#define MAX_DEPTH (1u<<24u)
	uint32_t compressed_diffuse = compress(diffuse);

	// Calculate indices
	uint32_t head_index = gl_FragCoord.x + gl_FragCoord.y * window_dimensions.x;
	uint32_t data_index = allocate();

	// Store fragment data in node
	data[data_index].compressed_diffuse = compressed_diffuse;
	data[data_index].depth = vertex.normalized_ec_position_z;
	
	// Update head index
	uint32_t old_head = atomicExchange(heads[head_index], data_index);
	// Store next index
	data[data_index].next = old_head;
}
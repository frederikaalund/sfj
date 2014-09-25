#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform float specular_exponent;
uniform ivec2 window_dimensions;
uniform float z_far, z_near;

layout (std430) buffer fragment_count_buffer
{
	uint32_t fragment_count[];
};

layout (std430) buffer data_buffer
{
	uint32_t data_storage[];
};

layout(binding = 0, offset = 0) uniform atomic_uint count;



struct vertex_data
{
	vec3 wc_normal;
	vec2 oc_texture_coordinate;
	float ec_position_z;
};
in vertex_data vertex;
layout(pixel_center_integer) in uvec2 gl_FragCoord;

layout(location = 0) out vec3 wc_normal;
layout(location = 1) out vec4 albedo;





uint32_t compress( vec4 value ) {
	float opacity = 0.5;
	return (uint32_t(value.x*255.0) << 24u) + (uint32_t(value.y*255.0) << 16u) + (uint32_t(value.z*255.0) << 8u) + (uint32_t(opacity*255.0));
}




#define MAX_ITER 200

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
	uint32_t data = compress(diffuse);
	uint32_t depth = uint32_t((vertex.ec_position_z * MAX_DEPTH));

	uint32_t index = gl_FragCoord.x + gl_FragCoord.y * window_dimensions.x;
	uint32_t ptr = index * fragment_count[index];

	uint32_t count = atomicAdd(data_storage[index], 1);
	ptr += count;

	data_storage[window_dimensions.x * window_dimensions.y + ptr * 2] = depth;
	data_storage[window_dimensions.x * window_dimensions.y + ptr * 2 + 1] = data;
}
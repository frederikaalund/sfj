#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform samplerBuffer lights;
uniform ivec2 window_dimensions;
//uniform mat4 view_projection_matrix;



struct view_type {
	mat4 view_matrix, projection_matrix, view_projection_matrix;
	vec4 eye;
	vec4 right, forward, up;
	ivec2 dimensions;
};

layout(std140) uniform user_view_block
{ view_type user_view; };

uniform int ldm_view_count;

readonly restrict layout(std430) buffer view_block
{ view_type views[]; };

readonly restrict layout(std430) buffer data_offset_block
{ uvec4 data_offsets[]; };



struct ldm_data {
	uint32_t next, compressed_diffuse;
	float depth;
};
readonly restrict layout (std430) buffer data_buffer
{ ldm_data data[]; };
writeonly restrict layout (std430) buffer debug_view_buffer
{ uint32_t debug_view[]; };




void draw_layered_depth_map( 
	in vec2 ndc_position, 
	in view_type view, 
	in uint32_t data_offset,
	in uint32_t id )
{
	uint32_t heads_index = data_offset + uint32_t(gl_FragCoord.x) + uint32_t(gl_FragCoord.y) * window_dimensions.x;
	uint32_t current = data[heads_index].next;

	// Constants
	const int max_list_length = 200;

	// Loop over each point
	int list_length = 0;
	while (0 != current && list_length < max_list_length) {
		float depth = data[current].depth;
		current = data[current].next;
		list_length++;

		vec3 wc_light_position = view.eye.xyz;
		vec3 right = view.right.xyz;
		vec3 forward = view.forward.xyz;
		vec3 up = view.up.xyz;

		// Othographic
		const float right_scale = 1000.0;
		const float top_scale = 1000.0;

		vec3 direction = (
			forward * (depth)
			+ right * right_scale * ndc_position.x
			+ up * top_scale * ndc_position.y);
		vec3 wc_sample_position = wc_light_position + direction;

		// Project into user view
		vec4 cc_sample_position = user_view.view_projection_matrix * vec4(wc_sample_position, 1.0);
		if (cc_sample_position.x > cc_sample_position.w || cc_sample_position.x < -cc_sample_position.w
			|| cc_sample_position.y > cc_sample_position.w || cc_sample_position.y < -cc_sample_position.w
			|| cc_sample_position.z > cc_sample_position.w || cc_sample_position.z < -cc_sample_position.w)
			continue;
		vec3 ndc_sample_position = cc_sample_position.xyz / cc_sample_position.w;
		vec2 tc_sample_position = (ndc_sample_position.xy + vec2(1.0)) * 0.5;
		uvec2 vc_sample_position = uvec2(tc_sample_position * user_view.dimensions);

		// Write to buffer (as if it was a texture)
		uint32_t index = vc_sample_position.x + vc_sample_position.y * user_view.dimensions.x;
		debug_view[index] = id;
	}
}



void main() {
	vec2 tc_position = vec2(gl_FragCoord) / window_dimensions;
	vec2 ndc_position = tc_position * 2.0 - vec2(1.0);

	for (int i = 0; i < ldm_view_count; ++i)
		draw_layered_depth_map(ndc_position, views[i], data_offsets[i][0], i + 1);
}

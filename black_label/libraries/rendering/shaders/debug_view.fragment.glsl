#extension GL_NV_shader_buffer_load: enable
#extension GL_NV_gpu_shader5: enable
#extension GL_EXT_shader_image_load_store: enable

uniform samplerBuffer lights;
uniform ivec2 window_dimensions;
uniform mat4 view_projection_matrix;



struct light_type {
	mat4 projection_matrix, view_projection_matrix;
	vec4 wc_direction;
	float radius;
};

layout(std140) uniform light_block
{ light_type light; };



struct oit_data {
	uint32_t next, compressed_diffuse;
	float depth;
};
layout (std430) buffer data_buffer
{ oit_data data[]; };
layout (std430) buffer debug_view_buffer
{ uint32_t debug_view[]; };



void main()
{
	uint32_t heads_index = uint32_t(gl_FragCoord.x - 0.5) + uint32_t(gl_FragCoord.y - 0.5) * window_dimensions.x;
	uint32_t current = data[heads_index].next;

	// Constants
	const int max_list_length = 200;

	// Loop over each point
	int list_length = 0;
	while (0 != current && list_length < max_list_length) {
		float depth = data[current].depth;
		current = data[current].next;
		list_length++;

		vec2 tc_window = vec2(gl_FragCoord) / window_dimensions;
		vec2 ndc_window = tc_window * 2.0 - vec2(1.0);

		vec3 wc_light_position = vec3(-1405.12451, 673.861877, -110.380501);
		vec3 right = vec3(0.00579266157, -0.000000000, 0.999983311);
		vec3 forward = vec3(0.987778485, -0.155759439, -0.00572196208);
		vec3 up = vec3(0.155756846, 0.987795115, -0.000902261701);
		float fovy = 0.785398185;
		float light_z_near = 10.0;

		right *= tan(fovy * 0.5); // Assuming fovy = fovx
		up *= tan(fovy * 0.5);

		vec3 direction = (forward + right * ndc_window.x + up * ndc_window.y) * light_z_near;


		vec3 wc_sample_position = wc_light_position + normalize(direction) * (depth + length(direction));
		vec4 cc_sample_position = view_projection_matrix * vec4(wc_sample_position, 1.0);
		if (cc_sample_position.x > cc_sample_position.w || cc_sample_position.x < -cc_sample_position.w
			|| cc_sample_position.y > cc_sample_position.w || cc_sample_position.y < -cc_sample_position.w
			|| cc_sample_position.z > cc_sample_position.w || cc_sample_position.z < -cc_sample_position.w)
			continue;
		vec3 ndc_sample_position = cc_sample_position.xyz / cc_sample_position.w;
		vec2 tc_sample_position = (ndc_sample_position.xy + vec2(1.0)) * 0.5;
		uvec2 vc_sampleposition = uvec2(tc_sample_position * window_dimensions);

		uint32_t index = vc_sampleposition.x + vc_sampleposition.y * window_dimensions.x;
		debug_view[index] = 1;
	}
}

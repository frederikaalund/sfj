#extension GL_NV_gpu_shader5: enable

uniform sampler2D depths, wc_positions, wc_normals, light_depths, light_wc_normals, light_albedos;

uniform ivec2 window_dimensions;
uniform mat4 projection_matrix;
uniform float z_far;
uniform vec3 wc_view_eye_position;



struct view_type {
	mat4 view_matrix, projection_matrix, view_projection_matrix;
	vec4 eye;
	vec4 right, forward, up;
	ivec2 dimensions;
};

layout(std140) uniform current_view_block
{ view_type current_view; };

layout(std140) uniform user_view_block
{ view_type user_view; };

layout(std140) uniform view_block
{ view_type views[9]; };

layout(std140) uniform data_offset_block
{ uvec4 data_offsets[9]; };



struct oit_data {
	uint32_t next, compressed_diffuse;
	float depth;
};
readonly restrict layout (std430) buffer data_buffer
{ oit_data data[]; };

struct color_data {
	uint32_t r, g, b;
};
volatile restrict layout (std430) buffer photon_splat_buffer
{ color_data photon_splats[]; };



struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;



uint32_t compress( in vec4 clr )
{ return (uint32_t(clr.x*255.0) << 24u) + (uint32_t(clr.y*255.0) << 16u) + (uint32_t(clr.z*255.0) << 8u) + (uint32_t(0.1*255.0)); } 

float get_tc_z( 
	in sampler2D sampler, 
	in vec2 tc_position )
{ return texture(sampler, tc_position).z; }

float get_ec_z( 
	in float tc_z,
	in mat4 projection_matrix )
{ return projection_matrix[3][2] / (-2.0 * tc_z + 1.0 - projection_matrix[2][2]); }

float get_ec_z( 
	in sampler2D sampler, 
	in vec2 tc_position,
	in mat4 projection_matrix)
{ return get_ec_z(get_tc_z(sampler, tc_position), projection_matrix); }



bool get_user_view_coordinates( in vec3 wc_position, out float ec_position_z, out ivec2 pc_position ) {
	vec4 cc_position = user_view.view_projection_matrix * vec4(wc_position, 1.0);
	if (cc_position.x > cc_position.w || cc_position.x < -cc_position.w
		|| cc_position.y > cc_position.w || cc_position.y < -cc_position.w
		|| cc_position.z > cc_position.w || cc_position.z < -cc_position.w)
		return false;
	vec3 ndc_position = cc_position.xyz / cc_position.w;
	vec2 tc_position = (ndc_position.xy + vec2(1.0)) * 0.5;
	ec_position_z = get_ec_z(depths, tc_position, user_view.projection_matrix);
	pc_position = ivec2(tc_position * user_view.dimensions);
	return true;
}

void splat( in uvec2 pc_position, in vec4 color ) {
	// Write to buffer (as if it was a texture)
	uint32_t index = pc_position.x + pc_position.y * user_view.dimensions.x;
	atomicAdd(photon_splats[index].r, uint(color.r * 255.0));
	atomicAdd(photon_splats[index].g, uint(color.g * 255.0));
	atomicAdd(photon_splats[index].b, uint(color.b * 255.0));
}

void splat( in uvec2 pc_bottom_left, in uvec2 pc_top_right, in vec4 color ) {
	for (uint x = pc_bottom_left.x; x < pc_top_right.x; x++)
		for (uint y = pc_bottom_left.y; y < pc_top_right.y; y++)
			splat(uvec2(x, y), color);
}

void splat_square( in ivec2 pc_position, in uint size, in vec4 color ) {
	uvec2 pc_bottom_left = uvec2(max(pc_position.x - size, 0), max(pc_position.y - size, 0));
	uvec2 pc_top_right = uvec2(min(pc_position.x + size, window_dimensions.x), min(pc_position.y + size, window_dimensions.y));

	splat(pc_bottom_left, pc_top_right, color);
}








vec2 get_one_over_tan_half_fov( in mat4 projection_matrix )
{ return vec2(projection_matrix[0][0], projection_matrix[1][1]); }

vec2 get_tc_length( in float ec_length, in float ec_position_z, in mat4 projection_matrix )
{
	vec2 one_over_tan_half_fov = get_one_over_tan_half_fov(projection_matrix);
	return 0.5 * ec_length * one_over_tan_half_fov / -ec_position_z;
}

float K( float x ) {
	if (x >= 1.0) return 0.0;
	return 3.0 / PI * (1.0 - x * x) * (1.0 - x * x);
}

struct vec3_differential {
	vec3 dx, dy;
};

struct ray_differential {
	vec3_differential dP, dD;
};

ray_differential construct_ray_differential( in vec3 d, in vec3 right, in vec3 up ) {
	vec3 dDdx = (dot(d, d) * right - dot(d, right) * d) / pow(dot(d, d), 3.0 / 2.0);
	vec3 dDdy = (dot(d, d) * up    - dot(d, up   ) * d) / pow(dot(d, d), 3.0 / 2.0);
	return ray_differential(
		vec3_differential(vec3(0.0), vec3(0.0)),
		vec3_differential(dDdx, dDdy));
}

void transfer( inout ray_differential ray, in vec3 D, in vec3 N, in float t ) {
	float dtdx = dot((ray.dP.dx + t * ray.dD.dx), N) / dot(D, N);
	float dtdy = dot((ray.dP.dy + t * ray.dD.dy), N) / dot(D, N);
	ray.dP.dx = (ray.dP.dx + t * ray.dD.dx) + dtdx * D;
	ray.dP.dy = (ray.dP.dy + t * ray.dD.dy) + dtdy * D;
}

void splat( 
	in ray_differential ray,
	in vec3 wc_position,
	in vec4 albedo )
{
	const float const_bias = 10.0;
	float ec_position_z_seen_by_user = (user_view.view_matrix * vec4(wc_position, 1.0)).z;

	// Project into user view
	float ec_user_position_z;
	ivec2 pc_user_position;
	if (!get_user_view_coordinates(wc_position, ec_user_position_z, pc_user_position)
		|| ec_user_position_z > ec_position_z_seen_by_user + const_bias)
	{ return; }

	vec3 wc_normal = texture(wc_normals, vec2(pc_user_position) / vec2(user_view.dimensions)).xyz;

	// Area
	float A = PI / 4.0 * length(cross(ray.dP.dx, ray.dP.dy));
	vec4 E = albedo * 0.03 / A;

	float r_max = 0.5 * max(length(ray.dP.dx), length(ray.dP.dy));
	vec2 tc_size = get_tc_length(r_max * 35.0, ec_position_z_seen_by_user, user_view.projection_matrix);
	ivec2 pc_size = ivec2(tc_size * window_dimensions);
	pc_size = min(pc_size, ivec2(10));

	mat3x2 temp = transpose(mat2x3(cross(ray.dP.dy, wc_normal), cross(wc_normal, ray.dP.dx)));
	mat3x2 M = 2 / dot(ray.dP.dx, cross(ray.dP.dy, wc_normal)) * temp;

	ivec2 pc_bottom_left = pc_user_position - pc_size;
	ivec2 pc_top_right = pc_user_position + pc_size;

	// Cap to screen
	pc_bottom_left = ivec2(max(pc_bottom_left.x, 0), max(pc_bottom_left.y, 0));
	pc_top_right = ivec2(min(pc_top_right.x, window_dimensions.x), min(pc_top_right.y, window_dimensions.y));

	for (uint x = pc_bottom_left.x; x < pc_top_right.x; x++)
		for (uint y = pc_bottom_left.y; y < pc_top_right.y; y++) {
			vec2 pc_sample = vec2(x, y);
			vec2 tc_sample = pc_sample / vec2(user_view.dimensions);
			vec3 wc_sample = texture(wc_positions, tc_sample).xyz;
			
			//float d = distance(wc_sample, wc_position);
			float d = length(M * (wc_sample - wc_position));
			float k = K(d / 35.0);
			splat(uvec2(pc_sample), PI * k * E);
		}
}



void splat_first_bounce( in int view_id, in ray_differential ray, in vec3 wc_position, in vec4 albedo) {
	view_type view = views[view_id];
	uint32_t data_offset = data_offsets[view_id];

	// Coordinate transformations
	vec4 cc_position = view.view_projection_matrix * vec4(wc_position, 1.0);
	if (cc_position.x > cc_position.w || cc_position.x < -cc_position.w
		|| cc_position.y > cc_position.w || cc_position.y < -cc_position.w
		|| cc_position.z > cc_position.w || cc_position.z < -cc_position.w)
		return;
	vec3 ndc_position = cc_position.xyz / cc_position.w;
	vec3 tc_position = (ndc_position + vec3(1.0)) * 0.5;
	uvec2 pc_position = uvec2(tc_position.xy * view.dimensions);

	vec3 wc_eye = view.eye.xyz;
	vec3 right = view.right.xyz;
	vec3 forward = view.forward.xyz;
	vec3 up = view.up.xyz;

	// Othographic
	const float right_scale = 1000.0;
	const float top_scale = 1000.0;

	// Get the head node
	uint32_t heads_index = data_offset + pc_position.x + pc_position.y * view.dimensions.x;
	uint32_t current = data[heads_index].next;

	const int max_list_length = 200;
	const float distance_threshold = 999999.0;
	float min_distance = distance_threshold;
	float previous_distance = min_distance;
	float next_distance = min_distance;
	vec3 wc_sample_position, 
		wc_last_sample_position,
		wc_previous,
		wc_next;
	bool get_next = false;

	int list_length = 0;
	while (0 != current && list_length < max_list_length) {
		float depth = data[current].depth;
		vec3 direction = (
			forward * depth 
			+ right * right_scale * ndc_position.x
			+ up * top_scale * ndc_position.y);
		wc_sample_position = wc_eye + direction;

		float sample_distance = distance(wc_sample_position, wc_position);

		if (get_next) {
			get_next = false;
			next_distance = sample_distance;
			wc_next = wc_sample_position;
		}

		if (sample_distance < min_distance) {
			previous_distance = min_distance;
			wc_previous = wc_last_sample_position;
			min_distance = sample_distance;
			get_next = true;
		}

		wc_last_sample_position = wc_sample_position;

		current = data[current].next;
		++list_length;
	}
	if (get_next) {
		wc_next = wc_sample_position;
	}




	const float const_bias = 10.0;
	{
		float ec_position_z_seen_by_user = (user_view.view_matrix * vec4(wc_previous, 1.0)).z;

		// Project into user view
		float ec_user_position_z;
		ivec2 pc_user_position;
		if (get_user_view_coordinates(wc_previous, ec_user_position_z, pc_user_position)
			&& ec_user_position_z < ec_position_z_seen_by_user + const_bias)
		{
			vec3 wc_normal = texture(wc_normals, vec2(pc_user_position) / vec2(user_view.dimensions)).xyz;
			vec3 global_normal = texture(light_wc_normals, gl_FragCoord.xy / window_dimensions).xyz;
			float t = distance(wc_position, wc_previous);
			vec3 D = normalize(-forward);

			transfer(ray, D, wc_normal, t);

			float w = dot(global_normal, D);
			if (0.0 < w)
				splat(ray, wc_previous, albedo * w);
		}
	}
}



void main()
{
	// Deferred parameters
	vec2 tc_window = gl_FragCoord.xy / window_dimensions;
	float ec_position_z = get_ec_z(light_depths, tc_window, projection_matrix);
	vec3 wc_position = wc_view_eye_position + vertex.wc_view_ray_direction * -ec_position_z / z_far;
	vec4 albedo = texture(light_albedos, tc_window);
	vec3 wc_normal = texture(light_wc_normals, tc_window).xyz;

	// Rename for consistency with theory
	vec3 d = vertex.wc_view_ray_direction;
	vec3 right = current_view.right.xyz;
	vec3 up = current_view.up.xyz;

	// Ray differential construction and initial transfer
	ray_differential ray = construct_ray_differential(d, right, up);
	transfer(ray, normalize(d), wc_normal, -ec_position_z);

	for (int i = 0; i < 9; i++)
		splat_first_bounce(i, ray, wc_position, albedo);

	// Direct splat
	//splat(ray, wc_position, albedo);
}
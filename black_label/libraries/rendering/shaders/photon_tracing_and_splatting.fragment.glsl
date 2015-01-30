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

struct color_data {
	uint32_t r, g, b;
};
coherent restrict layout (std430) buffer photon_splat_buffer
{ color_data photon_splats[]; };



struct vertex_data
{
	vec3 wc_view_ray_direction;
};
noperspective in vertex_data vertex;



uint32_t compress( in vec4 clr )
{ return (uint32_t(clr.x*255.0) << 24u) + (uint32_t(clr.y*255.0) << 16u) + (uint32_t(clr.z*255.0) << 8u) + (uint32_t(0.1*255.0)); } 

vec4 decompress(uint32_t rgba)
{ return vec4( float((rgba>>24u)&255u),float((rgba>>16u)&255u),float((rgba>>8u)&255u),float(rgba&255u) ) / 255.0; }

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

bool get_clamped_user_view_coordinates( in vec3 wc_position, out ivec2 pc_position ) {
	vec4 cc_position = user_view.view_projection_matrix * vec4(wc_position, 1.0);

	vec3 ndc_position = cc_position.xyz / cc_position.w;
	vec2 tc_position = (ndc_position.xy + vec2(1.0)) * 0.5;
	tc_position.x = clamp(tc_position.x, 0.0, 1.0);
	tc_position.y = clamp(tc_position.y, 0.0, 1.0);
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

	const float differential_bias = 10.0;
	if (max(max(abs(ray.dP.dx.x), abs(ray.dP.dx.y)), abs(ray.dP.dx.z)) > differential_bias
		|| max(max(abs(ray.dP.dy.x), abs(ray.dP.dy.y)), abs(ray.dP.dy.z)) > differential_bias)
		return;

	const float scale = 30.0;
	ray.dP.dx *= scale;
	ray.dP.dy *= scale;

	vec3 wc_sample1 = wc_position + ( ray.dP.dx + ray.dP.dy) * 0.5 ;
	vec3 wc_sample2 = wc_position + (-ray.dP.dx + ray.dP.dy) * 0.5 ;
	vec3 wc_sample3 = wc_position + ( ray.dP.dx - ray.dP.dy) * 0.5 ;
	vec3 wc_sample4 = wc_position + (-ray.dP.dx - ray.dP.dy) * 0.5 ;

	ivec2 pc_sample1;
	ivec2 pc_sample2;
	ivec2 pc_sample3;
	ivec2 pc_sample4;

	get_clamped_user_view_coordinates(wc_sample1, pc_sample1);
	get_clamped_user_view_coordinates(wc_sample2, pc_sample2);
	get_clamped_user_view_coordinates(wc_sample3, pc_sample3);
	get_clamped_user_view_coordinates(wc_sample4, pc_sample4);

	// if (!get_clamped_user_view_coordinates(wc_sample1, pc_sample1)
	// 	|| !get_clamped_user_view_coordinates(wc_sample2, pc_sample2)
	// 	|| !get_clamped_user_view_coordinates(wc_sample3, pc_sample3)
	// 	|| !get_clamped_user_view_coordinates(wc_sample4, pc_sample4))
	// 	return;


	ivec2 pc_bottom_left = min(min(min(pc_sample1, pc_sample2), pc_sample3), pc_sample4);
	ivec2 pc_top_right = max(max(max(pc_sample1, pc_sample2), pc_sample3), pc_sample4);

	ivec2 size = pc_top_right - pc_bottom_left;
	if (length(size) > 400) return;

/*
	float r_max = 0.5 * max(length(ray.dP.dx), length(ray.dP.dy));
	vec2 tc_size = get_tc_length(r_max * 35.0, ec_position_z_seen_by_user, user_view.projection_matrix);
	ivec2 pc_size = ivec2(tc_size * window_dimensions);
	pc_size = min(pc_size, ivec2(1));
	
	ivec2 pc_bottom_left = pc_user_position - pc_size;
	ivec2 pc_top_right = pc_user_position + pc_size;
*/


	vec3 wc_normal = texture(wc_normals, vec2(pc_user_position) / vec2(user_view.dimensions)).xyz;



	// Area
	float A = PI / 4.0 * length(cross(ray.dP.dx, ray.dP.dy));
	vec4 E = 10.0 * albedo / A;

	const float a = 100.0;
	mat3 temp = transpose(mat3(cross(ray.dP.dy, wc_normal), cross(wc_normal, ray.dP.dx), a * wc_normal));
	mat3 M = 2 / dot(ray.dP.dx, cross(ray.dP.dy, wc_normal)) * temp;



	// Cap to screen
	pc_bottom_left = ivec2(max(pc_bottom_left.x, 0), max(pc_bottom_left.y, 0));
	pc_top_right = ivec2(min(pc_top_right.x, window_dimensions.x), min(pc_top_right.y, window_dimensions.y));

	for (uint x = pc_bottom_left.x; x < pc_top_right.x; x++)
		for (uint y = pc_bottom_left.y; y < pc_top_right.y; y++) {
			vec2 pc_sample = vec2(x, y);
			vec2 tc_sample = pc_sample / vec2(user_view.dimensions);
			vec3 wc_sample = texture(wc_positions, tc_sample).xyz;
			
			float d = length(M * (wc_sample - wc_position));
			float k = K(d);
			splat(uvec2(pc_sample), PI * k * E);
		}
}


vec3 rotate_vector( vec4 quat, vec3 vec )
{ return vec + 2.0 * cross( cross( vec, quat.xyz ) + quat.w * vec, quat.xyz ); }



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
		uint32_t sample_diffuse, last_diffuse, previous_diffuse, next_diffuse;
	bool get_next = false;

	int list_length = 0;
	while (0 != current && list_length < max_list_length) {
		float depth = data[current].depth;
		sample_diffuse = data[current].compressed_diffuse;
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
			next_diffuse = sample_diffuse;
		}

		if (sample_distance < min_distance) {
			previous_distance = min_distance;
			wc_previous = wc_last_sample_position;
			previous_diffuse = last_diffuse;
			min_distance = sample_distance;
			get_next = true;
		}

		wc_last_sample_position = wc_sample_position;
		last_diffuse = sample_diffuse;

		current = data[current].next;
		++list_length;
	}
	if (get_next) {
		wc_next = wc_sample_position;
		next_diffuse = sample_diffuse;
	}

	if (1 >= list_length) return;


	const float const_bias = 10.0;
	vec3 wc_origin_normal = texture(light_wc_normals, gl_FragCoord.xy / window_dimensions).xyz;
	vec3 w_i = normalize(-vertex.wc_view_ray_direction);

	//float w1 = dot(wc_origin_normal, w_i);
	//albedo *= w1;
	albedo *= 0.5;
	{
		float ec_position_z_seen_by_user = (user_view.view_matrix * vec4(wc_previous, 1.0)).z;

		// Project into user view
		float ec_user_position_z;
		ivec2 pc_user_position;
		if (get_user_view_coordinates(wc_previous, ec_user_position_z, pc_user_position)
			&& ec_user_position_z < ec_position_z_seen_by_user + const_bias)
		{	
			vec3 w_o = normalize(-forward);

			vec4 q;
			q.xyz = cross(w_o, w_i);
			q.w = 1.0 + dot(w_o, w_i);
			q = normalize(q);
			ray.dD.dx = rotate_vector(q, ray.dD.dx);
			ray.dD.dy = rotate_vector(q, ray.dD.dy);

			vec3 wc_hit_normal = texture(wc_normals, vec2(pc_user_position) / vec2(user_view.dimensions)).xyz;
			float t = distance(wc_position, wc_previous);			

			transfer(ray, w_o, wc_hit_normal, t);

			float w = dot(wc_hit_normal, -w_o);
			if (0.0 < w)
				splat(ray, wc_previous, decompress(previous_diffuse) * albedo * w);
		}
	}
	{
		float ec_position_z_seen_by_user = (user_view.view_matrix * vec4(wc_next, 1.0)).z;

		// Project into user view
		float ec_user_position_z;
		ivec2 pc_user_position;
		if (get_user_view_coordinates(wc_next, ec_user_position_z, pc_user_position)
			&& ec_user_position_z < ec_position_z_seen_by_user + const_bias)
		{
			vec3 w_o = normalize(forward);

			vec4 q;
			q.xyz = cross(w_o, w_i);
			q.w = 1.0 + dot(w_o, w_i);
			q = normalize(q);
			ray.dD.dx = rotate_vector(q, ray.dD.dx);
			ray.dD.dy = rotate_vector(q, ray.dD.dy);

			vec3 wc_hit_normal = texture(wc_normals, vec2(pc_user_position) / vec2(user_view.dimensions)).xyz;
			
			float t = distance(wc_position, wc_next);			

			transfer(ray, w_o, wc_hit_normal, t);

			float w = dot(wc_hit_normal, -w_o);
			if (0.0 < w)
				splat(ray, wc_next, decompress(next_diffuse) * albedo * w);
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

	const int num_views = ldm_view_count;
	for (int i = 0; i < num_views; i++)
		splat_first_bounce(i, ray, wc_position, albedo);

	// Direct splat
	float f = max(dot(wc_normal, normalize(-vertex.wc_view_ray_direction)), 0.0);
	//float falloff = 100000000.0 / (length(vertex.wc_view_ray_direction) * length(vertex.wc_view_ray_direction));
	splat(ray, wc_position, albedo * f);
}
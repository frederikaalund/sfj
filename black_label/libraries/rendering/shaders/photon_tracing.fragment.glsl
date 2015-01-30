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
	uint32_t next;
	float depth;
};
readonly restrict layout(std430) buffer data_buffer
{ ldm_data data[]; };



layout(binding = 1, offset = 0) uniform atomic_uint photon_count;

struct photon_data {
	vec4 wc_position, wc_normal, Du_x, Dv_x, radiant_flux;
};
coherent restrict layout(std430) buffer photon_buffer
{ photon_data photons[]; };




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

struct photon_differential
{ vec3 Du_x, Dv_x, Du_d, Dv_d; };

photon_differential construct_photon_differential( in vec3 d_hat, in vec3 right, in vec3 up ) {
	vec3 Du_d = (dot(d_hat, d_hat) * right - dot(d_hat, right) * d_hat) / pow(dot(d_hat, d_hat), 3.0 / 2.0);
	vec3 Dv_d = (dot(d_hat, d_hat) * up    - dot(d_hat, up   ) * d_hat) / pow(dot(d_hat, d_hat), 3.0 / 2.0);
	return photon_differential(vec3(0.0), vec3(0.0), Du_d, Dv_d);
}

void transfer( inout photon_differential photon, in vec3 d, in vec3 n, in float t ) {
	float Du_t = -dot((photon.Du_x + t * photon.Du_d), n) / dot(d, n);
	float Dv_t = -dot((photon.Dv_x + t * photon.Dv_d), n) / dot(d, n);

	photon.Du_x = (photon.Du_x + t * photon.Du_d) + Du_t * d;
	photon.Dv_x = (photon.Dv_x + t * photon.Dv_d) + Dv_t * d;
}

vec4 alpha( in vec3 w_i, in vec3 w_o )
{ return normalize(vec4(cross(w_i, w_o), 1.0 + dot(w_i, w_o))); }

// See https://code.google.com/p/kri/wiki/Quaternions for reference
vec3 rotate_vector( vec4 q, vec3 v )
{ return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v); }

void diffusely_reflect( inout photon_differential photon, in vec3 w_i, in vec3 w_o ) {
	vec4 q = alpha(w_i, w_o);
	photon.Du_d = rotate_vector(q, photon.Du_d);
	photon.Dv_d = rotate_vector(q, photon.Dv_d);
}


void store_photon( in vec3 wc_position, in vec3 wc_normal, in photon_differential photon, in vec4 radiant_flux ) {
	vec3 abs_x = max(abs(photon.Du_x), abs(photon.Dv_x));
	float max_x = max(max(abs_x.x, abs_x.y), abs_x.z);
	const float photon_footprint_bias = 0.5;
	if (photon_footprint_bias < max_x) return;

	uint32_t id = atomicCounterIncrement(photon_count);

	photons[id].wc_position = vec4(wc_position, 1.0);
	photons[id].wc_normal = vec4(wc_normal, 0.0);
	photons[id].Du_x = vec4(photon.Du_x, 0.0);
	photons[id].Dv_x = vec4(photon.Dv_x, 0.0);
	photons[id].radiant_flux = vec4(radiant_flux.rgb, 1.0);
}


void store_first_bounce( in photon_differential photon, in vec3 wc_position, in vec3 wc_x, in vec3 wc_normal, in vec3 w_i, in vec3 w_o, in vec4 radiant_flux_and_f ) {
	const float const_bias = 0.1;
	float ec_z_actual = (user_view.view_matrix * vec4(wc_x, 1.0)).z;

	// Project into user view
	float ec_z_seen_by_user;
	ivec2 pc_user_position;
	if (get_user_view_coordinates(wc_x, ec_z_seen_by_user, pc_user_position)
		&& ec_z_seen_by_user < ec_z_actual + const_bias)
	{	
		vec3 wc_hit_normal = texture(wc_normals, vec2(pc_user_position) / vec2(user_view.dimensions)).xyz;
		float t = distance(wc_position, wc_x);			

		diffusely_reflect(photon, w_i, w_o);
		transfer(photon, w_o, wc_hit_normal, t);

		float cos_theta = dot(wc_normal, w_o); // [sr]
		if (0.0 < cos_theta)
			store_photon(wc_x, wc_hit_normal, photon, radiant_flux_and_f * cos_theta);
	}
}

void store_first_bounce_in_both_directions( in int view_id, in photon_differential photon, in vec3 wc_position, in vec3 wc_normal, in vec4 radiant_flux_and_f ) {
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
	const float right_scale = 2000.0;
	const float top_scale = 2000.0;

	// Get the head node
	uint32_t heads_index = data_offset + pc_position.x + pc_position.y * view.dimensions.x;
	uint32_t current = data[heads_index].next;

	const int max_list_length = 2048;
	const float FLOAT_MAX = 999999.0;
	float min_distance = FLOAT_MAX;
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
		} else break;

		wc_last_sample_position = wc_sample_position;

		current = data[current].next;
		++list_length;
	}
	if (get_next) wc_next = vec3(FLOAT_MAX);

	if (1 >= list_length) return;


	vec3 w_i = normalize(-vertex.wc_view_ray_direction);

	store_first_bounce(photon, wc_position, wc_previous, wc_normal, w_i, normalize(-forward), radiant_flux_and_f);
	store_first_bounce(photon, wc_position, wc_next,     wc_normal, w_i, normalize(forward),  radiant_flux_and_f);
}


void main()
{
	// Convert to spot light
	float radius = length(gl_FragCoord.xy / window_dimensions - vec2(0.5));
	if (radius > 0.5) discard;

	// Deferred parameters
	const ivec2 light_view_dimensions = ivec2(100);
	const vec2 window_scale_constant = vec2(light_view_dimensions) / float(user_view.dimensions);
	vec2 tc_window = gl_FragCoord.xy / window_dimensions * window_scale_constant;
	float ec_position_z = get_ec_z(light_depths, tc_window, projection_matrix);
	vec3 wc_position = wc_view_eye_position + vertex.wc_view_ray_direction * -ec_position_z / z_far;
	vec4 rho_d = texture(light_albedos, tc_window); // [1]
	vec3 wc_normal = texture(light_wc_normals, tc_window).xyz;
	const vec4 light_radiant_flux = vec4(vec3(1000000000.0), 1.0); // [W]
	const int light_view_size = light_view_dimensions.x * light_view_dimensions.y;
	const float spot_light_ratio = PI / 4.0; // A_circle / A_square
	const int photon_count = int(light_view_size * 2 * ldm_view_count * spot_light_ratio);
	const vec4 radiant_flux = light_radiant_flux / float(photon_count); // [W]

	// Rename for consistency with theory
	vec3 x = wc_view_eye_position;
	vec3 n = wc_normal;
	vec3 d_hat = vertex.wc_view_ray_direction;
	vec3 d = normalize(d_hat);
	vec3 right = current_view.right.xyz;
	vec3 up = current_view.up.xyz;

	// Ray differential construction and initial transfer
	photon_differential photon = construct_photon_differential(d_hat, right, up);

	float t = -ec_position_z;//-dot(x, n) / dot(d, n);
	transfer(photon, d, n, t);

	// Direct photon
	//store_photon(wc_position, wc_normal, photon, radiant_flux);

	// 1. bounce
	vec4 f = rho_d / PI; // [sr^-1]
	for (int i = 0; i < ldm_view_count; i++)
		store_first_bounce_in_both_directions(i, photon, wc_position, wc_normal, radiant_flux * f);
}